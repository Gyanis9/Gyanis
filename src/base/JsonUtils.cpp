#include <unordered_set>

#include "base/JsonUtils.h"
#include "base/Log.h"

namespace Gyanis::base
{
    static auto global_system_logger = LOG_NAME("system");

    bool JsonUtils::NeedEscape(const std::string& value)
    {
        static const std::unordered_set escape_chars = {'\f', '\t', '\r', '\n', '\b', '"', '\\'};
        return std::any_of(value.begin(), value.end(), [](const char c)
        {
            return escape_chars.find(c) != escape_chars.end();
        });
    }

    std::string JsonUtils::Escape(const std::string& value)
    {
        size_t size = 0;
        for (const auto& c : value)
        {
            switch (c)
            {
            case '\f':
            case '\t':
            case '\r':
            case '\n':
            case '\b':
            case '"':
            case '\\':
                size += 2;
                break;
            default:
                size += 1;
                break;
            }
        }

        if (size == value.size())
        {
            return value;
        }

        std::string result;
        result.reserve(size);

        for (const auto& c : value)
        {
            switch (c)
            {
            case '\f':
                result.append("\\f");
                break;
            case '\t':
                result.append("\\t");
                break;
            case '\r':
                result.append("\\r");
                break;
            case '\n':
                result.append("\\n");
                break;
            case '\b':
                result.append("\\b");
                break;
            case '"':
                result.append("\\\"");
                break;
            case '\\':
                result.append("\\\\");
                break;
            default:
                result.push_back(c);
                break;
            }
        }
        return result;
    }

    std::string JsonUtils::GetString(const nlohmann::json& json
                                     , const std::string& name
                                     , const std::string& default_value)
    {
        if (json.contains(name) && json[name].is_string())
        {
            return json[name].get<std::string>();
        }
        return default_value;
    }

    double JsonUtils::GetDouble(const nlohmann::json& json
                                , const std::string& name
                                , const double default_value)
    {
        if (json.contains(name) && json[name].is_number())
        {
            return json[name].get<double>();
        }
        return default_value;
    }

    int32_t JsonUtils::GetInt32(const nlohmann::json& json
                                , const std::string& name
                                , const int32_t default_value)
    {
        if (json.contains(name) && json[name].is_number_integer())
        {
            return json[name].get<int32_t>();
        }
        return default_value;
    }

    uint32_t JsonUtils::GetUint32(const nlohmann::json& json
                                  , const std::string& name
                                  , const uint32_t default_value)
    {
        if (json.contains(name) && json[name].is_number_unsigned())
        {
            return json[name].get<uint32_t>();
        }
        return default_value;
    }

    int64_t JsonUtils::GetInt64(const nlohmann::json& json
                                , const std::string& name
                                , const int64_t default_value)
    {
        if (json.contains(name) && json[name].is_number_integer())
        {
            return json[name].get<int64_t>();
        }
        return default_value;
    }

    uint64_t JsonUtils::GetUint64(const nlohmann::json& json
                                  , const std::string& name
                                  , const uint64_t default_value)
    {
        if (json.contains(name) && json[name].is_number_unsigned())
        {
            return json[name].get<uint64_t>();
        }
        return default_value;
    }

    bool JsonUtils::FromString(nlohmann::json& json, const std::string& value)
    {
        try
        {
            json = nlohmann::json::parse(value);
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(LOG_ROOT()) << "JsonUtils::FromString - fail: " << e.what()
                                  << " | Status: Invalid";
            return false;
        }
    }

    std::string JsonUtils::ToString(const nlohmann::json& json)
    {
        return json.dump();
    }
}
