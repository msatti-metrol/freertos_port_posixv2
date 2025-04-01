#include <mutex>
#include <atomic> 
#include <chrono>
#include <forward_list>
#include <pthread.h>
#include "posix_v2/peripheral.hpp"

#define SIGNAL_SCHEDULER_END (SIGRTMIN + 0)
#define SIGNAL_RESUME (SIGRTMIN + 1)
#define SIGNAL_PERIPHERAL_INTERRUPT (SIGRTMIN + 2)

namespace PosixV2::Kernel
{
    class Peripheral_t
    {
    public:
        Peripheral_t(Peripheral::IPeripheral_t& peripheral);

    private:
        friend class Kernel_t;
        
        static void* ThreadMain(void *v_self);
        bool RaiseInterrupt();

        Peripheral::IPeripheral_t& m_peripheral;
        std::optional<pthread_t> m_threadHandle;
        std::atomic_bool m_stopping;
    };

    struct Kernel_t
    {
        using PeripheralHandle_t = std::forward_list<Peripheral_t>::iterator;

        PeripheralHandle_t AddPeripheral(PosixV2::Peripheral::IPeripheral_t& peripheral);
        void RemovePeripheral(PeripheralHandle_t handle);
        void RemovePeripheralUnsafe(PeripheralHandle_t handle);
        void RemoveAllPeripherals();

        std::atomic_bool m_schedulerStarted;
        std::atomic<std::chrono::steady_clock::time_point> m_schedulerStartTimePoint;
        std::atomic<pthread_t> m_schedulerThreadHandle;
        std::mutex m_taskContextLock;
        std::mutex m_peripheralsLock;
        std::forward_list<Peripheral_t> m_peripherals;
    };

    extern Kernel_t g_kernel;
}
