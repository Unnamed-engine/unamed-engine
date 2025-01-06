/*! \file ThreadPool.test.cpp
    \author Alan Ramirez
    \date 2025-01-04
    \brief ThreadPool tests
*/

#include "ThreadPool.hpp"

#include <Logger.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <random>

TEST_CASE("Create ThreadPool")
{
    Hush::ThreadPool threadPool(1);

    SECTION("GetNumThreads")
    {
        REQUIRE(threadPool.GetNumThreads() == 1);
    }
}

TEST_CASE("WaitUntilDone")
{
    Hush::ThreadPool threadPool(4);
    threadPool.Start();

    SECTION("WaitUntilDone")
    {
        // Arrange
        std::atomic<std::uint32_t> counter = 0;
        auto threadFunction = [&counter]() -> Hush::Task<void> {
            counter.fetch_add(1);
            co_return;
        };

        std::vector<Hush::Task<void>> tasks;

        for (int i = 0; i < 20; ++i)
        {
            tasks.push_back(threadPool.ScheduleTask(threadFunction()));
        }

        // Act
        threadPool.WaitUntilDone();

        // Assert
        REQUIRE(counter.load() == 20);
    }
}

TEST_CASE("ScheduleFunction")
{
    Hush::ThreadPool threadPool(1);
    threadPool.Start();

    SECTION("ScheduleFunction")
    {
        // Arrange
        auto threadFunction = []() -> Hush::Task<void> {
            Hush::LogInfo("Hello from thread");
            co_return;
        };

        // Act
        auto task = threadPool.ScheduleTask(threadFunction());

        threadPool.WaitUntilDone();

        // Assert
        REQUIRE(task.GetCoroutine().done());
    }

    SECTION("ScheduleFunction lambda with captures")
    {
        // Arrange
        std::uint32_t result = 0;
        std::uint32_t a = 10;
        std::uint32_t b = 5;
        // Print address of a and b

        auto threadFunction = [&]() -> Hush::Task<void> {
            // Print address of a and b
            result = a + b;

            co_return;
        };
        // Act
        auto task = threadPool.ScheduleTask(threadFunction());
        threadPool.WaitUntilDone();

        // Assert
        REQUIRE(task.GetCoroutine().done());
        REQUIRE(result == 15);
    }
}
