#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "core/Fiber.h"

TEST_CASE("fiber coroutine yields and resumes", "[core][fiber]")
{
    std::vector<int> steps;

    auto callback = Gyanis::core::Fiber::CoroutineCallback([&steps]() -> Gyanis::core::Fiber::Task
    {
        steps.push_back(1);
        co_await Gyanis::core::Fiber::Suspend();
        steps.push_back(2);
    });

    auto fiber = std::make_shared<Gyanis::core::Fiber>(std::move(callback));

    REQUIRE(fiber->getState() == Gyanis::core::Fiber::INIT);

    fiber->resume();
    REQUIRE(steps == std::vector<int>{1});
    REQUIRE(fiber->getState() == Gyanis::core::Fiber::HOLD);

    fiber->resume();
    REQUIRE(steps == std::vector<int>{1, 2});
    REQUIRE(fiber->getState() == Gyanis::core::Fiber::TERM);
}

TEST_CASE("fiber reset replaces callback", "[core][fiber]")
{
    int value = 0;

    auto callback1 = Gyanis::core::Fiber::CoroutineCallback([&value]() -> Gyanis::core::Fiber::Task
    {
        value = 1;
        co_return;
    });

    auto fiber = std::make_shared<Gyanis::core::Fiber>(std::move(callback1));

    fiber->resume();
    REQUIRE(value == 1);
    REQUIRE(fiber->getState() == Gyanis::core::Fiber::TERM);

    auto callback2 = Gyanis::core::Fiber::CoroutineCallback([&value]() -> Gyanis::core::Fiber::Task
    {
        value = 2;
        co_return;
    });
    fiber->reset(std::move(callback2));

    REQUIRE(fiber->getState() == Gyanis::core::Fiber::INIT);
    fiber->resume();
    REQUIRE(value == 2);
    REQUIRE(fiber->getState() == Gyanis::core::Fiber::TERM);
}
