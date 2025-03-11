#pragma once

#include <cassert>
#include <memory>
#include "posix_v2/task_context.hpp"

namespace PosixV2::TaskContext
{
    inline bool Context_t::operator==(Context_t& rhs) const noexcept
    {
        return this == &rhs;
    }

    inline bool Context_t::operator!=(Context_t& rhs) const noexcept
    { 
        return !(*this == rhs);
    }

    inline Context_t& CreateIntoStack(StackType_t* stack) noexcept
    {
        return *new(stack) Context_t{};
    }

    inline void DeleteFromTaskHandle(TaskHandle_t taskHandle) noexcept
    {
        BorrowFromTaskHandle(taskHandle).~Context_t();
    }

    inline Context_t& BorrowFromTaskHandle(TaskHandle_t taskHandle) noexcept
    {
        assert(taskHandle != nullptr);
        return **reinterpret_cast<Context_t**>(taskHandle);
    }

    inline Context_t& BorrowFromCurrentTaskHandle() noexcept
    {
        return BorrowFromTaskHandle(xTaskGetCurrentTaskHandle());
    }
}
