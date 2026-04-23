#include "Poller.hpp"

#include <ranges>

namespace Core
{
    /**
     * @brief Poller 对象工厂函数。
     * @return 平台特定的 Poller 实例。
     */
    inline std::unique_ptr<Poller> createPoller()
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

            // 关联到 IOCP
            if (!::CreateIoCompletionPort(reinterpret_cast<HANDLE>(fd), m_iocp, reinterpret_cast<ULONG_PTR>(ctx), 0))
            {
                delete ctx;
                m_socket_contexts.erase(fd);
                return false;
            }
            // 如果感兴趣读事件，立即投递一次 WSARecv
            if (events & PollEvent::Read)
            {
                postRecv(ctx);
            }
        } else
        {
            it->second->user_data = user_data;
            it->second->interested_events = events;

            // 若之前未投递读而现在需要读，则投递
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
        if (!ok && count == 0)
        {
            return results; // 超时或错误
        }
        for (ULONG i = 0; i < count; ++i)
        {
            auto *ctx = reinterpret_cast<SocketContext *>(entries[i].lpCompletionKey);
            if (ctx->poller == this)
            {
                // 唤醒事件（来自 wake_socket_ 的接收完成）
                // 消费数据
                char buf[64];
                while (::recv(m_wake_socket, buf, sizeof(buf), 0) > 0)
                {
                }
                continue;
            }
            if (!ctx)
            {
                continue;
            }
            auto *ov = entries[i].lpOverlapped;
            DWORD bytes = entries[i].dwNumberOfBytesTransferred;
            bool error = !(entries[i].Internal == 0);

            // 根据 OVERLAPPED 类型处理
            if (ov == &ctx->read_overlapped)
            {
                // 读完成
                if (!error && bytes > 0)
                {
                    // 有数据可读，触发读事件
                    PollResult res;
                    res.fd = ctx->fd;
                    res.events = PollEvent::Read;
                    res.user_data = ctx->user_data;
                    results.push_back(res);
                    // 继续投递下一次读（如果仍然感兴趣）
                    if (ctx->interested_events & PollEvent::Read)
                    {
                        postRecv(ctx);
                    }
                } else
                {
                    // 连接关闭或错误
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
                // 写完成
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
        return results;
    }

    void IocpPoller::wake()
    {
        // 向自连接 socket 发送一个字节唤醒 GetQueuedCompletionStatusEx
        constexpr char dummy = 0;
        ::send(m_wake_socket, &dummy, 1, 0);
    }

    IocpPoller::SocketContext::SocketContext(const socket_t fd_, void *ud, const PollEvent ev, IocpPoller *p) : fd(fd_), user_data(ud), interested_events(ev), poller(p)
    {
        read_buf.buf = read_buffer;
        read_buf.len = sizeof(read_buffer);
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
        if (ret == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
        {
            ctx->read_pending = false;
            // 投递失败，可能连接已关闭，稍后 poll 会处理
        }
    }
}
