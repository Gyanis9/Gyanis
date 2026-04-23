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
         * @brief 创建当前平台对应的文件监听器实现。
         * @return std::unique_ptr<IFileWatcher> 监听器实例。
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
        /**
         * @brief 构造 Linux inotify 监听器并初始化 inotify fd。
         */
        InotifyFileWatcher();

        /**
         * @brief 析构 inotify 监听器并清理线程与文件描述符。
         */
        ~InotifyFileWatcher() override;

        InotifyFileWatcher(const InotifyFileWatcher &) = delete;

        InotifyFileWatcher &operator=(const InotifyFileWatcher &) = delete;

        InotifyFileWatcher(InotifyFileWatcher &&) = delete;

        InotifyFileWatcher &operator=(InotifyFileWatcher &&) = delete;

        /**
         * @brief 启动 inotify 监听线程。
         * @return bool 成功启动返回 true。
         */
        bool start() override;

        /**
         * @brief 停止 inotify 监听线程。
         */
        void stop() override;

        /**
         * @brief 添加监听路径，可选递归子目录。
         * @param path 监听路径。
         * @param recursive 是否递归监听。
         * @return bool 添加成功返回 true。
         */
        bool addWatch(std::string_view path, bool recursive = false) override;

        /**
         * @brief 移除指定监听路径。
         * @param path 监听路径。
         * @return bool 移除成功返回 true。
         */
        bool removeWatch(std::string_view path) override;

        /**
         * @brief 设置文件变更回调函数。
         * @param callback 回调函数对象。
         */
        void setCallback(FileChangeCallback callback) override;

        /**
         * @brief 查询监听器运行状态。
         * @return bool 运行中返回 true。
         */
        bool isRunning() const noexcept override;

        /**
         * @brief 设置事件防抖时间间隔。
         * @param interval 防抖间隔。
         */
        void setDebounceInterval(std::chrono::milliseconds interval) noexcept;

    private:
        /**
         * @brief inotify 事件循环，读取并分发文件变更事件。
         */
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
        /**
         * @brief 构造 Win32 文件监听器。
         */
        Win32FileWatcher();

        /**
         * @brief 析构 Win32 文件监听器并释放句柄资源。
         */
        ~Win32FileWatcher() override;

        Win32FileWatcher(const Win32FileWatcher &) = delete;

        Win32FileWatcher &operator=(const Win32FileWatcher &) = delete;

        Win32FileWatcher(Win32FileWatcher &&) = delete;

        Win32FileWatcher &operator=(Win32FileWatcher &&) = delete;

        /**
         * @brief 启动 Win32 监听线程。
         * @return bool 成功启动返回 true。
         */
        bool start() override;

        /**
         * @brief 停止 Win32 监听线程。
         */
        void stop() override;

        /**
         * @brief 添加目录监听。
         * @param path 目录路径。
         * @param recursive 是否递归监听子目录。
         * @return bool 添加成功返回 true。
         */
        bool addWatch(std::string_view path, bool recursive = false) override;

        /**
         * @brief 移除目录监听并释放相关句柄。
         * @param path 目录路径。
         * @return bool 移除成功返回 true。
         */
        bool removeWatch(std::string_view path) override;

        /**
         * @brief 设置 Win32 监听回调。
         * @param callback 回调函数对象。
         */
        void setCallback(FileChangeCallback callback) override;

        /**
         * @brief 查询 Win32 监听器是否正在运行。
         * @return bool 运行中返回 true。
         */
        bool isRunning() const noexcept override;

        /**
         * @brief 设置 Win32 监听防抖时间间隔。
         * @param interval 防抖间隔。
         */
        void setDebounceInterval(std::chrono::milliseconds interval) noexcept;

    private:
        /**
         * @brief Win32 监听事件循环，轮询目录变更通知。
         */
        void watchLoop();

        /**
         * @brief 处理 ReadDirectoryChangesW 原始通知缓冲区。
         * @param buffer 通知缓冲区。
         * @param bytes_transferred
         */
        void processNotification(BYTE *buffer, DWORD bytes_transferred);

        /**
         * @brief 将 UTF-8 字符串转换为宽字符字符串。
         * @param str UTF-8 输入字符串。
         * @return std::wstring 宽字符结果。
         */
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
        /**
         * @brief 构造轮询监听器。
         */
        PollingFileWatcher();

        /**
         * @brief 析构轮询监听器并停止后台线程。
         */
        ~PollingFileWatcher() override;

        /**
         * @brief 启动轮询监听线程。
         * @return bool 成功启动返回 true。
         */
        bool start() override;

        /**
         * @brief 停止轮询监听线程。
         */
        void stop() override;

        /**
         * @brief 添加轮询监听路径。
         * @param path 监听路径。
         * @param recursive 是否递归监听（轮询实现中仅记录该标志）。
         * @return bool 添加成功返回 true。
         */
        bool addWatch(std::string_view path, bool recursive = false) override;

        /**
         * @brief 移除轮询监听路径。
         * @param path 监听路径。
         * @return bool 移除成功返回 true。
         */
        bool removeWatch(std::string_view path) override;

        /**
         * @brief 设置轮询监听回调。
         * @param callback 回调函数对象。
         */
        void setCallback(FileChangeCallback callback) override;

        /**
         * @brief 查询轮询监听器运行状态。
         * @return bool 运行中返回 true。
         */
        bool isRunning() const noexcept override;

        /**
         * @brief 设置轮询周期。
         * @param interval 轮询时间间隔。
         */
        void setPollInterval(std::chrono::milliseconds interval) noexcept;

    private:
        /**
         * @brief 轮询循环，检测文件时间戳变化并触发回调。
         */
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
