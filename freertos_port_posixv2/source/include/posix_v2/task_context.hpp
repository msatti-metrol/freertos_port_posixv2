#pragma once

#include <string>
#include <pthread.h>
#include <FreeRTOS.h>
#include <task.h>
#include "posix_v2/interrupt_state.hpp"
#include "posix_v2/spinlock.hpp"

namespace PosixV2::TaskContext
{
    struct Context_t
    {
        inline bool operator==(Context_t& rhs) const noexcept;
        inline bool operator!=(Context_t& rhs) const noexcept;
    
        std::string m_name;
        pthread_t m_threadHandle;
        TaskFunction_t m_taskMainFunction;
        void* m_taskParameters;
        PosixV2::InterruptState_t m_interruptState;
        int m_criticalSectionNesting;
    };
    
    /// @remark For use only while initially creating the task (guaranteed there are no other uses).
    inline Context_t& CreateIntoStack(StackType_t* stack) noexcept;

    /// @remark Must only be called when the context lifetime is active.
    inline void DeleteFromTaskHandle(TaskHandle_t taskHandle) noexcept;

    /// @remark Must only be called when the context lifetime is active.
    inline Context_t& BorrowFromTaskHandle(TaskHandle_t taskHandle) noexcept;

    /// @remark Must only be called when the context lifetime is active.
    inline Context_t& BorrowFromCurrentTaskHandle() noexcept;
}

#include "posix_v2/task_context.tpp"
