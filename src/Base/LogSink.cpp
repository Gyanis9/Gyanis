#include "LogSink.h"

#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace Base
{
    std::string DefaultFormatter::format(const LogEvent &event)
    {
        std::ostringstream oss;
        oss << "[" << event.timestamp << "] "
                << "[" << event.thread_id << "] "
                << "[" << logLevelToString(event.level) << "] "
                << "[" << event.logger_name << "] "
                << "[" << event.location.shortFileName() << ":" << event.location.line << "] "
                << event.message;
        return oss.str();
    }

    std::string ColorFormatter::format(const LogEvent &event)
    {
        std::ostringstream oss;
        const char *color_code = color::getColorForLevel(event.level);
        oss << color_code
                << "[" << event.timestamp << "] "
                << "[" << event.thread_id << "] "
                << "[" << logLevelToString(event.level) << "] "
                << "[" << event.logger_name << "] "
                << "[" << event.location.shortFileName() << ":" << event.location.line << "] "
                << event.message
                << color::RESET;
        return oss.str();
    }

    void LogSink::setLevel(const LogLevel level)
    {
        m_level.store(level, std::memory_order_release);
    }

    LogLevel LogSink::getLevel() const
    {
        return m_level.load(std::memory_order_acquire);
    }

    bool LogSink::shouldLog(const LogLevel level) const
    {
        return level >= getLevel();
    }

    void LogSink::setFormatter(std::unique_ptr<LogFormatter> formatter)
    {
        m_formatter = std::move(formatter);
    }

    std::string LogSink::formatEvent(const LogEvent &event) const
    {
        if (m_formatter)
        {
            return m_formatter->format(event);
        }
        static DefaultFormatter default_fmt;
        return default_fmt.format(event);
    }

    // ============================================================================
    // ConsoleSink 实现
    // ============================================================================

    ConsoleSink::ConsoleSink(const bool enable_color) : m_color_enabled(enable_color)
    {
        setFormatter(std::make_unique<ColorFormatter>());
#ifdef _WIN32
        if (enable_color)
        {
            if (const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE); hOut != INVALID_HANDLE_VALUE)
            {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode))
                {
                    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(hOut, dwMode);
                }
            }
        }
#endif
    }

    void ConsoleSink::write(const LogEvent &event)
    {
        std::lock_guard lock(m_mutex);
        const std::string formatted = formatEvent(event);
        if (event.level >= LogLevel::WARN)
        {
            std::cerr << formatted << std::endl;
        } else
        {
            std::cout << formatted << std::endl;
        }
    }

    void ConsoleSink::flush()
    {
        std::lock_guard lock(m_mutex);
        std::cout.flush();
        std::cerr.flush();
    }

    void ConsoleSink::setColorEnabled(const bool enabled)
    {
        std::lock_guard lock(m_mutex);
        m_color_enabled = enabled;
        if (enabled)
        {
            setFormatter(std::make_unique<ColorFormatter>());
        } else
        {
            setFormatter(std::make_unique<DefaultFormatter>());
        }
    }

    // ============================================================================
    // FileSink 实现
    // ============================================================================

    FileSink::FileSink(const std::filesystem::path &file_path, bool truncate)
        : m_file_path(file_path)
    {
        if (auto parent = m_file_path.parent_path(); !parent.empty())
        {
            std::filesystem::create_directories(parent);
        }
        auto mode = std::ios::out | std::ios::app;
        if (truncate)
        {
            mode |= std::ios::trunc;
        }
        m_file.open(m_file_path, mode);
        if (!m_file.is_open())
        {
            throw std::runtime_error("Failed to open log file: " + m_file_path.string());
        }
    }

    FileSink::~FileSink()
    {
        FileSink::flush();
    }

    void FileSink::write(const LogEvent &event)
    {
        std::lock_guard lock(m_mutex);
        if (m_file.is_open())
        {
            m_file << formatEvent(event) << std::endl;
        }
    }

    void FileSink::flush()
    {
        std::lock_guard lock(m_mutex);
        if (m_file.is_open())
        {
            m_file.flush();
        }
    }

    void FileSink::reopen(const std::filesystem::path &new_path)
    {
        std::lock_guard lock(m_mutex);
        m_file.close();
        m_file_path = new_path;
        m_file.open(m_file_path, std::ios::out | std::ios::app);
    }

    // ============================================================================
    // RollingFileSink 实现
    // ============================================================================

    RollingFileSink::RollingFileSink(const std::string &base_filename,
                                     const std::filesystem::path &directory,
                                     const RollingPolicy policy,
                                     const size_t max_size_bytes,
                                     const size_t max_backup_files)
        : m_base_filename(base_filename)
          , m_directory(directory)
          , m_policy(policy)
          , m_max_size_bytes(max_size_bytes)
          , m_max_backup_files(max_backup_files)
    {
        std::filesystem::create_directories(m_directory);
        m_current_sink = std::make_unique<FileSink>(getCurrentFilename());
    }

    RollingFileSink::~RollingFileSink()
    {
        RollingFileSink::flush();
    }

    void RollingFileSink::write(const LogEvent &event)
    {
        std::lock_guard lock(m_mutex);
        checkAndRoll();
        if (m_current_sink)
        {
            m_current_sink->write(event);
        }
    }

    void RollingFileSink::flush()
    {
        std::lock_guard lock(m_mutex);
        if (m_current_sink)
        {
            m_current_sink->flush();
        }
    }

    void RollingFileSink::checkAndRoll()
    {
        bool should_roll = false;
        if (m_policy == RollingPolicy::Daily || m_policy == RollingPolicy::Hourly)
        {
            if (const std::string new_suffix = generateTimestampSuffix(); new_suffix != m_current_suffix)
            {
                should_roll = true;
                m_current_suffix = new_suffix;
            }
        }
        if (m_policy == RollingPolicy::Size && m_current_sink)
        {
            std::error_code ec;
            if (const auto file_size = std::filesystem::file_size(getCurrentFilename(), ec); !ec && file_size >= m_max_size_bytes)
            {
                should_roll = true;
            }
        }
        if (should_roll)
        {
            m_current_sink.reset();
            auto current_path = getCurrentFilename();
            if (std::filesystem::exists(current_path))
            {
                auto backup_name = m_base_filename;
                const auto dot_pos = m_base_filename.rfind('.');
                std::string name_part, ext_part;
                if (dot_pos != std::string::npos)
                {
                    name_part = m_base_filename.substr(0, dot_pos);
                    ext_part = m_base_filename.substr(dot_pos);
                } else
                {
                    name_part = m_base_filename;
                }
                std::string backup_filename;
                if (m_policy == RollingPolicy::Size)
                {
                    int index = 1;
                    do
                    {
                        backup_filename = name_part + "." + std::to_string(index) + ext_part;
                        ++index;
                    } while (std::filesystem::exists(m_directory / backup_filename));
                } else
                {
                    backup_filename = name_part + "." + m_current_suffix + ext_part;
                }
                std::filesystem::rename(current_path, m_directory / backup_filename);
            }
            m_current_sink = std::make_unique<FileSink>(current_path);
            cleanupOldFiles();
        }
    }

    std::filesystem::path RollingFileSink::getCurrentFilename() const
    {
        std::string filename = m_base_filename;
        if (m_policy == RollingPolicy::Daily || m_policy == RollingPolicy::Hourly)
        {
            if (const auto dot_pos = filename.rfind('.'); dot_pos != std::string::npos)
            {
                filename.insert(dot_pos, "." + m_current_suffix);
            } else
            {
                filename += "." + m_current_suffix;
            }
        }
        return m_directory / filename;
    }

    std::string RollingFileSink::generateTimestampSuffix() const
    {
        const auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_buf);
#endif
        std::ostringstream oss;
        if (m_policy == RollingPolicy::Daily)
        {
            oss << std::put_time(&tm_buf, "%Y-%m-%d");
        } else
        {
            oss << std::put_time(&tm_buf, "%Y-%m-%d_%H");
        }
        return oss.str();
    }

    void RollingFileSink::cleanupOldFiles() const
    {
        if (m_max_backup_files == 0)
        {
            return;
        }
        std::vector<std::filesystem::path> backup_files;
        const auto stem = m_base_filename;
        const auto dot_pos = stem.rfind('.');
        const std::string name_part = (dot_pos != std::string::npos) ? stem.substr(0, dot_pos) : stem;
        for (const auto &entry: std::filesystem::directory_iterator(m_directory))
        {
            if (auto filename = entry.path().filename().string(); filename.find(name_part) == 0 && filename != m_base_filename)
            {
                backup_files.push_back(entry.path());
            }
        }
        if (backup_files.size() > m_max_backup_files)
        {
            std::ranges::sort(backup_files,
                              [](const auto &a, const auto &b)
                              {
                                  return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                              });
            for (size_t i = m_max_backup_files; i < backup_files.size(); ++i)
            {
                std::error_code ec;
                std::filesystem::remove(backup_files[i], ec);
            }
        }
    }

    // ============================================================================
    // AsyncSink 实现
    // ============================================================================

    AsyncSink::AsyncSink(std::unique_ptr<LogSink> wrapped_sink,
                         const size_t queue_size,
                         const OverflowPolicy policy)
        : m_wrapped_sink(std::move(wrapped_sink))
          , m_max_queue_size(queue_size)
          , m_overflow_policy(policy)
    {
        m_worker_thread = std::thread(&AsyncSink::workerLoop, this);
    }

    AsyncSink::~AsyncSink()
    {
        stop();
    }

    void AsyncSink::write(const LogEvent &event)
    {
        std::unique_lock lock(m_queue_mutex);
        if (m_overflow_policy == OverflowPolicy::Drop)
        {
            if (m_queue.size() >= m_max_queue_size)
            {
                return;
            }
            m_queue.push(event);
        } else
        {
            m_queue_cv.wait(lock, [this]
            {
                return m_queue.size() < m_max_queue_size || !m_running.load();
            });
            if (m_running.load())
            {
                m_queue.push(event);
            }
        }
        lock.unlock();
        m_queue_cv.notify_one();
    }

    void AsyncSink::flush()
    {
        std::unique_lock lock(m_queue_mutex);
        m_flush_cv.wait(lock, [this]
        {
            return m_queue.empty();
        });
        if (m_wrapped_sink)
        {
            m_wrapped_sink->flush();
        }
    }

    void AsyncSink::stop()
    {
        if (!m_running.exchange(false))
        {
            return;
        }
        m_queue_cv.notify_all();
        if (m_worker_thread.joinable())
        {
            m_worker_thread.join();
        }
        if (m_wrapped_sink)
        {
            m_wrapped_sink->flush();
        }
    }

    void AsyncSink::workerLoop()
    {
        while (m_running.load())
        {
            std::unique_lock lock(m_queue_mutex);
            m_queue_cv.wait(lock, [this]
            {
                return !m_queue.empty() || !m_running.load();
            });
            while (!m_queue.empty())
            {
                LogEvent event = std::move(m_queue.front());
                m_queue.pop();
                lock.unlock();
                if (m_wrapped_sink)
                {
                    m_wrapped_sink->write(event);
                }
                lock.lock();
            }
            m_flush_cv.notify_all();
        }
        std::unique_lock lock(m_queue_mutex);
        while (!m_queue.empty())
        {
            LogEvent event = std::move(m_queue.front());
            m_queue.pop();
            lock.unlock();
            if (m_wrapped_sink)
            {
                m_wrapped_sink->write(event);
            }
            lock.lock();
        }
        m_flush_cv.notify_all();
    }
}
