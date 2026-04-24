#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <utility>

#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include "base/Log.h"


namespace
{
    constexpr std::string_view kAnsiColorReset = "\033[0m";

    std::string_view LevelAnsiColor(const Gyanis::base::LogLevel::Level level)
    {
        switch (level)
        {
            case Gyanis::base::LogLevel::Level::DEBUG:
                return "\033[36m";
            case Gyanis::base::LogLevel::Level::INFO:
                return "\033[32m";
            case Gyanis::base::LogLevel::Level::WARN:
                return "\033[33m";
            case Gyanis::base::LogLevel::Level::ERROR:
                return "\033[31m";
            case Gyanis::base::LogLevel::Level::FATAL:
                return "\033[1;31m";
            default:
                return "";
        }
    }

    std::string ColorizeLevelToken(std::string formatted_text, const Gyanis::base::LogLevel::Level level)
    {
        const auto color = LevelAnsiColor(level);
        if (color.empty())
        {
            return formatted_text;
        }

        const std::string level_text = Gyanis::base::LogLevel::ToString(level);
        const std::string level_tag  = "[" + level_text + "]";

        if (const auto pos = formatted_text.find(level_tag); pos != std::string::npos)
        {
            formatted_text.replace(pos, level_tag.size(),
                                   std::string(color) + level_tag + std::string(kAnsiColorReset));
            return formatted_text;
        }

        if (const auto pos = formatted_text.find(level_text); pos != std::string::npos)
        {
            formatted_text.replace(pos, level_text.size(),
                                   std::string(color) + level_text + std::string(kAnsiColorReset));
        }
        return formatted_text;
    }

    bool SupportsAnsiColorOnStdout()
    {
        static const bool supports = []
        {
            if (const char *no_color = std::getenv("NO_COLOR"); no_color != nullptr && *no_color != '\0')
            {
                return false;
            }

            if (const char *force_color = std::getenv("FORCE_COLOR");
                force_color != nullptr && *force_color != '\0' && std::string_view(force_color) != "0")
            {
                return true;
            }

            if (::isatty(STDOUT_FILENO) == 0)
            {
                return false;
            }

            const char *term = std::getenv("TERM");
            return term != nullptr && *term != '\0' && std::string_view(term) != "dumb";
        }();
        return supports;
    }

    bool IEquals(const std::string_view lhs, const std::string_view rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        return std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                          [](const char l, const char r)
                          {
                              return std::tolower(static_cast<unsigned char>(l))
                                     == std::tolower(static_cast<unsigned char>(r));
                          });
    }

    std::mutex g_stdoutMutex;
}

namespace Gyanis::base
{
    const char *LogLevel::ToString(const Level level)
    {
        switch (level)
        {
            case Level::DEBUG:
                return "DEBUG";
            case Level::INFO:
                return "INFO";
            case Level::WARN:
                return "WARN";
            case Level::ERROR:
                return "ERROR";
            case Level::FATAL:
                return "FATAL";
            default:
                return "UNKNOW";
        }
    }

    LogLevel::Level LogLevel::FromString(const std::string_view str)
    {
        if (IEquals(str, "debug"))
        {
            return Level::DEBUG;
        }
        if (IEquals(str, "info"))
        {
            return Level::INFO;
        }
        if (IEquals(str, "warn"))
        {
            return Level::WARN;
        }
        if (IEquals(str, "error"))
        {
            return Level::ERROR;
        }
        if (IEquals(str, "fatal"))
        {
            return Level::FATAL;
        }
        return Level::UNKNOW;
    }

    LogEvent::LogEvent(const std::shared_ptr<Logger> &logger,
                       const LogLevel::Level          level,
                       const char *                   file,
                       const int32_t                  line,
                       const uint32_t                 thread_id,
                       const uint64_t                 time) :
        m_file(file),
        m_line(line),
        m_threadId(thread_id),
        m_time(time),
        m_logger(logger),
        m_level(level)
    {
    }

    LogEvent::LogEvent(const std::shared_ptr<Logger> &logger,
                       const LogLevel::Level          level,
                       const std::source_location &   location,
                       const uint32_t                 thread_id,
                       const uint64_t                 time) :
        m_file(location.file_name()),
        m_line(static_cast<int32_t>(location.line())),
        m_threadId(thread_id),
        m_time(time),
        m_logger(logger),
        m_level(level)
    {
    }

    uint32_t LogEvent::GetCurrentThreadId() noexcept
    {
        return static_cast<uint32_t>(::syscall(SYS_gettid));
    }

    uint64_t LogEvent::GetCurrentTime() noexcept
    {
        using namespace std::chrono;
        return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
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
        va_list len_args;
        va_copy(len_args, al);
        const int len = std::vsnprintf(nullptr, 0, fmt, len_args);
        va_end(len_args);
        if (len <= 0)
        {
            return;
        }

        std::vector<char> buffer(static_cast<size_t>(len) + 1);
        va_list           value_args;
        va_copy(value_args, al);
        std::vsnprintf(buffer.data(), buffer.size(), fmt, value_args);
        va_end(value_args);
        m_ss.write(buffer.data(), len);
    }

    LogEventWrap::LogEventWrap(const std::shared_ptr<LogEvent> &event) :
        m_event(event)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        if (!m_event)
        {
            return;
        }

        if (const auto logger = m_event->getLogger())
        {
            logger->log(m_event->getLevel(), m_event);
        }
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

    std::string LogFormatter::format(const std::shared_ptr<Logger> &  logger,
                                     const LogLevel::Level            level,
                                     const std::shared_ptr<LogEvent> &event) const
    {
        std::stringstream ss;
        for (const auto &item: m_items)
        {
            item->format(ss, logger, level, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &                   ofs,
                                       const std::shared_ptr<Logger> &  logger,
                                       const LogLevel::Level            level,
                                       const std::shared_ptr<LogEvent> &event) const
    {
        for (const auto &item: m_items)
        {
            item->format(ofs, logger, level, event);
        }
        return ofs;
    }

    class MessageFormatItem final : public LogFormatter::FormatItem
    {
    public:
        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;
            os << event->getContent();
        }
    };

    class LevelFormatItem final : public LogFormatter::FormatItem
    {
    public:
        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) event;
            os << LogLevel::ToString(level);
        }
    };

    class NameFormatItem final : public LogFormatter::FormatItem
    {
    public:
        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem final : public LogFormatter::FormatItem
    {
    public:
        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;
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

        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;

            thread_local time_t cached_time = 0;
            thread_local tm     cached_tm   = {};

            const auto now = static_cast<time_t>(event->getTime());
            if (cached_time != now)
            {
                cached_time = now;
                localtime_r(&now, &cached_tm);
            }

            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &cached_tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit FilenameFormatItem(const std::string &width_spec = "")
        {
            if (width_spec.empty())
            {
                return;
            }

            try
            {
                if (const int width = std::stoi(width_spec); width > 0)
                {
                    m_width = width;
                }
            } catch (...)
            {
                m_width = 0;
            }
        }

        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;

            if (m_width <= 0)
            {
                os << event->getFile();
                return;
            }

            const auto old_flags = os.flags();
            const auto old_fill  = os.fill();
            os << std::setw(m_width) << std::setfill(' ') << std::left << event->getFile();
            os.flags(old_flags);
            os.fill(old_fill);
        }

    private:
        int m_width = 0;
    };

    class LineFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit LineFormatItem(const std::string &width_spec = "")
        {
            if (width_spec.empty())
            {
                return;
            }

            try
            {
                if (const int width = std::stoi(width_spec); width > 0)
                {
                    m_width = width;
                }
            } catch (...)
            {
                m_width = 0;
            }
        }

        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;

            if (m_width <= 0)
            {
                os << event->getLine();
                return;
            }

            const auto old_flags = os.flags();
            const auto old_fill  = os.fill();
            os << std::setw(m_width) << std::setfill(' ') << std::right << event->getLine();
            os.flags(old_flags);
            os.fill(old_fill);
        }

    private:
        int m_width = 0;
    };

    class NewLineFormatItem final : public LogFormatter::FormatItem
    {
    public:
        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;
            (void) event;
            os << '\n';
        }
    };

    class StringFormatItem final : public LogFormatter::FormatItem
    {
    public:
        explicit StringFormatItem(std::string str) :
            m_string(std::move(str))
        {
        }

        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;
            (void) event;
            os.write(m_string.data(), static_cast<std::streamsize>(m_string.size()));
        }

    private:
        std::string m_string;
    };

    class TabFormatItem final : public LogFormatter::FormatItem
    {
    public:
        void format(std::ostream &                   os,
                    const std::shared_ptr<Logger> &  logger,
                    const LogLevel::Level            level,
                    const std::shared_ptr<LogEvent> &event) override
        {
            (void) logger;
            (void) level;
            (void) event;
            os << '\t';
        }
    };

    void LogFormatter::init()
    {
        m_items.clear();
        m_error = false;

        std::vector<PatternSegment> pattern_segments;
        pattern_segments.reserve(m_pattern.size() / 2 + 1);
        std::string    literal_buffer;
        constexpr auto kPatternError = "<<pattern_error>>";

        for (size_t current_pos = 0; current_pos < m_pattern.size(); ++current_pos)
        {
            const char current_char = m_pattern[current_pos];
            if (current_char != '%')
            {
                literal_buffer += current_char;
                continue;
            }

            if (current_pos + 1 < m_pattern.size() && m_pattern[current_pos + 1] == '%')
            {
                literal_buffer += '%';
                ++current_pos;
                continue;
            }

            if (!literal_buffer.empty())
            {
                pattern_segments.emplace_back(std::move(literal_buffer), "", SegmentType::Literal);
                literal_buffer.clear();
            }

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
                    if (parse_char == '{')
                    {
                        format_key   = m_pattern.substr(current_pos + 1, parse_end - current_pos - 1);
                        state        = ParseState::ParsingFormat;
                        format_start = parse_end + 1;
                        ++parse_end;
                        continue;
                    }

                    if (!std::isalpha(static_cast<unsigned char>(parse_char)))
                    {
                        format_key = m_pattern.substr(current_pos + 1, parse_end - current_pos - 1);
                        break;
                    }
                } else if (parse_char == '}')
                {
                    format_specifier = m_pattern.substr(format_start, parse_end - format_start);
                    state            = ParseState::None;
                    ++parse_end;
                    break;
                }

                ++parse_end;
            }

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

            if (format_key.empty())
            {
                m_error    = true;
                format_key = kPatternError;
            }

            pattern_segments.emplace_back(std::move(format_key), std::move(format_specifier), SegmentType::FormatSpec);
            current_pos = parse_end - 1;
        }

        if (!literal_buffer.empty())
        {
            pattern_segments.emplace_back(std::move(literal_buffer), "", SegmentType::Literal);
        }

        m_items.reserve(pattern_segments.size());
        for (const auto &segment: pattern_segments)
        {
            if (segment.type == SegmentType::Literal)
            {
                m_items.push_back(std::make_shared<StringFormatItem>(segment.content));
                continue;
            }

            if (segment.content == "d")
            {
                m_items.push_back(std::make_shared<DateTimeFormatItem>(segment.format_spec));
            } else if (segment.content == "t")
            {
                m_items.push_back(std::make_shared<ThreadIdFormatItem>());
            } else if (segment.content == "m")
            {
                m_items.push_back(std::make_shared<MessageFormatItem>());
            } else if (segment.content == "p")
            {
                m_items.push_back(std::make_shared<LevelFormatItem>());
            } else if (segment.content == "c")
            {
                m_items.push_back(std::make_shared<NameFormatItem>());
            } else if (segment.content == "l")
            {
                m_items.push_back(std::make_shared<LineFormatItem>(segment.format_spec));
            } else if (segment.content == "n")
            {
                m_items.push_back(std::make_shared<NewLineFormatItem>());
            } else if (segment.content == "f")
            {
                m_items.push_back(std::make_shared<FilenameFormatItem>(segment.format_spec));
            } else if (segment.content == "T")
            {
                m_items.push_back(std::make_shared<TabFormatItem>());
            } else
            {
                m_items.push_back(std::make_shared<StringFormatItem>("<<error_format %" + segment.content + ">>"));
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

    LogFormatter::PatternSegment::PatternSegment(std::string       content,
                                                 std::string       format_spec,
                                                 const SegmentType type) :
        content(std::move(content)),
        format_spec(std::move(format_spec)),
        type(type)
    {
    }

    void LogAppender::setFormatter(const std::shared_ptr<LogFormatter> &value)
    {
        std::scoped_lock lock(m_mutex);
        m_formatter    = value;
        m_hasFormatter = (m_formatter != nullptr);
    }

    std::shared_ptr<LogFormatter> LogAppender::getFormatter()
    {
        std::scoped_lock lock(m_mutex);
        return m_formatter;
    }

    LogLevel::Level LogAppender::getLevel() const
    {
        std::scoped_lock lock(m_mutex);
        return m_level;
    }

    void LogAppender::setLevel(const LogLevel::Level value)
    {
        std::scoped_lock lock(m_mutex);
        m_level = value;
    }

    Logger::Logger(const std::string_view name) :
        m_name(name),
        m_level(LogLevel::Level::DEBUG)
    {
        m_formatter = std::make_shared<LogFormatter>("%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f{32}:%l{4}%T%m%n");
    }

    void Logger::log(const LogLevel::Level level, const std::shared_ptr<LogEvent> &event)
    {
        std::vector<std::shared_ptr<LogAppender>> appenders;
        std::shared_ptr<Logger>                   root;

        {
            std::scoped_lock lock(m_mutex);
            if (level < m_level)
            {
                return;
            }
            appenders.assign(m_appenders.begin(), m_appenders.end());
            root = m_root;
        }

        const auto self = shared_from_this();
        if (!appenders.empty())
        {
            for (const auto &appender: appenders)
            {
                appender->log(self, level, event);
            }
            return;
        }

        if (root)
        {
            root->log(level, event);
        }
    }

    void Logger::debug(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::Level::DEBUG, event);
    }

    void Logger::info(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::Level::INFO, event);
    }

    void Logger::warn(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::Level::WARN, event);
    }

    void Logger::error(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::Level::ERROR, event);
    }

    void Logger::fatal(const std::shared_ptr<LogEvent> &event)
    {
        log(LogLevel::Level::FATAL, event);
    }

    void Logger::addAppender(const std::shared_ptr<LogAppender> &appender)
    {
        if (!appender)
        {
            return;
        }

        std::scoped_lock logger_lock(m_mutex);
        std::scoped_lock appender_lock(appender->m_mutex);
        if (std::find(m_appenders.begin(), m_appenders.end(), appender) != m_appenders.end())
        {
            return;
        }

        if (!appender->m_formatter)
        {
            appender->m_formatter = m_formatter;
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(const std::shared_ptr<LogAppender> &appender)
    {
        std::scoped_lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
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
        std::scoped_lock lock(m_mutex);
        return m_level;
    }

    void Logger::setLevel(const LogLevel::Level value)
    {
        std::scoped_lock lock(m_mutex);
        m_level = value;
    }

    const std::string &Logger::getName() const
    {
        return m_name;
    }

    void Logger::setFormatter(const std::shared_ptr<LogFormatter> &value)
    {
        std::vector<std::shared_ptr<LogAppender>> appenders;
        {
            std::scoped_lock lock(m_mutex);
            m_formatter = value;
            appenders.assign(m_appenders.begin(), m_appenders.end());
        }

        for (const auto &appender: appenders)
        {
            std::scoped_lock appender_lock(appender->m_mutex);
            if (!appender->m_hasFormatter)
            {
                appender->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string &value)
    {
        const auto new_formatter = std::make_shared<LogFormatter>(value);
        if (new_formatter->isError())
        {
            std::cout << "Logger Configuration Error - Invalid formatter. "
                    << "Logger name: " << m_name
                    << " | Formatter value: " << value
                    << " is invalid." << std::endl;
            return;
        }
        setFormatter(new_formatter);
    }

    std::shared_ptr<LogFormatter> Logger::getFormatter()
    {
        std::scoped_lock lock(m_mutex);
        return m_formatter;
    }

    std::string Logger::toYamlString()
    {
        std::string                               name;
        LogLevel::Level                           level = LogLevel::Level::UNKNOW;
        std::shared_ptr<LogFormatter>             formatter;
        std::vector<std::shared_ptr<LogAppender>> appenders;

        {
            std::scoped_lock lock(m_mutex);
            name      = m_name;
            level     = m_level;
            formatter = m_formatter;
            appenders.assign(m_appenders.begin(), m_appenders.end());
        }

        YAML::Node node;
        node["name"] = name;
        if (level != LogLevel::Level::UNKNOW)
        {
            node["level"] = LogLevel::ToString(level);
        }
        if (formatter)
        {
            node["formatter"] = formatter->getPattern();
        }

        for (const auto &appender: appenders)
        {
            node["appenders"].push_back(YAML::Load(appender->toYamlString()));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void StdoutLogAppender::setColorEnabled(const bool value)
    {
        std::scoped_lock lock(m_mutex);
        m_enableColor = value;
    }

    bool StdoutLogAppender::isColorEnabled() const
    {
        std::scoped_lock lock(m_mutex);
        return m_enableColor;
    }

    void StdoutLogAppender::log(const std::shared_ptr<Logger> &  logger, const LogLevel::Level level,
                                const std::shared_ptr<LogEvent> &event)
    {
        LogLevel::Level               appender_level = LogLevel::Level::UNKNOW;
        std::shared_ptr<LogFormatter> formatter;
        bool                          enable_color{};

        {
            std::scoped_lock lock(m_mutex);
            appender_level = m_level;
            formatter      = m_formatter;
            enable_color   = m_enableColor;
        }

        if (level < appender_level)
        {
            return;
        }

        if (!formatter && logger)
        {
            formatter = logger->getFormatter();
        }
        if (!formatter)
        {
            return;
        }

        std::scoped_lock output_lock(g_stdoutMutex);

        if (enable_color && SupportsAnsiColorOnStdout())
        {
            std::string formatted_text = formatter->format(logger, level, event);
            std::cout << ColorizeLevelToken(std::move(formatted_text), level);
            return;
        }

        formatter->format(std::cout, logger, level, event);
    }

    std::string StdoutLogAppender::toYamlString()
    {
        std::scoped_lock lock(m_mutex);
        YAML::Node       node;
        node["type"] = "StdoutLogAppender";
        if (m_level != LogLevel::Level::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        node["color"] = m_enableColor;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(std::string filename) :
        m_filename(std::move(filename))
    {
        reopen();
        m_worker = std::thread(&FileLogAppender::workerLoop, this);
    }

    FileLogAppender::~FileLogAppender()
    {
        m_running.store(false, std::memory_order_release);
        m_queueCv.notify_all();
        if (m_worker.joinable())
        {
            m_worker.join();
        }

        std::scoped_lock lock(m_mutex);
        if (m_filestream.is_open())
        {
            m_filestream.flush();
            m_filestream.close();
        }
    }

    void FileLogAppender::setAsyncConfig(const size_t                    max_queue_size,
                                         const size_t                    flush_batch_size,
                                         const std::chrono::milliseconds flush_interval)
    {
        if (max_queue_size == 0 || flush_batch_size == 0 || flush_interval.count() <= 0)
        {
            return;
        }

        std::lock_guard lock(m_queueMutex);
        m_maxQueueSize   = max_queue_size;
        m_flushBatchSize = std::min(flush_batch_size, m_maxQueueSize);
        m_flushInterval  = flush_interval;
        m_queueCv.notify_all();
    }

    void FileLogAppender::enqueue(AsyncLogTask task)
    {
        std::unique_lock queue_lock(m_queueMutex);
        m_queueCv.wait(queue_lock,
                       [this]
                       {
                           return !m_running.load(std::memory_order_acquire)
                                  || m_pendingLogs.size() < m_maxQueueSize;
                       });

        if (!m_running.load(std::memory_order_acquire))
        {
            return;
        }

        m_pendingLogs.emplace_back(std::move(task));
        queue_lock.unlock();
        m_queueCv.notify_one();
    }

    void FileLogAppender::workerLoop()
    {
        std::vector<AsyncLogTask> batch;
        batch.reserve(m_flushBatchSize);

        while (true)
        {

            {
                std::chrono::milliseconds flush_interval;
                size_t                    flush_batch_size{};
                std::unique_lock          queue_lock(m_queueMutex);
                flush_interval   = m_flushInterval;
                flush_batch_size = m_flushBatchSize;

                m_queueCv.wait_for(queue_lock, flush_interval,
                                   [this]
                                   {
                                       return !m_running.load(std::memory_order_acquire)
                                              || !m_pendingLogs.empty();
                                   });

                if (m_pendingLogs.empty() && !m_running.load(std::memory_order_acquire))
                {
                    break;
                }

                const size_t pop_count = std::min(flush_batch_size, m_pendingLogs.size());
                for (size_t i = 0; i < pop_count; ++i)
                {
                    batch.emplace_back(std::move(m_pendingLogs.front()));
                    m_pendingLogs.pop_front();
                }

                if (m_pendingLogs.size() < m_maxQueueSize)
                {
                    m_queueCv.notify_all();
                }
            }

            if (batch.empty())
            {
                continue;
            }

            const auto now         = std::chrono::steady_clock::now();
            bool       need_reopen = false;
            {
                std::scoped_lock stream_lock(m_mutex);
                need_reopen = !m_filestream.is_open() || (now - m_lastReopen >= m_reopenInterval);
            }

            if (need_reopen)
            {
                reopen();
                m_lastReopen = now;
            }

            {
                std::scoped_lock stream_lock(m_mutex);
                if (!m_filestream.is_open())
                {
                    std::lock_guard queue_lock(m_queueMutex);
                    for (auto it = batch.rbegin(); it != batch.rend(); ++it)
                    {
                        m_pendingLogs.emplace_front(std::move(*it));
                    }
                    m_queueCv.notify_all();
                    batch.clear();
                    continue;
                }

                for (const auto &[logger, formatter, event, level]: batch)
                {
                    formatter->format(m_filestream, logger, level, event);
                }
                m_filestream.flush();
            }

            batch.clear();
        }

        std::scoped_lock lock(m_mutex);
        if (m_filestream.is_open())
        {
            m_filestream.flush();
        }
    }

    void FileLogAppender::log(const std::shared_ptr<Logger> &  logger,
                              const LogLevel::Level            level,
                              const std::shared_ptr<LogEvent> &event)
    {
        LogLevel::Level               appender_level = LogLevel::Level::UNKNOW;
        std::shared_ptr<LogFormatter> formatter;

        {
            std::scoped_lock lock(m_mutex);
            appender_level = m_level;
            formatter      = m_formatter;
        }

        if (level < appender_level)
        {
            return;
        }

        if (!formatter && logger)
        {
            formatter = logger->getFormatter();
        }
        if (!formatter)
        {
            return;
        }

        enqueue(AsyncLogTask{logger, std::move(formatter), event, level});
    }

    std::string FileLogAppender::toYamlString()
    {
        size_t                    max_queue_size   = 0;
        size_t                    flush_batch_size = 0;
        std::chrono::milliseconds flush_interval{};
        {
            std::lock_guard queue_lock(m_queueMutex);
            max_queue_size   = m_maxQueueSize;
            flush_batch_size = m_flushBatchSize;
            flush_interval   = m_flushInterval;
        }

        std::scoped_lock lock(m_mutex);
        YAML::Node       node;
        node["type"]              = "FileLogAppender";
        node["file"]              = m_filename;
        node["async"]             = true;
        node["max_queue_size"]    = max_queue_size;
        node["flush_batch_size"]  = flush_batch_size;
        node["flush_interval_ms"] = flush_interval.count();
        if (m_level != LogLevel::Level::UNKNOW)
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
        if (m_filestream.is_open())
        {
            m_filestream.flush();
            m_filestream.close();
        }

        const std::filesystem::path file_path(m_filename);
        const std::filesystem::path parent = file_path.parent_path();
        if (!parent.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec)
            {
                std::cerr << "FileLogAppender create_directories failed: " << ec.message()
                        << " | path=" << parent.string() << std::endl;
                return false;
            }
        }

        m_filestream.open(file_path, std::ios::app);
        if (!m_filestream.is_open())
        {
            std::cerr << "FileLogAppender open failed: " << m_filename << std::endl;
        }
        return m_filestream.is_open();
    }

    LoggerManager::LoggerManager()
    {
        m_root = std::make_shared<Logger>();
        m_root->addAppender(std::make_shared<StdoutLogAppender>());
        m_loggers[m_root->m_name] = m_root;
    }

    std::shared_ptr<Logger> LoggerManager::getLogger(const std::string_view name)
    {
        std::scoped_lock lock(m_mutex);
        if (const auto it = m_loggers.find(name); it != m_loggers.end())
        {
            return it->second;
        }

        auto logger               = std::make_shared<Logger>(name);
        logger->m_root            = m_root;
        m_loggers[logger->m_name] = logger;
        return logger;
    }

    std::shared_ptr<Logger> LoggerManager::getRoot() const
    {
        std::scoped_lock lock(m_mutex);
        return m_root;
    }

    std::string LoggerManager::toYamlString()
    {
        std::vector<std::shared_ptr<Logger>> loggers;
        {
            std::scoped_lock lock(m_mutex);
            loggers.reserve(m_loggers.size());
            for (const auto &[name, logger]: m_loggers)
            {
                (void) name;
                loggers.push_back(logger);
            }
        }

        YAML::Node node;
        for (const auto &logger: loggers)
        {
            node.push_back(YAML::Load(logger->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
}
