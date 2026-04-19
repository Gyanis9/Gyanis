#include "core/Hook.h"

#include <atomic>
#include <chrono>
#include <climits>
#include <limits>
#include <memory>
#include <utility>

#if defined(_WIN32)
#include <algorithm>
#include <cerrno>
#else
#include <cerrno>
#include <cstdarg>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

#include "Fiber.h"
#include "base/Config.h"
#include "base/Log.h"
#include "base/Macro.h"
#include "core/FdManager.h"
#include "core/IOManager.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#ifdef ERROR
#undef ERROR
#endif
#ifdef Yield
#undef Yield
#endif
#endif

static auto g_logger = LOG_NAME("system");

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

namespace Gyanis::core
{
    thread_local bool t_hook_enable = false;
    auto g_tcp_connect_timeout = base::Config::LookUp<uint64_t>("tcp.connect.timeout", 5000, "tcp connect timeout");
    uint64_t s_connect_timeout = std::numeric_limits<uint64_t>::max();

    void hook_init()
    {
        static std::atomic_bool is_inited{false};
        if (is_inited.exchange(true))
        {
            return;
        }

#if defined(_WIN32)
        LOG_INFO(g_logger) << "[Hook] Windows 平台使用回退模式，不进行 dlsym 系统调用劫持。";
#else
#define XX(name) name ## _f = reinterpret_cast<name ## _fun>(dlsym(RTLD_NEXT, #name));
        HOOK_FUN(XX)
#undef XX
#endif
    }

    struct HookIniter
    {
        HookIniter()
        {
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();
            g_tcp_connect_timeout->addListener([](const uint64_t &old_value, const uint64_t &new_value)
            {
                LOG_INFO(g_logger)
                    << "[Hook] TCP 连接超时已更新，旧值=" << old_value
                    << "，新值=" << new_value;
                s_connect_timeout = new_value;
            });
        }
    };

    [[maybe_unused]] static HookIniter s_hook_initer;

    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    void set_hook_enable(const bool flag)
    {
        t_hook_enable = flag;
    }
}

extern "C"
{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX)
#undef XX
}

#if defined(_WIN32)
extern "C" int connect_with_timeout(const int fd, const sockaddr *addr, const socklen_t addrlen, const uint64_t timeout_ms)
{
    if (fd < 0)
    {
        errno = EBADF;
        WSASetLastError(WSAENOTSOCK);
        return -1;
    }

    const SOCKET sock = static_cast<SOCKET>(fd);

    if (!Gyanis::core::t_hook_enable || timeout_ms == std::numeric_limits<uint64_t>::max())
    {
        const int ret = ::connect(sock, addr, static_cast<int>(addrlen));
        if (ret != 0)
        {
            errno = WSAGetLastError();
        }
        return ret;
    }

    u_long non_block = 1;
    if (ioctlsocket(sock, FIONBIO, &non_block) != 0)
    {
        const int ret = ::connect(sock, addr, static_cast<int>(addrlen));
        if (ret != 0)
        {
            errno = WSAGetLastError();
        }
        return ret;
    }

    const auto restore_blocking = [sock]()
    {
        u_long blocking = 0;
        static_cast<void>(ioctlsocket(sock, FIONBIO, &blocking));
    };

    const int connect_ret = ::connect(sock, addr, static_cast<int>(addrlen));
    if (connect_ret == 0)
    {
        restore_blocking();
        return 0;
    }

    const int wsa_error = WSAGetLastError();
    if (wsa_error != WSAEWOULDBLOCK && wsa_error != WSAEINPROGRESS && wsa_error != WSAEINVAL)
    {
        restore_blocking();
        errno = wsa_error;
        return -1;
    }

    fd_set write_set;
    FD_ZERO(&write_set);
    FD_SET(sock, &write_set);

    timeval timeout{};
    timeout.tv_sec = static_cast<long>(std::min<uint64_t>(timeout_ms / 1000, static_cast<uint64_t>(LONG_MAX)));
    timeout.tv_usec = static_cast<long>(std::min<uint64_t>((timeout_ms % 1000) * 1000, 999000));

    const int select_ret = ::select(0, nullptr, &write_set, nullptr, &timeout);
    if (select_ret <= 0)
    {
        if (select_ret == 0)
        {
            WSASetLastError(WSAETIMEDOUT);
            errno = ETIMEDOUT;
        }
        else
        {
            errno = WSAGetLastError();
        }
        restore_blocking();
        return -1;
    }

    int so_error = 0;
    int len = sizeof(so_error);
    if (::getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&so_error), &len) == SOCKET_ERROR)
    {
        errno = WSAGetLastError();
        restore_blocking();
        return -1;
    }

    restore_blocking();
    if (so_error != 0)
    {
        WSASetLastError(so_error);
        errno = so_error;
        return -1;
    }

    return 0;
}

#else
struct timer_info
{
    std::atomic<int> cancelled{0};
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name, uint32_t event, const int timeout_so, Args &&...args)
{
    if (!Gyanis::core::t_hook_enable)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    const auto ctx = Gyanis::core::FdMgr::GetInstance()->get(fd);
    if (!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose())
    {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket() || ctx->getUserNonblock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    const auto to = ctx->getTimeout(timeout_so);
    const auto tinfo = std::make_shared<timer_info>();

    while (true)
    {
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        while (n == -1 && errno == EINTR)
        {
            n = fun(fd, std::forward<Args>(args)...);
        }
        if (n == -1 && errno == EAGAIN)
        {
            auto *iom = Gyanis::core::IOManager::GetThis();
            if (iom == nullptr)
            {
                LOG_ERROR(g_logger)
                    << "[Hook] " << hook_fun_name << " 执行失败：当前线程没有 IOManager。";
                return -1;
            }

            uint64_t timer_id = 0;
            std::weak_ptr winfo(tinfo);
            if (to != std::chrono::milliseconds::max())
            {
                timer_id = iom->addConditionTimer(to.count(), [winfo, fd, iom, event]()
                {
                    const auto t = winfo.lock();
                    if (!t || t->cancelled.load(std::memory_order_relaxed))
                    {
                        return;
                    }
                    t->cancelled.store(ETIMEDOUT, std::memory_order_release);
                    iom->cancelEvent(fd, static_cast<Gyanis::core::IOManager::Event>(event));
                }, winfo);
            }

            if (const int rt = iom->addEvent(fd, static_cast<Gyanis::core::IOManager::Event>(event)); UNLIKELY(rt))
            {
                LOG_ERROR(g_logger)
                    << "[Hook] " << hook_fun_name << " 添加事件失败，fd=" << fd
                    << "，event=" << event;
                if (timer_id)
                {
                    iom->cancel(timer_id);
                }
                return -1;
            }

            Gyanis::core::Fiber::Yield();
            if (timer_id)
            {
                iom->cancel(timer_id);
            }
            if (const int cancelled = tinfo->cancelled.load(std::memory_order_acquire))
            {
                errno = cancelled;
                return -1;
            }
        }
        else
        {
            return n;
        }
    }
}

extern "C"
{
    unsigned int sleep(unsigned int seconds)
    {
        if (!Gyanis::core::t_hook_enable)
        {
            return sleep_f(seconds);
        }

        auto fiber = Gyanis::core::Fiber::GetThis();
        auto *iom  = Gyanis::core::IOManager::GetThis();
        iom->addTimer(seconds * 1000, [iom, fiber]()
        {
            iom->schedule(fiber, -1);
        });

        Gyanis::core::Fiber::Yield();
        return 0;
    }

    int usleep(useconds_t usec)
    {
        if (!Gyanis::core::t_hook_enable)
        {
            return usleep_f(usec);
        }

        auto fiber = Gyanis::core::Fiber::GetThis();
        auto *iom  = Gyanis::core::IOManager::GetThis();
        iom->addTimer(usec / 1000, [iom, fiber]()
        {
            iom->schedule(fiber, -1);
        });
        Gyanis::core::Fiber::Yield();
        return 0;
    }

    int nanosleep(const timespec *req, timespec *rem)
    {
        if (!Gyanis::core::t_hook_enable)
        {
            return nanosleep_f(req, rem);
        }

        const auto timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        auto fiber = Gyanis::core::Fiber::GetThis();
        auto *iom  = Gyanis::core::IOManager::GetThis();
        iom->addTimer(timeout_ms, [iom, fiber]()
        {
            iom->schedule(fiber, -1);
        });
        Gyanis::core::Fiber::Yield();
        return 0;
    }

    int socket(int domain, int type, int protocol)
    {
        if (!Gyanis::core::t_hook_enable)
        {
            return socket_f(domain, type, protocol);
        }
        const int fd = socket_f(domain, type, protocol);
        if (fd == -1)
        {
            return fd;
        }
        Gyanis::core::FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int fd, const sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms)
    {
        if (!Gyanis::core::t_hook_enable)
        {
            return connect_f(fd, addr, addrlen);
        }

        const auto ctx = Gyanis::core::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose())
        {
            errno = EBADF;
            return -1;
        }

        if (!ctx->isSocket() || ctx->getUserNonblock())
        {
            return connect_f(fd, addr, addrlen);
        }

        if (const int n = connect_f(fd, addr, addrlen); n == 0)
        {
            return 0;
        }
        else if (n != -1 || errno != EINPROGRESS)
        {
            return n;
        }

        auto *iom = Gyanis::core::IOManager::GetThis();
        if (iom == nullptr)
        {
            LOG_ERROR(g_logger) << "[Hook] connect_with_timeout 失败：当前线程没有 IOManager。";
            return -1;
        }

        uint64_t timer = 0;
        const auto tinfo = std::make_shared<timer_info>();
        std::weak_ptr winfo(tinfo);

        if (timeout_ms != std::numeric_limits<uint64_t>::max())
        {
            timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]()
            {
                const auto t = winfo.lock();
                if (!t || t->cancelled)
                {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, Gyanis::core::IOManager::WRITE);
            }, winfo);
        }

        if (const int rt = iom->addEvent(fd, Gyanis::core::IOManager::WRITE); rt == 0)
        {
            Gyanis::core::Fiber::Yield();
            if (timer)
            {
                iom->cancel(timer);
            }
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else
        {
            if (timer)
            {
                iom->cancel(timer);
            }
        }

        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
        {
            return -1;
        }
        if (error == 0)
        {
            return 0;
        }
        errno = error;
        return -1;
    }

    int connect(int sockfd, const sockaddr *addr, socklen_t addrlen)
    {
        return connect_with_timeout(sockfd, addr, addrlen, Gyanis::core::s_connect_timeout);
    }

    int accept(int s, sockaddr *addr, socklen_t *addrlen)
    {
        const auto fd = do_io(s, accept_f, "accept", Gyanis::core::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0)
        {
            Gyanis::core::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    ssize_t read(int fd, void *buf, size_t count)
    {
        return do_io(fd, read_f, "read", Gyanis::core::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const iovec *iov, int iovcnt)
    {
        return do_io(fd, readv_f, "readv", Gyanis::core::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        return do_io(sockfd, recv_f, "recv", Gyanis::core::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, sockaddr *src_addr, socklen_t *addrlen)
    {
        return do_io(sockfd, recvfrom_f, "recvfrom", Gyanis::core::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr,
                     addrlen);
    }

    ssize_t recvmsg(int sockfd, msghdr *msg, int flags)
    {
        return do_io(sockfd, recvmsg_f, "recvmsg", Gyanis::core::IOManager::READ, SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void *buf, size_t count)
    {
        return do_io(fd, write_f, "write", Gyanis::core::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const iovec *iov, int iovcnt)
    {
        return do_io(fd, writev_f, "writev", Gyanis::core::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int s, const void *msg, size_t len, int flags)
    {
        return do_io(s, send_f, "send", Gyanis::core::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
    }

    ssize_t sendto(int s, const void *msg, size_t len, int flags, const sockaddr *to, socklen_t tolen)
    {
        return do_io(s, sendto_f, "sendto", Gyanis::core::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
    }

    ssize_t sendmsg(int s, const msghdr *msg, int flags)
    {
        return do_io(s, sendmsg_f, "sendmsg", Gyanis::core::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
    }

    int close(int fd)
    {
        if (!Gyanis::core::t_hook_enable)
        {
            return close_f(fd);
        }

        if (auto ctx = Gyanis::core::FdMgr::GetInstance()->get(fd))
        {
            if (const auto iom = Gyanis::core::IOManager::GetThis())
            {
                iom->cancelAll(fd);
            }
            Gyanis::core::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }

    int fcntl(int fd, int cmd, ...)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
            case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                const auto ctx = Gyanis::core::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock())
                {
                    arg |= O_NONBLOCK;
                }
                else
                {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            case F_GETFL:
            {
                va_end(va);
                const int arg = fcntl_f(fd, cmd);
                const auto ctx = Gyanis::core::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return arg;
                }
                if (ctx->getUserNonblock())
                {
                    return arg | O_NONBLOCK;
                }
                return arg & ~O_NONBLOCK;
            }
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
#ifdef F_SETPIPE_SZ
            case F_SETPIPE_SZ:
#endif
            {
                const int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
#ifdef F_GETPIPE_SZ
            case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
            {
                auto *arg = va_arg(va, struct flock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
#ifdef F_GETOWN_EX
            case F_GETOWN_EX:
#endif
#ifdef F_SETOWN_EX
            case F_SETOWN_EX:
#endif
            {
                auto *arg = va_arg(va, struct f_owner_ex *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            default:
                va_end(va);
                return fcntl_f(fd, cmd);
        }
    }

    int ioctl(int d, unsigned long int request, ...)
    {
        va_list va;
        va_start(va, request);
        void *arg = va_arg(va, void *);
        va_end(va);

        if (request == FIONBIO)
        {
            const bool user_nonblock = !!*static_cast<int *>(arg);
            const auto ctx = Gyanis::core::FdMgr::GetInstance()->get(d);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(d, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if (!Gyanis::core::t_hook_enable)
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                if (const auto ctx = Gyanis::core::FdMgr::GetInstance()->get(sockfd))
                {
                    const auto *v = static_cast<const timeval *>(optval);
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}
#endif
