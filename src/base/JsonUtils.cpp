#include <algorithm>
#include <concepts>
#include <ranges>
#include <unordered_set>
#include <utility>

#include "base/JsonUtils.h"
#include "base/Log.h"

namespace Gyanis::base
{
    namespace
    {
        template<std::integral T>
        [[nodiscard]] T GetIntegralOrDefault(const nlohmann::json &json,
                                             const std::string &   name,
                                             const T               default_value)
        {
            const auto it = json.find(name);
            if (it == json.end())
            {
                return default_value;
            }

            if (!(it->is_number_integer() || it->is_number_unsigned()))
            {
                return default_value;
            }

            if constexpr (std::signed_integral<T>)
            {
                if (it->is_number_integer())
                {
                    const auto value = it->get<int64_t>();
                    return std::in_range<T>(value) ? static_cast<T>(value) : default_value;
                }

                const auto value = it->get<uint64_t>();
                return std::in_range<T>(value) ? static_cast<T>(value) : default_value;
            } else
            {
                if (it->is_number_unsigned())
                {
                    const auto value = it->get<uint64_t>();
                    return std::in_range<T>(value) ? static_cast<T>(value) : default_value;
                }

                const auto value = it->get<int64_t>();
                if (value < 0)
                {
                    return default_value;
                }
                return std::in_range<T>(value) ? static_cast<T>(value) : default_value;
            }
        }
    }

    static auto g_logger = LOG_NAME("system");

    bool JsonUtils::NeedEscape(const std::string &value)
    {
        static const std::unordered_set<char> escape_chars = {'\f', '\t', '\r', '\n', '\b', '"', '\\'};
        return std::ranges::any_of(value, [](const char c)
        {
            return escape_chars.contains(c);
        });
    }

    std::string JsonUtils::Escape(const std::string &value)
    {
        size_t size = 0;
        for (const auto c: value)
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

        for (const auto c: value)
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

    std::string JsonUtils::GetString(const nlohmann::json &json
                                     , const std::string & name
                                     , const std::string & default_value)
    {
        if (const auto it = json.find(name); it != json.end() && it->is_string())
        {
            return it->get<std::string>();
        }
        return default_value;
    }

    double JsonUtils::GetDouble(const nlohmann::json &json
                                , const std::string & name
                                , const double        default_value)
    {
        if (const auto it = json.find(name); it != json.end() && it->is_number())
        {
            return it->get<double>();
        }
        return default_value;
    }

    int32_t JsonUtils::GetInt32(const nlohmann::json &json
                                , const std::string & name
                                , const int32_t       default_value)
    {
        return GetIntegralOrDefault<int32_t>(json, name, default_value);
    }

    uint32_t JsonUtils::GetUint32(const nlohmann::json &json
                                  , const std::string & name
                                  , const uint32_t      default_value)
    {
        return GetIntegralOrDefault<uint32_t>(json, name, default_value);
    }

    int64_t JsonUtils::GetInt64(const nlohmann::json &json
                                , const std::string & name
                                , const int64_t       default_value)
    {
        return GetIntegralOrDefault<int64_t>(json, name, default_value);
    }

    uint64_t JsonUtils::GetUint64(const nlohmann::json &json
                                  , const std::string & name
                                  , const uint64_t      default_value)
    {
        return GetIntegralOrDefault<uint64_t>(json, name, default_value);
    }

    bool JsonUtils::FromString(nlohmann::json &json, const std::string &value)
    {
        try
        {
            json = nlohmann::json::parse(value);
            return true;
        } catch (const std::exception &e)
        {
            LOG_ERROR(g_logger) << "[JSON] 字符串解析失败"
                                << " | 错误: " << e.what();
            return false;
        }
    }

    std::string JsonUtils::ToString(const nlohmann::json &json)
    {
        return json.dump();
    }
}
