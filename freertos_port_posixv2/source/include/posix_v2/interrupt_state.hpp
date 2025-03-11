#pragma once

#include <concepts>
#include "posix_v2/spinlock.hpp"

namespace PosixV2
{
    /// @remark Async-signal-safe.
    class InterruptState_t
    {
    public:
        inline void SetInterruptsEnabled() noexcept;
        inline void SetInterruptsDisabled() noexcept;
        template<std::invocable<bool> ContinuationFn>
        inline bool TryQueryInterruptsStatus(ContinuationFn&& continuationFn) noexcept;
        inline bool GetInterruptsStatus() noexcept;

    private:
        PosixV2::Spinlock_t m_interruptsEnabledLock;
        bool m_interruptsEnabled;
    };
}

#include "posix_v2/interrupt_state.tpp"
