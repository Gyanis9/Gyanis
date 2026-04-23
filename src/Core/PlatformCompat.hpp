/**
 * @file PlatformCompat.hpp
 * @brief 网络与 I/O 平台兼容类型及工具函数。
 * @details 统一 Windows（Winsock2）与 POSIX（Linux/Unix）下的套接字类型、
 *          常量及常用操作，并提供时间相关的类型别名。
 *          所有函数尽力保持相同的签名与语义，以减少跨平台条件编译代码。
 */

#ifndef PLATFORMCOMPAT_HPP
#define PLATFORMCOMPAT_HPP

#include <chrono>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <errno.h>
#endif

/**
 * @namespace Core
 * @brief 核心基础设施命名空间，提供跨平台套接字、时间及工具函数。
 */
namespace Core
{
    // -------------------------------------------------------------------
    // 平台统一类型与常量
    // -------------------------------------------------------------------
#ifdef _WIN32
    /**
     * @brief 统一套接字描述符类型。
     * @note 在 Windows 上为 SOCKET，在 POSIX 上为 int。
     */
    using socket_t = SOCKET;

    /**
     * @brief 无效套接字常量。
     */
    constexpr socket_t INVALID_SOCKET_VAL = INVALID_SOCKET;

    /**
     * @brief 套接字错误返回值常量。
     */
    constexpr int SOCKET_ERROR_VAL = SOCKET_ERROR;

    /**
     * @brief 关闭套接字。
     * @param s 要关闭的套接字描述符。
     */
    inline void closeSocket(const socket_t s)
    {
        ::closesocket(s);
    }

    /**
     * @brief 获取最后一次套接字操作的错误码。
     * @return 错误码（Windows 下为 WSAGetLastError() 返回值）。
     */
    inline int getLastSocketError()
    {
        return ::WSAGetLastError();
    }

    /**
     * @brief 判断指定错误（或当前错误）是否表示“操作将阻塞”。
     * @param err 错误码；若为 -1 则自动获取当前错误。
     * @return true 如果错误为 WSAEWOULDBLOCK 或 WSA_IO_PENDING。
     */
    inline bool wouldBlock(const int err = -1)
    {
        const int e = (err == -1) ? ::WSAGetLastError() : err;
        return e == WSAEWOULDBLOCK || e == WSA_IO_PENDING;
    }

    /**
     * @brief 初始化网络库（Windows 下调用 WSAStartup）。
     * @note POSIX 下为空操作。
     */
    inline void initNetwork()
    {
        WSADATA wsaData;
        ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    /**
     * @brief 清理网络库（Windows 下调用 WSACleanup）。
     * @note POSIX 下为空操作。
     */
    inline void cleanupNetwork()
    {
        ::WSACleanup();
    }

    /**
     * @brief 设置套接字的非阻塞模式。
     * @param s 套接字描述符。
     * @param nonblock true 表示设置为非阻塞，false 表示阻塞。
     * @return 成功返回 true，失败返回 false。
     */
    inline bool setNonblocking(const socket_t s, const bool nonblock)
    {
        u_long mode = nonblock ? 1 : 0;
        return ::ioctlsocket(s, FIONBIO, &mode) == 0;
    }

#else // POSIX (Linux/Unix)

    /**
     * @brief 统一套接字描述符类型。
     * @note 在 POSIX 上为 int，Windows 上为 SOCKET。
     */
    using socket_t = int;

    /**
     * @brief 无效套接字常量。
     */
    constexpr socket_t INVALID_SOCKET_VAL = -1;

    /**
     * @brief 套接字错误返回值常量。
     */
    constexpr int SOCKET_ERROR_VAL = -1;

    /**
     * @brief 关闭套接字。
     * @param s 要关闭的套接字描述符。
     */
    inline void closeSocket(socket_t s)
    {
        ::close(s);
    }

    /**
     * @brief 获取最后一次套接字操作的错误码。
     * @return 错误码（POSIX 下为 errno）。
     */
    inline int getLastSocketError()
    {
        return errno;
    }

    /**
     * @brief 判断指定错误（或当前错误）是否表示“操作将阻塞”。
     * @param err 错误码；若为 -1 则自动获取当前 errno。
     * @return true 如果错误为 EAGAIN、EWOULDBLOCK 或 EINPROGRESS。
     */
    inline bool wouldBlock(int err = -1)
    {
        int e = (err == -1) ? errno : err;
        return e == EAGAIN || e == EWOULDBLOCK || e == EINPROGRESS;
    }

    /**
     * @brief 初始化网络库（POSIX 下为空操作）。
     * @note Windows 下调用 WSAStartup。
     */
    inline void initNetwork()
    {
    }

    /**
     * @brief 清理网络库（POSIX 下为空操作）。
     * @note Windows 下调用 WSACleanup。
     */
    inline void cleanupNetwork()
    {
    }

    /**
     * @brief 设置套接字的非阻塞模式。
     * @param s 套接字描述符。
     * @param nonblock true 表示设置为非阻塞，false 表示阻塞。
     * @return 成功返回 true，失败返回 false。
     */
    inline bool setNonblocking(socket_t s, bool nonblock)
    {
        int flags = ::fcntl(s, F_GETFL, 0);
        if (flags == -1)
        {
            return false;
        }
        flags = nonblock ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        return ::fcntl(s, F_SETFL, flags) == 0;
    }
#endif

    // -------------------------------------------------------------------
    // 时间相关别名
    // -------------------------------------------------------------------

    /**
     * @brief 稳定时钟类型别名。
     * @details 采用 std::chrono::steady_clock，保证时间单调递增，适合超时计算。
     */
    using steady_clock = std::chrono::steady_clock;

    /**
     * @brief 毫秒时长类型别名。
     */
    using milliseconds = std::chrono::milliseconds;

    /**
     * @brief 时间点类型别名。
     */
    using time_point = steady_clock::time_point;

    // -------------------------------------------------------------------
    // 辅助函数：设置 socket 选项
    // -------------------------------------------------------------------

    /**
     * @brief 设置套接字的地址重用选项（SO_REUSEADDR）。
     * @param s 套接字描述符。
     * @param enable true 表示允许地址重用，false 表示禁止。
     * @return 成功返回 true，失败返回 false。
     */
    inline bool setReuseAddress(socket_t s, const bool enable)
    {
#ifdef _WIN32
        const BOOL val = enable ? TRUE : FALSE;
        return ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&val), sizeof(val)) == 0;
#else
        int val = enable ? 1 : 0;
        return ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == 0;
#endif
    }

    /**
     * @brief 设置套接字的 Nagle 算法开关（TCP_NODELAY）。
     * @param s 套接字描述符。
     * @param enable true 表示禁用 Nagle 算法（立即发送），false 表示启用。
     * @return 成功返回 true，失败返回 false。
     */
    inline bool setNoDelay(socket_t s, const bool enable)
    {
#ifdef _WIN32
        const BOOL val = enable ? TRUE : FALSE;
        return ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&val), sizeof(val)) == 0;
#else
        int val = enable ? 1 : 0;
        return ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == 0;
#endif
    }
}

#endif // PLATFORMCOMPAT_HPP
