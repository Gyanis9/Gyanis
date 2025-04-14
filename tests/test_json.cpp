#include "base/JsonUtils.h"
#include "base/Log.h"

#include <iostream>
#include <nlohmann/json.hpp>

int main()
{
    const std::string test_str = "Hello\nWorld\"\\";
    if (Gyanis::base::JsonUtils::NeedEscape(test_str))
    {
        LOG_INFO(LOG_ROOT()) << "The string contains characters that need to be escaped.";
    }
    else
    {
        LOG_INFO(LOG_ROOT()) << "The string does not contain any characters that need to be escaped.";
    }

    const std::string escaped_str = Gyanis::base::JsonUtils::Escape(test_str);
    LOG_INFO(LOG_ROOT()) << "Escaped string: " << escaped_str << "";

    const auto test_json = R"({"name": "John", "age": 30})"_json;
    const std::string name = Gyanis::base::JsonUtils::GetString(test_json, "name", "Default");
    LOG_INFO(LOG_ROOT()) << "Name: " << name << "";

    const int32_t age = Gyanis::base::JsonUtils::GetInt32(test_json, "age", 0);
    LOG_INFO(LOG_ROOT()) << "Age: " << age << "";

    nlohmann::json parsed_json;
    if (const bool success = Gyanis::base::JsonUtils::FromString(parsed_json,
                                                           R"({"city": "New York", "population": 8000000})"); success)
    {
        LOG_INFO(LOG_ROOT()) << "Parsed JSON: " << parsed_json.dump() << "";
    }
    else
    {
        LOG_INFO(LOG_ROOT()) << "Failed to parse the string to JSON.";
    }

    const std::string json_str = Gyanis::base::JsonUtils::ToString(parsed_json);
    LOG_INFO(LOG_ROOT()) << "JSON string: " << json_str << "";

    return 0;
}
