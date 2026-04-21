/**
 * @file ConfigFileWatcher.h
 * @brief 配置文件变更监听器，支持跨平台热加载
 * @copyright Copyright (c) 2026
 */
#ifndef CONFIGFILEWATCHER_H
#define CONFIGFILEWATCHER_H
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <limits.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace Base
{
    /**
     * @brief 文件变更事件类型
     */
    enum class FileChangeEvent : uint8_t
    {
        Modified, ///< 文件内容被修改
        Created,  ///< 文件被创建
        Deleted,  ///< 文件被删除
        Moved     ///< 文件被移动/重命名
    };

    /**
     * @brief 文件变更回调函数类型
     * @param file_path 发生变更的文件路径
     * @param event 变更事件类型
     */
    using FileChangeCallback = std::function<void(std::string_view file_path, FileChangeEvent event)>;

    /**
     * @brief 文件监听器接口
     *
     * 定义跨平台文件监听器的公共接口。
     */
    class IFileWatcher
    {
    public:
        virtual ~IFileWatcher() = default;

        /**
         * @brief 启动监听
         * @return true 成功，false 失败
         */
        virtual bool start() = 0;

        /**
         * @brief 停止监听
         */
        virtual void stop() = 0;

        /**
         * @brief 添加要监听的文件或目录
         * @param path 文件或目录路径
         * @param recursive 是否递归监听子目录（仅对目录有效）
         * @return true 成功，false 失败
         */
        virtual bool addWatch(std::string_view path, bool recursive = false) = 0;

        /**
         * @brief 移除监听
         * @param path 文件或目录路径
         * @return true 成功，false 失败
         */
        virtual bool removeWatch(std::string_view path) = 0;

        /**
         * @brief 设置变更回调
         */
        virtual void setCallback(FileChangeCallback callback) = 0;

        /**
         * @brief 检查监听器是否正在运行
         */
        [[nodiscard]] virtual bool isRunning() const noexcept = 0;
    };

    /**
     * @brief 文件监听器工厂
     */
    class FileWatcherFactory
    {
    public:
        /**
         * @brief 创建平台相关的文件监听器实例
         */
        static std::unique_ptr<IFileWatcher> create();
    };

#ifdef __linux__

    /**
     * @brief Linux 平台文件监听器（基于 inotify）
     *
     * 使用 Linux 内核 inotify 机制监听文件变更。
     * 特性：
     *   - 监听 IN_CLOSE_WRITE 事件，确保文件写入完成后再触发回调[reference:7]
     *   - 监听 IN_MOVED_TO 事件，支持 vim 等编辑器的原子保存操作
     *   - 独立监听线程，不阻塞主线程
     *   - 支持防抖，避免短时间内重复触发
     */
    class InotifyFileWatcher : public IFileWatcher
    {
    public:
        InotifyFileWatcher();

        ~InotifyFileWatcher() override;

        InotifyFileWatcher(const InotifyFileWatcher &) = delete;

        InotifyFileWatcher &operator=(const InotifyFileWatcher &) = delete;

        InotifyFileWatcher(InotifyFileWatcher &&) = delete;

        InotifyFileWatcher &operator=(InotifyFileWatcher &&) = delete;

        bool start() override;

        void stop() override;

        bool addWatch(std::string_view path, bool recursive = false) override;

        bool removeWatch(std::string_view path) override;

        void setCallback(FileChangeCallback callback) override;

        bool isRunning() const noexcept override;

        /**
         * @brief 设置防抖间隔（毫秒）
         */
        void setDebounceInterval(std::chrono::milliseconds interval) noexcept;

    private:
        void watchLoop();

        void processEvents();

        std::string getEventDescription(const struct inotify_event *event) const;

        int inotify_fd_{-1};
        std::unordered_map<int, std::string> watch_descriptors_; // wd -> path
        std::unordered_map<std::string, int> path_to_wd_;        // path -> wd

        FileChangeCallback callback_;
        std::unique_ptr<std::thread> watch_thread_;
        std::atomic<bool> running_{false};
        std::atomic<bool> should_stop_{false};

        // 防抖配置
        std::chrono::milliseconds debounce_interval_{100};
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_event_time_;

        static constexpr size_t EVENT_BUFFER_SIZE = 4096;
        static constexpr uint32_t WATCH_MASK = IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF;
    };

#elif defined(_WIN32)

    /**
     * @brief Windows 平台文件监听器（基于 ReadDirectoryChangesW）
     *
     * 使用 Windows ReadDirectoryChangesW API 监听目录变更。
     * 特性：
     *   - 监听目录下的文件修改、创建、删除、重命名事件
     *   - 独立监听线程，不阻塞主线程
     *   - 支持防抖，避免短时间内重复触发
     */
    class Win32FileWatcher : public IFileWatcher
    {
    public:
        Win32FileWatcher();

        ~Win32FileWatcher() override;

        Win32FileWatcher(const Win32FileWatcher &) = delete;

        Win32FileWatcher &operator=(const Win32FileWatcher &) = delete;

        Win32FileWatcher(Win32FileWatcher &&) = delete;

        Win32FileWatcher &operator=(Win32FileWatcher &&) = delete;

        bool start() override;

        void stop() override;

        bool addWatch(std::string_view path, bool recursive = false) override;

        bool removeWatch(std::string_view path) override;

        void setCallback(FileChangeCallback callback) override;

        bool isRunning() const noexcept override;

        void setDebounceInterval(std::chrono::milliseconds interval) noexcept;

    private:
        void watchLoop();

        void processNotification(BYTE *buffer, DWORD bytes_transferred);

        std::wstring toWideString(std::string_view str) const;

        struct WatchInfo
        {
            HANDLE directory_handle{INVALID_HANDLE_VALUE};
            std::string directory_path;
            OVERLAPPED overlapped{};
            BYTE buffer[8192]{};
            bool recursive{false};
        };

        std::unordered_map<std::string, std::unique_ptr<WatchInfo> > m_watches;
        FileChangeCallback m_callback;
        std::unique_ptr<std::thread> m_watch_thread;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_should_stop{false};

        std::chrono::milliseconds m_debounce_interval{100};
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_last_event_time;
    };

#else

    // 回退实现：基于轮询的简易文件监听器
    class PollingFileWatcher : public IFileWatcher
    {
    public:
        PollingFileWatcher();

        ~PollingFileWatcher() override;

        bool start() override;

        void stop() override;

        bool addWatch(std::string_view path, bool recursive = false) override;

        bool removeWatch(std::string_view path) override;

        void setCallback(FileChangeCallback callback) override;

        bool isRunning() const noexcept override;

        void setPollInterval(std::chrono::milliseconds interval) noexcept;

    private:
        void pollLoop();

        std::chrono::milliseconds poll_interval_{1000};

        struct WatchEntry
        {
            std::string path;
            std::chrono::file_clock::time_point last_write_time;
            bool recursive;
        };

        std::vector<WatchEntry> watches_;
        FileChangeCallback callback_;
        std::unique_ptr<std::thread> poll_thread_;
        std::atomic<bool> running_{false};
        std::atomic<bool> should_stop_{false};
    };

#endif
}

#endif //CONFIGFILEWATCHER_H
