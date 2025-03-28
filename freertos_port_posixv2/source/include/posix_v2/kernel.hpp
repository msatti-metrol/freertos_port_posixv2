#include <mutex>
#include <atomic> 
#include <chrono>
#include <pthread.h>

#define SIGNAL_SCHEDULER_END (SIGRTMIN + 0)
#define SIGNAL_RESUME (SIGRTMIN + 1)
#define SIGNAL_PERIPHERAL_INTERRUPT (SIGRTMIN + 2)

namespace PosixV2::Kernel
{
    struct State_t
    {
        std::atomic_bool m_schedulerStarted;
        std::atomic<std::chrono::steady_clock::time_point> m_schedulerStartTimePoint;
        std::atomic<pthread_t> m_schedulerThreadHandle;
        std::mutex m_taskContextLock;
    };

    extern State_t g_state;
}
