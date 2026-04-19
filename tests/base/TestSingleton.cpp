#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include "base/Singleton.h"

namespace
{
    class SingletonProbe
    {
    public:
        SingletonProbe()
        {
            ++constructionCount;
        }

        static std::atomic<int> constructionCount;
    };

    std::atomic<int> SingletonProbe::constructionCount{0};

    using SingletonProbeMgr = Singleton<SingletonProbe>;
}

TEST_CASE("singleton exposes stable reference API", "[base][singleton]")
{
    auto &first  = SingletonProbeMgr::GetReference();
    auto &second = SingletonProbeMgr::GetReference();

    REQUIRE(&first == &second);
    REQUIRE(&first == SingletonProbeMgr::GetInstance());
}

TEST_CASE("singleton initializes only once under contention", "[base][singleton]")
{
    constexpr size_t kThreadCount = 16;

    std::vector<SingletonProbe *> instances(kThreadCount, nullptr);
    std::vector<std::thread>      workers;
    workers.reserve(kThreadCount);

    for (size_t i = 0; i < kThreadCount; ++i)
    {
        workers.emplace_back([&instances, i]
        {
            instances[i] = SingletonProbeMgr::GetInstance();
        });
    }

    for (auto &worker: workers)
    {
        worker.join();
    }

    const SingletonProbe *first = instances.front();
    REQUIRE(first != nullptr);

    for (const SingletonProbe *instance: instances)
    {
        REQUIRE(instance == first);
    }

    REQUIRE(SingletonProbe::constructionCount.load() == 1);
}
