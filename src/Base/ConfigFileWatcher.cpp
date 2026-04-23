#include "ConfigFileWatcher.h"

#include <algorithm>
#include <filesystem>
#include <ranges>

namespace Base
{
    // ============================================================================
    // 工厂方法
    // ============================================================================

    std::unique_ptr<IFileWatcher> FileWatcherFactory::create()
    {
#ifdef __linux__
        return std::make_unique<InotifyFileWatcher>();
#elif defined(_WIN32)
        return std::make_unique<Win32FileWatcher>();
#else
        return std::make_unique<PollingFileWatcher>();
#endif
    }

    // ============================================================================
    // Linux Inotify 实现
    // ============================================================================

#ifdef __linux__

    InotifyFileWatcher::InotifyFileWatcher()
    {
        inotify_fd_ = inotify_init1(IN_CLOEXEC);
        if (inotify_fd_ < 0)
        {
            throw std::runtime_error("Failed to initialize inotify");
        }
    }

    InotifyFileWatcher::~InotifyFileWatcher()
    {
        stop();
        if (inotify_fd_ >= 0)
        {
            close(inotify_fd_);
        }
    }

    bool InotifyFileWatcher::start()
    {
        if (running_.load(std::memory_order_acquire))
        {
            return true;
        }

        if (inotify_fd_ < 0)
        {
            return false;
        }

        should_stop_.store(false, std::memory_order_release);
        running_.store(true, std::memory_order_release);

        watch_thread_ = std::make_unique<std::thread>(&InotifyFileWatcher::watchLoop, this);

        return true;
    }

    void InotifyFileWatcher::stop()
    {
        if (!running_.load(std::memory_order_acquire))
        {
            return;
        }

        should_stop_.store(true, std::memory_order_release);

        if (watch_thread_ &&watch_thread_



        ->
        joinable()
        )
        {
            watch_thread_->join();
        }

        watch_thread_.reset();
        running_.store(false, std::memory_order_release);
    }

    bool InotifyFileWatcher::addWatch(std::string_view path, bool recursive)
    {
        std::error_code ec;
        auto abs_path = std::filesystem::absolute(path, ec).string();

        if (ec)
        {
            return false;
        }

        // 检查是否已经在监听
        if (path_to_wd_.find(abs_path) != path_to_wd_.end())
        {
            return true;
        }

        int wd = inotify_add_watch(inotify_fd_, abs_path.c_str(), WATCH_MASK);
        if (wd < 0)
        {
            return false;
        }

        watch_descriptors_[wd] = abs_path;
        path_to_wd_[abs_path] = wd;

        // 递归监听子目录
        if (recursive && std::filesystem::is_directory(abs_path, ec))
        {
            for (const auto &entry: std::filesystem::recursive_directory_iterator(abs_path, ec))
            {
                if (ec)
                    break;
                if (entry.is_directory())
                {
                    addWatch(entry.path().string(), false);
                }
            }
        }

        return true;
    }

    bool InotifyFileWatcher::removeWatch(std::string_view path)
    {
        std::error_code ec;
        auto abs_path = std::filesystem::absolute(path, ec).string();

        if (ec)
        {
            return false;
        }

        auto it = path_to_wd_.find(abs_path);
        if (it == path_to_wd_.end())
        {
            return false;
        }

        int wd = it->second;
        inotify_rm_watch(inotify_fd_, wd);

        watch_descriptors_.erase(wd);
        path_to_wd_.erase(it);

        return true;
    }

    void InotifyFileWatcher::setCallback(FileChangeCallback callback)
    {
        callback_ = std::move(callback);
    }

    bool InotifyFileWatcher::isRunning() const noexcept
    {
        return running_.load(std::memory_order_acquire);
    }

    void InotifyFileWatcher::setDebounceInterval(std::chrono::milliseconds interval) noexcept
    {
        debounce_interval_ = interval;
    }

    void InotifyFileWatcher::watchLoop()
    {
        char buffer[EVENT_BUFFER_SIZE];

        while (!should_stop_.load(std::memory_order_acquire))
        {
            struct pollfd pfd;
            pfd.fd = inotify_fd_;
            pfd.events = POLLIN;

            int ret = poll(&pfd, 1, 100); // 100ms 超时

            if (ret < 0)
            {
                if (errno == EINTR)
                    continue;
                break;
            }

            if (ret == 0)
                continue;

            if (pfd.revents & POLLIN)
            {
                ssize_t len = read(inotify_fd_, buffer, sizeof(buffer));
                if (len < 0)
                {
                    if (errno == EINTR)
                        continue;
                    break;
                }

                ssize_t i = 0;
                while (i < len)
                {
                    struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

                    if (event->len > 0)
                    {
                        auto wd_it = watch_descriptors_.find(event->wd);
                        if (wd_it != watch_descriptors_.end())
                        {
                            std::string full_path = wd_it->second;
                            if (!full_path.empty() && full_path.back() != '/')
                            {
                                full_path += '/';
                            }
                            full_path += event->name;

                            // 防抖检查
                            auto now = std::chrono::steady_clock::now();
                            auto last_it = last_event_time_.find(full_path);
                            if (last_it != last_event_time_.end())
                            {
                                if (now - last_it->second < debounce_interval_)
                                {
                                    i += sizeof(struct inotify_event) + event->len;
                                    continue;
                                }
                            }
                            last_event_time_[full_path] = now;

                            if (callback_)
                            {
                                FileChangeEvent evt = FileChangeEvent::Modified;
                                if (event->mask & IN_CLOSE_WRITE)
                                {
                                    evt = FileChangeEvent::Modified;
                                } else if (event->mask & IN_MOVED_TO)
                                {
                                    evt = FileChangeEvent::Created;
                                } else if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF))
                                {
                                    evt = FileChangeEvent::Deleted;
                                }
                                callback_(full_path, evt);
                            }
                        }
                    }

                    i += sizeof(struct inotify_event) + event->len;
                }
            }
        }
    }

    // ============================================================================
    // Windows ReadDirectoryChangesW 实现
    // ============================================================================

#elif defined(_WIN32)

    Win32FileWatcher::Win32FileWatcher() = default;

    Win32FileWatcher::~Win32FileWatcher()
    {
        Win32FileWatcher::stop();
        for (const auto &watch: m_watches | std::views::values)
        {
            if (watch->directory_handle != INVALID_HANDLE_VALUE)
            {
                CancelIo(watch->directory_handle);
                CloseHandle(watch->directory_handle);
            }
        }
    }

    bool Win32FileWatcher::start()
    {
        if (m_running.load(std::memory_order_acquire))
        {
            return true;
        }

        m_should_stop.store(false, std::memory_order_release);
        m_running.store(true, std::memory_order_release);

        m_watch_thread = std::make_unique<std::thread>(&Win32FileWatcher::watchLoop, this);

        return true;
    }

    void Win32FileWatcher::stop()
    {
        if (!m_running.load(std::memory_order_acquire))
        {
            return;
        }

        m_should_stop.store(true, std::memory_order_release);

        if (m_watch_thread && m_watch_thread->joinable())
        {
            m_watch_thread->join();
        }

        m_watch_thread.reset();
        m_running.store(false, std::memory_order_release);
    }

    bool Win32FileWatcher::addWatch(const std::string_view path, const bool recursive)
    {
        std::error_code ec;
        const auto abs_path = std::filesystem::absolute(path, ec).string();

        if (ec)
        {
            return false;
        }

        if (m_watches.contains(abs_path))
        {
            return true;
        }

        auto watch = std::make_unique<WatchInfo>();
        watch->directory_path = abs_path;
        watch->recursive = recursive;

        const auto wide_path = toWideString(abs_path);
        watch->directory_handle = CreateFileW(
                                              wide_path.c_str(),
                                              FILE_LIST_DIRECTORY,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                              nullptr,
                                              OPEN_EXISTING,
                                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                              nullptr
                                             );

        if (watch->directory_handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        watch->overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (!watch->overlapped.hEvent)
        {
            CloseHandle(watch->directory_handle);
            return false;
        }

        m_watches[abs_path] = std::move(watch);
        return true;
    }

    bool Win32FileWatcher::removeWatch(const std::string_view path)
    {
        std::error_code ec;
        const auto abs_path = std::filesystem::absolute(path, ec).string();

        if (ec)
        {
            return false;
        }

        const auto it = m_watches.find(abs_path);
        if (it == m_watches.end())
        {
            return false;
        }

        CancelIo(it->second->directory_handle);
        CloseHandle(it->second->overlapped.hEvent);
        CloseHandle(it->second->directory_handle);
        m_watches.erase(it);

        return true;
    }

    void Win32FileWatcher::setCallback(FileChangeCallback callback)
    {
        m_callback = std::move(callback);
    }

    bool Win32FileWatcher::isRunning() const noexcept
    {
        return m_running.load(std::memory_order_acquire);
    }

    void Win32FileWatcher::setDebounceInterval(const std::chrono::milliseconds interval) noexcept
    {
        m_debounce_interval = interval;
    }

    void Win32FileWatcher::watchLoop()
    {
        while (!m_should_stop.load(std::memory_order_acquire))
        {
            for (auto &[_, watch]: m_watches)
            {
                constexpr DWORD filter = FILE_NOTIFY_CHANGE_LAST_WRITE |
                                         FILE_NOTIFY_CHANGE_FILE_NAME |
                                         FILE_NOTIFY_CHANGE_DIR_NAME;

                const BOOL success = ReadDirectoryChangesW(
                                                           watch->directory_handle,
                                                           watch->buffer,
                                                           sizeof(watch->buffer),
                                                           watch->recursive ? TRUE : FALSE,
                                                           filter,
                                                           nullptr,
                                                           &watch->overlapped,
                                                           nullptr
                                                          );

                if (!success)
                {
                    continue;
                }

                DWORD bytes_transferred = 0;
                if (const DWORD result = WaitForSingleObject(watch->overlapped.hEvent, 100); result == WAIT_OBJECT_0)
                {
                    if (GetOverlappedResult(watch->directory_handle, &watch->overlapped,
                                            &bytes_transferred, FALSE))
                    {
                        processNotification(watch->buffer, bytes_transferred);
                    }
                    ResetEvent(watch->overlapped.hEvent);
                }
            }
        }
    }

    void Win32FileWatcher::processNotification(BYTE *buffer, DWORD)
    {
        BYTE *ptr = buffer;
        FILE_NOTIFY_INFORMATION *notify = nullptr;

        do
        {
            notify = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(ptr);
            std::wstring filename_w(notify->FileName, notify->FileNameLength / sizeof(WCHAR));
            const int size = WideCharToMultiByte(CP_UTF8, 0, filename_w.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string filename(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, filename_w.c_str(), -1, filename.data(), size, nullptr, nullptr);
            filename.pop_back(); // 移除 null 终止符

            // 防抖检查
            auto now = std::chrono::steady_clock::now();
            if (auto last_it = m_last_event_time.find(filename); last_it != m_last_event_time.end())
            {
                if (now - last_it->second < m_debounce_interval)
                {
                    if (notify->NextEntryOffset == 0)
                        break;
                    ptr += notify->NextEntryOffset;
                    continue;
                }
            }
            m_last_event_time[filename] = now;

            if (m_callback)
            {
                auto evt = FileChangeEvent::Modified;
                switch (notify->Action)
                {
                    case FILE_ACTION_MODIFIED:
                    case FILE_ACTION_ADDED:
                        evt = FileChangeEvent::Modified;
                        break;
                    case FILE_ACTION_REMOVED:
                        evt = FileChangeEvent::Deleted;
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        evt = FileChangeEvent::Created;
                        break;
                    default:
                        evt = FileChangeEvent::Modified;
                }
                m_callback(filename, evt);
            }

            if (notify->NextEntryOffset == 0)
                break;
            ptr += notify->NextEntryOffset;
        } while (true);
    }

    std::wstring Win32FileWatcher::toWideString(const std::string_view str) const
    {
        if (str.empty())
        {
            return std::wstring();
        }
        const int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
        std::wstring result(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);
        return result;
    }

    // ============================================================================
    // 回退实现：基于轮询
    // ============================================================================

#else

    PollingFileWatcher::PollingFileWatcher() = default;

    PollingFileWatcher::~PollingFileWatcher()
    {
        stop();
    }

    bool PollingFileWatcher::start()
    {
        if (running_.load(std::memory_order_acquire))
        {
            return true;
        }

        should_stop_.store(false, std::memory_order_release);
        running_.store(true, std::memory_order_release);

        poll_thread_ = std::make_unique<std::thread>(&PollingFileWatcher::pollLoop, this);

        return true;
    }

    void PollingFileWatcher::stop()
    {
        if (!running_.load(std::memory_order_acquire))
        {
            return;
        }

        should_stop_.store(true, std::memory_order_release);

        if (poll_thread_ &&poll_thread_



        ->
        joinable()
        )
        {
            poll_thread_->join();
        }

        poll_thread_.reset();
        running_.store(false, std::memory_order_release);
    }

    bool PollingFileWatcher::addWatch(std::string_view path, bool recursive)
    {
        std::error_code ec;
        auto abs_path = std::filesystem::absolute(path, ec).string();

        if (ec)
        {
            return false;
        }

        auto file_time = std::filesystem::last_write_time(abs_path, ec);
        if (ec)
        {
            file_time = std::chrono::file_clock::now();
        }

        WatchEntry entry;
        entry.path = abs_path;
        entry.last_write_time = file_time;
        entry.recursive = recursive;

        watches_.push_back(std::move(entry));
        return true;
    }

    bool PollingFileWatcher::removeWatch(std::string_view path)
    {
        std::error_code ec;
        auto abs_path = std::filesystem::absolute(path, ec).string();

        if (ec)
        {
            return false;
        }

        auto it = std::find_if(watches_.begin(), watches_.end(),
                               [&abs_path](const WatchEntry &e)
                               {
                                   return e.path == abs_path;
                               });

        if (it != watches_.end())
        {
            watches_.erase(it);
            return true;
        }

        return false;
    }

    void PollingFileWatcher::setCallback(FileChangeCallback callback)
    {
        callback_ = std::move(callback);
    }

    bool PollingFileWatcher::isRunning() const noexcept
    {
        return running_.load(std::memory_order_acquire);
    }

    void PollingFileWatcher::setPollInterval(std::chrono::milliseconds interval) noexcept
    {
        poll_interval_ = interval;
    }

    void PollingFileWatcher::pollLoop()
    {
        while (!should_stop_.load(std::memory_order_acquire))
        {
            for (auto &watch: watches_)
            {
                std::error_code ec;
                auto current_time = std::filesystem::last_write_time(watch.path, ec);

                if (!ec && current_time != watch.last_write_time)
                {
                    watch.last_write_time = current_time;
                    if (callback_)
                    {
                        callback_(watch.path, FileChangeEvent::Modified);
                    }
                }
            }

            std::this_thread::sleep_for(poll_interval_);
        }
    }

#endif
}
