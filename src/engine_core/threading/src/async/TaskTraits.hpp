/*! \file TaskTraits.hpp
    \author Alan Ramirez
    \date 2024-12-31
    \brief Task traits
*/

#pragma once

#include <coroutine>

#include "Task.hpp"

namespace Hush
{
    template <typename T>
    concept IsAwaitable = requires(T t)
    {
        t.await_ready();
        t.await_suspend(std::coroutine_handle<>{});
        t.await_resume();
    };

    template <typename T>
    concept IsTask = requires(T t)
    {
        t.get_return_object();
    };
} // namespace Hush