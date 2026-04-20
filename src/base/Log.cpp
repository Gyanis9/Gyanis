#include <iostream>
#include <cstdarg>
#include <yaml-cpp/yaml.h>
#include <memory>
#include <filesystem>
#include <functional>

#include "base/Log.h"

namespace Gyanis::base
{
    const char *LogLevel::ToString(const Level level)
    {
        static const std::unordered_map<Level, const char *> levelToString = {
                {DEBUG, "DEBUG"},
                {INFO, "INFO"},
                {WARN, "WARN"},
                {ERROR, "ERROR"},
                {FATAL, "FATAL"}
        };

        if (const auto it = levelToString.find(level); it != levelToString.end())
        {
            return it->second;
        }
        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
        static const std::unordered_map<std::string, Level> strToLevel = {
                {"debug", DEBUG},
                {"info", INFO},
                {"warn", WARN},
                {"error", ERROR},
                {"fatal", FATAL},
                {"DEBUG", DEBUG},
                {"INFO", INFO},
                {"WARN", WARN},
                {"ERROR", ERROR},
                {"FATAL", FATAL}
        };

        if (const auto it = strToLevel.find(str); it != strToLevel.end())
        {
            return it->second;
        }
        return UNKNOW;
    }

    LogEvent::LogEvent(const std::shared_ptr<Logger> &logger, const LogLevel::Level level, const char *       file,
                       const int32_t                  line, const uint32_t          thread_id, const uint64_t time
            ) :
        m_file(file), m_line(line), m_threadId(thread_id), m_time(time), m_logger(logger),
        m_level(level)
    {
    }

    const char *LogEvent::getFile() const
    {
        return m_file;
    }

    int32_t LogEvent::getLine() const
    {
        return m_line;
    }

    uint32_t LogEvent::getThreadId() const
    {
        return m_threadId;
    }

    uint64_t LogEvent::getTime() const
    {
        return m_time;
    }

    std::string LogEvent::getContent() const
    {
        return m_ss.str();
    }

    std::shared_ptr<Logger> LogEvent::getLogger() const
    {
        return m_logger;
    }

    LogLevel::Level LogEvent::getLevel() const
    {
        return m_level;
    }

    std::stringstream &LogEvent::getSS()
    {
        return m_ss;
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        std::vector<char> buf(1024);
        int               len = vsnprintf(buf.data(), buf.size(), fmt, al);

        if (len >= static_cast<int>(buf.size()))
        {
            buf.resize(len + 1);
            len = vsnprintf(buf.data(), buf.size(), fmt, al);
        }
        if (len >= 0)
        {
            m_ss << std::string(buf.data(), len);
        }
    }

    LogEventWrap::LogEventWrap(const std::shared_ptr<LogEvent> &event) :
        m_event(event)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }

    std::shared_ptr<LogEvent> LogEventWrap::getEvent() const
    {
        return m_event;
    }


    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getSS();
    }

    LogFormatter::LogFormatter(std::string pattern) :
        m_pattern(std::move(pattern))
    {
        init();
    }

    std::string LogFormatter::format(const std::shared_ptr<Logger> &  logger, const LogLevel::Level level,
                                     const std::shared_ptr<LogEvent> &event) const
    {
        std::stringstream ss;
        for (const auto &i: m_items)
        {
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &                   ofs, const std::shared_ptr<Logger> &logger,
                                       const LogLevel::Level            level,
                                       const std::shared_ptr<LogEvent> &event) const
    {
        for (const auto &i: m_items)
        {
            i->format(ofs, logger, level, event);
        }
        return ofs;
    }


    class MessageFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit MessageFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit LevelFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, const LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    class NameFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit NameFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit ThreadIdFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << event->getThreadId();
        }
    };

    class DateTimeFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit DateTimeFormatItem(std::string format = "%Y-%m-%d %H:%M:%S") :
            m_format(std::move(format))
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            tm         tm   = {};
            const auto time = static_cast<time_t>(event->getTime());
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit FilenameFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << event->getFile();
        }
    };

    class LineFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit LineFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << event->getLine();
        }
    };

    class NewLineFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit NewLineFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << std::endl;
        }
    };


    class StringFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit StringFormatItem(std::string str) :
            m_string(std::move(str))
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit TabFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &                   os, const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            os << "\t";
        }

    private:
        std::string m_string{};
    };

    ///"%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n"
    void LogFormatter::init()
    {
        std::vector<PatternSegment> pattern_segments;
        std::string                 literal_buffer;

        for (size_t current_pos = 0; current_pos < m_pattern.size(); ++current_pos)
        {
            constexpr auto kPatternError = "<<pattern_error>>";

            // 处理普通字符（非%开头）
            if (const char current_char = m_pattern[current_pos]; current_char != '%')
            {
                literal_buffer += current_char;
                continue;
            }

            // 处理转义%%（输出单个%）
            if (current_pos + 1 < m_pattern.size() && m_pattern[current_pos + 1] == '%')
            {
                literal_buffer += '%';
                ++current_pos; // 跳过已处理的%
                continue;
            }

            // 解析格式说明符（如%d{...}）
            size_t parse_end = current_pos + 1;
            enum class ParseState { None, ParsingFormat };
            auto        state = ParseState::None;
            std::string format_key;
            std::string format_specifier;
            size_t      format_start = 0;

            while (parse_end < m_pattern.size())
            {
                const char parse_char = m_pattern[parse_end];

                if (state == ParseState::None)
                {
                    // 遇到非字母且非{}时结束解析（如%d后面的空格）
                    if (!std::isalpha(parse_char) && parse_char != '{' && parse_char != '}')
                    {
                        format_key = m_pattern.substr(current_pos + 1, parse_end - current_pos - 1);
                        break;
                    }

                    // 遇到{进入格式说明解析
                    if (parse_char == '{')
                    {
                        format_key   = m_pattern.substr(current_pos + 1, parse_end - current_pos - 1);
                        state        = ParseState::ParsingFormat;
                        format_start = parse_end + 1;
                    }
                } else if (state == ParseState::ParsingFormat && parse_char == '}')
                {
                    // 遇到}结束格式说明
                    format_specifier = m_pattern.substr(format_start, parse_end - format_start);
                    state            = ParseState::None;
                    ++parse_end; // 跳过}
                    break;
                }

                ++parse_end;

                // 处理未闭合的格式说明（如%d{xxx未闭合）
                if (parse_end == m_pattern.size())
                {
                    if (format_key.empty())
                    {
                        format_key = m_pattern.substr(current_pos + 1);
                    }
                    if (state == ParseState::ParsingFormat)
                    {
                        m_error    = true;
                        format_key = kPatternError;
                    }
                }
            }

            // 保存解析结果
            if (!literal_buffer.empty())
            {
                pattern_segments.emplace_back(literal_buffer, "", SegmentType::Literal);
                literal_buffer.clear();
            }

            if (state == ParseState::ParsingFormat)
            {
                m_error = true;
                pattern_segments.emplace_back(kPatternError, "", SegmentType::FormatSpec);
            } else
            {
                pattern_segments.emplace_back(format_key, format_specifier, SegmentType::FormatSpec);
            }

            current_pos = parse_end - 1; // 更新主循环位置
        }

        // 处理末尾剩余字面量
        if (!literal_buffer.empty())
        {
            pattern_segments.emplace_back(literal_buffer, "", SegmentType::Literal);
        }

        static const std::unordered_map<std::string, std::function<std::shared_ptr<FormatItem>(const std::string &)>>
                format_item_registry = {
                        {"d", [](const std::string &fmt)
                        {
                            return std::make_shared<DateTimeFormatItem>(fmt);
                        }},
                        {"t", [](const std::string &fmt)
                        {
                            return std::make_shared<ThreadIdFormatItem>(fmt);
                        }},
                        {"m", [](const std::string &fmt)
                        {
                            return std::make_shared<MessageFormatItem>(fmt);
                        }},
                        {"p", [](const std::string &fmt)
                        {
                            return std::make_shared<LevelFormatItem>(fmt);
                        }},
                        {"c", [](const std::string &fmt)
                        {
                            return std::make_shared<NameFormatItem>(fmt);
                        }},
                        {"l", [](const std::string &fmt)
                        {
                            return std::make_shared<LineFormatItem>(fmt);
                        }},
                        {"n", [](const std::string &fmt)
                        {
                            return std::make_shared<NewLineFormatItem>(fmt);
                        }},
                        {"f", [](const std::string &fmt)
                        {
                            return std::make_shared<FilenameFormatItem>(fmt);
                        }},
                        {"T", [](const std::string &fmt)
                        {
                            return std::make_shared<TabFormatItem>(fmt);
                        }},
                };

        for (const auto &segment: pattern_segments)
        {
            if (segment.type == SegmentType::Literal)
            {
                m_items.push_back(std::make_shared<StringFormatItem>(segment.content));
                continue;
            }

            if (auto it = format_item_registry.find(segment.content); it != format_item_registry.end())
            {
                m_items.push_back(it->second(segment.format_spec));
            } else
            {
                constexpr auto kFormatErrorPrefix = "<<error_format %";
                m_items.push_back(std::make_shared<StringFormatItem>(
                        kFormatErrorPrefix + segment.content + ">>"));
                m_error = true;
            }
        }
    }

    bool LogFormatter::isError() const
    {
        return m_error;
    }

    std::string LogFormatter::getPattern() const
    {
        return m_pattern;
    }

    LogFormatter::PatternSegment::PatternSegment(std::string content, std::string format_spec, const SegmentType type) :
        content(std::move(content)), format_spec(std::move(format_spec)), type(type)
    {
    }

    void LogAppender::setFormatter(const std::shared_ptr<LogFormatter> &value)
    {
        std::scoped_lock lock(m_mutex);
        m_formatter = value;
        if (m_formatter)
        {
            m_hasFormatter = true;
        } else
        {
            m_hasFormatter = false;
        }
    }

    std::shared_ptr<LogFormatter> LogAppender::getFormatter()
    {
        std::scoped_lock lock(m_mutex);
        return m_formatter;
    }

    LogLevel::Level LogAppender::getLevel() const
    {
        return m_level;
    }

    void LogAppender::setLevel(const LogLevel::Level value)
    {
        m_level = value;
    }


    Logger::Logger(std::string name) :
        m_name(std::move(name)), m_level(LogLevel::DEBUG)
    {
        m_formatter = std::make_shared<LogFormatter>("%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n");
    }

    void Logger::log(const LogLevel::Level level, const std::shared_ptr<LogEvent> &event)
    {
        if (level >= m_level)
        {
            const auto       self = shared_from_this();
            std::scoped_lock lock(m_mutex);
            if (!m_appenders.empty())
            {
                for (const auto &i: m_appenders)
                {
                    i->log(self, level, event);
                }
            } else if (m_root)
            {
                m_root->log(level, event);
            }
        }
    }

    void Logger::debug(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::WARN, event);
    }

    void Logger::error(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::FATAL, event);
    }

    void Logger::addAppender(const std::shared_ptr<LogAppender> &appender)
    {
        std::scoped_lock lock1(m_mutex);
        if (!appender->getFormatter())
        {
            std::scoped_lock lock2(appender->m_mutex);
            appender->m_formatter = m_formatter;
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(const std::shared_ptr<LogAppender> &appender)
    {
        std::scoped_lock lock(m_mutex);
        for (auto it = m_appenders.begin();
             it != m_appenders.end(); ++it)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders()
    {
        std::scoped_lock lock(m_mutex);
        m_appenders.clear();
    }

    LogLevel::Level Logger::getLevel() const
    {
        return m_level;
    }

    void Logger::setLevel(const LogLevel::Level value)
    {
        m_level = value;
    }

    const std::string &Logger::getName() const
    {
        return m_name;
    }

    void Logger::setFormatter(const std::shared_ptr<LogFormatter> &value)
    {
        std::scoped_lock lock1(m_mutex);
        m_formatter = value;

        for (const auto &i: m_appenders)
        {
            std::scoped_lock lock2(i->m_mutex);
            if (!i->m_hasFormatter)
            {
                i->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string &value)
    {
        const auto new_val = std::make_shared<LogFormatter>(value);
        if (new_val->isError())
        {
            std::cout << "Logger Configuration Error - Invalid formatter. "
                    << "Logger name: " << m_name
                    << " | Formatter value: " << value
                    << " is invalid." << std::endl;
            return;
        }
        setFormatter(new_val);
    }

    std::shared_ptr<LogFormatter> Logger::getFormatter()
    {
        std::scoped_lock lock(m_mutex);
        return m_formatter;
    }

    std::string Logger::toYamlString()
    {
        std::scoped_lock lock(m_mutex);
        YAML::Node       node;
        node["name"] = m_name;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }

        for (const auto &i: m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void StdoutLogAppender::log(const std::shared_ptr<Logger> &  logger, const LogLevel::Level level,
                                const std::shared_ptr<LogEvent> &event)
    {
        if (level >= m_level)
        {
            std::scoped_lock lock(m_mutex);
            m_formatter->format(std::cout, logger, level, event);
        }
    }

    std::string StdoutLogAppender::toYamlString()
    {
        std::scoped_lock lock(m_mutex);
        YAML::Node       node;
        node["type"] = "StdoutLogAppender";
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(std::string filename) :
        m_filename(std::move(filename))
    {
        reopen();
    }

    void FileLogAppender::log(const std::shared_ptr<Logger> &  logger, const LogLevel::Level level,
                              const std::shared_ptr<LogEvent> &event)
    {
        if (level >= m_level)
        {
            if (const uint64_t now = event->getTime(); now >= (m_lastTime + 3))
            {
                reopen();
                m_lastTime = now;
            }
            std::scoped_lock lock(m_mutex);
            if (!m_formatter->format(m_filestream, logger, level, event))
            {
                std::cout << "error" << std::endl;
            }
        }
    }

    std::string FileLogAppender::toYamlString()
    {
        std::scoped_lock lock(m_mutex);
        YAML::Node       node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    bool FileLogAppender::reopen()
    {
        std::scoped_lock lock(m_mutex);
        if (m_filestream)
        {
            m_filestream.close();
        }

        if (const std::string dir = m_filename.substr(0, m_filename.find_last_of("/\\")); !std::filesystem::exists(
                dir))
        {
            std::filesystem::create_directories(dir);
        }
        m_filestream.open(m_filename, std::ios::app);
        return m_filestream.is_open();
    }

    LoggerManager::LoggerManager()
    {
        m_root = std::make_shared<Logger>();
        m_root->addAppender(std::make_shared<StdoutLogAppender>());
        m_loggers[m_root->m_name] = m_root;
    }

    std::shared_ptr<Logger> LoggerManager::getLogger(const std::string &name)
    {
        std::scoped_lock lock(m_mutex);
        if (const auto it = m_loggers.find(name); it != m_loggers.end())
        {
            return it->second;
        }

        auto logger     = std::make_shared<Logger>(name);
        logger->m_root  = m_root;
        m_loggers[name] = logger;
        return logger;
    }

    std::shared_ptr<Logger> LoggerManager::getRoot() const
    {
        return m_root;
    }

    std::string LoggerManager::toYamlString()
    {
        std::scoped_lock lock(m_mutex);
        YAML::Node       node;
        for (const auto &[fst, snd]: m_loggers)
        {
            node.push_back(YAML::Load(snd->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
}
