#include "Expection.h"

#include <sstream>

namespace Base
{
    ConfigException::ConfigException(const std::string &message, const std::source_location &loc) : std::runtime_error(formatMessage(message, loc)), m_location(loc)
    {
    }

    const std::source_location &ConfigException::location() const noexcept
    {
        return m_location;
    }

    std::string ConfigException::formatMessage(const std::string &msg, const std::source_location &loc)
    {
        std::ostringstream oss;
        oss << "[ConfigException] " << msg
                << " [" << loc.file_name() << ":" << loc.line()
                << " in " << loc.function_name() << "]";
        return oss.str();
    }

    ConfigFileException::ConfigFileException(const std::string &file_path, const std::string &reason,
                                             const std::source_location &loc) : ConfigException("File '" + file_path + "': " + reason, loc)
                                                                                , m_file_path(file_path)
    {
    }

    const std::string &ConfigFileException::filePath() const noexcept
    {
        return m_file_path;
    }

    ConfigParseException::ConfigParseException(const std::string &file_path, const std::string &reason,
                                               const std::source_location &loc) : ConfigException("Parse error in '" + file_path + "': " + reason, loc)
                                                                                  , m_file_path(file_path)
    {
    }

    const std::string &ConfigParseException::filePath() const noexcept
    {
        return m_file_path;
    }

    ConfigKeyNotFoundException::ConfigKeyNotFoundException(const std::string &key, const std::source_location &loc) : ConfigException("Configuration key not found: '" + key + "'",
                                                                                                                          loc)
                                                                                                                      , m_key(key)
    {
    }

    const std::string &ConfigKeyNotFoundException::key() const noexcept
    {
        return m_key;
    }

    ConfigTypeException::ConfigTypeException(const std::string &key, const std::string &expected_type, const std::string &actual_type,
                                             const std::source_location &loc) : ConfigException("Type mismatch for key '" + key +
                                                                                                "': expected " + expected_type +
                                                                                                ", got " + actual_type, loc)
                                                                                , m_key(key)
                                                                                , m_expected_type(expected_type)
                                                                                , m_actual_type(actual_type)
    {
    }

    const std::string &ConfigTypeException::key() const noexcept
    {
        return m_key;
    }

    const std::string &ConfigTypeException::expectedType() const noexcept
    {
        return m_expected_type;
    }

    const std::string &ConfigTypeException::actualType() const noexcept
    {
        return m_actual_type;
    }

    ConfigValidationException::ConfigValidationException(const std::string &key, const std::string &reason,
                                                         const std::source_location &loc) : ConfigException("Validation failed for key '" + key + "': " + reason, loc)
                                                                                            , m_key(key)
    {
    }

    const std::string &ConfigValidationException::key() const noexcept
    {
        return m_key;
    }
} // Base
