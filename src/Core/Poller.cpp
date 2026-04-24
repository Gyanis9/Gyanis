#include "Poller.hpp"

#include <iostream>
#include <ranges>
#include <vector>

namespace Core
{
    /**
     * @brief Poller 对象工厂函数。
     * @return 平台特定的 Poller 实例。
     */
    std::unique_ptr<Poller> createPoller()
    {
#ifdef __linux__
        return std::make_unique<EpollPoller>();
#elif defined(_WIN32)
        return std::make_unique<IocpPoller>();
#else
        static_assert(false, "Unsupported platform");
#endif
    }

    IocpPoller::IocpPoller()
    {
        m_iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
        if (!m_iocp)
        {
            throw std::system_error(::GetLastError(), std::system_category(), "CreateIoCompletionPort");
        }
        // 创建自连接 UDP socket 用于唤醒
        m_wake_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (m_wake_socket == INVALID_SOCKET)
        {
            ::CloseHandle(m_iocp);
            throw std::system_error(::WSAGetLastError(), std::system_category(), "socket for wake");
        }
        setNonblocking(m_wake_socket, true);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;

        if (::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1)
        {
            // 处理错误，比如抛异常或关闭资源
            ::closesocket(m_wake_socket);
            ::CloseHandle(m_iocp);
            throw std::system_error(::WSAGetLastError(), std::system_category(), "inet_pton failed");
        }

        addr.sin_port = 0;
        ::bind(m_wake_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
        int len = sizeof(addr);
        ::getsockname(m_wake_socket, reinterpret_cast<sockaddr *>(&addr), &len);
        ::connect(m_wake_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));

        // 关联到 IOCP，key 为 this
        if (!::CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_wake_socket), m_iocp, reinterpret_cast<ULONG_PTR>(this), 0))
        {
            ::closesocket(m_wake_socket);
            ::CloseHandle(m_iocp);
            throw std::system_error(::GetLastError(), std::system_category(), "CreateIoCompletionPort wake");
        }
    }

    IocpPoller::~IocpPoller()
    {
        if (m_wake_socket != INVALID_SOCKET)
        {
            ::closesocket(m_wake_socket);
        }
        if (m_iocp)
        {
            ::CloseHandle(m_iocp);
        }

        // 清理所有 PerIoData
        for (const auto &ctx: m_socket_contexts | std::views::values)
        {
            delete ctx;
        }
    }

    bool IocpPoller::add(const socket_t fd, const PollEvent events, void *user_data)
    {
        std::lock_guard lock(m_mutex);
        if (const auto it = m_socket_contexts.find(fd); it == m_socket_contexts.end())
        {
            auto *ctx = new SocketContext{fd, user_data, events, this};
            m_socket_contexts[fd] = ctx;

            // 关联到 IOCP。如果失败（例如 socket 已有关联），仍保留 ctx 以便 select 回退机制正常工作。
            if (!::CreateIoCompletionPort(reinterpret_cast<HANDLE>(fd), m_iocp, reinterpret_cast<ULONG_PTR>(ctx), 0))
            {
                // 不删除 ctx — select 回退机制仍可通过 m_socket_contexts 检测该 socket
            }
            // 如果感兴趣读事件，立即投递一次 WSARecv（零字节通知）
            if (events & PollEvent::Read)
            {
                postRecv(ctx);
            }
        } else
        {
            it->second->user_data = user_data;
            it->second->interested_events = events;

            if ((events & PollEvent::Read) && !(it->second->last_events & PollEvent::Read))
            {
                postRecv(it->second);
            }
        }
        return true;
    }

    bool IocpPoller::modify(const socket_t fd, const PollEvent events, void *user_data)
    {
        return add(fd, events, user_data);
    }

    bool IocpPoller::remove(const socket_t fd)
    {
        std::lock_guard lock(m_mutex);
        const auto it = m_socket_contexts.find(fd);
        if (it == m_socket_contexts.end())
        {
            return true;
        }
        const auto *ctx = it->second;
        // 取消该 socket 上的所有 I/O
        ::CancelIoEx(reinterpret_cast<HANDLE>(fd), nullptr);
        m_socket_contexts.erase(it);
        // 注意：可能有正在处理的完成包，需在完成处理中忽略
        // 延迟删除 ctx（此处简化，直接删除，完成回调需判空）
        delete ctx;
        return true;
    }

    std::vector<PollResult> IocpPoller::poll(const int timeout_ms)
    {
        const DWORD timeout = (timeout_ms < 0) ? INFINITE : static_cast<DWORD>(timeout_ms);
        OVERLAPPED_ENTRY entries[64];
        ULONG count = 0;
        const BOOL ok = ::GetQueuedCompletionStatusEx(m_iocp, entries, 64, &count, timeout, FALSE);
        std::vector<PollResult> results;

        // 处理 IOCP 完成事件
        if (ok || count > 0)
        {
            for (ULONG i = 0; i < count; ++i)
            {
                // 唤醒事件：completion key == this（通过 PostQueuedCompletionStatus 发起）
                if (entries[i].lpCompletionKey == reinterpret_cast<ULONG_PTR>(this))
                {
                    continue;
                }

                auto *ctx = reinterpret_cast<SocketContext *>(entries[i].lpCompletionKey);
                if (!ctx)
                {
                    continue;
                }
                const auto *ov = entries[i].lpOverlapped;
                const bool error = entries[i].Internal != 0;

                if (ov == &ctx->read_overlapped)
                {
                    ctx->read_pending = false;
                    if (!error)
                    {
                        // 零字节 WSARecv 完成 = 数据已到达（不消费数据）
                        PollResult res;
                        res.fd = ctx->fd;
                        res.events = PollEvent::Read;
                        res.user_data = ctx->user_data;
                        results.push_back(res);
                        // 继续投递下一次读通知
                        if (ctx->interested_events & PollEvent::Read)
                        {
                            postRecv(ctx);
                        }
                    } else
                    {
                        if (ctx->interested_events & PollEvent::Read)
                        {
                            PollResult res;
                            res.fd = ctx->fd;
                            res.events = PollEvent::Closed | PollEvent::Error;
                            res.user_data = ctx->user_data;
                            results.push_back(res);
                        }
                    }
                } else if (ov == &ctx->write_overlapped)
                {
                    if (!error)
                    {
                        PollResult res;
                        res.fd = ctx->fd;
                        res.events = PollEvent::Write;
                        res.user_data = ctx->user_data;
                        results.push_back(res);
                    } else
                    {
                        PollResult res;
                        res.fd = ctx->fd;
                        res.events = PollEvent::Error;
                        res.user_data = ctx->user_data;
                        results.push_back(res);
                    }
                }
            }
        }

        // 轮询监听 socket：WSARecv 因 WSAEOPNOTSUPP/WSAENOTCONN 失败后
        // postRecv 通过设置 Closed 标记将其标记为监听 socket。
        // 使用 select() 检测可读性（不消费连接），若有新连接则触发 Read 事件。
        {
            std::lock_guard lock(m_mutex);
            for (const auto &[fd, ctx]: m_socket_contexts)
            {
                if (!(ctx->interested_events & PollEvent::Read))
                    continue;
                if (ctx->read_pending || !(ctx->interested_events & PollEvent::Closed))
                    continue;

                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(fd, &read_fds);
                struct timeval tv = {0, 0};
                if (const int sel_ret = ::select(0, &read_fds, nullptr, nullptr, &tv); sel_ret > 0 && FD_ISSET(fd, &read_fds))
                {
                    PollResult res;
                    res.fd = fd;
                    res.events = PollEvent::Read;
                    res.user_data = ctx->user_data;
                    results.push_back(res);
                    break; // 每个 poll 周期只处理一个 fd，降低延迟
                }
            }
        }

        return results;
    }

    void IocpPoller::wake()
    {
        // PostQueuedCompletionStatus 是唤醒 GetQueuedCompletionStatusEx 的标准方式
        ::PostQueuedCompletionStatus(m_iocp, 0, reinterpret_cast<ULONG_PTR>(this), nullptr);
    }

    IocpPoller::SocketContext::SocketContext(const socket_t fd_, void *ud, const PollEvent ev, IocpPoller *p)
        : fd(fd_), user_data(ud), interested_events(ev), poller(p)
    {
        // 零字节 WSABUF — WSARecv 仅作为数据到达通知，不消费数据
        // 后续协程中的 ::recv() 仍能读取完整数据
        read_buf.buf = nullptr;
        read_buf.len = 0;
    }

    void IocpPoller::postRecv(SocketContext *ctx) const
    {
        if (ctx->read_pending)
        {
            return;
        }
        DWORD flags = 0;
        ctx->read_pending = true;
        const int ret = ::WSARecv(ctx->fd, &ctx->read_buf, 1, nullptr, &flags,
                                  &ctx->read_overlapped, nullptr);
        if (ret == SOCKET_ERROR)
        {
            const int err = ::WSAGetLastError();
            if (err == WSA_IO_PENDING)
            {
                return; // 正常 IOCP 等待
            }
            ctx->read_pending = false;
            if (err == WSAEOPNOTSUPP || err == WSAENOTCONN)
            {
                // 监听 socket：标记为非数据 socket，由 poll() 中使用 accept 轮询
                ctx->interested_events = ctx->interested_events | PollEvent::Closed;
            }
            // 其他错误：连接已关闭，不做特殊处理
        }
    }
}
