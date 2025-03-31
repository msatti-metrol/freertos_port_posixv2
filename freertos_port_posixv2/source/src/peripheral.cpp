#include <chrono>
#include <string>
#include <sys/types.h>
#include "FreeRTOS.h"
#include "task.h"
#include "posix_v2/peripheral.hpp"
#include "posix_v2/posix.hpp"
#include "posix_v2/kernel.hpp"
#include "posix_v2/task_context.hpp"

namespace PosixV2::Peripheral
{
    namespace Tick
    {
        const std::string& Name()
        {
            return "P. Tick";
        }

        bool Peripheral_t::Poll()
        {
            return true;
        }

        void Peripheral_t::OnInterruptRaised(bool success)
        {
            constexpr std::chrono::duration<useconds_t, std::micro> Period = 1000000 / configTICK_RATE_HZ;

            if (success)
                usleep(Period.count());
            else
                PosixV2::Pause();
        }

        void Peripheral_t::OnServiceInterrupt()
        {
            vPortEnterCritical();
        
            if (xTaskIncrementTick() == pdTRUE)
                vPortYield();
        
            vPortExitCritical();
        }
    }
}
