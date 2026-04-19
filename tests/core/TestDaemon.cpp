#include <catch2/catch_test_macros.hpp>

#include "core/Daemon.h"

namespace Gyanis::core
{
    TEST_CASE("ProcessInfo 可转换为可读字符串", "[core][daemon]")
    {
        ProcessInfo info{};
        info.parent_id = 100;
        info.main_id = 200;
        info.restart_count = 3;

        const auto text = info.toString();
        REQUIRE_FALSE(text.empty());
        REQUIRE(text.find("100") != std::string::npos);
        REQUIRE(text.find("200") != std::string::npos);
        REQUIRE(text.find("3") != std::string::npos);
    }

    TEST_CASE("start_daemon 前台模式执行回调", "[core][daemon]")
    {
        bool called = false;
        char app_name[] = "test_app";
        char *argv[] = {app_name, nullptr};

        const int ret = start_daemon(1, argv, [&called](int argc, char **args)
        {
            called = true;
            REQUIRE(argc == 1);
            REQUIRE(args != nullptr);
            return 42;
        }, false);

        REQUIRE(called);
        REQUIRE(ret == 42);
    }
}
