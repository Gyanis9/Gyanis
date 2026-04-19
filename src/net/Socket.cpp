#include <netinet/tcp.h>
#include <sstream>

#include "core/FdManager.h"
#include "core/Hook.h"
#include "base/Log.h"
#include "base/Macro.h"
#include "core/IOManager.h"
#include "Socket.h"

namespace Gyanis::net
{
    static auto g_logger = LOG_NAME("system");

    std::shared_ptr<Socket> Socket::CreateTCP(const std::shared_ptr<Address>& address)
    {
        auto sock = std::make_shared<Socket>(address->getFamily(), TCP, 0);
        return sock;
    }

    std::shared_ptr<Socket> Socket::CreateUDP(const std::shared_ptr<Address>& address)
    {
        auto sock = std::make_shared<Socket>(address->getFamily(), UDP, 0);
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    std::shared_ptr<Socket> Socket::CreateTCPSocket()
    {
        auto sock = std::make_shared<Socket>(IPv4, TCP, 0);
        return sock;
    }

    std::shared_ptr<Socket> Socket::CreateUDPSocket()
    {
        auto sock = std::make_shared<Socket>(IPv4, UDP, 0);
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    std::shared_ptr<Socket> Socket::CreateTCPSocket6()
    {
        auto sock = std::make_shared<Socket>(IPv6, TCP, 0);
        return sock;
    }

    std::shared_ptr<Socket> Socket::CreateUDPSocket6()
    {
        auto sock = std::make_shared<Socket>(IPv6, UDP, 0);
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    std::shared_ptr<Socket> Socket::CreateUnixTCPSocket()
    {
        auto sock = std::make_shared<Socket>(UNIX, TCP, 0);
        return sock;
    }

    std::shared_ptr<Socket> Socket::CreateUnixUDPSocket()
    {
        auto sock = std::make_shared<Socket>(UNIX, UDP, 0);
        return sock;
    }

    Socket::Socket(const int family, const int type, const int protocol): m_sock(-1), m_family(family), m_type(type),
                                                                          m_protocol(protocol), m_isConnected(false)
    {
    }

    Socket::~Socket()
    {
        close();
    }

    int64_t Socket::getSendTimeout() const
    {
        if (const auto ctx = core::FdMgr::GetInstance()->get(m_sock))
        {
            return ctx->getTimeout(SO_SNDTIMEO).count();
        }
        return std::chrono::milliseconds::max().count();
    }

    void Socket::setSendTimeout(const int64_t timeout)
    {
        const timeval tv{static_cast<int>(timeout / 1000), static_cast<int>(timeout % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    int64_t Socket::getRecvTimeout() const
    {
        if (const auto ctx = core::FdMgr::GetInstance()->get(m_sock))
        {
            return ctx->getTimeout(SO_RCVTIMEO).count();
        }
        return std::chrono::milliseconds::max().count();
    }

    void Socket::setRecvTimeout(const int64_t timeout)
    {
        const timeval tv{static_cast<int>(timeout / 1000), static_cast<int>(timeout % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    bool Socket::getOption(const int level, const int option, void* result, socklen_t* len) const
    {
        if (getsockopt(m_sock, level, option, result, len))
        {
            LOG_ERROR(g_logger)
                << "Socket::getOption - failed. "
                << "Socket: " << m_sock
                << " | Level: " << level
                << " | Option: " << option
                << " | Error code: " << errno
                << " | Error description: " << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(const int level, const int option, const void* result, const socklen_t len) const
    {
        if (setsockopt(m_sock, level, option, result, len))
        {
            LOG_ERROR(g_logger)
                << "Socket::setOption - failed. "
                << "Socket: " << m_sock
                << " | Level: " << level
                << " | Option: " << option
                << " | Error code: " << errno
                << " | Error description: " << strerror(errno);
            return false;
        }
        return true;
    }

    std::shared_ptr<Socket> Socket::accept()
    {
        auto sock = std::make_shared<Socket>(m_family, m_type, m_protocol);
        const int newSock = ::accept(m_sock, nullptr, nullptr);
        if (newSock == -1)
        {
            LOG_ERROR(g_logger)
                << "Socket::accept - failed. "
                << "Socket: " << m_sock
                << " | Error code: " << errno
                << " | Error description: " << strerror(errno);
            return nullptr;
        }
        if (sock->init(newSock))
        {
            return sock;
        }
        return nullptr;
    }

    bool Socket::bind(const std::shared_ptr<Address>& address)
    {
        if (!isValid())
        {
            newSock();
            if (UNLIKELY(!isValid()))
            {
                return false;
            }
        }
        if (UNLIKELY(address->getFamily()!=m_family))
        {
            LOG_ERROR(g_logger)
                << "Socket::bind - failed. "
                << "Socket family: " << m_family
                << " | Address family: " << address->getFamily()
                << " | Mismatch detected. "
                << "Address: " << address->toString();
            return false;
        }
        if (const auto uaddress = std::dynamic_pointer_cast<UnixAddress>(address); uaddress)
        {
            if (const auto sock = CreateUnixTCPSocket(); sock->connect(
                uaddress, std::chrono::milliseconds::max().count()))
            {
                return false;
            }
            base::FSUtil::Unlink(uaddress->getPath(), true);
        }
        if (::bind(m_sock, address->getSockAddr(), address->getSockAddrLen()))
        {
            LOG_ERROR(g_logger)
                << "Socket::bind - failed. "
                << "Socket: " << m_sock
                << " | Error code: " << errno
                << " | Error description: " << strerror(errno);
            return false;
        }
        getLocalAddress();
        return true;
    }

    bool Socket::connect(const std::shared_ptr<Address>& address, const uint64_t timeout_ms)
    {
        m_remoteAddress = address;
        if (!isValid())
        {
            newSock();
            if (UNLIKELY(!isValid()))
            {
                return false;
            }
        }
        if (UNLIKELY(address->getFamily()!=m_family))
        {
            LOG_ERROR(g_logger)
                << "Socket::bind - failed. "
                << "Socket family: " << m_family
                << " | Address family: " << address->getFamily()
                << " | Families do not match. "
                << "Address: " << address->toString();
        }
        if (timeout_ms == std::chrono::milliseconds::max().count())
        {
            if (::connect(m_sock, address->getSockAddr(), address->getSockAddrLen()))
            {
                LOG_ERROR(g_logger)
                    << "Socket::bind - failed. "
                    << "Socket: " << m_sock
                    << " | Error code: " << errno
                    << " | Error description: " << strerror(errno);
                close();
                return false;
            }
        }
        else
        {
            if (connect_with_timeout(m_sock, address->getSockAddr(), address->getSockAddrLen(), timeout_ms))
            {
                LOG_ERROR(g_logger)
                    << "Socket::bind - failed. "
                    << "Socket: " << m_sock
                    << " | Error code: " << errno
                    << " | Error description: " << strerror(errno);
                close();
                return false;
            }
        }
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;
    }

    bool Socket::reconnect(const uint64_t timeout_ms)
    {
        if (!m_remoteAddress)
        {
            LOG_ERROR(g_logger)
                << "Socket::reconnect - failed. "
                << "Remote address (m_remoteAddress) is null. ";
            return false;
        }
        m_localAddress.reset();
        return connect(m_remoteAddress, timeout_ms);
    }

    bool Socket::listen(const int backlog) const
    {
        if (!isValid())
        {
            LOG_ERROR(g_logger)
                << "Socket::listen - failed. "
                << "Socket: " << m_sock
                << " | Error code: " << errno
                << " | Error description: " << strerror(errno);
            return false;
        }
        if (::listen(m_sock, backlog))
        {
            LOG_ERROR(g_logger)
                << "Socket::listen - failed. "
                << "Socket: " << m_sock
                << " | Error code: " << errno
                << " | Error description: " << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::close()
    {
        if (!m_isConnected && m_sock == -1)
        {
            return true;
        }
        m_isConnected = false;
        if (m_sock != -1)
        {
            ::close(m_sock);
            m_sock = -1;
        }
        return false;
    }

    long Socket::send(const void* buffer, const size_t length, const int flags) const
    {
        if (isConnected())
        {
            return ::send(m_sock, buffer, length, flags);
        }
        return -1;
    }

    long Socket::send(iovec* buffer, const size_t len, const int flags) const
    {
        if (isConnected())
        {
            msghdr msg = {};
            msg.msg_iov = buffer;
            msg.msg_iovlen = len;
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    long Socket::sendTo(const void* buffer, const size_t length, const std::shared_ptr<Address>& address,
                        const int flags) const
    {
        if (isConnected())
        {
            return ::sendto(m_sock, buffer, length, flags, address->getSockAddr(), address->getSockAddrLen());
        }
        return -1;
    }

    long Socket::sendTo(iovec* buffer, const size_t len, const std::shared_ptr<Address>& address,
                        const int flags) const
    {
        if (isConnected())
        {
            msghdr msg = {};
            msg.msg_iov = buffer;
            msg.msg_iovlen = len;
            msg.msg_name = address->getSockAddr();
            msg.msg_namelen = address->getSockAddrLen();
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    long Socket::recv(void* buffer, const size_t length, const int flags) const
    {
        if (isConnected())
        {
            return ::recv(m_sock, buffer, length, flags);
        }
        return -1;
    }

    long Socket::recv(iovec* buffer, const size_t len, const int flags) const
    {
        if (isConnected())
        {
            msghdr msg = {};
            msg.msg_iov = buffer;
            msg.msg_iovlen = len;
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    long Socket::recvFrom(void* buffer, const size_t length, const std::shared_ptr<Address>& address,
                          const int flags) const
    {
        if (isConnected())
        {
            socklen_t len = address->getSockAddrLen();
            return ::recvfrom(m_sock, buffer, length, flags, address->getSockAddr(), &len);
        }
        return -1;
    }

    long Socket::recvFrom(iovec* buffer, const size_t len, const std::shared_ptr<Address>& address,
                          const int flags) const
    {
        if (isConnected())
        {
            msghdr msg = {};
            msg.msg_iov = buffer;
            msg.msg_iovlen = len;
            msg.msg_name = address->getSockAddr();
            msg.msg_namelen = address->getSockAddrLen();
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    std::shared_ptr<Address> Socket::getRemoteAddress()
    {
        if (m_remoteAddress)
        {
            return m_remoteAddress;
        }

        std::shared_ptr<Address> result = nullptr;
        switch (m_family)
        {
        case AF_INET:
            result = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            result = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            result = std::make_shared<UnixAddress>();
        default:
            result = std::make_shared<UnknownAddress>(m_family);
            break;
        }
        socklen_t addrLen = result->getSockAddrLen();
        if (getpeername(m_sock, result->getSockAddr(), &addrLen))
        {
            return std::make_shared<UnknownAddress>(m_family);
        }
        if (m_family == AF_UNIX)
        {
            const auto addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrLen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    std::shared_ptr<Address> Socket::getLocalAddress()
    {
        if (m_localAddress)
        {
            return m_localAddress;
        }

        std::shared_ptr<Address> result = nullptr;
        switch (m_family)
        {
        case AF_INET:
            result = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            result = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            result = std::make_shared<UnixAddress>();
        default:
            result = std::make_shared<UnknownAddress>(m_family);
            break;
        }
        socklen_t addrLen = result->getSockAddrLen();
        if (getsockname(m_sock, result->getSockAddr(), &addrLen))
        {
            LOG_ERROR(g_logger)
                << "Socket::getLocalAddress - failed. "
                << "Error occurred while calling getPeerName. ";
            return std::make_shared<UnknownAddress>(m_family);
        }
        if (m_family == AF_UNIX)
        {
            const auto addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrLen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    int Socket::getFamily() const
    {
        return m_family;
    }

    int Socket::getType() const
    {
        return m_type;
    }

    int Socket::getProtocol() const
    {
        return m_protocol;
    }

    bool Socket::isConnected() const
    {
        return m_isConnected;
    }

    bool Socket::isValid() const
    {
        return m_sock != -1;
    }

    int Socket::getError() const
    {
        int error = 0;
        socklen_t len = sizeof(error);
        if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
        {
            error = errno;
        }
        return error;
    }

    std::ostream& Socket::dump(std::ostream& os) const
    {
        os << "[Socket Information: "
            << "Socket ID: " << m_sock
            << " | Connected: " << m_isConnected
            << " | Address family: " << m_family
            << " | Socket type: " << m_type
            << " | Protocol: " << m_protocol;

        if (m_localAddress)
        {
            os << " | Local address: " << m_localAddress->toString();
        }

        if (m_remoteAddress)
        {
            os << " | Remote address: " << m_remoteAddress->toString();
        }

        os << "]";
        return os;
    }

    std::string Socket::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    int Socket::getSocket() const
    {
        return m_sock;
    }

    bool Socket::cancelRead() const
    {
        return core::IOManager::GetThis()->cancelEvent(m_sock, core::IOManager::READ);
    }

    bool Socket::cancelWrite() const
    {
        return core::IOManager::GetThis()->cancelEvent(m_sock, core::IOManager::WRITE);
    }

    bool Socket::cancelAccept() const
    {
        return core::IOManager::GetThis()->cancelEvent(m_sock, core::IOManager::READ);
    }

    bool Socket::cancelAll() const
    {
        return core::IOManager::GetThis()->cancelAll(m_sock);
    }

    void Socket::initSock()
    {
        constexpr int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        if (m_type == SOCK_STREAM)
        {
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }

    void Socket::newSock()
    {
        m_sock = socket(m_family, m_type, m_protocol);
        if (LIKELY(m_sock != -1))
        {
            initSock();
        }
        else
        {
            LOG_ERROR(g_logger)
                << "Socket::newSock - failed. "
                << "Socket family: " << m_family
                << " | Socket type: " << m_type
                << " | Protocol: " << m_protocol
                << " | Error code: " << errno
                << " | Error description: " << strerror(errno);
        }
    }

    bool Socket::init(const int sock)
    {
        if (const auto ctx = core::FdMgr::GetInstance()->get(sock); ctx && ctx->isSocket() && !ctx->isClose())
        {
            m_sock = sock;
            m_isConnected = true;
            initSock();
            getLocalAddress();
            getRemoteAddress();

            return true;
        }
        return false;
    }

    namespace
    {
        struct SSLInit
        {
            SSLInit()
            {
                SSL_library_init();
                SSL_load_error_strings();
                OpenSSL_add_all_algorithms();
            }
        };

        [[maybe_unused]] SSLInit s_init;
    }

    std::shared_ptr<SSLSocket> SSLSocket::CreateTCP(const std::shared_ptr<Address>& address)
    {
        return std::make_shared<SSLSocket>(address->getFamily(), TCP, 0);
    }

    std::shared_ptr<SSLSocket> SSLSocket::CreateTCPSocket()
    {
        return std::make_shared<SSLSocket>(IPv4, TCP, 0);
    }

    std::shared_ptr<SSLSocket> SSLSocket::CreateTCPSocket6()
    {
        return std::make_shared<SSLSocket>(IPv6, TCP, 0);
    }

    SSLSocket::SSLSocket(const int family, const int type, const int protocol): Socket(family, type, protocol)
    {
    }

    std::shared_ptr<Socket> SSLSocket::accept()
    {
        auto sock = std::make_shared<SSLSocket>(m_family, m_type, m_protocol);
        const int newsock = ::accept(m_sock, nullptr, nullptr);
        if (newsock == -1)
        {
            LOG_ERROR(g_logger)
                << "SSLSocket::accept - failed. "
                << "An error occurred while accepting the SSL connection.";
            return nullptr;
        }
        sock->m_ctx = m_ctx;
        if (sock->init(newsock))
        {
            return sock;
        }
        return nullptr;
    }

    bool SSLSocket::bind(const std::shared_ptr<Address>& address)
    {
        return Socket::bind(address);
    }

    bool SSLSocket::connect(const std::shared_ptr<Address>& address, const uint64_t timeout_ms)
    {
        bool v = Socket::connect(address, timeout_ms);
        if (v)
        {
            m_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
            m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
            SSL_set_fd(m_ssl.get(), m_sock);
            v = (SSL_connect(m_ssl.get()) == 1);
        }
        return v;
    }

    bool SSLSocket::listen(const int backlog) const
    {
        return Socket::listen(backlog);
    }

    long SSLSocket::send(const void* buffer, const size_t length, int flags) const
    {
        if (m_ssl)
        {
            return SSL_write(m_ssl.get(), buffer, static_cast<int>(length));
        }
        return -1;
    }

    long SSLSocket::send(iovec* buffer, const size_t len, int flags) const
    {
        if (!m_ssl)
        {
            return -1;
        }
        int total = 0;
        for (size_t i = 0; i < len; ++i)
        {
            const int tmp = SSL_write(m_ssl.get(), buffer[i].iov_base, static_cast<int>(buffer[i].iov_len));
            if (tmp <= 0)
            {
                return tmp;
            }
            total += tmp;
            if (tmp != static_cast<int>(buffer[i].iov_len))
            {
                break;
            }
        }
        return total;
    }

    long SSLSocket::sendTo(const void* buffer, size_t length, const std::shared_ptr<Address>& address, int flags) const
    {
        ASSERT(false);
        return -1;
    }

    long SSLSocket::sendTo(iovec* buffer, size_t len, const std::shared_ptr<Address>& address, int flags) const
    {
        ASSERT(false);
        return -1;
    }

    long SSLSocket::recv(void* buffer, const size_t length, int flags) const
    {
        if (m_ssl)
        {
            return SSL_read(m_ssl.get(), buffer, static_cast<int>(length));
        }
        return -1;
    }

    long SSLSocket::recv(iovec* buffer, const size_t len, int flags) const
    {
        if (!m_ssl)
        {
            return -1;
        }
        int total = 0;
        for (size_t i = 0; i < len; ++i)
        {
            const int tmp = SSL_read(m_ssl.get(), buffer[i].iov_base, static_cast<int>(buffer[i].iov_len));
            if (tmp <= 0)
            {
                return tmp;
            }
            total += tmp;
            if (tmp != static_cast<int>(buffer[i].iov_len))
            {
                break;
            }
        }
        return total;
    }

    long SSLSocket::recvFrom(void* buffer, size_t length, const std::shared_ptr<Address>& address, int flags) const
    {
        ASSERT(false);
        return -1;
    }

    long SSLSocket::recvFrom(iovec* buffer, size_t len, const std::shared_ptr<Address>& address, int flags) const
    {
        ASSERT(false);
        return -1;
    }

    bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file)
    {
        m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
        if (SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1)
        {
            LOG_ERROR(g_logger)
                << "SSL_CTX_use_certificate_chain_file - failed. "
                << "An error occurred while loading the certificate chain.";
            return false;
        }
        if (SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1)
        {
            LOG_ERROR(g_logger)
                << "SSL_CTX_use_PrivateKey_file - failed. "
                << "An error occurred while loading the private key.";
            return false;
        }
        if (SSL_CTX_check_private_key(m_ctx.get()) != 1)
        {
            LOG_ERROR(g_logger)
                << "SSL_CTX_check_private_key - failed. "
                << "The private key does not match the certificate.";
            return false;
        }
        return true;
    }

    std::ostream& SSLSocket::dump(std::ostream& os) const
    {
        os << "[SSLSocket Information: "
            << "Socket ID: " << m_sock
            << " | Connected: " << m_isConnected
            << " | Address family: " << m_family
            << " | Socket type: " << m_type
            << " | Protocol: " << m_protocol;

        if (m_localAddress)
        {
            os << " | Local address: " << m_localAddress->toString();
        }

        if (m_remoteAddress)
        {
            os << " | Remote address: " << m_remoteAddress->toString();
        }

        os << "]";
        return os;
    }

    bool SSLSocket::init(const int sock)
    {
        bool value = Socket::init(sock);
        if (value)
        {
            m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
            SSL_set_fd(m_ssl.get(), m_sock);
            value = (SSL_accept(m_ssl.get()) == 1);
        }
        return value;
    }
}
