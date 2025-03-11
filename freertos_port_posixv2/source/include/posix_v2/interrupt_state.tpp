#pragma once

#include <concepts>
#include "posix_v2/interrupt_state.hpp"

namespace PosixV2
{
    inline void InterruptState_t::SetInterruptsEnabled() noexcept
    {
        m_interruptsEnabledLock.Lock();
        m_interruptsEnabled = true;
        m_interruptsEnabledLock.Unlock();
    }

    inline void InterruptState_t::SetInterruptsDisabled() noexcept
    {
        m_interruptsEnabledLock.Lock();
        m_interruptsEnabled = false;
        m_interruptsEnabledLock.Unlock();
    }

    template<std::invocable<bool> ContinuationFn>
    inline bool InterruptState_t::TryQueryInterruptsStatus(ContinuationFn&& continuationFn) noexcept
    {
        if (!m_interruptsEnabledLock.TryLock())
            return false;

        auto result = continuationFn(m_interruptsEnabled);

        m_interruptsEnabledLock.Unlock();

        return result;
    }

    inline bool InterruptState_t::GetInterruptsStatus() noexcept
    {
        m_interruptsEnabledLock.Lock();
        bool result = m_interruptsEnabled;
        m_interruptsEnabledLock.Unlock();
        return result;
    }
}
