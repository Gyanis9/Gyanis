#include "Timer.hpp"


namespace Core
{
    WindowsTimerService::WindowsTimerService()
    {
        m_timer_queue = ::CreateTimerQueue();
        if (!m_timer_queue)
        {
            throw std::system_error(::GetLastError(), std::system_category(), "CreateTimerQueue");
        }
        // 创建一个自连接的 UDP socket 用于唤醒 IOCP 循环（可选，IOCP 本身可通过 PostQueuedCompletionStatus 唤醒）
        m_wake_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (m_wake_socket == INVALID_SOCKET)
        {
            ::DeleteTimerQueueEx(m_timer_queue, nullptr);
            throw std::system_error(::WSAGetLastError(), std::system_category(), "socket for wake");
        }
        setNonblocking(m_wake_socket, true);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;

        if (::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1)
        {
            // 处理错误，比如抛异常或关闭资源
            ::closesocket(m_wake_socket);
            ::DeleteTimerQueueEx(m_timer_queue, nullptr);
            throw std::system_error(::WSAGetLastError(), std::system_category(), "inet_pton failed");
        }
        addr.sin_port = 0;
        ::bind(m_wake_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
        int len = sizeof(addr);
        ::getsockname(m_wake_socket, reinterpret_cast<sockaddr *>(&addr), &len);
        ::connect(m_wake_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    }

    WindowsTimerService::~WindowsTimerService()
    {
        if (m_timer_queue)
        {
            ::DeleteTimerQueueEx(m_timer_queue, nullptr);
        }
        if (m_wake_socket != INVALID_SOCKET)
        {
            closeSocket(m_wake_socket);
        }
    }

    void WindowsTimerService::scheduleAfter(const milliseconds delay, Callback cb)
    {
        // 使用定时器队列，到期时在线程池线程执行回调
        auto *ctx = new TimerContext{std::move(cb), this};
        if (const DWORD due = static_cast<DWORD>(delay.count()); !::CreateTimerQueueTimer(
                                                                                          &ctx->timer, m_timer_queue, TimerRoutine, ctx,
                                                                                          due, 0, WT_EXECUTEDEFAULT))
        {
            delete ctx;
            throw std::system_error(::GetLastError(), std::system_category(), "CreateTimerQueueTimer");
        }
    }

    socket_t WindowsTimerService::getNotifierFd() const
    {
        return m_wake_socket;
    }

    void WindowsTimerService::processExpired()
    {
        // 清空唤醒 socket 的接收缓冲区
        char buf[64];
        while (::recv(m_wake_socket, buf, sizeof(buf), 0) > 0)
        {
        }
    }

    void WindowsTimerService::TimerRoutine(const PVOID lpParam, BOOLEAN)
    {
        const auto *ctx = static_cast<TimerContext *>(lpParam);
        // 在删除 ctx 前保存需要使用的字段
        const socket_t wake_socket = ctx->service->m_wake_socket;
        const HANDLE timer_queue = ctx->service->m_timer_queue;
        const HANDLE timer = ctx->timer;

        ctx->callback();
        ::DeleteTimerQueueTimer(timer_queue, timer, nullptr);
        delete ctx;

        // 发送一个字节到自连接 socket 以唤醒 IOCP 循环（使得 poll 能够返回）
        constexpr char dummy = 0;
        ::send(wake_socket, &dummy, 1, 0);
    }
}
