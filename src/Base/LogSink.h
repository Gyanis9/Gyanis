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
        std::string format(const LogEvent &event) override;
    };

    class ColorFormatter : public LogFormatter
    {
    public:
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

        void setLevel(LogLevel level);

        LogLevel getLevel() const;

        bool shouldLog(LogLevel level) const;

        void setFormatter(std::unique_ptr<LogFormatter> formatter);

    protected:
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
        explicit ConsoleSink(bool enable_color = true);

        void write(const LogEvent &event) override;

        void flush() override;

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
        explicit FileSink(const std::filesystem::path &file_path, bool truncate = false);

        ~FileSink() override;

        void write(const LogEvent &event) override;

        void flush() override;

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
        RollingFileSink(const std::string &base_filename,
                        const std::filesystem::path &directory,
                        RollingPolicy policy,
                        size_t max_size_bytes = 10 * 1024 * 1024,
                        size_t max_backup_files = 10);

        ~RollingFileSink() override;

        void write(const LogEvent &event) override;

        void flush() override;

    private:
        void checkAndRoll();

        std::filesystem::path getCurrentFilename() const;

        std::string generateTimestampSuffix() const;

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

        explicit AsyncSink(std::unique_ptr<LogSink> wrapped_sink,
                           size_t queue_size = 1024,
                           OverflowPolicy policy = OverflowPolicy::Block);

        ~AsyncSink() override;

        void write(const LogEvent &event) override;

        void flush() override;

        void stop();

    private:
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
