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
        explicit ConfigException(const std::string &message, const std::source_location &loc = std::source_location::current());

        [[nodiscard]] const std::source_location &location() const noexcept;

    private:
        static std::string formatMessage(const std::string &msg, const std::source_location &loc);

        std::source_location m_location;
    };

    /**
     * @brief 文件读取异常
     */
    class ConfigFileException : public ConfigException
    {
    public:
        ConfigFileException(const std::string &file_path, const std::string &reason, const std::source_location &loc = std::source_location::current());

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
        ConfigParseException(const std::string &file_path, const std::string &reason, const std::source_location &loc = std::source_location::current());

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
        explicit ConfigKeyNotFoundException(const std::string &key, const std::source_location &loc = std::source_location::current());

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
        ConfigTypeException(const std::string &key, const std::string &expected_type, const std::string &actual_type,
                            const std::source_location &loc = std::source_location::current());

        [[nodiscard]] const std::string &key() const noexcept;

        [[nodiscard]] const std::string &expectedType() const noexcept;

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
        ConfigValidationException(const std::string &key, const std::string &reason, const std::source_location &loc = std::source_location::current());

        [[nodiscard]] const std::string &key() const noexcept;

    private:
        std::string m_key;
    };
} // Base

#endif //EXPECTION_H
