/*! \file Task.hpp
    \author Alan Ramirez
    \date 2024-12-25
    \brief Job implementation
*/

#pragma once

#include <coroutine>
#include <exception>

namespace Hush
{
    /// A job is a task that can be executed by a worker thread.
    /// It is a C++20 coroutine that suspends and resumes execution.
    template <typename T>
    class Task
    {
    public:
        struct promise_type
        {
            T value;

            /// The promise type is responsible for creating the coroutine.
            Task get_return_object()
            {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            /// The coroutine will suspend immediately after it is created.
            std::suspend_always initial_suspend()
            {
                return {};
            }

            /// The coroutine will suspend immediately after it is destroyed.
            std::suspend_always final_suspend()
            {
                return {};
            }

            void return_value(T value)
            {
                this->value = value;
            }

            void unhandled_exception()
            {
                std::terminate();
            }
        };

        void Resume()
        {
            m_coroutine.resume();
        }

        decltype(auto) GetValue()
        {
            return m_coroutine.promise().value;
        }

    private:
        std::coroutine_handle<promise_type> m_coroutine;

        Task(std::coroutine_handle<promise_type> coroutine)
            : m_coroutine(coroutine)
        {
        }
    };
}