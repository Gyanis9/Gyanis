#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "base/Config.h"
#include "base/Utils.h"

TEST_CASE("config fromString reports success when parsing valid scalar", "[base][config]")
{
    auto value = Gyanis::base::Config::LookUp<int>("test.config.from_string", 0, "unit test");

    REQUIRE(value != nullptr);
    REQUIRE(value->fromString("42"));
    REQUIRE(value->getValue() == 42);
}

TEST_CASE("url encode uses hexadecimal percent-encoding", "[base][utils][url]")
{
    const std::string raw = "a/b c";

    const std::string encoded = Gyanis::base::StringUtil::UrlEncode(raw, true);

    REQUIRE(encoded == "a%2Fb+c");
    REQUIRE(Gyanis::base::StringUtil::UrlDecode(encoded, true) == raw);
}

TEST_CASE("json to yaml supports numeric and boolean values", "[base][utils][json]")
{
    const nlohmann::json input = {
        {"port", 8080},
        {"ssl", false},
        {"name", "svc"}
    };

    YAML::Node output;

    REQUIRE(Gyanis::base::JsonToYaml(input, output));
    REQUIRE(output["port"].as<int>() == 8080);
    REQUIRE(output["ssl"].as<bool>() == false);
    REQUIRE(output["name"].as<std::string>() == "svc");
}
