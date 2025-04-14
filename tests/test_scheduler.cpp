#include <iostream>

#include "core/Scheduler.h"
#include "base/Log.h"

using namespace Gyanis::base;
using namespace Gyanis::core;


void run_fiber()
{
    // LOG_INFO(LOG_ROOT()) << "运行中.....";
}


void test_run()
{
    // 记录开始时间
    auto start = std::chrono::high_resolution_clock::now();

    const auto scheduler = std::make_shared<Scheduler>(12);
    scheduler->start();

    for (int i = 0; i < 1000000; ++i)
    {
        scheduler->schedule(run_fiber);
    }

    scheduler->stop();

    // 记录结束时间
    auto end = std::chrono::high_resolution_clock::now();

    // 计算程序运行的时间差
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 打印运行时长
    std::cout << "test_run executed in " << duration.count() << " milliseconds." << std::endl;
}

int main()
{
    test_run();
}
