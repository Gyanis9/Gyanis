#include <catch2/catch_test_macros.hpp>

#include "core/Hook.h"

namespace Gyanis::core
{
    TEST_CASE("Hook 开关状态可读写", "[core][hook]")
    {
        set_hook_enable(false);
        REQUIRE_FALSE(is_hook_enable());

        set_hook_enable(true);
        REQUIRE(is_hook_enable());

        set_hook_enable(false);
        REQUIRE_FALSE(is_hook_enable());
    }
}
