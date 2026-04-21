/**
 * @file LogCommon.h
 * @brief 日志模块公共类型定义、枚举、工具函数
 * @copyright Copyright (c) 2026
 */
#ifndef LOGCOMMON_H
#define LOGCOMMON_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>


#ifdef _WIN32
#include <windows.h>
#endif

namespace Base
{
    // ============================================================================
    // 日志等级
    // ============================================================================

#ifdef ERROR
#undef ERROR
#endif

    enum class LogLevel : uint8_t
    {
        TRACE = 0,
        DEBUG = 1,
        INFO  = 2,
        WARN  = 3,
        ERROR = 4,
        FATAL = 5,
        OFF   = 6
    };

    inline const char *logLevelToString(const LogLevel level) noexcept
    {
        switch (level)
        {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO ";
            case LogLevel::WARN: return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "?????";
        }
    }

    inline LogLevel logLevelFromString(const std::string_view str)
    {
        if (str == "TRACE")
            return LogLevel::TRACE;
        if (str == "DEBUG")
            return LogLevel::DEBUG;
        if (str == "INFO")
            return LogLevel::INFO;
        if (str == "WARN")
            return LogLevel::WARN;
        if (str == "ERROR")
            return LogLevel::ERROR;
        if (str == "FATAL")
            return LogLevel::FATAL;
        if (str == "OFF")
            return LogLevel::OFF;
        return LogLevel::INFO;
    }

    // ============================================================================
    // 时间戳工具
    // ============================================================================

    inline std::string currentTimestamp()
    {
        const auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_buf);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    inline std::string threadIdString()
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        return oss.str();
    }

    // ============================================================================
    // 源码位置
    // ============================================================================

    struct SourceLocation
    {
        const char *file_name = nullptr;
        int line = 0;
        const char *function_name = nullptr;

        SourceLocation() = default;

        SourceLocation(const char *file, const int l, const char *func)
            : file_name(file), line(l), function_name(func)
        {
        }

        const char *shortFileName() const
        {
            if (!file_name)
            {
                return "";
            }
            const char *last_slash = file_name;
            for (const char *p = file_name; *p; ++p)
            {
                if (*p == '/' || *p == '\\')
                {
                    last_slash = p + 1;
                }
            }
            return last_slash;
        }
    };

#define LOG_SOURCE_LOCATION() \
    Base::SourceLocation(__FILE__, __LINE__, __FUNCTION__)

    // ============================================================================
    // 日志事件
    // ============================================================================

    struct LogEvent
    {
        LogLevel level;
        std::string timestamp;
        std::string thread_id;
        SourceLocation location;
        std::string logger_name;
        std::string message;

        LogEvent() = default;

        LogEvent(const LogLevel lvl, std::string ts, std::string tid, const SourceLocation &loc, std::string logger, std::string msg)
            : level(lvl)
              , timestamp(std::move(ts))
              , thread_id(std::move(tid))
              , location(loc)
              , logger_name(std::move(logger))
              , message(std::move(msg))
        {
        }
    };

    // ============================================================================
    // 颜色支持
    // ============================================================================

    namespace color
    {
        constexpr auto RESET = "\033[0m";
        constexpr auto RED = "\033[31m";
        constexpr auto GREEN = "\033[32m";
        constexpr auto YELLOW = "\033[33m";
        constexpr auto BLUE = "\033[34m";
        constexpr auto MAGENTA = "\033[35m";
        constexpr auto CYAN = "\033[36m";
        constexpr auto WHITE = "\033[37m";
        constexpr auto BRIGHT_BLACK = "\033[90m";
        constexpr auto BRIGHT_RED = "\033[91m";
        constexpr auto BRIGHT_GREEN = "\033[92m";
        constexpr auto BRIGHT_YELLOW = "\033[93m";
        constexpr auto BRIGHT_BLUE = "\033[94m";
        constexpr auto BRIGHT_MAGENTA = "\033[95m";
        constexpr auto BRIGHT_CYAN = "\033[96m";
        constexpr auto BRIGHT_WHITE = "\033[97m";

        inline const char *getColorForLevel(const LogLevel level)
        {
            switch (level)
            {
                case LogLevel::TRACE: return BRIGHT_BLACK;
                case LogLevel::DEBUG: return CYAN;
                case LogLevel::INFO: return GREEN;
                case LogLevel::WARN: return YELLOW;
                case LogLevel::ERROR: return RED;
                case LogLevel::FATAL: return BRIGHT_RED;
                default: return RESET;
            }
        }

        inline bool terminalSupportsColor()
        {
#ifdef _WIN32
            return true;
#else
            const char *term = std::getenv("TERM");
            return term && std::string_view(term).find("color") != std::string_view::npos;
#endif
        }
    }
}


#endif //LOGCOMMON_H
