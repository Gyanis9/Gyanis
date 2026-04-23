/**
 * @file Poller.hpp
 * @brief 跨平台 I/O 多路复用抽象与实现。
 * @details 封装了 Linux 下的 epoll 与 Windows 下的 IOCP，
 *          提供统一的 Poller 接口，用于异步事件驱动。
 *          支持监听可读、可写、错误及连接关闭等事件，
 *          并通过工厂函数 createPoller() 自动选择平台实现。
 */

#ifndef POLLER_HPP
#define POLLER_HPP

#include "PlatformCompat.hpp"

#include <cassert>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#ifdef __linux__
#include <sys/epoll.h>
#include <sys/eventfd.h>
#elif defined(_WIN32)
#endif

namespace Core
{
    /**
     * @enum PollEvent
     * @brief 描述 I/O 事件的标志位枚举。
     * @details 可读、可写、错误、连接关闭等事件可以按位组合，
     *          通过 operator| 和 operator& 进行组合与检测。
     */
    enum class PollEvent : uint32_t
    {
        None   = 0,      ///< 无事件
        Read   = 1 << 0, ///< 可读事件
        Write  = 1 << 1, ///< 可写事件
        Error  = 1 << 2, ///< 错误事件
        Closed = 1 << 3  ///< 连接关闭事件
    };

    /**
     * @brief 按位或操作，组合多个 PollEvent。
     * @param a 第一个事件标志
     * @param b 第二个事件标志
     * @return 组合后的事件集合
     */
    inline PollEvent operator|(PollEvent a, PollEvent b)
    {
        return static_cast<PollEvent>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    /**
     * @brief 按位与操作，检测事件集合中是否包含指定事件。
     * @param a 事件标志集合
     * @param b 待检测的事件
     * @return 若 a 中包含 b 的任意标志位则返回 true
     */
    inline bool operator&(PollEvent a, PollEvent b)
    {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }

    /**
     * @struct PollResult
     * @brief 封装一次就绪事件的结果。
     *
     * @details poll() 返回的列表中包含若干 PollResult，
     *          每个结果指明就绪的描述符（fd）、发生的事件类型以及
     *          注册时关联的用户数据指针。
     */
    struct PollResult
    {
        socket_t fd;      ///< 发生事件的套接字描述符
        PollEvent events; ///< 就绪的事件集合
        void *user_data;  ///< 用户自定义数据指针
    };

    /**
     * @class Poller
     * @brief I/O 多路复用抽象基类。
     *
     * @details 定义统一的添加、修改、移除监听以及阻塞等待就绪事件的接口。
     *          所有实现需保证线程安全（内部加锁），
     *          但 poll() 调用通常由单一线程独占执行。
     *          子类实现 epoll（Linux）或 IOCP（Windows）的具体细节。
     */
    class Poller
    {
    public:
        virtual ~Poller() = default;

        /**
         * @brief 添加一个需要监听的描述符。
         *
         * @param fd 套接字描述符。
         * @param events 感兴趣的事件集合（可组合）。
         * @param user_data 关联的用户数据指针，将在对应就绪事件中返回。
         * @return true 成功，false 失败。
         *
         * @note 若 fd 已经处于监听列表中，则等效于调用 modify()，
         *       更新事件掩码与用户数据。
         */
        virtual bool add(socket_t fd, PollEvent events, void *user_data) = 0;

        /**
         * @brief 修改已监听描述符的事件和用户数据。
         *
         * @param fd 已添加的套接字描述符。
         * @param events 新的事件集合。
         * @param user_data 新的用户数据指针。
         * @return true 成功，false 失败（包括 fd 未添加的情况）。
         */
        virtual bool modify(socket_t fd, PollEvent events, void *user_data) = 0;

        /**
         * @brief 移除指定描述符的监听。
         *
         * @param fd 要移除的套接字描述符。
         * @return true 成功，false 失败。
         */
        virtual bool remove(socket_t fd) = 0;

        /**
         * @brief 阻塞等待就绪事件。
         *
         * @param timeout_ms 超时时间，单位毫秒。负值表示无限等待。
         * @return 包含所有就绪事件的列表。
         */
        virtual std::vector<PollResult> poll(int timeout_ms) = 0;

        /**
         * @brief 唤醒正阻塞在 poll() 调用中的线程。
         *
         * @details 通常由其他线程调用，通过向内部唤醒描述符写入数据，
         *          使得 poll() 立即返回，以便处理外部信号或执行清理。
         */
        virtual void wake() = 0;
    };

    /**
     * @brief Poller 对象的工厂函数。
     * @return 平台相关的 Poller 实现实例（unique_ptr）。
     *         在 Linux 上返回 EpollPoller，
     *         在 Windows 上返回 IocpPoller。
     */
    std::unique_ptr<Poller> createPoller();

    // -------------------------------------------------------------------
    // Linux: epoll + eventfd
    // -------------------------------------------------------------------
#ifdef __linux__
    /**
     * @brief 基于 epoll 和 eventfd 的 Linux 平台 Poller 实现。
     *
     * @details 使用 edge-triggered (EPOLLET) 模式提高性能，
     *          并通过 eventfd 作为唤醒机制。
     *          所有对监听列表的修改都由内部 mutex 保护。
     */
    class EpollPoller : public Poller
    {
    public:
        /**
         * @brief 构造 EpollPoller 实例。
         * @throw std::system_error 如果创建 epoll 实例或 eventfd 失败。
         *
         * @details 创建 epoll 文件描述符及用作唤醒的 eventfd，
         *          并将 eventfd 注册到 epoll 中，用于 wake() 通知。
         */
        EpollPoller()
        {
            epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
            if (epoll_fd_ == -1)
            {
                throw std::system_error(errno, std::generic_category(), "epoll_create1");
            }
            wake_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (wake_fd_ == -1)
            {
                ::close(epoll_fd_);
                throw std::system_error(errno, std::generic_category(), "eventfd");
            }
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLET;
            ev.data.ptr = this; // 标记为唤醒事件
            if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wake_fd_, &ev) == -1)
            {
                ::close(wake_fd_);
                ::close(epoll_fd_);
                throw std::system_error(errno, std::generic_category(), "epoll_ctl wake_fd");
            }
        }

        /**
         * @brief 析构函数，关闭 epoll 及唤醒文件描述符。
         */
        ~EpollPoller() override
        {
            if (epoll_fd_ != -1)
            {
                ::close(epoll_fd_);
            }
            if (wake_fd_ != -1)
            {
                ::close(wake_fd_);
            }
        }

        /**
         * @brief 添加一个需要监听的描述符。
         *
         * @param fd 套接字描述符。
         * @param events 感兴趣的事件集合（可组合）。
         * @param user_data 关联的用户数据指针，将在对应就绪事件中返回。
         * @return true 成功，false 失败。
         *
         * @note 若 fd 已经处于监听列表中，则等效于调用 modify()，
         *       更新事件掩码与用户数据。
         */
        bool add(socket_t fd, PollEvent events, void *user_data) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            epoll_event ev{};
            ev.events = to_epoll_events(events) | EPOLLET;
            ev.data.ptr = user_data;
            int op = fds_.count(fd) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
            if (::epoll_ctl(epoll_fd_, op, fd, &ev) == -1)
            {
                return false;
            }
            fds_[fd] = user_data;
            return true;
        }

        /**
         * @brief 修改已监听描述符的事件和用户数据。
         *
         * @param fd 已添加的套接字描述符。
         * @param events 新的事件集合。
         * @param user_data 新的用户数据指针。
         * @return true 成功，false 失败（包括 fd 未添加的情况）。
         */
        bool modify(socket_t fd, PollEvent events, void *user_data) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!fds_.count(fd))
            {
                return false;
            }
            epoll_event ev{};
            ev.events = to_epoll_events(events) | EPOLLET;
            ev.data.ptr = user_data;
            if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1)
            {
                return false;
            }
            fds_[fd] = user_data;
            return true;
        }

        /**
         * @brief 移除指定描述符的监听。
         *
         * @param fd 要移除的套接字描述符。
         * @return true 成功，false 失败。
         */
        bool remove(socket_t fd) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!fds_.count(fd))
            {
                return true;
            }
            if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1)
            {
                return false;
            }
            fds_.erase(fd);
            return true;
        }

        /**
         * @brief 使用 epoll_wait 等待就绪事件。
         *
         * @param timeout_ms 超时毫秒数。
         * @return 就绪事件列表。
         *
         * @note 由于 epoll 事件不直接返回 fd，返回的 PollResult::fd
         *       会被设置为 INVALID_SOCKET_VAL；调用者应通过 user_data 来识别套接字。
         */
        std::vector<PollResult> poll(int timeout_ms) override
        {
            constexpr int MAX_EVENTS = 256;
            epoll_event events[MAX_EVENTS];
            int nfds = ::epoll_wait(epoll_fd_, events, MAX_EVENTS, timeout_ms);
            std::vector<PollResult> results;
            if (nfds > 0)
            {
                results.reserve(nfds);
                for (int i = 0; i < nfds; ++i)
                {
                    if (events[i].data.ptr == this)
                    {
                        // 唤醒事件
                        uint64_t dummy;
                        ::read(wake_fd_, &dummy, sizeof(dummy));
                        continue;
                    }
                    PollResult res;
                    res.user_data = events[i].data.ptr;
                    res.events = from_epoll_events(events[i].events);
                    // fd 信息需从 user_data 获取（实际使用中我们让 user_data 指向包含 fd 的结构）
                    // 此处为了通用性，不从 epoll 直接获取 fd。
                    res.fd = INVALID_SOCKET_VAL; // 调用者应通过 user_data 识别
                    results.push_back(res);
                }
            }
            return results;
        }

        /**
         * @brief 唤醒正阻塞在 poll() 调用中的线程。
         *
         * @details 通常由其他线程调用，通过向内部唤醒描述符写入数据，
         *          使得 poll() 立即返回，以便处理外部信号或执行清理。
         */
        void wake() override
        {
            uint64_t one = 1;
            ::write(wake_fd_, &one, sizeof(one));
        }

    private:
        /**
         * @brief 将核心事件标志转换为 epoll 事件掩码。
         * @param ev PollEvent 组合。
         * @return epoll 事件掩码。
         */
        static uint32_t toEpollEvents(PollEvent ev)
        {
            uint32_t e = 0;
            if (ev & PollEvent::Read)
            {
                e |= EPOLLIN;
            }
            if (ev & PollEvent::Write)
            {
                e |= EPOLLOUT;
            }
            return e;
        }

        /**
         * @brief 将 epoll 事件掩码转换为核心事件标志。
         * @param ev epoll 返回的事件掩码。
         * @return PollEvent 组合。
         */
        static PollEvent fromEpollEvents(uint32_t ev)
        {
            PollEvent res = PollEvent::None;
            if (ev & EPOLLIN)
            {
                res = res | PollEvent::Read;
            }
            if (ev & EPOLLOUT)
            {
                res = res | PollEvent::Write;
            }
            if (ev & EPOLLERR || ev & EPOLLHUP)
            {
                res = res | PollEvent::Error;
            }
            return res;
        }

        int epoll_fd_ = -1;                        ///< epoll 文件描述符
        int wake_fd_ = -1;                         ///< 用于唤醒的 eventfd 描述符
        std::mutex mutex_;                         ///< 互斥量，保护监听列表的访问
        std::unordered_map<socket_t, void *> fds_; ///< 记录所有被监听的描述符及其用户数据
    };
#endif // __linux__

    // -------------------------------------------------------------------
    // Windows: IOCP 完整实现（含 OVERLAPPED 管理）
    // -------------------------------------------------------------------
#ifdef _WIN32
    /**
     * @class IocpPoller
     * @brief 基于 IOCP 的 Windows 平台 Poller 实现。
     *
     * @details 利用 `GetQueuedCompletionStatusEx` 获取完成通知，
     *          并通过自连接 UDP socket 实现唤醒。
     *          借助挂起的 `WSARecv` 来检测可读事件，
     *          挂起的 `WSASend` 或类似机制检测可写（此处通过写完成触发）。
     *          内部维护每个 socket 的上下文（SocketContext），
     *          管理重叠 I/O 操作和缓冲区。
     */
    class IocpPoller : public Poller
    {
    public:
        /**
         * @brief 构造 IocpPoller 实例。
         * @throw std::system_error 若创建 IOCP 或唤醒 socket 失败。
         */
        IocpPoller();

        /**
         * @brief 析构函数，清理 IOCP 句柄、唤醒 socket 及所有 SocketContext。
         */
        ~IocpPoller() override;

        /**
        * @copydoc Poller::add()
         */
        bool add(socket_t fd, PollEvent events, void *user_data) override;

        /**
         * @copydoc Poller::modify()
         * @note 本实现中直接复用 add()。
         */
        bool modify(socket_t fd, PollEvent events, void *user_data) override;

        /**
         * @copydoc Poller::remove()
         */
        bool remove(socket_t fd) override;

        /**
        * @copydoc Poller::poll()
         */
        std::vector<PollResult> poll(int timeout_ms) override;

        /**
         * @copydoc Poller::wake()
         */
        void wake() override;

    private:
        /**
         * @struct SocketContext
         * @brief 每个被监听 socket 的上下文信息。
         */
        struct SocketContext
        {
            socket_t fd;                             ///< 套接字描述符
            void *user_data;                         ///< 用户自定义数据
            PollEvent interested_events;             ///< 当前感兴趣的事件集合
            PollEvent last_events = PollEvent::None; ///< 上次返回的事件（用于状态跟踪）
            IocpPoller *poller;                      ///< 所属的 IocpPoller 指针
            OVERLAPPED read_overlapped{};            ///< 用于读取操作的重叠结构
            OVERLAPPED write_overlapped{};           ///< 用于写入操作的重叠结构
            WSABUF read_buf{};                       ///< WSARecv 使用的缓冲区描述
            char read_buffer[4096];                  ///< 实际接收缓冲区
            bool read_pending = false;               ///< 是否已有读操作挂起
            bool write_pending = false;              ///< 是否已有写操作挂起

            /**
             * @brief 构造 SocketContext 并初始化缓冲区。
             * @param fd_ 套接字。
             * @param ud 用户数据。
             * @param ev 感兴趣的事件。
             * @param p 所属 Poller。
             */
            SocketContext(socket_t fd_, void *ud, PollEvent ev, IocpPoller *p);
        };

        /**
         * @brief 向指定 socket 投递一次 WSARecv 用于触发可读通知。
         * @param ctx 对应的 SocketContext。
         */
        void postRecv(SocketContext *ctx) const;

        HANDLE m_iocp = nullptr;                                         ///< IOCP 完成端口句柄
        socket_t m_wake_socket = INVALID_SOCKET;                         ///< 用于唤醒的自连接 UDP socket
        std::mutex m_mutex;                                              ///< 互斥量，保护 socket_contexts_
        std::unordered_map<socket_t, SocketContext *> m_socket_contexts; ///< 所有被监听的 socket 上下文
    };
#endif // _WIN32
}

#endif
