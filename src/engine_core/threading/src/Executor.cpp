/*! \file Executor.cpp
    \author Alan Ramirez
    \date 2024-12-25
    \brief Executor class implementation
*/
#include "Executor.hpp"
#include "Platform.hpp"
#include "async/Task.hpp"

#include <Logger.hpp>
#include <mutex>
#include <thread>

Hush::impl::WorkerQueue::WorkerQueue(ThreadPool *threadPool)
    : m_threadPool(threadPool),
      m_tasks(),
      m_bottom(0),
      m_top(0)
{
}

Hush::impl::WorkerQueue::WorkerQueue(WorkerQueue &&other) noexcept
    : m_threadPool(other.m_threadPool),
      m_bottom(other.m_bottom.load(std::memory_order_acquire)),
      m_top(other.m_top.load(std::memory_order_acquire)),
      m_tasks(std::move(other.m_tasks))
{
    // We use release memory order since we are moving the tasks to another queue.
    other.m_bottom.store(0, std::memory_order_release);
    other.m_top.store(0, std::memory_order_release);
}

Hush::impl::WorkerQueue &Hush::impl::WorkerQueue::operator=(WorkerQueue &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    this->m_threadPool = std::move(other.m_threadPool);
    this->m_bottom = other.m_bottom.load(std::memory_order_acquire);
    this->m_top = other.m_top.load(std::memory_order_acquire);
    this->m_tasks = std::move(other.m_tasks);

    other.m_bottom.store(0, std::memory_order_release);
    other.m_top.store(0, std::memory_order_release);
    other.m_threadPool = nullptr;

    return *this;
}

void Hush::impl::WorkerQueue::Push(TaskOperation *task)
{
    // Push a task to the bottom of the queue
    std::int64_t bottom = m_bottom.load(std::memory_order_acquire);
    std::int64_t newBottom = (bottom + 1) % WORKER_QUEUE_SIZE;

    if (newBottom == m_top.load(std::memory_order_acquire))
    {
        // The queue is full
        // Push the task to the global queue
        m_threadPool->PushToGlobalQueue(task);
        return;
    }

    m_tasks[bottom] = task;

    // Ok, just update the bottom
    m_bottom.store(newBottom, std::memory_order_release);
}

Hush::TaskOperation *Hush::impl::WorkerQueue::Pop()
{
    // Pops a task from the bottom of the queue.
    std::int64_t bottom = m_bottom.fetch_sub(1, std::memory_order_acq_rel);
    std::int64_t top = m_top.load(std::memory_order_acquire);

    if (top >= bottom)
    {
        // The queue is empty
        return nullptr;
    }

    // Non-empty queue, just pop the task
    TaskOperation *task = m_tasks[bottom];

    if (top != bottom)
    {
        // There are more tasks in the queue.
        return task;
    }

    // Last task in the queue
    if (!m_top.compare_exchange_strong(top, (top + 1) % WORKER_QUEUE_SIZE))
    {
        // Another thread stole the task
        return nullptr;
    }

    m_bottom.store(top + 1, std::memory_order_release);

    return task;
}

Hush::TaskOperation *Hush::impl::WorkerQueue::Steal()
{
    // Steals a task from the top of the queue.
    std::int64_t top = m_top.load(std::memory_order_acquire);
    std::int64_t bottom = m_bottom.load(std::memory_order_acquire);

    // Check if the queue is empty
    if (top >= bottom)
    {
        return nullptr;
    }

    // Non-empty queue, just steal the task
    TaskOperation *task = m_tasks[top];

    if (!m_top.compare_exchange_strong(top, (top + 1) % WORKER_QUEUE_SIZE))
    {
        // Another thread stole the task
        return nullptr;
    }

    // Task stolen
    return task;
}
Hush::TaskOperation *Hush::impl::WorkerQueue::StealFromGlobalQueue()
{
    return m_threadPool->StealFromGlobalQueue();
}

#if HUSH_PLATFORM_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

static void SetCurrentThreadAffinity(std::uint32_t affinity)
{
#if HUSH_PLATFORM_WIN
    const DWORD_PTR mask = 1ULL << static_cast<std::uint64_t>(affinity);
    SetThreadAffinityMask(GetCurrentThread(), mask);
#endif
}

Hush::impl::WorkerThread::WorkerThread(std::unique_ptr<WorkerQueue> workerQueue,
                                       std::uint32_t threadIndex,
                                       ThreadOptions options) noexcept
    : m_workerQueue(std::move(workerQueue)),
      m_threadIndex(threadIndex),
      m_options(options)
{
    m_thread = std::jthread([this](std::stop_token stopToken) {
        if (m_options.affinity >= 0)
        {
            SetCurrentThreadAffinity(m_options.affinity);
        }

        // Wait until the thread is started
        {
            std::unique_lock lock(m_mutex);
            m_conditionVariable.wait(lock, [this] { return m_state == EWorkerThreadState::Starting; });
        }

        // Start the thread function
        this->ThreadFunction(stopToken);
    });
}

void Hush::impl::WorkerThread::Start()
{
    // Notify the thread to start with the condition variable
    m_state = EWorkerThreadState::Starting;
    m_conditionVariable.notify_one();
}

void Hush::impl::WorkerThread::Stop(EStopMode stopMode)
{
    m_stopMode = stopMode;
    m_state = EWorkerThreadState::Stopping;

    // Stop the thread
    m_thread.request_stop();

    // Just in case the thread is sleeping, wake it up
    m_conditionVariable.notify_one();
}

void Hush::impl::WorkerThread::Notify()
{
    m_conditionVariable.notify_one();
}

void Hush::impl::WorkerThread::ThreadFunction(std::stop_token stopToken)
{
    m_state = EWorkerThreadState::Running;

    while (!stopToken.stop_requested())
    {
        TaskOperation *task = m_workerQueue->Pop();

        if (task == nullptr)
        {
            // Steal a task
            task = m_workerQueue->Steal();
        }

        // No task could be stolen, check the global queue
        if (task == nullptr)
        {

            task = m_workerQueue->StealFromGlobalQueue();
        }

        if (task != nullptr)
        {
            task->m_awaitingCoroutine.resume();
        }
        else
        {
            // No task could be found either in the local, external local, or global queue.
            // Perform some busy waiting
            // TODO: Perform some busy waiting while checking for tasks in the current thread's queue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // No tasks to execute, put the thread to sleep
            m_state = EWorkerThreadState::Idle;
            std::unique_lock lock(m_mutex);
            m_conditionVariable.wait(lock, [this] { return m_state != EWorkerThreadState::Idle; });

            m_state = EWorkerThreadState::Running;

            lock.unlock();

            // Thread woke up, continue execution
        }
    }

    if (m_stopMode == EStopMode::FinishPendingTasks)
    {
        // Finish pending tasks
        TaskOperation *task = m_workerQueue->Pop();

        while (task != nullptr)
        {
            task->m_awaitingCoroutine.resume();
            task = m_workerQueue->Pop();
        }
    }
}

void Hush::TaskOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
{
    m_awaitingCoroutine = awaitingCoroutine;
    m_executor.PushToGlobalQueue(this);

    // Notify any sleeping thread
    m_executor.NotifyWorkerThreads();
}

void Hush::TaskOperation::await_resume() noexcept
{
}

Hush::ThreadPool::ThreadPool(std::uint32_t numThreads)
{
    const std::uint32_t numHardwareThreads = std::thread::hardware_concurrency();
    // Get hardware threads
    if (numThreads == 0)
    {
        numThreads = std::thread::hardware_concurrency();
    }

    for (std::uint32_t i = 0; i < numThreads; ++i)
    {
        const std::int32_t affinity = numThreads == numHardwareThreads ? static_cast<std::int32_t>(i) : -1;
        auto workerQueue = std::make_unique<impl::WorkerQueue>(this);

        const auto threadOptions = impl::WorkerThread::ThreadOptions{.affinity = affinity};

        m_workerThreads.emplace_back(std::make_unique<impl::WorkerThread>(std::move(workerQueue), i, threadOptions));
    }

    // Start the threads
    for (auto &thread : m_workerThreads)
    {
        thread->Start();
    }
}
Hush::ThreadPool::~ThreadPool()
{
    for (auto &thread : m_workerThreads)
    {
        thread->Stop(impl::WorkerThread::EStopMode::StopImmediately);
    }
}
Hush::TaskOperation Hush::ThreadPool::ScheduleCurrentTask()
{
    return TaskOperation(*this);
}

Hush::Task<void> WrapTask(Hush::ThreadPool& threadPool, Hush::Task<void> function)
{
    co_await threadPool.ScheduleCurrentTask();
    co_await function;
}

Hush::Task<void> Hush::ThreadPool::ScheduleTask(Task<void> &&task)
{
    auto wrapper = WrapTask(*this, std::move(task));

    wrapper.GetCoroutine().resume();

    return std::move(wrapper);
}

void Hush::ThreadPool::WaitUntilDone()
{
    // Wait until all threads are Idle AND the global queue is empty
    bool allThreadsIdle = false;
    bool globalQueueEmpty = false;

    while (!allThreadsIdle || !globalQueueEmpty)
    {
        allThreadsIdle = true;
        globalQueueEmpty = true;

        for (auto &thread : m_workerThreads)
        {
            if (thread->GetState() != impl::WorkerThread::EWorkerThreadState::Idle)
            {
                allThreadsIdle = false;
                break;
            }
        }

        {
            std::lock_guard lock(m_globalQueueMutex);
            globalQueueEmpty = m_globalQueue.empty();
        }

        if (!allThreadsIdle || !globalQueueEmpty)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

Hush::TaskOperation *Hush::ThreadPool::StealFromGlobalQueue()
{
    // Lock the global queue
    std::lock_guard lock(m_globalQueueMutex);

    if (m_globalQueue.empty())
    {
        return nullptr;
    }

    TaskOperation *task = m_globalQueue.back();
    m_globalQueue.pop_back();

    return task;
}

void Hush::ThreadPool::NotifyWorkerThreads()
{
    for (auto &thread : m_workerThreads)
    {
        if (thread->GetState() == impl::WorkerThread::EWorkerThreadState::Idle)
        {
            thread->Notify();
        }
    }
}

void Hush::ThreadPool::PushToGlobalQueue(TaskOperation *task)
{
    // Lock the global queue
    std::lock_guard lock(m_globalQueueMutex);
    m_globalQueue.push_back(task);
}

