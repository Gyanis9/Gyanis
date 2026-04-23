#include "ConfigValue.h"

namespace Base
{
    ConfigValue::ConfigValue() noexcept : m_value(nullptr)
    {
    }

    ConfigValue::ConfigValue(std::nullptr_t) noexcept : m_value(nullptr)
    {
    }

    ConfigValue::ConfigValue(bool v) noexcept : m_value(v)
    {
    }

    ConfigValue::ConfigValue(const int v) noexcept : m_value(static_cast<int64_t>(v))
    {
    }

    ConfigValue::ConfigValue(int64_t v) noexcept : m_value(v)
    {
    }

    ConfigValue::ConfigValue(double v) noexcept : m_value(v)
    {
    }

    ConfigValue::ConfigValue(const char *v) : m_value(std::string(v))
    {
    }

    ConfigValue::ConfigValue(std::string v) noexcept : m_value(std::move(v))
    {
    }

    ConfigValue::ConfigValue(ConfigArray v) noexcept : m_value(std::move(v))
    {
    }

    ConfigValue::ConfigValue(ConfigObject v) noexcept : m_value(std::move(v))
    {
    }

    ConfigValueType ConfigValue::type() const noexcept
    {
        return static_cast<ConfigValueType>(m_value.index());
    }

    bool ConfigValue::isNull() const noexcept
    {
        return is<std::nullptr_t>();
    }

    bool ConfigValue::empty() const noexcept
    {
        return std::visit([]<typename T0>(const T0 &v) -> bool
        {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, std::nullptr_t>)
            {
                return true;
            } else if constexpr (std::is_same_v<T, std::string>)
            {
                return v.empty();
            } else if constexpr (std::is_same_v<T, ConfigArray>)
            {
                return v.empty();
            } else if constexpr (std::is_same_v<T, ConfigObject>)
            {
                return v.empty();
            } else
            {
                return false;
            }
        }, m_value);
    }

    bool ConfigValue::asBool() const
    {
        return as<bool>();
    }

    int64_t ConfigValue::asInt() const
    {
        return as<int64_t>();
    }

    double ConfigValue::asDouble() const
    {
        return as<double>();
    }

    const std::string &ConfigValue::asString() const
    {
        return as<std::string>();
    }

    const ConfigArray &ConfigValue::asArray() const
    {
        return as<ConfigArray>();
    }

    const ConfigObject &ConfigValue::asObject() const
    {
        return as<ConfigObject>();
    }

    std::optional<bool> ConfigValue::getBool() const noexcept
    {
        return get<bool>();
    }

    std::optional<int64_t> ConfigValue::getInt() const noexcept
    {
        return get<int64_t>();
    }

    std::optional<double> ConfigValue::getDouble() const noexcept
    {
        return get<double>();
    }

    std::optional<std::string> ConfigValue::getString() const noexcept
    {
        return get<std::string>();
    }

    std::optional<ConfigArray> ConfigValue::getArray() const noexcept
    {
        return get<ConfigArray>();
    }

    std::optional<ConfigObject> ConfigValue::getObject() const noexcept
    {
        return get<ConfigObject>();
    }

    bool ConfigValue::boolOr(bool default_value) const noexcept
    {
        return valueOr(default_value);
    }

    int64_t ConfigValue::intOr(int64_t default_value) const noexcept
    {
        return valueOr(default_value);
    }

    double ConfigValue::doubleOr(double default_value) const noexcept
    {
        return valueOr(default_value);
    }

    std::string ConfigValue::stringOr(const std::string &default_value) const
    {
        return valueOr(default_value);
    }

    bool ConfigValue::contains(const std::string_view key) const noexcept
    {
        if (!is<ConfigObject>())
        {
            return false;
        }
        const auto &obj = std::get<ConfigObject>(m_value);
        return obj.contains(key);
    }

    const ConfigValue &ConfigValue::operator[](const std::string_view key) const
    {
        const auto &obj = as<ConfigObject>();
        const auto it = obj.find(key);
        if (it == obj.end())
        {
            throw ConfigKeyNotFoundException(
                                             std::string(key)
                                            );
        }
        return it->second;
    }

    std::optional<std::reference_wrapper<const ConfigValue> > ConfigValue::get(const std::string_view key) const noexcept
    {
        if (!is<ConfigObject>())
        {
            return std::nullopt;
        }
        const auto &obj = std::get<ConfigObject>(m_value);
        const auto it = obj.find(key);
        if (it == obj.end())
        {
            return std::nullopt;
        }
        return std::cref(it->second);
    }

    const ConfigValue &ConfigValue::operator[](const size_t index) const
    {
        const auto &arr = as<ConfigArray>();
        if (index >= arr.size())
        {
            throw ConfigKeyNotFoundException(
                                             "[" + std::to_string(index) + "]"
                                            );
        }
        return arr[index];
    }

    size_t ConfigValue::size() const noexcept
    {
        if (is<ConfigArray>())
        {
            return std::get<ConfigArray>(m_value).size();
        }
        if (is<ConfigObject>())
        {
            return std::get<ConfigObject>(m_value).size();
        }
        if (is<std::string>())
        {
            return std::get<std::string>(m_value).size();
        }
        return 0;
    }

    const ConfigValue::VariantType &ConfigValue::variant() const noexcept
    {
        return m_value;
    }

    ConfigValue::VariantType &ConfigValue::variant() noexcept
    {
        return m_value;
    }
}
