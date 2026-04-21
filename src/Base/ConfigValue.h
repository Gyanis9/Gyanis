/**
 * @file ConfigValue.h
 * @brief 配置值类型封装，提供类型安全的访问接口
 * @copyright Copyright (c) 2026
 */
#ifndef CONFIGVALUE_H
#define CONFIGVALUE_H

#include "ConfigType.h"
#include "Expection.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace Base
{
    // 前向声明
    class ConfigValue;

    // 类型别名
    using ConfigArray = std::vector<ConfigValue>;
    using ConfigObject = std::map<std::string, ConfigValue, std::less<> >;

    /**
     * @brief 配置值类
     *
     * 使用 std::variant 存储多种类型的配置值，提供类型安全的访问接口。
     * 支持以下类型：
     *   - std::nullptr_t (Null)
     *   - bool
     *   - int64_t (Int)
     *   - double (Double)
     *   - std::string (String)
     *   - ConfigArray (Array)
     *   - ConfigObject (Object)
     *
     * 访问方法分为两类：
     *   - as<T>()：强类型转换，类型不匹配时抛出异常
     *   - get<T>()：返回 std::optional<T>，类型不匹配时返回 std::nullopt
     */
    class ConfigValue
    {
    public:
        using VariantType = std::variant<
            std::nullptr_t, ///< Null
            bool,           ///< Bool
            int64_t,        ///< Int
            double,         ///< Double
            std::string,    ///< String
            ConfigArray,    ///< Array
            ConfigObject    ///< Object
        >;

        // ========================================================================
        // 构造函数
        // ========================================================================

        ConfigValue() noexcept;

        explicit ConfigValue(std::nullptr_t) noexcept;

        explicit ConfigValue(bool v) noexcept;

        explicit ConfigValue(int v) noexcept;

        explicit ConfigValue(int64_t v) noexcept;

        explicit ConfigValue(double v) noexcept;

        explicit ConfigValue(const char *v);

        explicit ConfigValue(std::string v) noexcept;

        explicit ConfigValue(ConfigArray v) noexcept;

        explicit ConfigValue(ConfigObject v) noexcept;

        ConfigValue(const ConfigValue &) = default;

        ConfigValue(ConfigValue &&) noexcept = default;

        ConfigValue &operator=(const ConfigValue &) = default;

        ConfigValue &operator=(ConfigValue &&) noexcept = default;

        ~ConfigValue() = default;

        // ========================================================================
        // 类型查询
        // ========================================================================

        /**
         * @brief 获取值的类型
         */
        [[nodiscard]] ConfigValueType type() const noexcept;

        /**
         * @brief 检查是否为指定类型
         */
        template<typename T>
        [[nodiscard]] bool is() const noexcept
        {
            return std::holds_alternative<T>(m_value);
        }

        /**
         * @brief 检查是否为 Null
         */
        [[nodiscard]] bool isNull() const noexcept;

        /**
         * @brief 检查值是否为空（Null 或空字符串/空容器）
         */
        [[nodiscard]] bool empty() const noexcept;

        // ========================================================================
        // 强类型访问（类型不匹配时抛出异常）
        // ========================================================================

        template<typename T>
        [[nodiscard]] const T &as() const
        {
            if (auto *p = std::get_if<T>(&m_value))
            {
                return *p;
            }
            throw ConfigTypeException(
                                      "<unknown>",
                                      typeid(T).name(),
                                      typeName(type())
                                     );
        }

        template<typename T>
        T &as()
        {
            if (auto *p = std::get_if<T>(&m_value))
            {
                return *p;
            }
            throw ConfigTypeException(
                                      "<unknown>",
                                      typeid(T).name(),
                                      typeName(type())
                                     );
        }

        // 便捷类型别名方法
        [[nodiscard]] bool asBool() const;

        [[nodiscard]] int64_t asInt() const;

        [[nodiscard]] double asDouble() const;

        [[nodiscard]] const std::string &asString() const;

        [[nodiscard]] const ConfigArray &asArray() const;

        [[nodiscard]] const ConfigObject &asObject() const;

        // ========================================================================
        // 安全访问（返回 std::optional）
        // ========================================================================

        template<typename T>
        [[nodiscard]] std::optional<T> get() const noexcept
        {
            if (auto *p = std::get_if<T>(&m_value))
            {
                return *p;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<bool> getBool() const noexcept;

        [[nodiscard]] std::optional<int64_t> getInt() const noexcept;

        [[nodiscard]] std::optional<double> getDouble() const noexcept;

        [[nodiscard]] std::optional<std::string> getString() const noexcept;

        [[nodiscard]] std::optional<ConfigArray> getArray() const noexcept;

        [[nodiscard]] std::optional<ConfigObject> getObject() const noexcept;

        // ========================================================================
        // 带默认值的访问
        // ========================================================================

        template<typename T>
        [[nodiscard]] std::decay_t<T> valueOr(T &&default_value) const noexcept
        {
            using ValueType = std::decay_t<T>;
            return get<ValueType>().value_or(ValueType(std::forward<T>(default_value)));
        }

        [[nodiscard]] bool boolOr(bool default_value) const noexcept;

        [[nodiscard]] int64_t intOr(int64_t default_value) const noexcept;

        [[nodiscard]] double doubleOr(double default_value) const noexcept;

        [[nodiscard]] std::string stringOr(const std::string &default_value) const;

        // ========================================================================
        // 对象类型访问（用于嵌套结构）
        // ========================================================================

        /**
         * @brief 判断是否包含指定键（仅当类型为 Object 时有效）
         */
        [[nodiscard]] bool contains(std::string_view key) const noexcept;

        /**
         * @brief 通过键访问子配置值（仅当类型为 Object 时有效）
         */
        const ConfigValue &operator[](std::string_view key) const;

        /**
         * @brief 通过键安全访问子配置值
         */
        [[nodiscard]] std::optional<std::reference_wrapper<const ConfigValue> > get(std::string_view key) const noexcept;

        /**
         * @brief 通过索引访问数组元素（仅当类型为 Array 时有效）
         */
        const ConfigValue &operator[](size_t index) const;

        /**
         * @brief 获取数组大小
         */
        [[nodiscard]] size_t size() const noexcept;

        // ========================================================================
        // 底层 variant 访问
        // ========================================================================

        [[nodiscard]] const VariantType &variant() const noexcept;

        VariantType &variant() noexcept;

    private:
        VariantType m_value;
    };
}

#endif //CONFIGVALUE_H
