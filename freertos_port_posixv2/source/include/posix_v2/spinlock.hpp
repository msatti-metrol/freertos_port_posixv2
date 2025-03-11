#pragma once

/// Adapted from here:
/// https://github.com/boostorg/sync/blob/master/include/boost/sync/detail/mutexes/spin_mutex.hpp
/// SPDX-License-Identifier: BSL-1.0

#include <cassert>
#include <atomic>
#include "posix_v2/pause.hpp"

namespace PosixV2
{
    /// @remark Async-signal-safe.
    class Spinlock_t
    {
    public:
        static_assert(std::atomic_bool::is_always_lock_free, "Can only use a spinlock if atomic_bool is lock free");
        static_assert(std::is_trivially_destructible_v<std::atomic_bool>, "Can only use a spinlock if atomic_bool is trivially destructable");

        inline Spinlock_t() noexcept;
        inline ~Spinlock_t() noexcept;

        inline void Lock() noexcept;
        inline bool TryLock() noexcept;
        inline void Unlock() noexcept;

    private:
        static constexpr bool LockedState = true;
        static constexpr bool UnlockedState = false;

        std::atomic_bool m_state;
    };
}

#include "posix_v2/spinlock.tpp"
