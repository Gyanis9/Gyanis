#include <catch2/catch_test_macros.hpp>

#include "core/IOManager.h"

namespace Gyanis::core
{
    TEST_CASE("IOManager 无效 fd 操作返回失败", "[core][iomanager]")
    {
        IOManager io_manager(1, "io_test");

        REQUIRE(io_manager.addEvent(-1, IOManager::READ) == -1);
        REQUIRE_FALSE(io_manager.delEvent(-1, IOManager::READ));
        REQUIRE_FALSE(io_manager.cancelEvent(-1, IOManager::READ));
        REQUIRE_FALSE(io_manager.cancelAll(-1));
    }
}
