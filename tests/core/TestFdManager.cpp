#include <catch2/catch_test_macros.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include "core/FdManager.h"

namespace Gyanis::core
{
    TEST_CASE("FdManager 基础获取与删除行为", "[core][fdmanager]")
    {
        FdManager manager;

        REQUIRE(manager.get(-1) == nullptr);

        constexpr int test_fd = 128;
        auto ctx = manager.get(test_fd, true);
        REQUIRE(ctx != nullptr);

        REQUIRE(manager.get(test_fd) == ctx);

        manager.del(test_fd);
        REQUIRE(manager.get(test_fd) == nullptr);
    }

    TEST_CASE("FdContext 超时设置可回读", "[core][fdmanager]")
    {
        FdContext ctx(-1);

        ctx.setTimeout(SO_RCVTIMEO, 1234);
        ctx.setTimeout(SO_SNDTIMEO, 4321);

        REQUIRE(ctx.getTimeout(SO_RCVTIMEO) == std::chrono::milliseconds(1234));
        REQUIRE(ctx.getTimeout(SO_SNDTIMEO) == std::chrono::milliseconds(4321));
    }
}
