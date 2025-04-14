#include "net/Address.h"
#include "base/Log.h"
#include "base/Endian.h"

#include <bitset>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sstream>

namespace Gyanis::net
{
    static auto g_logger = LOG_NAME("system");

    /**
     * @brief 创建一个掩码（mask）
     * @param[in] bits 掩码的位数，即要设置为 1 的低位的个数
     * @return 返回创建的掩码，类型为 `T`
     */
    template <typename T>
    static T CreateMask(const uint32_t bits)
    {
        std::bitset<sizeof(T) * 8> bitsSet; /// 创建一个指定大小的 bitset
        for (uint32_t i = 0; i < bits; ++i)
        {
            bitsSet.set(i); /// 设置低位为 1
        }
        return static_cast<T>(bitsSet.to_ulong()); /// 转换为目标类型
    }

    /**
     * @brief 计算一个整数（无符号类型 T）中 1 的位数
     * @param[in] value 要计算的整数
     * @return 返回整数中 1 的位数
     */
    template <class T>
    static uint32_t CountBytes(T value)
    {
        uint32_t result = 0;
        for (; value; ++result)
        {
            value &= value - 1;
        }
        return result;
    }

    std::shared_ptr<Address> Address::Create(const sockaddr* address, socklen_t)
    {
        if (!address)
        {
            return nullptr;
        }
        std::shared_ptr<Address> result = nullptr;
        switch (address->sa_family)
        {
        case AF_INET:
            result = std::make_shared<IPv4Address>(*reinterpret_cast<const sockaddr_in*>(address));
            break;
        case AF_INET6:
            result = std::make_shared<IPv6Address>(*reinterpret_cast<const sockaddr_in6*>(address));
            break;
        default:
            result = std::make_shared<UnknownAddress>(*address);
        }
        return result;
    }

    bool Address::Lookup(std::vector<std::shared_ptr<Address>>& result, const std::string& host, int family, int type,
                         const int protocol)
    {
        if (host.empty()) /// 检查主机名是否为空
        {
            LOG_ERROR(g_logger)
                << "Address::Lookup() - Host is empty. "
                << "Please verify the input host address.";
            return false;
        }
        const addrinfo hints{0, family, type, protocol, 0, nullptr, nullptr, nullptr};
        std::string node; /// 用于存储解析后的主机名部分
        const char* service = nullptr; /// 用于存储服务（端口号）部分
        if (host.front() == '[') /// 如果 host 是IPv6地址的形式，格式如 [::1]:8080，首先找到 ']' 字符并提取IPv6地址和端口
        {
            /// 查找']'字符，判断是否是IPv6地址
            if (const auto endipv6 = static_cast<const char*>(memchr(host.c_str() + 1, ']', host.size() - 1)))
            {
                service = *(endipv6 + 1) == ':' ? endipv6 + 2 : nullptr; /// 如果 ']' 后面跟着 ':'，表示后面是端口号
                node.assign(host.c_str() + 1, endipv6 - host.c_str() - 1); /// 将 '[]' 内的IPv6地址部分提取出来
            }
        }
        if (node.empty()) // 如果 node 为空，尝试查找主机名和端口号分隔符 ':'
        {
            if (const auto service_pos = static_cast<const char*>(memchr(host.c_str(), ':', host.size())))
            {
                if (!memchr(service_pos + 1, ':', host.c_str() + host.size() - service_pos - 1))
                // 确保 ':' 后面没有更多的 ':' 字符（即只包含一个端口号）
                {
                    node.assign(host.c_str(), service_pos - host.c_str()); // 提取主机名部分
                    service = service_pos + 1; // 提取端口号部分
                }
            }
        }
        node = node.empty() ? host : node; // 如果 node 为空，表示没有提取出主机名部分，使用整个 host 作为主机名
        addrinfo* results = nullptr;
        if (const int error = getaddrinfo(node.c_str(), service, &hints, &results))
        {
            LOG_ERROR(g_logger)
                << "Address::Lookup - Failed to get address for host: " << host
                << " | Address family: " << family
                << " | Socket type: " << type
                << " | Error code: " << error
                << " | Error description: " << strerror(error);
            freeaddrinfo(results);
            return false;
        }
        const addrinfo* next = results;
        while (next)
        {
            result.push_back(Create(next->ai_addr, next->ai_addrlen));
            next = next->ai_next;
        }
        freeaddrinfo(results);
        return !result.empty();
    }

    std::shared_ptr<Address> Address::LookupAny(const std::string& host, const int family, const int type,
                                                const int protocol)
    {
        if (std::vector<std::shared_ptr<Address>> result; Lookup(result, host, family, type, protocol))
        {
            return result.front();
        }
        return nullptr;
    }

    std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host, const int family, const int type,
                                                           const int protocol)
    {
        if (std::vector<std::shared_ptr<Address>> result; Lookup(result, host, family, type, protocol))
        {
            for (auto& i : result)
            {
                if (std::shared_ptr<IPAddress> value = std::dynamic_pointer_cast<IPAddress>(i))
                {
                    return value;
                }
            }
        }
        return nullptr;
    }

    bool Address::GetInterfaceAddresses(
        std::multimap<std::string, std::pair<std::shared_ptr<Address>, uint32_t>>& result, const int family)
    {
        ifaddrs* results = nullptr;
        if (getifaddrs(&results) != 0)
        {
            LOG_ERROR(g_logger)
                << "Address::GetInterfaceAddresses - getifaddrs() failed. "
                << "Error code: " << errno
                << " | Error description: " << strerror(errno);
            return false;
        }

        try
        {
            for (ifaddrs* next = results; next; next = next->ifa_next)
            {
                std::shared_ptr<Address> address = nullptr;
                uint32_t prefix_len = std::numeric_limits<uint32_t>::max();
                if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
                {
                    continue;
                }
                switch (next->ifa_addr->sa_family)
                {
                case AF_INET:
                    {
                        address = Create(next->ifa_addr, sizeof(sockaddr_in));
                        const uint32_t netmask = reinterpret_cast<sockaddr_in*>(next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        address = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        const in6_addr& netmask = reinterpret_cast<sockaddr_in6*>(next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for (int i = 0; i < 16; ++i)
                        {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
                }
                if (address)
                {
                    result.insert(std::make_pair(next->ifa_name, std::make_pair(address, prefix_len)));
                }
            }
        }
        catch (...)
        {
            LOG_ERROR(g_logger)
                << "Address::GetInterfaceAddresses - getifaddrs() failed. "
                << "Error code: " << errno;
            freeifaddrs(results);
            return false;
        }
        freeifaddrs(results);
        return !result.empty();
    }

    bool Address::GetInterfaceAddresses(std::vector<std::pair<std::shared_ptr<Address>, uint32_t>>& result,
                                        const std::string& iface, const int family)
    {
        if (iface.empty() || iface == "*")
        {
            if (family == AF_INET || family == AF_UNSPEC)
            {
                result.emplace_back(std::static_pointer_cast<Address>(std::make_shared<IPv4Address>()), 0u);
            }
            if (family == AF_INET6 || family == AF_UNSPEC)
            {
                result.emplace_back(std::static_pointer_cast<Address>(std::make_shared<IPv6Address>()), 0u);
            }
            return true;
        }
        std::multimap<std::string, std::pair<std::shared_ptr<Address>, uint32_t>> results;
        if (!GetInterfaceAddresses(results, family))
        {
            return false;
        }
        for (auto its = results.equal_range(iface); its.first != its.second; ++its.first)
        {
            result.push_back(its.first->second);
        }
        return !result.empty();
    }

    int Address::getFamily() const
    {
        return getSockAddr()->sa_family;
    }

    std::string Address::toString() const
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    bool Address::operator<(const Address& other) const
    {
        const socklen_t minLen = std::min(getSockAddrLen(), other.getSockAddrLen());

        if (const int result = memcmp(getSockAddr(), other.getSockAddr(), minLen); result < 0)
        {
            return true;
        }
        else if (result > 0)
        {
            return false;
        }
        return getSockAddrLen() < other.getSockAddrLen();
    }


    bool Address::operator==(const Address& other) const
    {
        return getSockAddrLen() == other.getSockAddrLen() && memcmp(getSockAddr(), other.getSockAddr(),
                                                                    getSockAddrLen()) == 0;
    }

    bool Address::operator!=(const Address& other) const
    {
        return !(*this == other);
    }

    std::shared_ptr<IPAddress> IPAddress::Create(const char* address, const uint16_t port)
    {
        addrinfo hints{}, *results = nullptr;
        hints.ai_family = AF_UNSPEC; /// 根据目标主机的名字或者其他信息自动选择 IPv4 或 IPv6 地址
        hints.ai_flags = AI_NUMERICHOST; ///告诉 getaddrinfo 函数，传入的字符串是一个 IP 地址而不是主机名，因此不会进行 DNS 查找，只解析该 IP 地址。
        if (const int error = getaddrinfo(address, nullptr, &hints, &results))
        {
            LOG_ERROR(g_logger)
                << "IPAddress::Create - failed. "
                << "Address: " << address
                << " | Port: " << port
                << " | Error code: " << error
                << " | errno: " << errno
                << " | Error description: " << strerror(errno);
            return nullptr;
        }
        try
        {
            std::shared_ptr<IPAddress> result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, results->ai_addrlen));
            if (result)
            {
                result->setPort(port);
            }
            freeaddrinfo(results);
            return result;
        }
        catch (...)
        {
            freeaddrinfo(results);
            return nullptr;
        }
    }

    std::shared_ptr<IPv4Address> IPv4Address::Create(const char* address, const uint16_t port)
    {
        auto rt = std::make_shared<IPv4Address>();
        rt->m_address.sin_port = base::byteswapOnLittleEndian(port);
        if (const int result = inet_pton(AF_INET, address, &rt->m_address.sin_addr); result < 0)
        {
            LOG_ERROR(g_logger)
                << "IPv4Address::Create - failed. "
                << "Address: " << address << ":" << port
                << " | Result: " << result
                << " | errno: " << errno
                << " | Error description: " << strerror(errno);
            return nullptr;
        }
        return rt;
    }

    IPv4Address::IPv4Address(const sockaddr_in& address): m_address(address)
    {
    }

    IPv4Address::IPv4Address(const uint32_t address, const uint16_t port)
    {
        m_address.sin_port = base::byteswapOnLittleEndian(port);
        m_address.sin_addr.s_addr = base::byteswapOnLittleEndian(address);
        m_address.sin_family = AF_INET;
    }

    const sockaddr* IPv4Address::getSockAddr() const
    {
        return reinterpret_cast<const sockaddr*>(&m_address);
    }

    sockaddr* IPv4Address::getSockAddr()
    {
        return reinterpret_cast<sockaddr*>(&m_address);
    }

    socklen_t IPv4Address::getSockAddrLen() const
    {
        return sizeof(m_address);
    }

    std::ostream& IPv4Address::insert(std::ostream& os) const
    {
        const uint32_t addr = base::byteswapOnLittleEndian(m_address.sin_addr.s_addr);
        os << (addr >> 24 & 0xff) << "."
            << (addr >> 16 & 0xff) << "."
            << (addr >> 8 & 0xff) << "."
            << (addr & 0xff);
        os << ":" << base::byteswapOnLittleEndian(m_address.sin_port);
        return os;
    }

    std::shared_ptr<IPAddress> IPv4Address::broadcastAddress(const uint32_t prefix_len)
    {
        if (prefix_len > 32)
        {
            return nullptr;
        }
        sockaddr_in broadcast(m_address);
        broadcast.sin_addr.s_addr |= base::byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(broadcast);
    }

    std::shared_ptr<IPAddress> IPv4Address::networdAddress(const uint32_t prefix_len)
    {
        if (prefix_len > 32)
        {
            return nullptr;
        }
        sockaddr_in broadcast(m_address);
        broadcast.sin_addr.s_addr &= base::byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(broadcast);
    }

    std::shared_ptr<IPAddress> IPv4Address::subnetMask(const uint32_t prefix_len)
    {
        sockaddr_in subnet{};
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~base::byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(subnet);
    }

    uint32_t IPv4Address::getPort() const
    {
        return base::byteswapOnLittleEndian(m_address.sin_port);
    }

    void IPv4Address::setPort(const uint16_t port)
    {
        m_address.sin_port = base::byteswapOnLittleEndian(port);
    }

    std::shared_ptr<IPv6Address> IPv6Address::Create(const char* address, uint16_t port)
    {
        auto rt = std::make_shared<IPv6Address>();
        rt->m_address.sin6_port = base::byteswapOnLittleEndian(port);
        if (const int result = inet_pton(AF_INET6, address, &rt->m_address.sin6_addr); result < 0)
        {
            LOG_ERROR(g_logger)
                << "IPv6Address::Create - failed. "
                << "Address: " << address << ":" << port
                << " | errno: " << errno
                << " | Error description: " << strerror(errno);
            return nullptr;
        }
        return rt;
    }

    IPv6Address::IPv6Address()
    {
        m_address.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const sockaddr_in6& address): m_address(address)
    {
    }

    IPv6Address::IPv6Address(const uint8_t address[16], const uint16_t port)
    {
        m_address.sin6_family = AF_INET6;
        m_address.sin6_port = base::byteswapOnLittleEndian(port);
        memcpy(&m_address.sin6_addr.s6_addr, address, 16);
    }

    const sockaddr* IPv6Address::getSockAddr() const
    {
        return reinterpret_cast<const sockaddr*>(&m_address);
    }

    sockaddr* IPv6Address::getSockAddr()
    {
        return reinterpret_cast<sockaddr*>(&m_address);
    }

    socklen_t IPv6Address::getSockAddrLen() const
    {
        return sizeof(m_address);
    }

    std::ostream& IPv6Address::insert(std::ostream& os) const
    {
        os << "[";
        const auto* addr = (uint16_t*)m_address.sin6_addr.s6_addr;
        bool used_zeros = false;
        for (size_t i = 0; i < 8; ++i)
        {
            if (addr[i] == 0 && !used_zeros)
            {
                continue;
            }
            if (i && addr[i - 1] == 0 && !used_zeros)
            {
                os << ":";
                used_zeros = true;
            }
            if (i)
            {
                os << ":";
            }
            os << std::hex << static_cast<int>(base::byteswapOnLittleEndian(addr[i])) << std::dec;
        }

        if (!used_zeros && addr[7] == 0)
        {
            os << "::";
        }

        os << "]:" << base::byteswapOnLittleEndian(m_address.sin6_port);
        return os;
    }

    std::shared_ptr<IPAddress> IPv6Address::broadcastAddress(const uint32_t prefix_len)
    {
        sockaddr_in6 baddr(m_address);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |=
            CreateMask<uint8_t>(prefix_len % 8);
        for (int i = static_cast<int>(prefix_len / 8) + 1; i < 16; ++i)
        {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return std::make_shared<IPv6Address>(baddr);
    }

    std::shared_ptr<IPAddress> IPv6Address::networdAddress(const uint32_t prefix_len)
    {
        sockaddr_in6 baddr(m_address);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &=
            CreateMask<uint8_t>(prefix_len % 8);
        for (int i = static_cast<int>(prefix_len / 8 + 1); i < 16; ++i)
        {
            baddr.sin6_addr.s6_addr[i] = 0x00;
        }
        return std::make_shared<IPv6Address>(baddr);
    }

    std::shared_ptr<IPAddress> IPv6Address::subnetMask(const uint32_t prefix_len)
    {
        sockaddr_in6 subnet = {};
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefix_len / 8] =
            ~CreateMask<uint8_t>(prefix_len % 8);

        for (uint32_t i = 0; i < prefix_len / 8; ++i)
        {
            subnet.sin6_addr.s6_addr[i] = 0xff;
        }
        return std::make_shared<IPv6Address>(subnet);
    }

    uint32_t IPv6Address::getPort() const
    {
        return base::byteswapOnLittleEndian(m_address.sin6_port);
    }

    void IPv6Address::setPort(const uint16_t port)
    {
        m_address.sin6_port = base::byteswapOnLittleEndian(port);
    }

    static constexpr size_t MAX_PATH_LEN = sizeof(static_cast<sockaddr_un*>(nullptr)->sun_path) - 1;

    UnixAddress::UnixAddress()
    {
        m_address.sun_family = AF_UNIX;
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string& path)
    {
        m_address.sun_family = AF_UNIX;
        m_length = path.size() + 1;
        if (!path.empty() && path.front() == '\0')
        {
            --m_length;
        }
        if (m_length > sizeof(m_address.sun_path))
        {
            throw std::logic_error("The specified path exceeds the allowed length.");
        }
        memcpy(m_address.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);
    }

    const sockaddr* UnixAddress::getSockAddr() const
    {
        return reinterpret_cast<const sockaddr*>(&m_address);
    }

    sockaddr* UnixAddress::getSockAddr()
    {
        return reinterpret_cast<sockaddr*>(&m_address);
    }

    socklen_t UnixAddress::getSockAddrLen() const
    {
        return m_length;
    }

    std::ostream& UnixAddress::insert(std::ostream& os) const
    {
        if (m_length > offsetof(sockaddr_un, sun_path)
            && m_address.sun_path[0] == '\0')
        {
            return os << "\\0" << std::string(m_address.sun_path + 1,
                                              m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        return os << m_address.sun_path;
    }

    void UnixAddress::setAddrLen(const uint32_t len)
    {
        m_length = len;
    }

    std::string UnixAddress::getPath() const
    {
        std::stringstream ss;
        if (m_length > offsetof(sockaddr_un, sun_path)
            && m_address.sun_path[0] == '\0')
        {
            ss << "\\0" << std::string(m_address.sun_path + 1,
                                       m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        else
        {
            ss << m_address.sun_path;
        }
        return ss.str();
    }

    UnknownAddress::UnknownAddress(const int family)
    {
        m_address.sa_family = family;
    }

    UnknownAddress::UnknownAddress(const sockaddr& addr): m_address(addr)
    {
    }

    const sockaddr* UnknownAddress::getSockAddr() const
    {
        return &m_address;
    }

    sockaddr* UnknownAddress::getSockAddr()
    {
        return &m_address;
    }

    socklen_t UnknownAddress::getSockAddrLen() const
    {
        return sizeof(m_address);
    }

    std::ostream& UnknownAddress::insert(std::ostream& os) const
    {
        os << "[UnknownAddress - Address Family: " << m_address.sa_family << "]";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Address& address)
    {
        return address.insert(os);
    }
}
