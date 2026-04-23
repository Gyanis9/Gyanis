/**
 * @file Expection.h
 * @brief 配置模块异常类型声明。
 */

#ifndef EXPECTION_H
#define EXPECTION_H

#include <source_location>
#include <stdexcept>

namespace Base
{
    /**
     * @brief 配置模块基础异常类
     */
    class ConfigException : public std::runtime_error
    {
    public:
        /**
         * @brief 构造配置基础异常并拼接位置信息。
         * @param message 异常消息。
         * @param loc 触发异常的源码位置信息。
         */
        explicit ConfigException(const std::string &message, const std::source_location &loc = std::source_location::current());

        /**
         * @brief 获取异常记录的源码位置。
         * @return const std::source_location& 源码位置信息引用。
         */
        [[nodiscard]] const std::source_location &location() const noexcept;

    private:
        /**
         * @brief 生成包含文件、行号和函数名的完整异常消息。
         * @param msg 原始异常描述。
         * @param loc 源码位置信息。
         * @return std::string 格式化后的完整异常消息。
         */
        static std::string formatMessage(const std::string &msg, const std::source_location &loc);

        std::source_location m_location;
    };

    /**
     * @brief 文件读取异常
     */
    class ConfigFileException : public ConfigException
    {
    public:
        /**
         * @brief 构造配置文件异常。
         * @param file_path 出错文件路径。
         * @param reason 失败原因。
         * @param loc 源码位置信息。
         */
        ConfigFileException(const std::string &file_path, const std::string &reason, const std::source_location &loc = std::source_location::current());

        /**
         * @brief 获取异常对应的配置文件路径。
         * @return const std::string& 文件路径引用。
         */
        [[nodiscard]] const std::string &filePath() const noexcept;

    private:
        std::string m_file_path;
    };

    /**
     * @brief YAML 解析异常
     */
    class ConfigParseException : public ConfigException
    {
    public:
        /**
         * @brief 构造配置解析异常。
         * @param file_path 出错文件路径。
         * @param reason 解析失败原因。
         * @param loc 源码位置信息。
         */
        ConfigParseException(const std::string &file_path, const std::string &reason, const std::source_location &loc = std::source_location::current());

        /**
         * @brief 获取解析异常关联的文件路径。
         * @return const std::string& 文件路径引用。
         */
        [[nodiscard]] const std::string &filePath() const noexcept;

    private:
        std::string m_file_path;
    };

    /**
     * @brief 配置键未找到异常
     */
    class ConfigKeyNotFoundException : public ConfigException
    {
    public:
        /**
         * @brief 构造配置键缺失异常。
         * @param key 缺失的配置键。
         * @param loc 源码位置信息。
         */
        explicit ConfigKeyNotFoundException(const std::string &key, const std::source_location &loc = std::source_location::current());

        /**
         * @brief 获取缺失的配置键。
         * @return const std::string& 配置键引用。
         */
        [[nodiscard]] const std::string &key() const noexcept;

    private:
        std::string m_key;
    };

    /**
     * @brief 类型转换异常
     */
    class ConfigTypeException : public ConfigException
    {
    public:
        /**
         * @brief 构造配置类型不匹配异常。
         * @param key 出错的配置键。
         * @param expected_type 期望类型名称。
         * @param actual_type 实际类型名称。
         * @param loc 源码位置信息。
         */
        ConfigTypeException(const std::string &key, const std::string &expected_type, const std::string &actual_type,
                            const std::source_location &loc = std::source_location::current());

        /**
         * @brief 获取类型不匹配异常中的配置键。
         * @return const std::string& 配置键引用。
         */
        [[nodiscard]] const std::string &key() const noexcept;

        /**
         * @brief 获取期望类型名称。
         * @return const std::string& 期望类型字符串引用。
         */
        [[nodiscard]] const std::string &expectedType() const noexcept;

        /**
         * @brief 获取实际类型名称。
         * @return const std::string& 实际类型字符串引用。
         */
        [[nodiscard]] const std::string &actualType() const noexcept;

    private:
        std::string m_key;
        std::string m_expected_type;
        std::string m_actual_type;
    };

    /**
     * @brief 配置验证异常
     */
    class ConfigValidationException : public ConfigException
    {
    public:
        /**
         * @brief 构造配置验证失败异常。
         * @param key 验证失败的配置键。
         * @param reason 验证失败原因。
         * @param loc 源码位置信息。
         */
        ConfigValidationException(const std::string &key, const std::string &reason, const std::source_location &loc = std::source_location::current());

        /**
         * @brief 获取验证失败的配置键。
         * @return const std::string& 配置键引用。
         */
        [[nodiscard]] const std::string &key() const noexcept;

    private:
        std::string m_key;
    };
} // Base

#endif //EXPECTION_H
