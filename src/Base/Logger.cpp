#include "Logger.h"

#include <ranges>

namespace Base
{
    Logger::Logger(std::string name) : m_name(std::move(name))
    {
    }

    Logger::~Logger()
    {
        clearSinks();
    }

    void Logger::log(const LogLevel level, const SourceLocation &location, const std::string_view message) const
    {
        if (!shouldLog(level))
        {
            return;
        }

        const LogEvent event{
            level,
            currentTimestamp(),
            threadIdString(),
            location,
            m_name,
            std::string(message)
        };

        writeToSinks(event);
    }

    void Logger::addSink(std::unique_ptr<LogSink> sink)
    {
        std::unique_lock lock(m_sinks_mutex);
        m_sinks.push_back(std::move(sink));
    }

    void Logger::clearSinks()
    {
        std::unique_lock lock(m_sinks_mutex);
        m_sinks.clear();
    }

    void Logger::setLevel(const LogLevel level)
    {
        m_level.store(level, std::memory_order_release);
    }

    LogLevel Logger::getLevel() const
    {
        return m_level.load(std::memory_order_acquire);
    }

    const std::string &Logger::name() const
    {
        return m_name;
    }

    void Logger::flush() const
    {
        std::shared_lock lock(m_sinks_mutex);
        for (auto &sink: m_sinks)
        {
            if (sink)
            {
                sink->flush();
            }
        }
    }

    bool Logger::shouldLog(const LogLevel level) const
    {
        return level >= getLevel();
    }

    void Logger::writeToSinks(const LogEvent &event) const
    {
        std::shared_lock lock(m_sinks_mutex);
        for (auto &sink: m_sinks)
        {
            if (sink && sink->shouldLog(event.level))
            {
                sink->write(event);
            }
        }
    }

    // ============================================================================
    // LoggerRegistry 实现
    // ============================================================================

    LoggerRegistry &LoggerRegistry::instance()
    {
        static LoggerRegistry instance;
        return instance;
    }

    Logger &LoggerRegistry::getLogger(const std::string &name)
    {
        std::unique_lock lock(m_mutex);
        if (const auto it = m_loggers.find(name); it != m_loggers.end())
        {
            return *it->second;
        }
        auto logger = std::make_unique<Logger>(name);
        Logger &ref = *logger;
        m_loggers[name] = std::move(logger);
        return std::ref(ref);
    }

    Logger &LoggerRegistry::getRootLogger()
    {
        return getLogger("root");
    }

    void LoggerRegistry::registerLogger(std::unique_ptr<Logger> logger)
    {
        if (!logger)
        {
            return;
        }
        std::unique_lock lock(m_mutex);
        m_loggers[logger->name()] = std::move(logger);
    }

    void LoggerRegistry::unregisterLogger(const std::string &name)
    {
        std::unique_lock lock(m_mutex);
        m_loggers.erase(name);
    }

    std::vector<std::string> LoggerRegistry::getLoggerNames() const
    {
        std::shared_lock lock(m_mutex);
        std::vector<std::string> names;
        names.reserve(m_loggers.size());
        for (const auto &key: m_loggers | std::views::keys)
        {
            names.push_back(key);
        }
        return names;
    }

    void LoggerRegistry::clear()
    {
        std::unique_lock lock(m_mutex);
        m_loggers.clear();
    }

    void LoggerRegistry::forEachLogger(const std::function<void(Logger &)> &func) const
    {
        std::shared_lock lock(m_mutex);
        for (const auto &val: m_loggers | std::views::values)
        {
            if (val)
            {
                func(*val);
            }
        }
    }
}
