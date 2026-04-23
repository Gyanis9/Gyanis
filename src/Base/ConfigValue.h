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

        /**
         * @brief 构造空配置值（Null）。
         */
        ConfigValue() noexcept;

        /**
         * @brief 从空指针字面量构造 Null 配置值。
         */
        explicit ConfigValue(std::nullptr_t) noexcept;

        /**
         * @brief 从布尔值构造配置值。
         * @param v 布尔值。
         */
        explicit ConfigValue(bool v) noexcept;

        /**
         * @brief 从整型值构造配置值。
         * @param v 整型值。
         */
        explicit ConfigValue(int v) noexcept;

        /**
         * @brief 从 64 位整型构造配置值。
         * @param v 64 位整型值。
         */
        explicit ConfigValue(int64_t v) noexcept;

        /**
         * @brief 从双精度浮点值构造配置值。
         * @param v 浮点值。
         */
        explicit ConfigValue(double v) noexcept;

        /**
         * @brief 从 C 字符串构造配置值。
         * @param v C 字符串。
         */
        explicit ConfigValue(const char *v);

        /**
         * @brief 从字符串构造配置值。
         * @param v 字符串值。
         */
        explicit ConfigValue(std::string v) noexcept;

        /**
         * @brief 从数组对象构造配置值。
         * @param v 数组值。
         */
        explicit ConfigValue(ConfigArray v) noexcept;

        /**
         * @brief 从对象映射构造配置值。
         * @param v 对象值。
         */
        explicit ConfigValue(ConfigObject v) noexcept;

        /**
         * @brief 获取当前配置值类型。
         * @return ConfigValueType 当前值类型枚举。
         */
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
         * @brief 判断当前值是否为 Null。
         * @return bool 为 Null 时返回 true。
         */
        [[nodiscard]] bool isNull() const noexcept;

        /**
         * @brief 判断值是否为空（Null、空字符串、空数组或空对象）。
         * @return bool 满足空语义时返回 true。
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

        /**
         * @brief 将值转换为布尔类型。
         * @return bool 布尔值。
         */
        [[nodiscard]] bool asBool() const;

        /**
         * @brief 将值转换为整型。
         * @return int64_t 整型值。
         */
        [[nodiscard]] int64_t asInt() const;

        /**
         * @brief 将值转换为浮点类型。
         * @return double 浮点值。
         */
        [[nodiscard]] double asDouble() const;

        /**
         * @brief 将值转换为字符串类型。
         * @return const std::string& 字符串引用。
         */
        [[nodiscard]] const std::string &asString() const;

        /**
         * @brief 将值转换为数组类型。
         * @return const ConfigArray& 数组引用。
         */
        [[nodiscard]] const ConfigArray &asArray() const;

        /**
         * @brief 将值转换为对象类型。
         * @return const ConfigObject& 对象引用。
         */
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

        /**
         * @brief 安全获取布尔值。
         * @return std::optional<bool> 类型匹配时返回值，否则返回空。
         */
        [[nodiscard]] std::optional<bool> getBool() const noexcept;

        /**
         * @brief 安全获取整型值。
         * @return std::optional<int64_t> 类型匹配时返回值，否则返回空。
         */
        [[nodiscard]] std::optional<int64_t> getInt() const noexcept;

        /**
         * @brief 安全获取浮点值。
         * @return std::optional<double> 类型匹配时返回值，否则返回空。
         */
        [[nodiscard]] std::optional<double> getDouble() const noexcept;

        /**
         * @brief 安全获取字符串值。
         * @return std::optional<std::string> 类型匹配时返回值，否则返回空。
         */
        [[nodiscard]] std::optional<std::string> getString() const noexcept;

        /**
         * @brief 安全获取数组值。
         * @return std::optional<ConfigArray> 类型匹配时返回值，否则返回空。
         */
        [[nodiscard]] std::optional<ConfigArray> getArray() const noexcept;

        /**
         * @brief 安全获取对象值。
         * @return std::optional<ConfigObject> 类型匹配时返回值，否则返回空。
         */
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

        /**
         * @brief 获取布尔值，类型不匹配时返回默认值。
         * @param default_value 默认值。
         * @return bool 布尔结果。
         */
        [[nodiscard]] bool boolOr(bool default_value) const noexcept;

        /**
         * @brief 获取整型值，类型不匹配时返回默认值。
         * @param default_value 默认值。
         * @return int64_t 整型结果。
         */
        [[nodiscard]] int64_t intOr(int64_t default_value) const noexcept;

        /**
         * @brief 获取浮点值，类型不匹配时返回默认值。
         * @param default_value 默认值。
         * @return double 浮点结果。
         */
        [[nodiscard]] double doubleOr(double default_value) const noexcept;

        /**
         * @brief 获取字符串值，类型不匹配时返回默认值。
         * @param default_value 默认值。
         * @return std::string 字符串结果。
         */
        [[nodiscard]] std::string stringOr(const std::string &default_value) const;

        // ========================================================================
        // 对象类型访问（用于嵌套结构）
        // ========================================================================

        /**
         * @brief 判断对象中是否包含指定键。
         * @param key 配置键。
         * @return bool 键存在返回 true。
         */
        [[nodiscard]] bool contains(std::string_view key) const noexcept;

        /**
         * @brief 通过键访问对象成员，不存在时抛出异常。
         * @param key 配置键。
         * @return const ConfigValue& 对应配置值引用。
         */
        const ConfigValue &operator[](std::string_view key) const;

        /**
         * @brief 安全通过键访问对象成员。
         * @param key 配置键。
         * @return std::optional<std::reference_wrapper<const ConfigValue>> 键存在时返回值引用。
         */
        [[nodiscard]] std::optional<std::reference_wrapper<const ConfigValue> > get(std::string_view key) const noexcept;

        /**
         * @brief 通过键安全访问子配置值并转换为指定类型
         * @tparam T 目标类型 (bool, int64_t, double, std::string, ConfigArray, ConfigObject)
         * @param key 配置键
         * @return std::optional<T> 如果键存在且类型匹配则返回值，否则返回 std::nullopt
         */
        template<typename T>
        [[nodiscard]] std::optional<T> get(const std::string_view key) const noexcept
        {
            if (const auto opt = get(key))
            {
                return opt->get().get<T>();
            }
            return std::nullopt;
        }

        /**
         * @brief 通过索引访问数组元素，越界时抛出异常。
         * @param index 数组下标。
         * @return const ConfigValue& 对应元素引用。
         */
        const ConfigValue &operator[](size_t index) const;

        /**
         * @brief 获取当前值的大小语义（字符串长度、数组/对象元素数）。
         * @return size_t 语义大小，不适用类型返回 0。
         */
        [[nodiscard]] size_t size() const noexcept;

        // ========================================================================
        // 底层 variant 访问
        // ========================================================================

        /**
         * @brief 获取底层变体常量引用。
         * @return const ConfigValue::VariantType& 变体常量引用。
         */
        [[nodiscard]] const VariantType &variant() const noexcept;

        /**
         * @brief 获取底层变体可变引用。
         * @return ConfigValue::VariantType& 变体可变引用。
         */
        VariantType &variant() noexcept;

    private:
        VariantType m_value;
    };
}

#endif //CONFIGVALUE_H
