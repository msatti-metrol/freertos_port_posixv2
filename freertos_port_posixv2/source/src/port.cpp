#include <cassert>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <string_view>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "portmacro.h"
#include "FreeRTOS.h"
#include "task.h"
#include "posix_v2/interrupt_state.hpp"
#include "posix_v2/posix.hpp"
#include "posix_v2/task_context.hpp"
#include "posix_v2/kernel.hpp"
#include "posix_v2/peripheral.hpp"

/// Notes:
/// - Each task has an associated POSIX thread (pthread).
/// - A real task stack doesn't exist; the stack is a buffer used to store the port task state only.
/// - No nested "interrupts" support; all interrupts (like tick) must be handled within a critical section.

static_assert(portBYTE_ALIGNMENT >= alignof(PosixV2::TaskContext::Context_t), "Byte alignment requirement not met");
static_assert(portSTACK_GROWTH == 1, "Stack growth direction must be set to 1 (low to high)");
static_assert(configMINIMAL_STACK_SIZE >= (sizeof(PosixV2::TaskContext::Context_t) + portBYTE_ALIGNMENT), "Stack size not large enough");
static_assert(configCHECK_FOR_STACK_OVERFLOW == 0, "Stack overflow checking must be set to 0");
static_assert(std::is_same_v<long, configRUN_TIME_COUNTER_TYPE>, "Expecting run time counter type to be of type long");

static PosixV2::Peripheral::Tick::Peripheral_t g_tickPeripheral{"Tick"};

static void HandleSignalPeripheralInterrupt(int signal, siginfo_t* info, [[maybe_unused]] void* ucontext)
{
    assert((signal == SIGNAL_PERIPHERAL_INTERRUPT) && "Unexpected signal");
    auto peripheral = reinterpret_cast<PosixV2::Peripheral::Peripheral_t*>(info->si_value.sival_ptr);
    peripheral->OnHandleInterrupt();
}

static void* TaskThreadMain(void* parameters)
{
    assert(PosixV2::Posix::IsSignalBlocked(SIGNAL_RESUME) && "SIGNAL_RESUME should be blocked when starting new task thread");
    
    // Suspend this thread now, wait for SIGNAL_RESUME.
    // Interrupts are disabled to start with, preventing ticks & other ISRs.
    PosixV2::Posix::WaitForSignal(SIGNAL_RESUME);
    PosixV2::Posix::UnblockSignal(SIGNAL_RESUME);
    PosixV2::Posix::UnblockSignal(SIGNAL_PERIPHERAL_INTERRUPT);

    std::atomic_signal_fence(std::memory_order::acquire);

    auto& self = *reinterpret_cast<PosixV2::TaskContext::Context_t*>(parameters);
    assert((self == PosixV2::TaskContext::BorrowFromCurrentTaskHandle()) && "Expecting to be started only when it becomes the current task");

    self.m_name = pcTaskGetName(xTaskGetCurrentTaskHandle());
    PosixV2::Posix::SetThreadName(self.m_name);

    assert((!self.m_interruptState.GetInterruptsStatus()) && "Expected to start with interrupts disabled");
    assert((self.m_criticalSectionNesting == 0) && "Expected to start outside a critical section");

    self.m_interruptState.SetInterruptsEnabled();
    self.m_taskMainFunction(self.m_taskParameters);

    assert(false && "RTOS task tried to return from main");

    return nullptr;
}

static void SwitchActiveTask(PosixV2::TaskContext::Context_t& self, PosixV2::TaskContext::Context_t& next)
{
    if (self == next)
        return;
    
    assert((self.m_threadHandle == pthread_self()) && "Expected to suspend own thread");
    assert((next.m_threadHandle != pthread_self()) && "Cannot resume own thread");
    assert((!self.m_interruptState.GetInterruptsStatus()) && "Expected interrupts to be disabled while switching tasks");

    std::atomic_thread_fence(std::memory_order::release);

    // Note: we want the resume & suspend action to appear atomic;
    //  not doing so could result in the next thread resuming this one again
    //  and the subsequent wait for signal would never trigger.
    auto mask = PosixV2::Posix::SaveSignalMask();
    PosixV2::Posix::RaiseSignal(next.m_threadHandle, SIGNAL_RESUME);
    PosixV2::Posix::WaitForSignal(SIGNAL_RESUME);
    PosixV2::Posix::RestoreSignalMask(mask);
    
    std::atomic_thread_fence(std::memory_order::acquire);

    // Should now be made the current task again.
    assert((PosixV2::TaskContext::BorrowFromCurrentTaskHandle() == self) && "Unexpected FreeRTOS current task state");
}

static void HandleSignalResume(int signal)
{
    assert((signal == SIGNAL_RESUME) && "Unexpected signal");
}

StackType_t* pxPortInitialiseStack(StackType_t* stack, TaskFunction_t taskFunction, void* taskParameters)
{
    auto& task = PosixV2::TaskContext::CreateIntoStack(stack);

    task.m_taskMainFunction = taskFunction;
    task.m_taskParameters = taskParameters;
    task.m_name = "<not initially resumed>";
    
    return stack;
}

void vPortInitializeTask(TaskHandle_t taskHandle)
{
    std::atomic_thread_fence(std::memory_order::release);
    
    auto& task = PosixV2::TaskContext::BorrowFromTaskHandle(taskHandle);

    // Note: need to start the task with SIGNAL_RESUME blocked;
    //  techincally it's possible for the initial resume to be missed otherwise.
    auto mask = PosixV2::Posix::SaveSignalMask();
    task.m_threadHandle = PosixV2::Posix::CreateThread(TaskThreadMain, &task);
    PosixV2::Posix::RestoreSignalMask(mask);
}

void vPortDestructTask(TaskHandle_t taskHandle)
{
    std::unique_lock lock{PosixV2::Kernel::g_state.m_taskContextLock};

    auto& task = PosixV2::TaskContext::BorrowFromTaskHandle(taskHandle);

    PosixV2::Posix::CancelThread(task.m_threadHandle);
    PosixV2::Posix::JoinThread(task.m_threadHandle);
    PosixV2::TaskContext::DeleteFromTaskHandle(taskHandle);
}

static void HandleSignalSchedulerEnd(int signal)
{
    assert((signal == SIGNAL_SCHEDULER_END) && "Unexpected signal");
}

BaseType_t xPortStartScheduler(void)
{
    // Ensure internal FreeRTOS state is visible to other threads (ie: pxCurrentTCB).
    std::atomic_thread_fence(std::memory_order::release);

    // Ensure SIGNAL_SCHEDULER_END cannot be missed later on. 
    auto mask = PosixV2::Posix::SaveSignalMask(false);
    PosixV2::Posix::BlockSignal(SIGNAL_SCHEDULER_END);

    // Initialize scheduler state.
    {
        PosixV2::Posix::SetThreadName("Scheduler");
        PosixV2::Posix::InstallSignalHandler(SIGNAL_SCHEDULER_END, HandleSignalSchedulerEnd);
        PosixV2::Posix::InstallSignalHandler(SIGNAL_RESUME, HandleSignalResume);
        PosixV2::Posix::InstallExtendedSignalHandler(SIGNAL_PERIPHERAL_INTERRUPT, HandleSignalPeripheralInterrupt);

        PosixV2::Kernel::g_state.m_schedulerStarted.store(true, std::memory_order::relaxed);
        auto selfThreadHandle = pthread_self();
        PosixV2::Kernel::g_state.m_schedulerThreadHandle.store(selfThreadHandle, std::memory_order::relaxed); 
    }

    // Transfer control to current task.
    {
        auto& task = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();
        PosixV2::Posix::RaiseSignal(task.m_threadHandle, SIGNAL_RESUME);
    }

    // Initialize required peripheral simulator(s).
    {
        PosixV2::Peripheral::Peripheral_t::StartPeripheral(g_tickPeripheral);
    }
        
    // Wait for signal to end the scheduler.
    PosixV2::Posix::WaitForSignal(SIGNAL_SCHEDULER_END);

    // Ensure internal FreeRTOS state is visible to this thread (ie: pxCurrentTCB).
    std::atomic_thread_fence(std::memory_order::acquire);

    // Destruct peripheral simulator(s).
    {
        PosixV2::Peripheral::Peripheral_t::StopAllPeripherals();
    }

    // Destruct remaining task and scheduler state.
    {
        PosixV2::Kernel::g_state.m_schedulerStarted.store(false, std::memory_order::relaxed);
        
        vTaskDelete(xTaskGetCurrentTaskHandle());

        PosixV2::Posix::InstallDefaultSignalHandler(SIGNAL_SCHEDULER_END);
        PosixV2::Posix::InstallDefaultSignalHandler(SIGNAL_RESUME);
        PosixV2::Posix::InstallDefaultSignalHandler(SIGNAL_PERIPHERAL_INTERRUPT);
    }

    // Restore signal mask from before.
    PosixV2::Posix::RestoreSignalMask(mask);

    return pdTRUE;
}

void vPortEndScheduler(void)
{
    std::atomic_thread_fence(std::memory_order::release);

    auto& self = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();

    self.m_interruptState.SetInterruptsDisabled();
    PosixV2::Posix::SaveSignalMask();
    PosixV2::Posix::RaiseSignal(PosixV2::Kernel::g_state.m_schedulerThreadHandle, SIGNAL_SCHEDULER_END);
    PosixV2::Posix::ExitThread();
}

void vPortMemoryBarrier()
{
    std::atomic_signal_fence(std::memory_order::seq_cst);
}

void vPortYield()
{
    assert(PosixV2::Kernel::g_state.m_schedulerStarted.load(std::memory_order::relaxed) && "Expecting scheduler to be started");
    
    vPortEnterCritical();
    
    auto& self = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();

    vTaskSwitchContext();

    auto& next = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();

    SwitchActiveTask(self, next);

    vPortExitCritical();
}

void vPortDisableInterrupts()
{
    if (!PosixV2::Kernel::g_state.m_schedulerStarted.load(std::memory_order::relaxed))
        return;

    auto& self = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();
    self.m_interruptState.SetInterruptsDisabled();
}

void vPortEnableInterrupts()
{
    if (!PosixV2::Kernel::g_state.m_schedulerStarted.load(std::memory_order::relaxed))
        return;

    auto& self = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();
    self.m_interruptState.SetInterruptsEnabled();
}

UBaseType_t xPortSetInterruptMask()
{
    vPortDisableInterrupts();
    return 0;
}

void vPortClearInterruptMask([[maybe_unused]] UBaseType_t xMask)
{
    vPortEnableInterrupts();
}

void vPortEnterCritical(void)
{
    if (!PosixV2::Kernel::g_state.m_schedulerStarted.load(std::memory_order::relaxed))
        return;
    
    auto& self = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();

    if (self.m_criticalSectionNesting == 0)
    {
        assert(self.m_interruptState.GetInterruptsStatus() && "Interrupt state inconsistent");
        self.m_interruptState.SetInterruptsDisabled();
    }

    assert(!self.m_interruptState.GetInterruptsStatus() && "Interrupt state inconsistent");
    self.m_criticalSectionNesting += 1;
}

void vPortExitCritical(void)
{
    if (!PosixV2::Kernel::g_state.m_schedulerStarted.load(std::memory_order::relaxed))
        return;

    auto& self = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();

    assert(!self.m_interruptState.GetInterruptsStatus() && "Interrupt state inconsistent");
    assert((self.m_criticalSectionNesting > 0) && "Critical section nesting underflow");
    self.m_criticalSectionNesting -= 1;

    if (self.m_criticalSectionNesting == 0)
        self.m_interruptState.SetInterruptsEnabled();
}

void vPortInitializeTimer(void)
{
    auto now = std::chrono::steady_clock::now();
    PosixV2::Kernel::g_state.m_schedulerStartTimePoint.store(now, std::memory_order::relaxed);
}

long lPortGetTimerCounter(void)
{
    auto start = PosixV2::Kernel::g_state.m_schedulerStartTimePoint.load(std::memory_order::relaxed);
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<long, std::nano> duration = now - start;
    auto count = duration.count();
    assert((count >= 0) && "Expecting duration to be positive");
    return count;
}
