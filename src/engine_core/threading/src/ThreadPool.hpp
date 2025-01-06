/*! \file ThreadPool.hpp
    \author Alan Ramirez
    \date 2024-12-25
    \brief ThreadPool implementation
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

            /// Steals a number of tasks from the global queue.
            /// @return The number of stolen tasks.
            [[nodiscard]]
            std::size_t TakeFromGlobalQueue();

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
            constexpr static std::uint32_t DEFAULT_STEAL_COUNT = 10;

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

            ~WorkerThread();

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

            /// Notifies the worker thread.
            void Notify();

        private:
            /// The main function of the worker thread.
            /// @param stopToken Stop token for the worker thread.
            void ThreadFunction(std::stop_token stopToken);

            /// The worker queue for the worker thread.
            std::unique_ptr<WorkerQueue> m_workerQueue;
            std::jthread m_thread;
            std::uint32_t m_threadIndex;
            ThreadOptions m_options;
            std::atomic<EWorkerThreadState> m_state = EWorkerThreadState::None;
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
        /// Constructs a new thread pool.
        /// @param numThreads The number of threads in the thread pool.
        ThreadPool(std::uint32_t numThreads);

        /// Destroys the thread pool.
        ~ThreadPool();

        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;

        ThreadPool(ThreadPool &&) = delete;
        ThreadPool &operator=(ThreadPool &&) = delete;

        /// Schedules the current task.
        /// @return TaskOperation that schedules the current task.
        TaskOperation ScheduleCurrentTask();

        /// Schedules a task.
        /// @param task The task to schedule.
        /// @return Task wrapped that will be executed by the thread pool.
        Task<void> ScheduleTask(Task<void> &&task);

        /// Starts the thread pool.
        void Start();

        /// Waits until all tasks are done.
        /// TODO: should we remove this function?, i.e. implement a way to wait for a specific task?, something like
        /// Hush::SyncWait(task)
        void WaitUntilDone();

        /// @return The number of threads in the thread pool.
        [[nodiscard]]
        std::uint32_t GetNumThreads() const noexcept
        {
            return static_cast<std::uint32_t>(m_workerThreads.size());
        }

    private:
        /// Steals a task from the global queue.
        /// @return A task from the global queue.
        TaskOperation *StealFromGlobalQueue();

        /// Notifies the worker threads.
        /// @param tasks Span that will contain the tasks that were stolen.
        /// @return The number of tasks that were stolen.
        std::size_t StealFromGlobalQueue(std::span<TaskOperation *> tasks);

        /// Notifies the worker threads.
        void NotifyWorkerThreads();

        void PushToGlobalQueue(TaskOperation *task);

        friend class impl::WorkerQueue;
        friend class TaskOperation;

        std::vector<std::unique_ptr<impl::WorkerThread>> m_workerThreads;
        std::vector<TaskOperation *> m_globalQueue;
        std::mutex m_globalQueueMutex;
    };
} // namespace Hush