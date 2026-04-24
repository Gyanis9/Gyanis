/**
 * @file Socket.h
 * @brief RAII 封装的原生 socket 类，支持 TCP/UDP，自动管理描述符生命周期。
 * @details 提供移动语义，禁止拷贝，跨平台兼容 Windows 和 POSIX 系统。
 *          集成了地址类 IpAddress 和 IpEndpoint，简化套接字编程。
 */

#ifndef SOCKET_H
#define SOCKET_H

#include "Address.h"
#include "Core/PlatformCompat.hpp"

#include <optional>
#include <utility>

#ifdef _MSC_VER
typedef ptrdiff_t ssize_t; ///< MSVC 兼容 POSIX ssize_t 类型
#endif

namespace Net
{
    /**
     * @brief RAII Socket 封装类。
     * @details 支持 TCP/UDP/原始套接字，自动管理描述符生命周期。
     *          禁止拷贝，允许移动，可转换为原生句柄用于系统调用。
     *          所有可能阻塞的操作均保持原语意，错误处理通过返回值或异常。
     */
    class Socket
    {
    public:
        /**
         * @brief 默认构造函数，构造一个无效 socket。
         */
        constexpr Socket() noexcept : m_socket(Core::INVALID_SOCKET_VAL)
        {
        }

        /**
         * @brief 从已有原生描述符构造，接管所有权。
         * @param sock 原生 socket 描述符，必须已由 ::socket 等函数创建。
         * @note 调用者不应再自行关闭该描述符，析构时会自动关闭。
         */
        explicit Socket(Core::socket_t sock) noexcept;

        /**
         * @brief 创建指定地址族、类型、协议的 socket。
         * @param family 地址族（IpAddress::Family::V4 / V6 / None）
         * @param type   socket 类型，如 SOCK_STREAM, SOCK_DGRAM
         * @param protocol 协议，默认为 0（自动选择）
         * @throw std::system_error 创建失败时抛出，错误码为原生 socket 错误。
         */
        Socket(IpAddress::Family family, int type, int protocol = 0);

        /**
         * @brief 移动构造函数。
         * @param other 右值引用源对象，移动后源对象变为无效。
         */
        Socket(Socket &&other) noexcept;

        /**
         * @brief 移动赋值运算符。
         * @param other 右值引用源对象。
         * @return 本对象引用。
         */
        Socket &operator=(Socket &&other) noexcept
        {
            if (this != &other)
            {
                close();
                m_socket = std::exchange(other.m_socket, Core::INVALID_SOCKET_VAL);
            }
            return *this;
        }

        // 禁止拷贝构造与拷贝赋值
        Socket(const Socket &) = delete;

        Socket &operator=(const Socket &) = delete;

        /**
         * @brief 析构函数，自动关闭 socket。
         */
        ~Socket()
        {
            close();
        }

        /**
         * @brief 主动关闭 socket，释放资源。
         * @note 多次调用安全，无效 socket 调用无影响。
         */
        void close() noexcept;

        /**
         * @brief 返回原生 socket 描述符。
         * @return socket_t 类型，可用于 epoll、IOCP、select 等系统函数。
         */
        [[nodiscard]] Core::socket_t nativeHandle() const noexcept;

        /**
         * @brief 判断 socket 是否有效。
         * @return true 表示有效，false 表示无效。
         */
        [[nodiscard]] bool isValid() const noexcept;

        // ---- 基本操作 ----

        /**
         * @brief 将 socket 绑定到指定本地端点。
         * @param endpoint 包含 IP 地址和端口的端点。
         * @return 绑定成功返回 true，失败返回 false（可通过 lastError() 获取错误码）。
         */
        bool bind(const IpEndpoint &endpoint) const noexcept;

        /**
         * @brief 使流式 socket 进入监听状态。
         * @param backlog 最大待处理连接数，默认为 SOMAXCONN（系统最大值）。
         * @return 成功返回 true，失败返回 false。
         */
        bool listen(int backlog = SOMAXCONN) const noexcept;

        /**
         * @brief 接受一个连接，返回新的 Socket 对象。
         * @return 成功建立连接时返回包含客户端 socket 的 optional；无可用连接（非阻塞模式）或出错时返回 std::nullopt。
         * @note 调用者必须确保 socket 已被 listen。
         */
        [[nodiscard]] std::optional<Socket> accept() const noexcept;

        /**
         * @brief 连接到远端端点。
         * @param endpoint 目标地址与端口。
         * @return 连接成功返回 true；失败（同步错误）返回 false。
         * @note 对于非阻塞 socket，如果连接立即无法完成，函数返回 true 表示连接操作已启动（状态为进行中）。
         *       可使用 wouldBlock() 检测此情况，后续通过 select/poll 等待完成。
         */
        bool connect(const IpEndpoint &endpoint) const noexcept;

        // ---- I/O 操作 ----

        /**
         * @brief 发送数据（面向连接的 socket）。
         * @param buf 数据缓冲区指针。
         * @param len 要发送的字节数。
         * @param flags 标志位（通常为 0）。
         * @return 成功返回实际发送的字节数；失败返回 -1（可通过 lastError() 获取详情）。
         */
        ssize_t send(const void *buf, size_t len, int flags = 0) const noexcept;

        /**
         * @brief 接收数据（面向连接的 socket）。
         * @param buf 接收缓冲区。
         * @param len 缓冲区大小。
         * @param flags 标志位。
         * @return 成功返回实际接收的字节数（0 表示对端关闭连接）；失败返回 -1。
         */
        ssize_t recv(void *buf, size_t len, int flags = 0) const noexcept;

        /**
         * @brief 发送数据到指定端点（UDP 等无连接 socket）。
         * @param buf 数据缓冲区。
         * @param len 数据长度。
         * @param endpoint 目标地址和端口。
         * @param flags 标志位。
         * @return 成功返回发送的字节数，失败返回 -1。
         */
        ssize_t sendTo(const void *buf, size_t len, const IpEndpoint &endpoint, int flags = 0) const noexcept;

        /**
         * @brief 接收数据并获取对端地址（UDP 等无连接 socket）。
         * @param buf 接收缓冲区。
         * @param len 缓冲区大小。
         * @param endpoint 输出参数，接收对端地址和端口。
         * @param flags 标志位。
         * @return 成功返回接收的字节数，失败返回 -1。
         */
        ssize_t recvFrom(void *buf, size_t len, IpEndpoint &endpoint, int flags = 0) const noexcept;

        // ---- 非阻塞设置 ----

        /**
         * @brief 设置或清除 socket 的非阻塞模式。
         * @param enable true 设为非阻塞，false 恢复为阻塞。
         * @return 成功返回 true，失败返回 false。
         */
        bool setNonBlocking(bool enable) const noexcept;

        // ---- 常用 socket 选项 ----

        /**
         * @brief 设置地址重用选项 (SO_REUSEADDR)。
         * @param enable 启用或禁用。
         * @return 成功返回 true。
         */
        bool setReuseAddr(bool enable) const noexcept;

        /**
         * @brief 设置 TCP_NODELAY 选项（禁用 Nagle 算法）。
         * @param enable 启用或禁用。
         * @return 成功返回 true。
         */
        bool setNoDelay(const bool enable) const noexcept;

        // ---- 地址查询 ----

        /**
         * @brief 获取 socket 的本地端点（地址和端口）。
         * @return 成功返回包含本地端点的 optional，失败返回 std::nullopt。
         */
        [[nodiscard]] std::optional<IpEndpoint> localEndpoint() const noexcept;

        /**
         * @brief 获取 socket 的远端端点（对于已连接的 socket）。
         * @return 成功返回包含远端端点的 optional，失败返回 std::nullopt。
         */
        [[nodiscard]] std::optional<IpEndpoint> remoteEndpoint() const noexcept;

        // ---- 错误处理 ----

        /**
         * @brief 获取最后一次 socket 操作发生的错误码（线程安全，类似 errno）。
         * @return 平台无关的错误码。
         */
        [[nodiscard]] static int lastError() noexcept;

        /**
         * @brief 将最后一个错误码转换为 std::system_error 并抛出。
         * @param msg 附加错误信息。
         * @throw std::system_error 总是抛出。
         */
        [[noreturn]] static void throwLastError(const char *msg = "socket error");

    private:
        Core::socket_t m_socket; ///< 原生 socket 描述符
    };

    // ---- 预定义常用 socket 工厂函数 ----

    /**
     * @brief 创建 IPv4 TCP 流式 socket。
     * @return Socket 对象，可能抛出异常。
     */
    inline Socket makeTcpSocketV4()
    {
        return Socket(IpAddress::Family::V4, SOCK_STREAM);
    }

    /**
     * @brief 创建 IPv6 TCP 流式 socket。
     * @return Socket 对象。
     */
    inline Socket makeTcpSocketV6()
    {
        return Socket(IpAddress::Family::V6, SOCK_STREAM);
    }

    /**
     * @brief 创建 IPv4 UDP 数据报 socket。
     * @return Socket 对象。
     */
    inline Socket makeUdpSocketV4()
    {
        return Socket(IpAddress::Family::V4, SOCK_DGRAM);
    }

    /**
     * @brief 创建 IPv6 UDP 数据报 socket。
     * @return Socket 对象。
     */
    inline Socket makeUdpSocketV6()
    {
        return Socket(IpAddress::Family::V6, SOCK_DGRAM);
    }
}

#endif // SOCKET_H
