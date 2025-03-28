#pragma once

#include <cassert>
#include <string>
#include <signal.h>
#include <pthread.h>
#include "posix_v2/posix.hpp"

namespace PosixV2::Posix
{
    inline sigset_t SaveSignalMask(bool blockAll) noexcept
    {
        sigset_t all{}, mask{};
        sigset_t* newmask = blockAll ? &all : nullptr;

        sigfillset(&all);
        
        if (pthread_sigmask(SIG_SETMASK, newmask, &mask) != 0)
            assert(false && "Failed to save signals mask for current thread");
        
        return mask;
    }

    inline void RestoreSignalMask(sigset_t mask) noexcept
    {
        if (pthread_sigmask(SIG_SETMASK, &mask, nullptr) != 0)
            assert(false && "Failed to restore signals mask for current thread");
    }

    inline void BlockSignal(int signal) noexcept
    {
        sigset_t set{};
        sigaddset(&set, signal);

        if (pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0)
            assert(false && "Failed to block signal for current thread");
    }
    
    inline void UnblockSignal(int signal) noexcept
    {
        sigset_t set{};
        sigaddset(&set, signal);

        if (pthread_sigmask(SIG_UNBLOCK, &set, nullptr) != 0)
            assert(false && "Failed to unblock signal for current thread");
    }

    inline bool IsSignalBlocked(int signal) noexcept
    {
        sigset_t mask{};

        if (pthread_sigmask(0, nullptr, &mask) != 0)
            assert(false && "Failed to get signals mask for current thread");

        return sigismember(&mask, signal) == 1;
    }

    inline void RaiseSignal(pthread_t thread, int signal) noexcept
    {
        if (pthread_kill(thread, signal) != 0)
            assert(false && "Failed to send signal to thread");
    }
    
    inline void RaiseContextualSignal(pthread_t thread, int signal, void* parameter) noexcept
    {
        sigval_t sigval{};
        sigval.sival_ptr = parameter;

        if (pthread_sigqueue(thread, signal, sigval) != 0)
            assert(false && "Failed to send signal to thread");
    }

    inline void WaitForSignal(int signal)
    {
        sigset_t set{};
        sigemptyset(&set);
        sigaddset(&set, signal);
    
        // Note: raises an uncatchable exception for stack unwinding on Linux/glibc when cancelled.
        while (sigwaitinfo(&set, nullptr) != signal);
    }

    inline void InstallSignalHandler(int signal, SignalHandler_t signalHandler) noexcept
    {
        struct sigaction signalAction{};
        signalAction.sa_handler = signalHandler;
        
        if (sigaction(signal, &signalAction, nullptr) != 0)
            assert(false && "Setting up signal handler failed");
    }
    
    inline void InstallExtendedSignalHandler(int signal, ExtendedSignalHandler_t signalHandler) noexcept
    {
        struct sigaction signalAction{};
        signalAction.sa_flags |= SA_SIGINFO;
        signalAction.sa_sigaction = signalHandler;
        
        if (sigaction(signal, &signalAction, nullptr) != 0)
            assert(false && "Setting up signal handler failed");
    }

    inline void InstallDefaultSignalHandler(int signal) noexcept
    {
        struct sigaction signalAction{};
        signalAction.sa_handler = SIG_DFL;
        
        if (sigaction(signal, &signalAction, nullptr) != 0)
            assert(false && "Setting up signal handler failed");
    }

    inline pthread_t CreateThread(StartRoutine_t start, void* parameters) noexcept
    {
        pthread_t thread;

        if (pthread_create(&thread, nullptr, start, parameters) != 0)
            assert(false && "Failed to create pthread");

        return thread;
    }

    inline void CancelThread(pthread_t thread) noexcept
    {
        if (pthread_cancel(thread) != 0)
            assert(false && "Cancelling thread failed");
    }

    inline void JoinThread(pthread_t thread) noexcept
    {
        if (pthread_join(thread, nullptr) != 0)
            assert(false && "Joining thread failed");
    }

    inline void ExitThread()
    {
        // Note: raises an uncatchable exception for stack unwinding on Linux/glibc.
        pthread_exit(nullptr);
    }
    
    inline void SetThreadName(const std::string& name) noexcept
    {
        if (pthread_setname_np(pthread_self(), name.c_str()) != 0)
            assert(false && "Failed to set thread name");
    }
}
