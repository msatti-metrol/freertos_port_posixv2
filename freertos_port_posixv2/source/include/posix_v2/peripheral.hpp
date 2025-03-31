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
    struct IPeripheral_t
    {
        virtual ~IPeripheral_t() = default;

        /// @brief Gets the name of the peripheral, used for debugging.
        /// @return Name of the peripheral.
        virtual std::string Name() = 0;

        /// @brief Determines whether an event is pending and an interrupt should be raised.
        /// @return Whether a peripheral interrupt should be raised (within a FreeRTOS context).
        /// @remarks
        /// Called from outside a FreeRTOS task context.
        /// Must eventually yield (can block for short periods of time).
        virtual bool Poll() = 0;

        /// @brief Interrupt raised callback.
        /// @remarks
        /// Whenever `Poll()` indicates an event is ready (returns true), an interrupt is attempted to be raised.
        /// However it may not succeed due to the task servicing another interrupt or being inside a critical section.
        /// Called from outside a FreeRTOS task context.
        /// Must eventually yield (can block for short periods of time).
        virtual void OnInterruptRaised(bool success) = 0;

        /// @brief Service interrupt callback (effectively the ISR handler).
        /// @remarks
        /// Called from within FreeRTOS task context, inside signal handler.
        /// Must not block on any function calls, and they must be async-signal-safe.
        /// The FreeRTOS ISR API can be used freely.
        virtual void OnServiceInterrupt() = 0;
    };

    namespace Tick
    {
        class Peripheral_t : public IPeripheral_t
        {
        public:
            virtual ~Peripheral_t() = default;
            virtual std::string Name() override;
            virtual bool Poll() override;
            virtual void OnInterruptRaised(bool success) override;
            virtual void OnServiceInterrupt() override;
        };
    }
}
