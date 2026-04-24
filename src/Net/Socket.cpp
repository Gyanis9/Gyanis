#include "Socket.h"

namespace Net
{
    Socket::Socket(const Core::socket_t sock) noexcept : m_socket(sock)
    {
    }

    Socket::Socket(IpAddress::Family family, const int type, const int protocol)
    {
        m_socket = ::socket(static_cast<int>(family), type, protocol);
        if (m_socket == Core::INVALID_SOCKET_VAL)
        {
            throw std::system_error(Core::getLastSocketError(),
                                    std::generic_category(), "socket() failed");
        }
    }

    Socket::Socket(Socket &&other) noexcept : m_socket(std::exchange(other.m_socket, Core::INVALID_SOCKET_VAL))
    {
    }

    void Socket::close() noexcept
    {
        if (m_socket != Core::INVALID_SOCKET_VAL)
        {
            Core::closeSocket(m_socket);
            m_socket = Core::INVALID_SOCKET_VAL;
        }
    }

    Core::socket_t Socket::nativeHandle() const noexcept
    {
        return m_socket;
    }

    bool Socket::isValid() const noexcept
    {
        return m_socket != Core::INVALID_SOCKET_VAL;
    }

    bool Socket::bind(const IpEndpoint &endpoint) const noexcept
    {
        const auto ss = endpoint.toSockaddrStorage();
        const int ret = ::bind(m_socket, reinterpret_cast<const sockaddr *>(&ss), sizeof(ss));
        return ret != Core::SOCKET_ERROR_VAL;
    }

    bool Socket::listen(const int backlog) const noexcept
    {
        return ::listen(m_socket, backlog) != Core::SOCKET_ERROR_VAL;
    }

    std::optional<Socket> Socket::accept() const noexcept
    {
        sockaddr_storage ss{};
        socklen_t len = sizeof(ss);
        const Core::socket_t client = ::accept(m_socket, reinterpret_cast<sockaddr *>(&ss), &len);
        if (client == Core::INVALID_SOCKET_VAL)
        {
            return std::nullopt;
        }
        return Socket(client);
    }

    bool Socket::connect(const IpEndpoint &endpoint) const noexcept
    {
        const auto ss = endpoint.toSockaddrStorage();
        if (const int ret = ::connect(m_socket, reinterpret_cast<const sockaddr *>(&ss), sizeof(ss)); ret == Core::SOCKET_ERROR_VAL)
        {
            // 非阻塞连接正在处理中也被视为“成功”（异步连接）
            if (Core::wouldBlock())
            {
                return true;
            }
            return false;
        }
        return true;
    }

    ssize_t Socket::send(const void *buf, const size_t len, const int flags) const noexcept
    {
        return ::send(m_socket, static_cast<const char *>(buf), static_cast<int>(len), flags);
    }

    ssize_t Socket::recv(void *buf, const size_t len, const int flags) const noexcept
    {
        return ::recv(m_socket, static_cast<char *>(buf), static_cast<int>(len), flags);
    }

    ssize_t Socket::sendTo(const void *buf, const size_t len, const IpEndpoint &endpoint, const int flags) const noexcept
    {
        const auto ss = endpoint.toSockaddrStorage();
        return ::sendto(m_socket, static_cast<const char *>(buf), static_cast<int>(len), flags,
                        reinterpret_cast<const sockaddr *>(&ss), sizeof(ss));
    }

    ssize_t Socket::recvFrom(void *buf, const size_t len, IpEndpoint &endpoint, const int flags) const noexcept
    {
        sockaddr_storage ss{};
        socklen_t sslen = sizeof(ss);
        const ssize_t ret = ::recvfrom(m_socket, static_cast<char *>(buf), static_cast<int>(len), flags,
                                       reinterpret_cast<sockaddr *>(&ss), &sslen);
        if (ret >= 0)
        {
            endpoint = IpEndpoint(reinterpret_cast<const sockaddr *>(&ss));
        }
        return ret;
    }

    bool Socket::setNonBlocking(const bool enable) const noexcept
    {
        return Core::setNonblocking(m_socket, enable);
    }

    bool Socket::setReuseAddr(const bool enable) const noexcept
    {
        const int opt = enable ? 1 : 0;
        return ::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,
#ifdef _WIN32
                            reinterpret_cast<const char *>(&opt)
#else
                            &opt
#endif
                            , sizeof(opt)) == 0;
    }

    bool Socket::setNoDelay(const bool enable) const noexcept
    {
        int opt = enable ? 1 : 0;
        const int ret = ::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY,
#ifdef _WIN32
                                     reinterpret_cast<const char *>(&opt)
#else
                                     &opt
#endif
                                     , sizeof(opt));
        return ret == 0;
    }

    std::optional<IpEndpoint> Socket::localEndpoint() const noexcept
    {
        sockaddr_storage ss{};
        socklen_t len = sizeof(ss);
        if (::getsockname(m_socket, reinterpret_cast<sockaddr *>(&ss), &len) == 0)
        {
            return IpEndpoint::fromSockaddr(ss);
        }
        return std::nullopt;
    }

    std::optional<IpEndpoint> Socket::remoteEndpoint() const noexcept
    {
        sockaddr_storage ss{};
        socklen_t len = sizeof(ss);
        if (::getpeername(m_socket, reinterpret_cast<sockaddr *>(&ss), &len) == 0)
        {
            return IpEndpoint::fromSockaddr(ss);
        }
        return std::nullopt;
    }

    int Socket::lastError() noexcept
    {
        return Core::getLastSocketError();
    }

    void Socket::throwLastError(const char *msg)
    {
        throw std::system_error(Core::getLastSocketError(), std::generic_category(), msg);
    }
} // Net
