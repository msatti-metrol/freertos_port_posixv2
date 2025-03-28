#include <cassert>
#include <mutex>
#include <unordered_set>
#include "FreeRTOS.h"
#include "task.h"
#include "posix_v2/peripheral.hpp"
#include "posix_v2/posix.hpp"
#include "posix_v2/kernel.hpp"
#include "posix_v2/task_context.hpp"

namespace PosixV2::Peripheral
{
    Peripheral_t::Handle_t Peripheral_t::StartPeripheral(Peripheral_t& peripheral)
    {
        assert(!peripheral.m_threadHandle && "Expected the peripheral thread handle to be empty");
        peripheral.m_stopping.store(false, std::memory_order::relaxed);
        
        std::unique_lock lock{s_peripheralsMutex};

        peripheral.m_threadHandle = PosixV2::Posix::CreateThread(&Peripheral_t::ThreadMain, &peripheral);
        auto result = s_peripherals.insert(&peripheral);
        assert(result.second && "Expected a unique handle to insert");

        return result.first;
    }

    void Peripheral_t::StopAllPeripherals()
    {
        std::unique_lock lock{s_peripheralsMutex};

        for (auto peripheral : s_peripherals)
        {
            peripheral->m_stopping.store(true, std::memory_order::relaxed);
            assert(peripheral->m_threadHandle && "Expected a valid thread handle");
            PosixV2::Posix::JoinThread(*peripheral->m_threadHandle);
            peripheral->m_threadHandle = std::nullopt;
        }
    }
    
    bool Peripheral_t::RaiseInterrupt()
    {
        std::unique_lock lock{PosixV2::Kernel::g_state.m_taskContextLock};
    
        std::atomic_thread_fence(std::memory_order::acquire);

        auto& task = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();

        bool result = task.m_interruptState.TryQueryInterruptsStatus(
            [&](bool interruptsEnabled) 
            {
                if (!interruptsEnabled)
                    return false;

                PosixV2::Posix::RaiseContextualSignal(task.m_threadHandle, SIGNAL_PERIPHERAL_INTERRUPT, this);
                return true;
            });

        return result;
    }

    Peripheral_t::Peripheral_t(const std::string& name) :
        m_name{name}
    {
    }

    void* Peripheral_t::ThreadMain(void* v_self)
    {
        auto& self = *reinterpret_cast<Peripheral_t*>(v_self); 

        PosixV2::Posix::SetThreadName(self.m_name);
    
        while (!self.m_stopping.load(std::memory_order::relaxed))
        {
            bool success = self.OnPoll() && self.RaiseInterrupt();
            self.OnInterruptRaised(success);
        }

        return nullptr;
    }
    
    std::mutex Peripheral_t::s_peripheralsMutex;
    std::unordered_set<Peripheral_t*> Peripheral_t::s_peripherals;

    namespace Tick
    {
        bool Peripheral_t::OnPoll()
        {
            return true;
        }

        void Peripheral_t::OnHandleInterrupt()
        {
            vPortEnterCritical();
        
            if (xTaskIncrementTick() == pdTRUE)
                vPortYield();
        
            vPortExitCritical();
        }
        
        void Peripheral_t::OnInterruptRaised(bool success)
        {
            if (success)
                usleep(portTICK_RATE_MICROSECONDS);
            else
                PosixV2::Pause();
        }
    }
}
