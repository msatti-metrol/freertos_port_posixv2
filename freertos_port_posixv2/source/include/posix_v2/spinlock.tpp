#pragma once

/// Adapted from here:
/// https://github.com/boostorg/sync/blob/master/include/boost/sync/detail/mutexes/spin_mutex.hpp
/// SPDX-License-Identifier: BSL-1.0

#include <cassert>
#include <atomic>
#include "posix_v2/spinlock.hpp"
#include "posix_v2/pause.hpp"

namespace PosixV2
{
    inline Spinlock_t::Spinlock_t() noexcept :
        m_state{}
    {
    }

    inline Spinlock_t::~Spinlock_t() noexcept
    {
        assert(m_state.load(std::memory_order::relaxed) == UnlockedState);
    }
    
    inline void Spinlock_t::Lock() noexcept
    {
        while (true)
        {
            while (m_state.load(std::memory_order::relaxed) != UnlockedState)
                PosixV2::Pause();

            if (TryLock())
                break;
        }
    }

    inline bool Spinlock_t::TryLock() noexcept
    {
        return m_state.exchange(LockedState, std::memory_order::acquire) == UnlockedState;
    }
    
    inline void Spinlock_t::Unlock() noexcept
    {
        assert(m_state.load(std::memory_order::relaxed) == LockedState);
        m_state.store(UnlockedState, std::memory_order::release);
    }
}
