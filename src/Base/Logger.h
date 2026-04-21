/**
 * @file logger.h
 * @brief 日志器类与全局注册表
 * @copyright Copyright (c) 2026
 */
#ifndef LOGGER_H
#define LOGGER_H
#include "LogSink.h"

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Base
{
    // ============================================================================
    // 前向声明
    // ============================================================================

    class Logger;
    class LoggerRegistry;

    // ============================================================================
    // 日志器类
    // ============================================================================

    /**
     * @brief 日志器实例
     *
     * 每个日志器拥有自己的名字、等级过滤和一组 Sink。
     * 线程安全，但通常通过注册表获取，并由注册表保证线程安全。
     */
    class Logger
    {
    public:
        explicit Logger(std::string name);

        ~Logger();

        // 禁止拷贝，允许移动
        Logger(const Logger &) = delete;

        Logger &operator=(const Logger &) = delete;

        Logger(Logger &&) = default;

        Logger &operator=(Logger &&) = default;

        /**
         * @brief 记录日志
         */
        void log(LogLevel level, const SourceLocation &location, std::string_view message) const;

        /**
         * @brief 添加 Sink
         */
        void addSink(std::unique_ptr<LogSink> sink);

        /**
         * @brief 清空 Sink
         */
        void clearSinks();

        /**
         * @brief 设置日志器等级
         */
        void setLevel(LogLevel level);

        /**
         * @brief 获取日志器等级
         */
        LogLevel getLevel() const;

        /**
         * @brief 获取日志器名称
         */
        const std::string &name() const;

        /**
         * @brief 刷新所有 Sink
         */
        void flush() const;

        // 便捷方法（用于宏）
        bool shouldLog(LogLevel level) const;

    private:
        void writeToSinks(const LogEvent &event) const;

        std::string m_name;
        std::atomic<LogLevel> m_level{LogLevel::TRACE};
        std::vector<std::unique_ptr<LogSink> > m_sinks;
        mutable std::shared_mutex m_sinks_mutex;
    };

    // ============================================================================
    // 日志器注册表（单例）
    // ============================================================================

    /**
     * @brief 全局日志器注册表
     *
     * 管理所有命名的 Logger 实例。
     * 提供获取或创建 Logger 的接口。
     * 支持默认根日志器（名称为 "root"）。
     */
    class LoggerRegistry
    {
    public:
        static LoggerRegistry &instance();

        // 禁止拷贝移动
        LoggerRegistry(const LoggerRegistry &) = delete;

        LoggerRegistry &operator=(const LoggerRegistry &) = delete;

        /**
         * @brief 获取或创建指定名称的日志器
         * @param name 日志器名称
         * @return Logger 引用
         */
        Logger &getLogger(const std::string &name);

        /**
         * @brief 获取默认根日志器
         */
        Logger &getRootLogger();

        /**
         * @brief 注册一个已创建的日志器（若已存在则替换）
         */
        void registerLogger(std::unique_ptr<Logger> logger);

        /**
         * @brief 移除指定日志器
         */
        void unregisterLogger(const std::string &name);

        /**
         * @brief 获取所有日志器名称
         */
        std::vector<std::string> getLoggerNames() const;

        /**
         * @brief 清空所有日志器
         */
        void clear();

        /**
         * @brief 对所有日志器应用操作
         */
        void forEachLogger(const std::function<void(Logger &)> &func) const;

    private:
        LoggerRegistry() = default;

        mutable std::shared_mutex m_mutex{};
        std::unordered_map<std::string, std::unique_ptr<Logger> > m_loggers{};
    };

    // ============================================================================
    // 便捷宏（支持多日志器）
    // ============================================================================

    // 内部使用：获取日志器并记录
#define LOG_INTERNAL(logger_expr, level, message) \
    do { \
        auto& __logger = (logger_expr); \
        if (__logger.shouldLog(level)) { \
            __logger.log(level, LOG_SOURCE_LOCATION(), (message)); \
        } \
    } while (0)

    // 使用指定日志器的宏
#define LOG_LOGGER(logger, level, message) \
    LOG_INTERNAL(logger, level, message)

    // 使用默认根日志器的宏
#define LOG_TRACE(message) LOG_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::TRACE, message)
#define LOG_DEBUG(message) LOG_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::DEBUG, message)
#define LOG_INFO(message)  LOG_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::INFO,  message)
#define LOG_WARN(message)  LOG_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::WARN,  message)
#define LOG_ERROR(message) LOG_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::ERROR, message)
#define LOG_FATAL(message) LOG_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::FATAL, message)

    // 流式宏（延迟构造字符串，性能优化）
#define LOG_STREAM_INTERNAL(logger_expr, level, stream_expr) \
    do { \
        auto& __logger = (logger_expr); \
        if (__logger.shouldLog(level)) { \
            std::ostringstream __oss; \
            __oss << stream_expr; \
            __logger.log(level, LOG_SOURCE_LOCATION(), __oss.str()); \
        } \
    } while (0)

#define LOG_TRACE_STREAM(expr) LOG_STREAM_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::TRACE, expr)
#define LOG_DEBUG_STREAM(expr) LOG_STREAM_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::DEBUG, expr)
#define LOG_INFO_STREAM(expr)  LOG_STREAM_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::INFO,  expr)
#define LOG_WARN_STREAM(expr)  LOG_STREAM_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::WARN,  expr)
#define LOG_ERROR_STREAM(expr) LOG_STREAM_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::ERROR, expr)
#define LOG_FATAL_STREAM(expr) LOG_STREAM_INTERNAL(Base::LoggerRegistry::instance().getRootLogger(), Base::LogLevel::FATAL, expr)

    // 带日志器名的流式宏
#define LOG_LOGGER_TRACE_STREAM(logger, expr) LOG_STREAM_INTERNAL(logger, Base::LogLevel::TRACE, expr)
#define LOG_LOGGER_DEBUG_STREAM(logger, expr) LOG_STREAM_INTERNAL(logger, Base::LogLevel::DEBUG, expr)
#define LOG_LOGGER_INFO_STREAM(logger, expr)  LOG_STREAM_INTERNAL(logger, Base::LogLevel::INFO,  expr)
#define LOG_LOGGER_WARN_STREAM(logger, expr)  LOG_STREAM_INTERNAL(logger, Base::LogLevel::WARN,  expr)
#define LOG_LOGGER_ERROR_STREAM(logger, expr) LOG_STREAM_INTERNAL(logger, Base::LogLevel::ERROR, expr)
#define LOG_LOGGER_FATAL_STREAM(logger, expr) LOG_STREAM_INTERNAL(logger, Base::LogLevel::FATAL, expr)
}


#endif //LOGGER_H
