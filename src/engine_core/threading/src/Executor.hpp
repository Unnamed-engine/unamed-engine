/*! \file Executor.hpp
    \author Alan Ramirez
    \date 2024-12-25
    \brief Executor class implementation
*/

#pragma once

#include "async/Task.hpp"
#include "async/TaskTraits.hpp"

#include <Result.hpp>
#include <array>
#include <chrono>
#include <coroutine>
#include <deque>
#include <functional>
#include <mutex>
#include <random>
#include <span>
#include <thread>
#include <vector>

namespace Hush
{
    class ThreadPool;
    class TaskOperation;

    namespace impl
    {
        /// WorkerQueue is a queue of tasks that a worker thread will execute.
        /// This is a lock-free queue.
        class WorkerQueue
        {
            /// The size of the worker queue. This is fixed.
            constexpr static std::size_t WORKER_QUEUE_SIZE = 256;

        public:
            /// Constructs a new worker queue.
            WorkerQueue(ThreadPool *threadPool);

            /// Move constructor.
            /// @param other Other WorkerQueue to move from.
            WorkerQueue(WorkerQueue &&other) noexcept;

            /// Move assignment operator.
            /// @param other Other WorkerQueue to move from.
            /// @return self
            WorkerQueue &operator=(WorkerQueue &&other) noexcept;

            /// Pushes a task to the queue.
            /// @param task The task to push to the queue.
            void Push(TaskOperation *task);

            /// Pops a task from the queue.
            /// @return A task from the queue. nullptr if the queue is empty.
            TaskOperation *Pop();

            /// Steals a task from the queue.
            /// @return A task from the queue. nullptr if the queue is empty.
            TaskOperation *Steal();

            /// Steals a task from the global queue.
            /// @return A task from the global queue. nullptr if the global queue is empty.
            TaskOperation *StealFromGlobalQueue();

        private:
            /// Reference to the thread pool, it is used to push tasks to the global queue in case the worker queue is
            /// full.
            ThreadPool *m_threadPool;

            /// The worker queue
            std::array<TaskOperation *, WORKER_QUEUE_SIZE> m_tasks;

            /// Bottom of the queue
            std::atomic<std::int64_t> m_bottom;

            /// Top of the queue
            std::atomic<std::int64_t> m_top;
        };

        class WorkerThread
        {
        public:

            // Default number of tasks to steal from the global queue.
            constexpr static std::uint32_t DEFAULT_STEAL_COUNT = 1;

            /// Options for the worker thread.
            struct ThreadOptions
            {
                /// The affinity of the worker thread, if 0, the worker thread will have no affinity.
                std::int32_t affinity = 0;
            };

            /// Enum class that represents the state of the worker thread.
            enum class EWorkerThreadState
            {
                None,
                Starting,
                Running,
                Idle,
                Stopping,
                Stopped
            };

            /// Enum class that represents the stop mode of the worker thread.
            enum class EStopMode : bool
            {
                FinishPendingTasks,
                StopImmediately
            };

            /// Constructs a new worker thread.
            /// @param threadPool The thread pool that the worker thread belongs to.
            /// @param threadIndex The index of the worker thread.
            /// @param options The options for the worker thread.
            WorkerThread(std::unique_ptr<WorkerQueue> workerQueue,
                         std::uint32_t threadIndex,
                         ThreadOptions options) noexcept;

            WorkerThread(const WorkerThread &) = delete;
            WorkerThread &operator=(const WorkerThread &) = delete;

            /// Starts the worker thread.
            void Start();

            /// Gets the state of the worker thread.
            EWorkerThreadState GetState() const noexcept
            {
                return m_state;
            }

            /// Stops the worker thread.
            /// @param stopMode Stop mode to use.
            void Stop(EStopMode stopMode);

            /// Gets the options for the worker thread.
            /// @return The options for the worker thread.
            [[nodiscard]]
            ThreadOptions GetOptions() const noexcept
            {
                return m_options;
            }

            void Notify();

        private:
            /// The main function of the worker thread.
            /// @param stopToken Stop token for the worker thread.
            void ThreadFunction(std::stop_token stopToken);

            /// The worker queue for the worker thread.
            std::unique_ptr<WorkerQueue> m_workerQueue;
            std::jthread m_thread;
            std::condition_variable m_conditionVariable;
            std::mutex m_mutex;
            std::uint32_t m_threadIndex;
            ThreadOptions m_options;
            EWorkerThreadState m_state = EWorkerThreadState::None;
            EStopMode m_stopMode = EStopMode::FinishPendingTasks;
        };

    }; // namespace impl

    class TaskOperation
    {
        friend class ThreadPool;
        friend class impl::WorkerThread;

        explicit TaskOperation(ThreadPool &executor, std::int32_t threadAffinity = -1) noexcept
            : m_executor(executor),
              m_threadAffinity(threadAffinity)
        {
        }

    public:
        bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;

        void await_resume() noexcept;

    private:
        ThreadPool &m_executor;
        std::coroutine_handle<> m_awaitingCoroutine = nullptr;
        std::int32_t m_threadAffinity = -1;
    };

    class ThreadPool
    {
    public:
        ThreadPool(std::uint32_t numThreads);

        ~ThreadPool();

        static ThreadPool Create(std::uint32_t numThreads);

        TaskOperation ScheduleCurrentTask();

        template <typename Fn, typename... Args>
            requires std::invocable<Fn, Args...>
        Task<void> ScheduleFunction(Fn &&function, Args &&...args)
        {
            auto task = [this, function = std::forward<Fn>(function),
                         args = std::make_tuple(std::forward<Args>(args)...)]() -> Task<void> {
                co_await ScheduleCurrentTask();
                std::apply(function, args);
                co_return;
            };

            auto operation = task();
            operation.GetCoroutine().resume();

            return std::move(operation);
        }

        Task<void> ScheduleTask(Task<void> &&task);

        void WaitUntilDone();

    private:
        TaskOperation *StealFromGlobalQueue();

        void NotifyWorkerThreads();

        void PushToGlobalQueue(TaskOperation *task);

        friend class impl::WorkerQueue;
        friend class TaskOperation;

        std::vector<std::unique_ptr<impl::WorkerThread>> m_workerThreads;
        std::vector<TaskOperation *> m_globalQueue;
        std::mutex m_globalQueueMutex;
    };
} // namespace Hush