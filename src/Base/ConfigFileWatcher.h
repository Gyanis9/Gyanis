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
     * @details 定义跨平台文件监听器的公共接口，所有平台实现均需遵循此接口。
     */
    class IFileWatcher
    {
    public:
        virtual ~IFileWatcher() = default;

        /**
         * @brief 启动监听
         * @return true 成功启动；false 启动失败
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
         * @return true 添加成功；false 添加失败
         */
        virtual bool addWatch(std::string_view path, bool recursive = false) = 0;

        /**
         * @brief 移除监听
         * @param path 文件或目录路径
         * @return true 移除成功；false 移除失败
         */
        virtual bool removeWatch(std::string_view path) = 0;

        /**
         * @brief 设置变更回调函数
         * @param callback 回调函数对象
         */
        virtual void setCallback(FileChangeCallback callback) = 0;

        /**
         * @brief 检查监听器是否正在运行
         * @return true 正在运行；false 未运行
         */
        [[nodiscard]] virtual bool isRunning() const noexcept = 0;
    };

    /**
     * @brief 文件监听器工厂类
     */
    class FileWatcherFactory
    {
    public:
        /**
         * @brief 创建当前平台对应的文件监听器实现
         * @return std::unique_ptr<IFileWatcher> 监听器实例指针
         */
        static std::unique_ptr<IFileWatcher> create();
    };

#ifdef __linux__

    /**
     * @brief Linux 平台文件监听器（基于 inotify）
     *
     * @details 使用 Linux 内核 inotify 机制监听文件变更。
     *          特性：
     *            - 监听 IN_CLOSE_WRITE 事件，确保文件写入完成后再触发回调
     *            - 监听 IN_MOVED_TO 事件，支持 vim 等编辑器的原子保存操作
     *            - 独立监听线程，不阻塞主线程
     *            - 支持防抖，避免短时间内重复触发
     */
    class InotifyFileWatcher : public IFileWatcher
    {
    public:
        /**
         * @brief 构造 Linux inotify 监听器，初始化 inotify 文件描述符
         */
        InotifyFileWatcher();

        /**
         * @brief 析构 inotify 监听器，清理线程与文件描述符
         */
        ~InotifyFileWatcher() override;

        InotifyFileWatcher(const InotifyFileWatcher &) = delete;

        InotifyFileWatcher &operator=(const InotifyFileWatcher &) = delete;

        InotifyFileWatcher(InotifyFileWatcher &&) = delete;

        InotifyFileWatcher &operator=(InotifyFileWatcher &&) = delete;

        /**
         * @brief 启动 inotify 监听线程
         * @return bool 成功启动返回 true，否则返回 false
         */
        bool start() override;

        /**
         * @brief 停止 inotify 监听线程
         */
        void stop() override;

        /**
         * @brief 添加监听路径，可选递归子目录
         * @param path 监听路径
         * @param recursive 是否递归监听子目录
         * @return bool 添加成功返回 true，否则返回 false
         */
        bool addWatch(std::string_view path, bool recursive = false) override;

        /**
         * @brief 移除指定监听路径
         * @param path 监听路径
         * @return bool 移除成功返回 true，否则返回 false
         */
        bool removeWatch(std::string_view path) override;

        /**
         * @brief 设置文件变更回调函数
         * @param callback 回调函数对象
         */
        void setCallback(FileChangeCallback callback) override;

        /**
         * @brief 查询监听器运行状态
         * @return bool 运行中返回 true，否则返回 false
         */
        bool isRunning() const noexcept override;

        /**
         * @brief 设置事件防抖时间间隔
         * @param interval 防抖间隔（毫秒）
         */
        void setDebounceInterval(std::chrono::milliseconds interval) noexcept;

    private:
        /**
         * @brief inotify 事件循环，读取并分发文件变更事件
         */
        void watchLoop();

        /**
         * @brief 处理已发生的 inotify 事件
         */
        void processEvents();

        /**
         * @brief 获取事件的描述信息（用于调试）
         * @param event inotify 事件指针
         * @return std::string 描述字符串
         */
        std::string getEventDescription(const struct inotify_event *event) const;

        int inotify_fd_{-1};                                     ///< inotify 文件描述符
        std::unordered_map<int, std::string> watch_descriptors_; ///< 监视描述符 -> 路径映射
        std::unordered_map<std::string, int> path_to_wd_;        ///< 路径 -> 监视描述符映射

        FileChangeCallback callback_;               ///< 用户回调函数
        std::unique_ptr<std::thread> watch_thread_; ///< 监听线程
        std::atomic<bool> running_{false};          ///< 是否正在运行
        std::atomic<bool> should_stop_{false};      ///< 是否应该停止

        std::chrono::milliseconds debounce_interval_{100};                                       ///< 防抖间隔（毫秒）
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_event_time_; ///< 上次触发时间

        static constexpr size_t EVENT_BUFFER_SIZE = 4096;                                                    ///< 事件缓冲区大小
        static constexpr uint32_t WATCH_MASK = IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF; ///< 监听的事件掩码
    };

#elif defined(_WIN32)

    /**
     * @brief Windows 平台文件监听器（基于 ReadDirectoryChangesW）
     *
     * @details 使用 Windows ReadDirectoryChangesW API 监听目录变更。
     *          特性：
     *            - 监听目录下的文件修改、创建、删除、重命名事件
     *            - 独立监听线程，不阻塞主线程
     *            - 支持防抖，避免短时间内重复触发
     */
    class Win32FileWatcher : public IFileWatcher
    {
    public:
        /**
         * @brief 构造 Win32 文件监听器
         */
        Win32FileWatcher();

        /**
         * @brief 析构 Win32 文件监听器，释放句柄资源
         */
        ~Win32FileWatcher() override;

        Win32FileWatcher(const Win32FileWatcher &) = delete;

        Win32FileWatcher &operator=(const Win32FileWatcher &) = delete;

        Win32FileWatcher(Win32FileWatcher &&) = delete;

        Win32FileWatcher &operator=(Win32FileWatcher &&) = delete;

        /**
         * @brief 启动 Win32 监听线程
         * @return bool 成功启动返回 true，否则返回 false
         */
        bool start() override;

        /**
         * @brief 停止 Win32 监听线程
         */
        void stop() override;

        /**
         * @brief 添加目录监听
         * @param path 目录路径
         * @param recursive 是否递归监听子目录
         * @return bool 添加成功返回 true，否则返回 false
         */
        bool addWatch(std::string_view path, bool recursive = false) override;

        /**
         * @brief 移除目录监听并释放相关句柄
         * @param path 目录路径
         * @return bool 移除成功返回 true，否则返回 false
         */
        bool removeWatch(std::string_view path) override;

        /**
         * @brief 设置 Win32 监听回调
         * @param callback 回调函数对象
         */
        void setCallback(FileChangeCallback callback) override;

        /**
         * @brief 查询 Win32 监听器是否正在运行
         * @return bool 运行中返回 true，否则返回 false
         */
        bool isRunning() const noexcept override;

        /**
         * @brief 设置 Win32 监听防抖时间间隔
         * @param interval 防抖间隔（毫秒）
         */
        void setDebounceInterval(std::chrono::milliseconds interval) noexcept;

    private:
        /**
         * @brief Win32 监听事件循环，轮询目录变更通知
         */
        void watchLoop();

        /**
         * @brief 处理 ReadDirectoryChangesW 原始通知缓冲区
         * @param buffer 通知缓冲区指针
         * @param bytes_transferred 实际传输的字节数
         */
        void processNotification(BYTE *buffer, DWORD bytes_transferred);

        /**
         * @brief 将 UTF-8 字符串转换为宽字符字符串
         * @param str UTF-8 输入字符串
         * @return std::wstring 宽字符结果
         */
        std::wstring toWideString(std::string_view str) const;

        /**
         * @brief 目录监听信息结构体
         */
        struct WatchInfo
        {
            HANDLE directory_handle{INVALID_HANDLE_VALUE}; ///< 目录句柄
            std::string directory_path;                    ///< 目录路径
            OVERLAPPED overlapped{};                       ///< 异步 I/O 重叠结构
            BYTE buffer[8192]{};                           ///< 事件缓冲区
            bool recursive{false};                         ///< 是否递归监听
        };

        std::unordered_map<std::string, std::unique_ptr<WatchInfo> > m_watches; ///< 路径->监听信息映射
        FileChangeCallback m_callback;                                          ///< 用户回调函数
        std::unique_ptr<std::thread> m_watch_thread;                            ///< 监听线程
        std::atomic<bool> m_running{false};                                     ///< 是否正在运行
        std::atomic<bool> m_should_stop{false};                                 ///< 是否应该停止

        std::chrono::milliseconds m_debounce_interval{100};                                       ///< 防抖间隔（毫秒）
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_last_event_time; ///< 上次触发时间
    };

#else

    /**
     * @brief 回退实现：基于轮询的简易文件监听器
     *
     * @details 当系统不支持 inotify 或 ReadDirectoryChangesW 时，使用轮询方式实现文件变更监听。
     *          通过定期检查文件最后修改时间来判断是否变更。
     */
    class PollingFileWatcher : public IFileWatcher
    {
    public:
        /**
         * @brief 构造轮询监听器
         */
        PollingFileWatcher();

        /**
         * @brief 析构轮询监听器，停止后台线程
         */
        ~PollingFileWatcher() override;

        /**
         * @brief 启动轮询监听线程
         * @return bool 成功启动返回 true，否则返回 false
         */
        bool start() override;

        /**
         * @brief 停止轮询监听线程
         */
        void stop() override;

        /**
         * @brief 添加轮询监听路径
         * @param path 监听路径
         * @param recursive 是否递归监听（轮询实现中仅记录该标志，实际不递归）
         * @return bool 添加成功返回 true（总是成功）
         */
        bool addWatch(std::string_view path, bool recursive = false) override;

        /**
         * @brief 移除轮询监听路径
         * @param path 监听路径
         * @return bool 移除成功返回 true，路径不存在返回 false
         */
        bool removeWatch(std::string_view path) override;

        /**
         * @brief 设置轮询监听回调
         * @param callback 回调函数对象
         */
        void setCallback(FileChangeCallback callback) override;

        /**
         * @brief 查询轮询监听器运行状态
         * @return bool 运行中返回 true，否则返回 false
         */
        bool isRunning() const noexcept override;

        /**
         * @brief 设置轮询周期
         * @param interval 轮询时间间隔（毫秒）
         */
        void setPollInterval(std::chrono::milliseconds interval) noexcept;

    private:
        /**
         * @brief 轮询循环，检测文件时间戳变化并触发回调
         */
        void pollLoop();

        std::chrono::milliseconds poll_interval_{1000}; ///< 轮询间隔（毫秒）

        /**
         * @brief 监听条目结构体
         */
        struct WatchEntry
        {
            std::string path;                                    ///< 路径
            std::chrono::file_clock::time_point last_write_time; ///< 上次写入时间
            bool recursive;                                      ///< 是否递归（轮询实现仅存储）
        };

        std::vector<WatchEntry> watches_;          ///< 监听列表
        FileChangeCallback callback_;              ///< 用户回调函数
        std::unique_ptr<std::thread> poll_thread_; ///< 轮询线程
        std::atomic<bool> running_{false};         ///< 是否正在运行
        std::atomic<bool> should_stop_{false};     ///< 是否应该停止
    };

#endif
}

#endif
