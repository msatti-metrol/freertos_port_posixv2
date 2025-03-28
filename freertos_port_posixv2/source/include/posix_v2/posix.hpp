#pragma once

#include <string>
#include <signal.h>
#include <pthread.h>

namespace PosixV2::Posix
{
    using SignalHandler_t = void (*)(int);
    using ExtendedSignalHandler_t = void (*)(int, siginfo_t*, void*);
    using StartRoutine_t = void* (*)(void *);

    /// Signaling

    inline sigset_t SaveSignalMask(bool blockAll = true) noexcept;
    inline void RestoreSignalMask(sigset_t mask) noexcept;
    inline void BlockSignal(int signal) noexcept;
    inline void UnblockSignal(int signal) noexcept;
    inline bool IsSignalBlocked(int signal) noexcept;
    inline void RaiseSignal(pthread_t thread, int signal) noexcept;
    inline void RaiseContextualSignal(pthread_t thread, int signal, void* parameter) noexcept;
    inline void WaitForSignal(int signal);
    inline void InstallSignalHandler(int signal, SignalHandler_t signalHandler) noexcept;
    inline void InstallExtendedSignalHandler(int signal, ExtendedSignalHandler_t signalHandler) noexcept;
    inline void InstallDefaultSignalHandler(int signal) noexcept;

    /// Thread (pthread)

    inline pthread_t CreateThread(StartRoutine_t start, void* parameters) noexcept;
    inline void CancelThread(pthread_t thread) noexcept;
    inline void JoinThread(pthread_t thread) noexcept;
    inline void ExitThread();
    inline void SetThreadName(const std::string& name) noexcept;
}

#include "posix_v2/posix.tpp"
