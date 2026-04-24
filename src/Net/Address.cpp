#include "Address.h"


namespace Net
{
    int detail::safe_inet_pton(const int af, const char *src, void *dst)
    {
#ifdef _WIN32
        // Windows 从 Vista 开始提供 InetPton，但为兼容性使用 WSAStringToAddress
        SOCKADDR_STORAGE ss;
        int len = sizeof(ss);
        if (af == AF_INET)
        {
            const auto sa = reinterpret_cast<sockaddr_in *>(&ss);
            WCHAR wide[64];
            MultiByteToWideChar(CP_UTF8, 0, src, -1, wide, 64);
            if (const int ret = WSAStringToAddressW(wide, AF_INET, nullptr, reinterpret_cast<sockaddr *>(sa), &len); ret == 0)
            {
                memcpy(dst, &sa->sin_addr, sizeof(in_addr));
                return 1;
            }
            return 0;
        } else if (af == AF_INET6)
        {
            const auto sa6 = reinterpret_cast<sockaddr_in6 *>(&ss);
            WCHAR wide[64];
            MultiByteToWideChar(CP_UTF8, 0, src, -1, wide, 64);
            if (const int ret = WSAStringToAddressW(wide, AF_INET6, nullptr, reinterpret_cast<sockaddr *>(sa6), &len); ret == 0)
            {
                memcpy(dst, &sa6->sin6_addr, sizeof(in6_addr));
                return 1;
            }
            return 0;
        }
        return -1; // 不支持的地址族
#else
        return ::inet_pton(af, src, dst);
#endif
    }

    const char *detail::safe_inet_ntop(const int af, const void *src, char *dst, const socklen_t size)
    {
#ifdef _WIN32
        // Windows 提供 inet_ntop 自 Vista，但也可以回退到 WSAAddressToString
        return ::inet_ntop(af, src, dst, size);
#else
        return ::inet_ntop(af, src, dst, size);
#endif
    }


    IpAddress::IpAddress(const in_addr &addr4) noexcept : m_family(Family::V4)
    {
        // 映射为 IPv6 地址
        std::memset(&m_addr6, 0, sizeof(m_addr6));
        m_addr6.s6_addr[10] = 0xFF;
        m_addr6.s6_addr[11] = 0xFF;
        std::memcpy(&m_addr6.s6_addr[12], &addr4, sizeof(addr4));
    }

    IpAddress::IpAddress(const in6_addr &addr6) noexcept : m_family(Family::V6), m_addr6(addr6)
    {
        // 检测是否是 IPv4 映射地址
        if (isV4Mapped())
        {
            m_family = Family::V4;
        }
    }

    IpAddress::IpAddress(const sockaddr *sa) noexcept
    {
        if (!sa)
        {
            m_family = Family::None;
            m_addr6 = in6_addr{};
            return;
        }
        if (sa->sa_family == AF_INET)
        {
            const auto *sin = reinterpret_cast<const sockaddr_in *>(sa);
            *this = IpAddress(sin->sin_addr);
        } else if (sa->sa_family == AF_INET6)
        {
            const auto *sin6 = reinterpret_cast<const sockaddr_in6 *>(sa);
            *this = IpAddress(sin6->sin6_addr);
        } else
        {
            m_family = Family::None;
            m_addr6 = in6_addr{};
        }
    }

    std::optional<IpAddress> IpAddress::parse(const std::string_view addr)
    {
        if (addr.empty())
        {
            return std::nullopt;
        }

        // 尝试作为 IPv4 解析
        in_addr ipv4;
        if (detail::safe_inet_pton(AF_INET, std::string(addr).c_str(), &ipv4) == 1)
        {
            return IpAddress(ipv4);
        }

        // 尝试作为 IPv6 解析
        in6_addr ipv6;
        if (detail::safe_inet_pton(AF_INET6, std::string(addr).c_str(), &ipv6) == 1)
        {
            return IpAddress(ipv6);
        }

        return std::nullopt;
    }

    IpAddress::Family IpAddress::family() const noexcept
    {
        return m_family;
    }

    bool IpAddress::isValid() const noexcept
    {
        return m_family != Family::None;
    }

    bool IpAddress::isV4() const noexcept
    {
        return m_family == Family::V4;
    }

    bool IpAddress::isV6() const noexcept
    {
        return m_family == Family::V6;
    }

    std::string IpAddress::toString() const
    {
        if (!isValid())
        {
            return {};
        }

        char buf[INET6_ADDRSTRLEN] = {};
        if (isV4())
        {
            in_addr v4;
            std::memcpy(&v4, &m_addr6.s6_addr[12], sizeof(v4));
            detail::safe_inet_ntop(AF_INET, &v4, buf, sizeof(buf));
        } else
        {
            detail::safe_inet_ntop(AF_INET6, &m_addr6, buf, sizeof(buf));
        }
        return std::string(buf);
    }

    in_addr IpAddress::toV4() const noexcept
    {
        in_addr addr{};
        if (isV4())
        {
            std::memcpy(&addr, &m_addr6.s6_addr[12], sizeof(addr));
        }
        return addr;
    }

    const in6_addr &IpAddress::toV6() const noexcept
    {
        return m_addr6;
    }

    sockaddr_storage IpAddress::toSockaddrStorage(const uint16_t port) const noexcept
    {
        sockaddr_storage ss{};
        if (isV4())
        {
            auto *sin = reinterpret_cast<sockaddr_in *>(&ss);
            sin->sin_family = AF_INET;
            sin->sin_port = htons(port);
            std::memcpy(&sin->sin_addr, &m_addr6.s6_addr[12], sizeof(in_addr));
        } else if (isV6())
        {
            auto *sin6 = reinterpret_cast<sockaddr_in6 *>(&ss);
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons(port);
            sin6->sin6_addr = m_addr6;
            sin6->sin6_scope_id = 0; // 作用域 ID 可后续扩展
        }
        return ss;
    }

    std::optional<IpAddress> IpAddress::fromSockaddr(const sockaddr_storage &ss) noexcept
    {
        return IpAddress(reinterpret_cast<const sockaddr *>(&ss));
    }

    bool IpAddress::isV4Mapped() const noexcept
    {
        // 前10个字节为0
        for (int i = 0; i < 10; ++i)
        {
            if (m_addr6.s6_addr[i] != 0)
            {
                return false;
            }
        }
        // 第11,12字节为 0xFF, 0xFF
        return m_addr6.s6_addr[10] == 0xFF && m_addr6.s6_addr[11] == 0xFF;
    }

    IpEndpoint::IpEndpoint(const IpAddress &addr, const uint16_t port) noexcept : m_address(addr), m_port(port)
    {
    }

    IpEndpoint::IpEndpoint(const sockaddr *sa) noexcept
    {
        if (!sa)
        {
            m_address = IpAddress();
            m_port = 0;
            return;
        }
        if (sa->sa_family == AF_INET)
        {
            const auto *sin = reinterpret_cast<const sockaddr_in *>(sa);
            m_address = IpAddress(sin->sin_addr);
            m_port = ntohs(sin->sin_port);
        } else if (sa->sa_family == AF_INET6)
        {
            const auto *sin6 = reinterpret_cast<const sockaddr_in6 *>(sa);
            m_address = IpAddress(sin6->sin6_addr);
            m_port = ntohs(sin6->sin6_port);
        } else
        {
            m_address = IpAddress();
            m_port = 0;
        }
    }

    std::optional<IpEndpoint> IpEndpoint::parse(const std::string_view endpoint)
    {
        if (endpoint.empty())
            return std::nullopt;

        std::string_view host;
        std::string_view port_str;
        uint16_t port = 0;

        // IPv6 方括号解析
        if (endpoint.front() == '[')
        {
            const auto closing = endpoint.find(']');
            if (closing == std::string_view::npos)
            {
                return std::nullopt;
            }
            host = endpoint.substr(1, closing - 1);
            const auto colon_pos = endpoint.find(':', closing);
            if (colon_pos == std::string_view::npos)
            {
                return std::nullopt;
            }
            port_str = endpoint.substr(colon_pos + 1);
        } else
        {
            // IPv4 或主机名: 查找最后一个冒号
            const auto colon_pos = endpoint.rfind(':');
            if (colon_pos == std::string_view::npos)
            {
                return std::nullopt;
            }
            host = endpoint.substr(0, colon_pos);
            port_str = endpoint.substr(colon_pos + 1);
        }

        // 解析端口
        try
        {
            const int p = std::stoi(std::string(port_str));
            if (p < 0 || p > 65535)
            {
                return std::nullopt;
            }
            port = static_cast<uint16_t>(p);
        } catch (...)
        {
            return std::nullopt;
        }

        // 解析地址
        const auto addr = IpAddress::parse(host);
        if (!addr)
        {
            return std::nullopt;
        }

        return IpEndpoint(*addr, port);
    }

    const IpAddress & IpEndpoint::address() const noexcept
    {
        return m_address;
    }

    uint16_t IpEndpoint::port() const noexcept
    {
        return m_port;
    }

    void IpEndpoint::setAddress(const IpAddress &addr) noexcept
    {
        m_address = addr;
    }

    void IpEndpoint::setPort(const uint16_t port) noexcept
    {
        m_port = port;
    }

    bool IpEndpoint::isValid() const noexcept
    {
        return m_address.isValid();
    }

    sockaddr_storage IpEndpoint::toSockaddrStorage() const noexcept
    {
        return m_address.toSockaddrStorage(m_port);
    }

    std::optional<IpEndpoint> IpEndpoint::fromSockaddr(const sockaddr_storage &ss) noexcept
    {
        const auto addr = IpAddress::fromSockaddr(ss);
        if (!addr)
        {
            return std::nullopt;
        }
        uint16_t port = 0;
        if (ss.ss_family == AF_INET)
        {
            const auto *sin = reinterpret_cast<const sockaddr_in *>(&ss);
            port = ntohs(sin->sin_port);
        } else if (ss.ss_family == AF_INET6)
        {
            const auto *sin6 = reinterpret_cast<const sockaddr_in6 *>(&ss);
            port = ntohs(sin6->sin6_port);
        }
        return IpEndpoint(*addr, port);
    }

    std::string IpEndpoint::toString() const
    {
        if (!isValid())
        {
            return {};
        }
        const bool needBrackets = m_address.isV6();
        std::string result;
        if (needBrackets)
        {
            result += '[';
        }
        result += m_address.toString();
        if (needBrackets)
        {
            result += ']';
        }
        result += ':' + std::to_string(m_port);
        return result;
    }
}
