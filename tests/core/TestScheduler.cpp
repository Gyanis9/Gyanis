#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "core/Scheduler.h"

TEST_CASE("scheduler executes callback task", "[core][scheduler]")
{
    using namespace std::chrono_literals;

    Gyanis::core::Scheduler scheduler(1, "test_scheduler");

    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> done = false;

    scheduler.start();

    scheduler.schedule([&]()
    {
        done.store(true, std::memory_order_release);
        std::scoped_lock lock(mutex);
        condition.notify_one();
    });

    std::unique_lock lock(mutex);
    const bool finished = condition.wait_for(lock, 1000ms, [&]()
    {
        return done.load(std::memory_order_acquire);
    });

    scheduler.stop();

    REQUIRE(finished);
}
