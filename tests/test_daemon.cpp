#include "core/Daemon.h"

#include "base/Log.h"
#include "core/IOManager.h"

using namespace Gyanis::core;

auto g_logger = LOG_ROOT();

int server_main(int argc, char** argv)
{
    LOG_INFO(g_logger) << ProcessInfoMgr::GetInstance()->toString();
    IOManager iom(1);
    const auto timer = iom.addTimer(1000, []()
    {
        LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
        if (++count > 10)
        {
            exit(1);
        }
    }, true);

    LOG_INFO(g_logger) << " id:" << timer;
    return 0;
}

int main(const int argc, char** argv)
{
    return start_daemon(argc, argv, server_main, argc != 1);
}
