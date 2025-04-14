#include "core/Fiber.h"
#include "base/Log.h"

using namespace Gyanis::core;
using namespace Gyanis::base;

void test_fiber()
{
    LOG_INFO(LOG_ROOT()) << "running test_fiber";
}


void test_run()
{
    const auto fiber = std::make_shared<Fiber>(test_fiber);
    fiber->resume();
}

int main()
{
    test_run();
}
