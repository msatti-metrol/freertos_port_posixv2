#include <cassert>
#include "posix_v2/kernel.hpp"
#include "posix_v2/peripheral.hpp"
#include "posix_v2/posix.hpp"
#include "posix_v2/task_context.hpp"

namespace PosixV2::Kernel
{
    void* Peripheral_t::ThreadMain(void *v_self)
    {
        auto& self = *reinterpret_cast<PosixV2::Kernel::Peripheral_t*>(v_self); 

        PosixV2::Posix::SetThreadName(self.m_peripheral.Name());
    
        while (!self.m_stopping.load(std::memory_order::relaxed))
        {
            if (self.m_peripheral.Poll())
                self.m_peripheral.OnInterruptRaised(self.RaiseInterrupt());
        }
    
        return nullptr;
    }
    
    Peripheral_t::Peripheral_t(PosixV2::Peripheral::IPeripheral_t& peripheral) :
        m_peripheral{peripheral},
        m_threadHandle{},
        m_stopping{}
    {
    }

    bool Peripheral_t::RaiseInterrupt()
    {
        std::unique_lock lock{PosixV2::Kernel::g_kernel.m_taskContextLock};

        std::atomic_thread_fence(std::memory_order::acquire);

        auto& task = PosixV2::TaskContext::BorrowFromCurrentTaskHandle();

        bool result = task.m_interruptState.TryQueryInterruptsStatus(
            [&](bool interruptsEnabled) 
            {
                if (!interruptsEnabled)
                    return false;

                PosixV2::Posix::RaiseContextualSignal(task.m_threadHandle, SIGNAL_PERIPHERAL_INTERRUPT, &m_peripheral);
                return true;
            });

        return result;
    }

    Kernel_t::PeripheralHandle_t Kernel_t::AddPeripheral(PosixV2::Peripheral::IPeripheral_t& peripheral)
    {
        std::unique_lock lock{m_peripheralsLock};

        auto& kernelPeripheral = m_peripherals.emplace_front(peripheral);
        kernelPeripheral.m_threadHandle = PosixV2::Posix::CreateThread(&Peripheral_t::ThreadMain, &kernelPeripheral);
        return m_peripherals.begin();
    }
    
    void Kernel_t::RemovePeripheral(PeripheralHandle_t handle)
    {
        std::unique_lock lock{m_peripheralsLock};

        RemovePeripheralUnsafe(handle);
    }

    void Kernel_t::RemovePeripheralUnsafe(PeripheralHandle_t handle)
    {
        assert(handle->m_threadHandle && "Expected the peripheral thread handle to be valid");
        handle->m_stopping.store(true, std::memory_order::relaxed);
        PosixV2::Posix::JoinThread(*handle->m_threadHandle);
        handle->m_threadHandle = std::nullopt;
    }
    
    void Kernel_t::RemoveAllPeripherals()
    {
        std::unique_lock lock{m_peripheralsLock};

        for (auto handle = m_peripherals.begin(); handle != m_peripherals.end(); handle++)
            RemovePeripheralUnsafe(handle);
    }
    
    Kernel_t g_kernel;
}
