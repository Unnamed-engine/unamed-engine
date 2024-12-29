/*! \file Executor.hpp
    \author Alan Ramirez
    \date 2024-12-25
    \brief Executor class implementation
*/

#pragma once

#include "Task.hpp"
#include <chrono>
#include <coroutine>
#include <deque>
#include <mutex>
#include <random>
#include <semaphore>
#include <thread>
#include <vector>

namespace Hush
{
    struct JobTask
    {
        struct promise_type
        {
          public:
            JobTask get_return_object()
            {
                return JobTask{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend()
            {
                return {};
            }

            std::suspend_always final_suspend()
            {
                return {};
            }

            void return_void()
            {
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

        void WaitUntilFinished()
        {
            m_semaphore.acquire();
        }

        [[nodiscard]] bool WaitUntil(std::chrono::steady_clock::time_point time)
        {
            return m_semaphore.try_acquire_until(time);
        }

      private:
        JobTask(std::coroutine_handle<promise_type> coroutine) : m_coroutine(coroutine), m_semaphore(0)
        {
        }

        std::coroutine_handle<promise_type> m_coroutine;
        std::binary_semaphore m_semaphore;
    };

    /// The executor class is responsible for managing a thread pool and executing tasks (C++20 coroutines) in parallel.
    /// The executor follows the following architecture:
    /// A main thread creates a pool of worker threads, each worker thread is responsible for executing tasks.
    /// Each worker thread has a task queue, where tasks are pushed to.
    ///
    /// The main thread is responsible for pushing tasks to the worker threads.
    ///
    /// If a worker thread finishes executing a task, it will pop a new task from the task queue.
    /// If the task queue is empty, the worker thread will steal a task from another random worker thread.
    class Executor
    {
        /// A job queue is a queue of tasks that a worker thread will execute.
        /// TODO: Use a lock-free queue instead of a mutex
        struct JobQueue
        {
            std::deque<JobTask> tasks;
            std::mutex mutex;
            std::mt19937_64 randomEngine;
        };

        struct ThreadInfo
        {
            std::jthread thread;
            std::stop_token stopToken;
        };

      public:
        // A TaskHandle is a handle to a task that is being executed by a worker thread.
        // It allows the main thread to wait for the task to finish executing
        struct TaskHandle
        {
            void wait()
            {
                // Use a wait condition variable
            }

          private:
        };

        /// Constructor that creates a thread pool with a specified number of threads.
        /// @param numThreads The number of threads in the thread pool. If 0, the number of threads will be equal to the
        /// number of hardware threads.
        explicit Executor(std::uint32_t numThreads);

        /// Default constructor. Creates a thread pool with the number of threads equal to the number of hardware
        /// threads. If this value is 1, the executor will create 4 threads.
        Executor();

        ~Executor();

        Executor(const Executor &) = delete;
        Executor &operator=(const Executor &) = delete;

        Executor(Executor &&) = default;
        Executor &operator=(Executor &&) = default;

        TaskHandle execute(std::coroutine_handle<> task);

      private:
        void CreateThreads(std::uint32_t numThreads);

        void ThreadFunction(std::stop_token stopToken, uint32_t threadIndex);

        std::coroutine_handle<void> StealTask(uint32_t currentThreadIndex);

        std::random_device m_randomDevice;
        std::mt19937_64 m_randomEngine;
        std::vector<std::jthread> m_threads;
        std::vector<JobQueue> m_jobQueues;
    };
} // namespace Hush