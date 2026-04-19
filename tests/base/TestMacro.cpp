#include <catch2/catch_test_macros.hpp>

#include <source_location>
#include <string>

#include "base/Macro.h"

TEST_CASE("likely and unlikely keep expression semantics", "[base][macro][branch]")
{
    int value = 0;

    const bool likely_result = LIKELY(++value == 1);
    const bool unlikely_result = UNLIKELY(++value == 2);

    REQUIRE(likely_result);
    REQUIRE(unlikely_result);
    REQUIRE(value == 2);
}

TEST_CASE("assert helper builds chinese failure message", "[base][macro][assert]")
{
    const auto message = Gyanis::base::detail::BuildAssertFailureMessage(
        "v > 0",
        "自定义提示",
        std::source_location::current());

    REQUIRE(message.find("断言失败") != std::string::npos);
    REQUIRE(message.find("表达式: v > 0") != std::string::npos);
    REQUIRE(message.find("自定义提示") != std::string::npos);
}

TEST_CASE("assert macros do not abort on true condition", "[base][macro][assert]")
{
    ASSERT(true);
    ASSERT_MSG(1 + 1 == 2, "不会触发断言");

    SUCCEED();
}
