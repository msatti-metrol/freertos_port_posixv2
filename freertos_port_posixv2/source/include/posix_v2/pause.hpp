#pragma once

/// Adapted from here:
/// https://github.com/boostorg/sync/blob/master/include/boost/sync/detail/pause.hpp
/// SPDX-License-Identifier: BSL-1.0

#if defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_IX86))
extern "C" void _mm_pause(void);
#if defined(_MSC_VER)
#pragma intrinsic(_mm_pause)
#endif
#endif

namespace PosixV2
{
    inline void Pause() noexcept
    {
#if defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_IX86))
        _mm_pause();
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
        __asm__ __volatile__("pause;");
#endif
    }
}
