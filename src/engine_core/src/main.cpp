#include "HushEngine.hpp"

#include <Executor.hpp>
#include <FileSystem.hpp>
#include <Logger.hpp>
#include <VirtualFilesystem.hpp>
#include <filesystem/CFileSystem/CFileSystem.hpp>

Hush::Task<int> func()
{
    co_return 5;
}

Hush::Task<void> ExecutorTask()
{
    uint32_t counter = 0;
    while (counter < 5)
    {
        ++counter;
    }
    co_return;
}

int main()
{
    Hush::ThreadPool threadPool(std::thread::hardware_concurrency());

    auto returnResult = threadPool.ScheduleTask(ExecutorTask());

    auto threadFunction = []() {
        auto currentThreadId = std::this_thread::get_id();
        auto currentThreadIdHash = std::hash<std::thread::id>{}(currentThreadId);
        LogFormat(Hush::ELogLevel::Info, "Thread ID: {}", currentThreadIdHash);
    };

    std::vector<Hush::Task<void>> tasks;

    for (int i = 0; i < 20; ++i)
    {
        tasks.push_back(threadPool.ScheduleFunction(threadFunction));
    }

    threadPool.WaitUntilDone();

    Hush::LogInfo("Finished");

    // executor.Execute(func());

    // Hush::HushEngine engine;
    //
    // engine.Run();
    //
    // engine.Quit();
    return 0;
}
