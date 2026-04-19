#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "base/JsonUtils.h"

TEST_CASE("json utils escape identifies control characters", "[base][json][escape]")
{
    REQUIRE(Gyanis::base::JsonUtils::NeedEscape("line\nfeed"));
    REQUIRE_FALSE(Gyanis::base::JsonUtils::NeedEscape("plain_text"));

    const auto escaped = Gyanis::base::JsonUtils::Escape("tab\tquote\"");
    REQUIRE(escaped == "tab\\tquote\\\"");
}

TEST_CASE("json utils integer getters accept signed and unsigned numbers", "[base][json][number]")
{
    const nlohmann::json input = {
        {"u32_from_signed", 12},
        {"u32_negative", -3},
        {"i64_from_unsigned", static_cast<uint64_t>(33)}
    };

    REQUIRE(input["u32_from_signed"].is_number_integer());

    REQUIRE(Gyanis::base::JsonUtils::GetUint32(input, "u32_from_signed", 0) == 12u);
    REQUIRE(Gyanis::base::JsonUtils::GetUint32(input, "u32_negative", 7u) == 7u);
    REQUIRE(Gyanis::base::JsonUtils::GetInt64(input, "i64_from_unsigned", -1) == 33);
}

TEST_CASE("json utils parse and dump preserve content", "[base][json][parse]")
{
    const std::string text = R"({"name":"svc","port":8080})";
    nlohmann::json json;

    REQUIRE(Gyanis::base::JsonUtils::FromString(json, text));
    REQUIRE(Gyanis::base::JsonUtils::GetString(json, "name", "") == "svc");
    REQUIRE(Gyanis::base::JsonUtils::GetInt32(json, "port", 0) == 8080);

    const auto dumped = Gyanis::base::JsonUtils::ToString(json);
    REQUIRE_FALSE(dumped.empty());
}
