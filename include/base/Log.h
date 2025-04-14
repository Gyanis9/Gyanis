/**
 * @file Log.h
 * @brief 日志模块封装
 * @date 2025-03-12
 */
#ifndef LOG_H
#define LOG_H

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <list>
#include <fstream>
#include <sys/syscall.h>
#include <unistd.h>

#include "Singleton.h"
#include "base/Mutex.h"

/**
 * @brief 根据日志级别，使用流式方式将日志写入 logger
 */
#define LOG_LEVEL(logger, level) \
    if((logger)->getLevel() <= (level)) \
        Gyanis::base::LogEventWrap(std::make_shared<Gyanis::base::LogEvent>(logger, level,__FILE__, __LINE__, syscall(SYS_gettid),time(nullptr))).getSS()
/**
 * @brief 输出 debug 级别日志
 */
#define LOG_DEBUG(logger) LOG_LEVEL(logger, Gyanis::base::LogLevel::DEBUG)

/**
 * @brief 输出 info 级别日志
 */
#define LOG_INFO(logger) LOG_LEVEL(logger, Gyanis::base::LogLevel::INFO)

/**
 * @brief 输出 warn 级别日志
 */
#define LOG_WARN(logger) LOG_LEVEL(logger, Gyanis::base::LogLevel::WARN)

/**
 * @brief 输出 error 级别日志
 */
#define LOG_ERROR(logger) LOG_LEVEL(logger, Gyanis::base::LogLevel::ERROR)

/**
 * @brief 输出 fatal 级别日志
 */
#define LOG_FATAL(logger) LOG_LEVEL(logger, Gyanis::base::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志写入 logger
 */
#define LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        Gyanis::base::LogEventWrap(std::make_shared<Gyanis::base::LogEvent>(logger, level, __FILE__, __LINE__, syscall(SYS_gettid),\
                time(nullptr))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 输出 debug 级别格式化日志
 */
#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, Gyanis::base::LogLevel::DEBUG, fmt, __VA_ARGS__

/**
 * @brief 输出 info 级别格式化日志
 */
#define LOG_FMT_INFO(logger, fmt, ...)  LOG_FMT_LEVEL(logger, Gyanis::base::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 输出 warn 级别格式化日志
 */
#define LOG_FMT_WARN(logger, fmt, ...)  LOG_FMT_LEVEL(logger, Gyanis::base::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 输出 error 级别格式化日志
 */
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, Gyanis::base::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 输出 fatal 级别格式化日志
 */
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, Gyanis::base::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取根日志器
 */
#define LOG_ROOT() Gyanis::base::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取指定名称的日志器
 */
#define LOG_NAME(name) Gyanis::base::LoggerMgr::GetInstance()->getLogger(name)

namespace Gyanis::base
{
    class Logger;

    class LoggerManager;

    /**
     * @brief 日志级别定义
     */
    class LogLevel
    {
    public:
        /**
         * @brief 日志级别枚举
         */
        enum Level
        {
            UNKNOW = 0, ///< 未知级别
            DEBUG = 1, ///< DEBUG 级别
            INFO = 2, ///< INFO 级别
            WARN = 3, ///< WARN 级别
            ERROR = 4, ///< ERROR 级别
            FATAL = 5 ///< FATAL 级别
        };

        /**
         * @brief 将日志级别转换为字符串
         */
        static const char* ToString(Level level);

        /**
         * @brief 将字符串转换为日志级别
         */
        static Level FromString(const std::string& str);
    };

    /**
     * @brief 日志事件，包含日志详细信息
     */
    class LogEvent
    {
    public:
        /**
         * @brief 构造日志事件
         *
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] file 文件名
         * @param[in] line 行号
         * @param[in] thread_id 线程ID
         * @param[in] time 时间戳
         */
        explicit LogEvent(const std::shared_ptr<Logger>& logger, LogLevel::Level level, const char* file, int32_t line,
                          uint32_t thread_id, uint64_t time
        );

        /**
         * @brief 获取当前日志文件名
         */
        const char* getFile() const;

        /**
         * @brief 获取当前日志行号
         */
        int32_t getLine() const;

        /**
         * @brief 获取当前线程ID
         */
        uint32_t getThreadId() const;

        /**
         * @brief 获取当前时间戳
         */
        uint64_t getTime() const;

        /**
         * @brief 获取日志内容
         */
        std::string getContent() const;

        /**
         * @brief 获取与该日志相关联的Logger对象
         */
        std::shared_ptr<Logger> getLogger() const;

        /**
         * @brief 获取日志级别
         */
        LogLevel::Level getLevel() const;

        /**
         * @brief 获取输出日志内容的字符串流
         */
        std::stringstream& getSS();

        /**
         * @brief 格式化日志内容（支持可变参数的格式化）
         * @param fmt 格式化字符串
         * @param ... 可变参数
         */
        void format(const char* fmt, ...);

        /**
         * @brief 格式化日志内容（支持使用va_list进行可变参数格式化）
         * @param fmt 格式化字符串
         * @param al va_list类型的可变参数
         */
        void format(const char* fmt, va_list al);

    private:
        const char* m_file = nullptr; ///< 文件名
        int32_t m_line = 0; ///< 行号
        uint32_t m_threadId = 0; ///< 线程ID
        uint64_t m_time = 0; ///< 时间戳
        std::stringstream m_ss; ///< 日志内容流
        std::shared_ptr<Logger> m_logger; ///< 日志器
        LogLevel::Level m_level; ///< 日志等级
    };

    /**
     * @brief 日志事件包装器
     */
    class LogEventWrap
    {
    public:
        /**
         * @brief 构造函数
         * @param[in] event 日志事件
         */
        explicit LogEventWrap(const std::shared_ptr<LogEvent>& event);

        /**
         * @brief 析构函数
         */
        ~LogEventWrap();

        /**
         * @brief 获取日志事件
         */
        [[nodiscard]] std::shared_ptr<LogEvent> getEvent() const;

        /**
         * @brief 获取日志内容流
         */
        [[nodiscard]] std::stringstream& getSS();

    private:
        std::shared_ptr<LogEvent> m_event; ///< 日志事件
    };

    /**
     * @brief 日志格式化器
     */
    class LogFormatter
    {
    public:
        /**
         * @brief 构造函数
         * @param[in] pattern 格式化模板
         */
        explicit LogFormatter(std::string pattern);

        /**
         * @brief 格式化日志
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         * @return 格式化后的日志字符串
         */
        [[nodiscard]] std::string format(const std::shared_ptr<Logger>& logger, LogLevel::Level level,
                                         const std::shared_ptr<LogEvent>& event) const;

        /**
         * @brief 格式化日志到输出流
         * @param[in] ofs 输出流
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         * @return 输出流
         */
        std::ostream& format(std::ostream& ofs, const std::shared_ptr<Logger>& logger, LogLevel::Level level,
                             const std::shared_ptr<LogEvent>& event) const;

        /**
         * @brief 格式项基类
         */
        class FormatItem
        {
        public:
            virtual ~FormatItem() = default;

            /**
             * @brief 格式化日志到输出流
             * @param[in, out] os 输出流
             * @param[in] logger 日志器
             * @param[in] level 日志级别
             * @param[in] event 日志事件
             */
            virtual void format(std::ostream& os, const std::shared_ptr<Logger>& logger, LogLevel::Level level,
                                const std::shared_ptr<LogEvent>& event) = 0;
        };

        /**
         * @brief 初始化解析日志模板
         */
        void init();

        /**
         * @brief 是否发生错误
         */
        [[nodiscard]] bool isError() const;

        /**
         * @brief 获取格式化模板
         */
        [[nodiscard]] std::string getPattern() const;

    private:
        enum class SegmentType { Literal, FormatSpec };

        struct PatternSegment
        {
            std::string content; ///< 内容（字面量或格式符）
            std::string format_spec; ///< 格式说明（如时间格式）
            SegmentType type; ///< 类型标记
            explicit PatternSegment(std::string content, std::string format_spec, SegmentType type);
        };

        std::string m_pattern; ///< 日志格式模板
        std::vector<std::shared_ptr<FormatItem>> m_items; ///< 日志格式解析后格式
        bool m_error = false; ///< 是否有错误
    };

    /**
     * @brief 日志输出目标（基类）
     */
    class LogAppender
    {
        friend class Logger;

    public:
        using MutexType = Spinlock;

        /**
         * @brief 析构函数
         */
        virtual ~LogAppender() = default;

        /**
         * @brief 写入日志
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         */
        virtual void log(const std::shared_ptr<Logger>& logger, LogLevel::Level level,
                         const std::shared_ptr<LogEvent>& event) = 0;

        /**
         * @brief 将日志输出目标的配置转为 YAML 字符串
         */
        virtual std::string toYamlString() = 0;

        /**
         * @brief 设置日志格式器
         */
        void setFormatter(const std::shared_ptr<LogFormatter>& value);

        /**
         * @brief 获取日志格式器
         */
        std::shared_ptr<LogFormatter> getFormatter();

        /**
         * @brief 获取日志级别
         */
        [[nodiscard]] LogLevel::Level getLevel() const;

        /**
         * @brief 设置日志级别
         */
        void setLevel(LogLevel::Level value);

    protected:
        LogLevel::Level m_level = LogLevel::INFO; /// 日志级别
        bool m_hasFormatter = false; /// 是否有自己的日志格式器
        MutexType m_mutex; ///< Mutex
        std::shared_ptr<LogFormatter> m_formatter; ///< 日志格式器
    };

    /**
     * @brief 日志器
     */
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;

    public:
        using MutexType = Spinlock;

        /**
         * @brief 构造函数
         * @param[in] name 日志器名称
         */
        explicit Logger(std::string name = "root");

        /**
         * @brief 写入日志
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         */
        void log(LogLevel::Level level, const std::shared_ptr<LogEvent>& event);

        /**
         * @brief 写 debug 级别日志
         */
        void debug(const std::shared_ptr<LogEvent>& event);

        /**
         * @brief 写 info 级别日志
         */
        void info(const std::shared_ptr<LogEvent>& event);

        /**
         * @brief 写 warn 级别日志
         */
        void warn(const std::shared_ptr<LogEvent>& event);

        /**
         * @brief 写 error 级别日志
         */
        void error(const std::shared_ptr<LogEvent>& event);

        /**
         * @brief 写 fatal 级别日志
         */
        void fatal(const std::shared_ptr<LogEvent>& event);

        /**
         * @brief 添加日志目标
         */
        void addAppender(const std::shared_ptr<LogAppender>& appender);

        /**
         * @brief 删除日志目标
         */
        void delAppender(const std::shared_ptr<LogAppender>& appender);

        /**
         * @brief 清空日志目标
         */
        void clearAppenders();

        /**
         * @brief 获取日志级别
         */
        LogLevel::Level getLevel() const;

        /**
         * @brief 设置日志级别
         */
        void setLevel(LogLevel::Level value);

        /**
         * @brief 获取日志器名称
         */
        const std::string& getName() const;

        /**
         * @brief 设置日志格式器
         */
        void setFormatter(const std::shared_ptr<LogFormatter>& value);

        /**
         * @brief 设置日志格式器
         */
        void setFormatter(const std::string& value);

        /**
         * @brief 获取日志格式器
         */
        std::shared_ptr<LogFormatter> getFormatter();

        /**
         * @brief 将日志器的配置转为 YAML 字符串
         * @return YAML 配置字符串
         */
        std::string toYamlString();

    private:
        std::string m_name; ///< 日志名称
        LogLevel::Level m_level; ///< 日志级别
        MutexType m_mutex; ///< Mutex
        std::list<std::shared_ptr<LogAppender>> m_appenders; ///< 日志目标集合
        std::shared_ptr<LogFormatter> m_formatter; ///< 日志格式器
        std::shared_ptr<Logger> m_root; ///< 主日志器
    };

    /**
     * @brief 输出到控制台的 Appender
     */
    class StdoutLogAppender final : public LogAppender
    {
    public:
        void log(const std::shared_ptr<Logger>& logger, LogLevel::Level level,
                 const std::shared_ptr<LogEvent>& event) override;

        std::string toYamlString() override;
    };

    /**
     * @brief 输出到文件的 Appender
     */
    class FileLogAppender final : public LogAppender
    {
    public:
        explicit FileLogAppender(std::string filename);

        void log(const std::shared_ptr<Logger>& logger, LogLevel::Level level,
                 const std::shared_ptr<LogEvent>& event) override;

        std::string toYamlString() override;

        bool reopen(); ///< 重新打开日志文件

    private:
        std::string m_filename; ///< 文件路径
        std::ofstream m_filestream; ///< 文件流
        uint64_t m_lastTime = 0; ///<< 上次重新打开时间
    };

    /**
     * @brief 日志器管理类
     */
    class LoggerManager
    {
    public:
        using MutexType = Spinlock;

        LoggerManager();

        /**
         * @brief 获取日志器
         * @param[in] name 日志器名称
         */
        std::shared_ptr<Logger> getLogger(const std::string& name);

        /**
         * @brief 获取根日志器
         */
        [[nodiscard]] std::shared_ptr<Logger> getRoot() const;

        /**
         * @brief 将所有日志器配置转为 YAML 字符串
         */
        std::string toYamlString();

    private:
        MutexType m_mutex; ///< 互斥锁
        std::map<std::string, std::shared_ptr<Logger>> m_loggers; ///< 日志器容器
        std::shared_ptr<Logger> m_root; ///< 根日志器
    };

    /// 日志器管理类单例模式
    using LoggerMgr = Singleton<LoggerManager>;
}

#endif
