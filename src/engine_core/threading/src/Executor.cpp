/*! \file Executor.cpp
    \author Alan Ramirez
    \date 2024-12-25
    \brief Executor class implementation
*/
#include "Executor.hpp"
#include "Job.hpp"

#include <mutex>
#include <thread>

Hush::Executor::Executor(std::uint32_t numThreads)
{
    if (numThreads == 0)
    {
        // Get the number of hardware threads
        numThreads = std::thread::hardware_concurrency();
    }

    CreateThreads(numThreads);
}

Hush::Executor::Executor()
{
    // Get the number of hardware threads
    std::uint32_t numThreads = std::thread::hardware_concurrency();

    if (numThreads == 1)
    {
        numThreads = 4;
    }

    CreateThreads(numThreads);
}

Hush::Executor::TaskHandle Hush::Executor::execute(std::coroutine_handle<> task)
{
    // Push the task to any of the worker threads
    auto distribution = std::uniform_int_distribution<std::uint32_t>(0, m_jobQueues.size() - 1);
    std::uint32_t selectedQueueIndex = distribution(m_randomDevice);

    // Lock the selected queue
    auto& selectedQueue = m_jobQueues[selectedQueueIndex];
    {
        // Create the TaskHandle
        TaskHandle taskHandle {};

        std::lock_guard lock(selectedQueue.mutex);
        selectedQueue.tasks.push_back(task);
    }
}

void Hush::Executor::CreateThreads(std::uint32_t numThreads)
{
    for (std::uint32_t i = 0; i < numThreads; ++i)
    {
        JobQueue& queue = m_jobQueues.emplace_back();
        queue.randomEngine.seed(m_randomDevice());


        m_threads.emplace_back([this, i](const std::stop_token &stopToken) {
            ThreadFunction(stopToken, i);
        });
    }
}

void Hush::Executor::ThreadFunction(std::stop_token stopToken, std::uint32_t threadIndex)
{
    // Get the job queue for this thread.
    JobQueue& jobQueue = m_jobQueues[threadIndex];

    while (!stopToken.stop_requested())
    {
        std::coroutine_handle<> task;
        {
            std::lock_guard lock(jobQueue.mutex);
            if (!jobQueue.tasks.empty())
            {
                task = jobQueue.tasks.front();
                jobQueue.tasks.pop_front();
            }
            else
            {
                task = StealTask(threadIndex);
            }
        }

        if (task)
        {
            task.resume();
        }
    }
}

std::coroutine_handle<> Hush::Executor::StealTask(uint32_t currentThreadIndex)
{
    // Get a random job queue to steal a task from.
    auto distribution = std::uniform_int_distribution<std::uint32_t>(0, m_jobQueues.size() - 1);

    std::uint32_t selectedQueueIndex = distribution(m_jobQueues[currentThreadIndex].randomEngine);

    // Make sure the selected queue is not the same as the current thread's queue.
    while (selectedQueueIndex == currentThreadIndex)
    {
        selectedQueueIndex = distribution(m_jobQueues[currentThreadIndex].randomEngine);
    }

    // Steal from the selected queue
    JobQueue& selectedQueue = m_jobQueues[selectedQueueIndex];

    std::lock_guard lock(selectedQueue.mutex);
    // Ensure the selected queue is not empty
    if (selectedQueue.tasks.empty())
    {
        return nullptr;
    }
    std::coroutine_handle<> task = selectedQueue.tasks.back();
    selectedQueue.tasks.pop_back();

    return task;
}
