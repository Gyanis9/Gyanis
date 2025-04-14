#include "base/Timer.h"
#include "base/Log.h"
using namespace Gyanis::base;


class temp final : public TimerManager
{
public:
    void onTimerInsertedAtFront() override
    {
    }
};

void test_timer()
{
    const auto timer = std::make_shared<temp>();
    const auto id = timer->addTimer(1, []()
    {
        static int num = 0;
        LOG_INFO(LOG_ROOT()) << "第: " << num++ << "次定时";
    }, true);
    LOG_INFO(LOG_ROOT()) << "定时器ID:" << id;
}


int main(int argc, char* argv[])
{
    test_timer();
}
