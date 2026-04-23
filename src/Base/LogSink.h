/**
 * @file LogSink.h
 * @brief 日志输出目标基类及具体实现
 * @copyright Copyright (c) 2026
 */

#ifndef LOGSINK_H
#define LOGSINK_H

#include "LogCommon.h"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace Base
{
    // ============================================================================
    // 格式化器
    // ============================================================================

    class LogFormatter
    {
    public:
        virtual ~LogFormatter() = default;

        virtual std::string format(const LogEvent &event) = 0;
    };

    class DefaultFormatter : public LogFormatter
    {
    public:
        /**
         * @brief 将日志事件格式化为纯文本输出。
         * @param event 日志事件。
         * @return std::string 格式化后的日志行。
         */
        std::string format(const LogEvent &event) override;
    };

    class ColorFormatter : public LogFormatter
    {
    public:
        /**
         * @brief 将日志事件格式化为带颜色的终端输出文本。
         * @param event 日志事件。
         * @return std::string 格式化后的日志行。
         */
        std::string format(const LogEvent &event) override;
    };

    // ============================================================================
    // Sink 基类
    // ============================================================================

    class LogSink
    {
    public:
        virtual ~LogSink() = default;

        virtual void write(const LogEvent &event) = 0;

        virtual void flush() = 0;

        /**
         * @brief 设置 Sink 的最小日志级别。
         * @param level 目标日志级别。
         */
        void setLevel(LogLevel level);

        /**
         * @brief 获取 Sink 当前日志级别。
         * @return LogLevel 当前日志级别。
         */
        LogLevel getLevel() const;

        /**
         * @brief 判断给定日志级别是否允许输出。
         * @param level 待判断级别。
         * @return bool 满足级别阈值时返回 true。
         */
        bool shouldLog(LogLevel level) const;

        /**
         * @brief 设置自定义格式化器。
         * @param formatter 格式化器对象所有权。
         */
        void setFormatter(std::unique_ptr<LogFormatter> formatter);

    protected:
        /**
         * @brief 使用当前格式化器将事件转换为文本。
         * @param event 日志事件。
         * @return std::string 格式化文本。
         */
        std::string formatEvent(const LogEvent &event) const;

    private:
        std::atomic<LogLevel> m_level{LogLevel::TRACE};
        std::unique_ptr<LogFormatter> m_formatter;
    };

    // ============================================================================
    // 控制台 Sink
    // ============================================================================

    class ConsoleSink : public LogSink
    {
    public:
        /**
         * @brief 构造控制台 Sink，并按需启用彩色输出。
         * @param enable_color 是否启用彩色输出。
         */
        explicit ConsoleSink(bool enable_color = true);

        /**
         * @brief 将日志事件写入标准输出或标准错误。
         * @param event 日志事件。
         */
        void write(const LogEvent &event) override;

        /**
         * @brief 刷新控制台输出缓冲区。
         */
        void flush() override;

        /**
         * @brief 动态切换控制台彩色输出能力。
         * @param enabled 是否启用彩色输出。
         */
        void setColorEnabled(bool enabled);

    private:
        bool m_color_enabled;
        std::mutex m_mutex;
    };

    // ============================================================================
    // 文件 Sink
    // ============================================================================

    class FileSink : public LogSink
    {
    public:
        /**
         * @brief 构造文件 Sink，并按模式打开目标日志文件。
         * @param file_path 日志文件路径。
         * @param truncate 是否以截断模式打开文件。
         */
        explicit FileSink(const std::filesystem::path &file_path, bool truncate = false);

        /**
         * @brief 析构文件 Sink 并确保缓存落盘。
         */
        ~FileSink() override;

        /**
         * @brief 将日志事件写入文件。
         * @param event 日志事件。
         */
        void write(const LogEvent &event) override;

        /**
         * @brief 刷新文件缓冲区。
         */
        void flush() override;

        /**
         * @brief 重新打开并切换输出文件路径。
         * @param new_path 新日志文件路径。
         */
        void reopen(const std::filesystem::path &new_path);

    private:
        std::filesystem::path m_file_path;
        std::ofstream m_file;
        std::mutex m_mutex;
    };

    // ============================================================================
    // 滚动文件 Sink
    // ============================================================================

    enum class RollingPolicy
    {
        Size,
        Daily,
        Hourly
    };

    class RollingFileSink : public LogSink
    {
    public:
        /**
         * @brief 构造滚动文件 Sink。
         * @param base_filename 基础文件名。
         * @param directory 日志目录。
         * @param policy 滚动策略。
         * @param max_size_bytes 按大小滚动时的阈值（字节）。
         * @param max_backup_files 最大保留备份文件数。
         */
        RollingFileSink(const std::string &base_filename,
                        const std::filesystem::path &directory,
                        RollingPolicy policy,
                        size_t max_size_bytes = 10 * 1024 * 1024,
                        size_t max_backup_files = 10);

        /**
         * @brief 析构滚动文件 Sink 并刷新残留数据。
         */
        ~RollingFileSink() override;

        /**
         * @brief 写入日志前执行滚动检查。
         * @param event 日志事件。
         */
        void write(const LogEvent &event) override;

        /**
         * @brief 刷新当前活动文件 Sink。
         */
        void flush() override;

    private:
        /**
         * @brief 根据策略判断并执行日志文件滚动。
         */
        void checkAndRoll();

        /**
         * @brief 获取当前活动日志文件完整路径。
         * @return std::filesystem::path 活动日志文件路径。
         */
        std::filesystem::path getCurrentFilename() const;

        /**
         * @brief 生成按天或按小时滚动时的时间后缀。
         * @return std::string 时间后缀字符串。
         */
        std::string generateTimestampSuffix() const;

        /**
         * @brief 清理超出保留上限的历史备份文件。
         */
        void cleanupOldFiles() const;

        std::string m_base_filename;
        std::filesystem::path m_directory;
        RollingPolicy m_policy;
        size_t m_max_size_bytes;
        size_t m_max_backup_files;

        std::unique_ptr<FileSink> m_current_sink;
        std::string m_current_suffix;
        std::mutex m_mutex;
    };

    // ============================================================================
    // 异步 Sink
    // ============================================================================

    class AsyncSink : public LogSink
    {
    public:
        enum class OverflowPolicy
        {
            Block,
            Drop
        };

        /**
         * @brief 构造异步 Sink 并启动后台消费线程。
         * @param wrapped_sink 被包装的真实 Sink。
         * @param queue_size 队列容量上限。
         * @param policy 队列满时溢出策略。
         */
        explicit AsyncSink(std::unique_ptr<LogSink> wrapped_sink,
                           size_t queue_size = 1024,
                           OverflowPolicy policy = OverflowPolicy::Block);

        /**
         * @brief 析构异步 Sink 并停止后台线程。
         */
        ~AsyncSink() override;

        /**
         * @brief 将日志事件写入异步队列。
         * @param event 日志事件。
         */
        void write(const LogEvent &event) override;

        /**
         * @brief 阻塞等待队列清空并刷新下游 Sink。
         */
        void flush() override;

        /**
         * @brief 停止异步处理线程并尽力排空队列。
         */
        void stop();

    private:
        /**
         * @brief 后台工作循环，持续消费并转发队列日志事件。
         */
        void workerLoop();

        std::unique_ptr<LogSink> m_wrapped_sink;
        std::queue<LogEvent> m_queue;
        size_t m_max_queue_size;
        OverflowPolicy m_overflow_policy;

        std::mutex m_queue_mutex;
        std::condition_variable m_queue_cv;
        std::condition_variable m_flush_cv;

        std::atomic<bool> m_running{true};
        std::thread m_worker_thread;
    };
}


#endif //LOGSINK_H
