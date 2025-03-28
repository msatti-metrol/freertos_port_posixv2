#pragma once

#include <mutex>
#include <atomic>
#include <functional>
#include <optional>
#include <unordered_set>
#include <string>
#include <pthread.h>

namespace PosixV2::Peripheral
{
    class Peripheral_t
    {
    public:
        using Handle_t = std::unordered_set<Peripheral_t*>::iterator;

        static Handle_t StartPeripheral(Peripheral_t& peripheral);
        static void StopAllPeripherals();

    private:
        static void* ThreadMain(void* self);

        static std::mutex s_peripheralsMutex;
        static std::unordered_set<Peripheral_t*> s_peripherals;

    public:
        Peripheral_t(const std::string& name);

        // Called from within FreeRTOS task context, inside signal handler.
        // Must not block on I/O (FreeRTOS API excluded).
        virtual void OnHandleInterrupt() = 0;

    protected:
        // Called from outside a FreeRTOS context within a separate thread.
        // Must eventually yield (can block for short periods of time).
        virtual bool OnPoll() = 0;
        
        // Called from outside a FreeRTOS context within a separate thread.
        // Must eventually yield (can block for short periods of time).
        virtual void OnInterruptRaised(bool success) = 0;
        
    private:
        bool RaiseInterrupt();

        std::optional<pthread_t> m_threadHandle;
        std::atomic_bool m_stopping;
        std::string m_name;
    };

    namespace Tick
    {
        class Peripheral_t : public Peripheral::Peripheral_t
        {
        public:
            using Peripheral::Peripheral_t::Peripheral_t;
            
            virtual bool OnPoll() override;
            virtual void OnHandleInterrupt() override;
            virtual void OnInterruptRaised(bool success) override;
        };
    }
}
