#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "base/Timer.h"

namespace
{
    class TestTimerManager final : public Gyanis::base::TimerManager
    {
    public:
        void drainAndRun()
        {
            std::vector<std::function<void()> > callbacks;
            ListExpiredCb(callbacks);
            for (auto &cb: callbacks)
            {
                cb();
            }
        }

    protected:
        void onTimerInsertedAtFront() override
        {
            ++m_front_inserted;
        }

    private:
        std::atomic<uint32_t> m_front_inserted{0};
    };
}

TEST_CASE("timer reset updates recurring interval for next cycle", "[base][timer][reset]")
{
    using namespace std::chrono_literals;

    TestTimerManager manager;
    std::atomic<int> trigger_count{0};

    const auto timer_id = manager.addTimer(10, [&trigger_count]
    {
        ++trigger_count;
    }, true);

    REQUIRE(manager.reset(timer_id, 80, true));

    std::this_thread::sleep_for(90ms);
    manager.drainAndRun();
    REQUIRE(trigger_count.load() == 1);

    std::this_thread::sleep_for(25ms);
    manager.drainAndRun();
    REQUIRE(trigger_count.load() == 1);
}

TEST_CASE("condition timer skips callback when condition expired", "[base][timer][condition]")
{
    using namespace std::chrono_literals;

    TestTimerManager manager;
    std::atomic<int> trigger_count{0};

    std::weak_ptr<void> weak_condition;
    {
        auto condition = std::make_shared<int>(1);
        weak_condition = condition;
    }

    manager.addConditionTimer(10, [&trigger_count]
    {
        ++trigger_count;
    }, weak_condition, false);

    std::this_thread::sleep_for(20ms);
    manager.drainAndRun();

    REQUIRE(trigger_count.load() == 0);
}
