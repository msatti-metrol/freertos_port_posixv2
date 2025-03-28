#pragma once

#include <concepts>
#include <mutex>
#include "posix_v2/interrupt_state.hpp"

namespace PosixV2
{
    inline void InterruptState_t::SetInterruptsEnabled() noexcept
    {
        std::unique_lock lock{m_interruptsEnabledLock};
        
        m_interruptsEnabled = true;
    }

    inline void InterruptState_t::SetInterruptsDisabled() noexcept
    {
        std::unique_lock lock{m_interruptsEnabledLock};

        m_interruptsEnabled = false;
    }

    template<std::invocable<bool> ContinuationFn>
    inline bool InterruptState_t::TryQueryInterruptsStatus(ContinuationFn&& continuationFn) noexcept
    {
        std::unique_lock lock{m_interruptsEnabledLock, std::try_to_lock};

        if (!lock)
            return false;

        return continuationFn(m_interruptsEnabled);
    }

    inline bool InterruptState_t::GetInterruptsStatus() noexcept
    {
        std::unique_lock lock{m_interruptsEnabledLock};

        return m_interruptsEnabled;
    }
}
