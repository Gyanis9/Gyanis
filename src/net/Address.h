/**
 * @file Address.h
 * @brief 网络地址的封装(IPv4,IPv6,Unix)
 * @date 2025-03-14
 */
#ifndef ADDRESS_H
#define ADDRESS_H
#include <memory>
#include <ostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <vector>
#include <map>

namespace Gyanis::net
{
    class IPAddress;

    /**
     * @brief 网络地址的基类,抽象类
     */
    class Address : public std::enable_shared_from_this<Address>
    {
    public:
        /**
         * @brief 通过sockaddr指针创建Address
         * @param[in] address sockaddr指针
         * @param[in] addrLen sockaddr的长度
         */
        static std::shared_ptr<Address> Create(const sockaddr* address, socklen_t addrLen);

        /**
         * @brief 通过host地址返回对应条件的所有Address
         * @param[out] result 保存满足条件的Address
         * @param[in] host 域名、服务器名等
         * @param[in] family 协议族（如 `AF_INET`, `AF_INET6`, `AF_UNIX`）
         * @param[in] type socket类型（如 `SOCK_STREAM`, `SOCK_DGRAM`）
         * @param[in] protocol 协议类型（如 `IPPROTO_TCP`, `IPPROTO_UDP`）
         */
        static bool Lookup(std::vector<std::shared_ptr<Address>>& result, const std::string& host,
                           int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 通过host地址返回对应条件的任意Address
         * @param[in] host 域名、服务器名等
         * @param[in] family 协议族（如 `AF_INET`, `AF_INET6`, `AF_UNIX`）
         * @param[in] type socket类型（如 `SOCK_STREAM`, `SOCK_DGRAM`）
         * @param[in] protocol 协议类型（如 `IPPROTO_TCP`, `IPPROTO_UDP`）
         */
        static std::shared_ptr<Address> LookupAny(const std::string& host,
                                                  int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 通过host地址返回对应条件的任意IPAddress
         * @param[in] host 域名、服务器名等
         * @param[in] family 协议族（如 `AF_INET`, `AF_INET6`）
         * @param[in] type socket类型（如 `SOCK_STREAM`, `SOCK_DGRAM`）
         * @param[in] protocol 协议类型（如 `IPPROTO_TCP`, `IPPROTO_UDP`）
         */
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                                                             int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
         * @param[out] result 保存本机所有地址
         * @param[in] family 协议族（如 `AF_INET`, `AF_INET6`）
         */
        static bool GetInterfaceAddresses(std::multimap<std::string
                                                        , std::pair<std::shared_ptr<Address>, uint32_t>>& result,
                                          int family = AF_INET);

        /**
         * @brief 获取指定网卡的地址和子网掩码位数
         * @param[out] result 保存指定网卡所有地址
         * @param[in] iface 网卡名称
         * @param[in] family 协议族（如 `AF_INET`, `AF_INET6`）
         */
        static bool GetInterfaceAddresses(std::vector<std::pair<std::shared_ptr<Address>, uint32_t>>& result
                                          , const std::string& iface, int family = AF_INET);

        /**
         * @brief 虚析构函数
         */
        virtual ~Address() = default;

        /**
         * @brief 返回协议簇
         */
        int getFamily() const;

        /**
         * @brief 返回sockaddr指针,只读
         */
        virtual const sockaddr* getSockAddr() const =0;

        /**
         * @brief 返回sockaddr指针,读写
         */
        virtual sockaddr* getSockAddr() =0;

        /**
         * @brief 返回sockaddr的长度
         */
        virtual socklen_t getSockAddrLen() const =0;

        /**
         * @brief 可读性输出地址
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        virtual std::ostream& insert(std::ostream& os) const =0;

        /**
         * @brief 返回可读性字符串
         */
        std::string toString() const;

        /**
         * @brief 小于号比较函数
         * @param other 另一个地址
         * @return bool 如果当前地址小于另一个地址，返回 `true`
         */
        bool operator<(const Address& other) const;

        /**
         * @brief 等于函数
         * @param other 另一个地址
         * @return bool 如果两个地址相等，返回 `true`
         */
        bool operator==(const Address& other) const;

        /**
         * @brief 不等于函数
         * @param other 另一个地址
         * @return bool 如果两个地址不相等，返回 `true`
         */
        bool operator!=(const Address& other) const;
    };

    /**
     * @brief IP地址的基类
     */
    class IPAddress : public Address
    {
    public:
        /**
         * @brief 通过域名、IP、服务器名创建IPAddress
         * @param[in] address 域名、IP 地址等
         * @param[in] port 端口号
         */
        static std::shared_ptr<IPAddress> Create(const char* address, uint16_t port = 0);

        /**
         * @brief 获取该地址的广播地址
         * @param[in] prefix_len 子网掩码位数
         */
        virtual std::shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) =0;

        /**
         * @brief 获取该地址的网段
         * @param[in] prefix_len 子网掩码位数
         */
        virtual std::shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) =0;

        /**
         * @brief 获取子网掩码地址
         * @param[in] prefix_len 子网掩码位数
         */
        virtual std::shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) =0;

        /**
         * @brief 返回端口号
         */
        virtual uint32_t getPort() const =0;

        /**
         * @brief 设置端口号
         */
        virtual void setPort(uint16_t port) =0;
    };

    /**
     * @brief IPv4地址
     */
    class IPv4Address final : public IPAddress
    {
    public:
        /**
         * @brief 使用点分十进制地址创建IPv4Address
         * @param[in] address 点分十进制地址，如 "192.168.1.1"
         * @param[in] port 端口号
         */
        static std::shared_ptr<IPv4Address> Create(const char* address, uint16_t port = 0);

        /**
         * @brief 通过sockaddr_in构造IPv4Address
         * @param[in] address `sockaddr_in` 结构体
         */
        explicit IPv4Address(const sockaddr_in& address);

        /**
         * @brief 通过二进制地址构造IPv4Address
         * @param[in] address 二进制地址
         * @param[in] port 端口号
         */
        explicit IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

        /**
         * @brief 返回sockaddr指针,只读
         */
        const sockaddr* getSockAddr() const override;

        /**
         * @brief 返回sockaddr指针,读写
         */
        sockaddr* getSockAddr() override;

        /**
         * @brief 返回sockaddr的长度
         */
        socklen_t getSockAddrLen() const override;

        /**
         * @brief 可读性输出地址
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& insert(std::ostream& os) const override;

        /**
         * @brief 获取该地址的广播地址
         * @param[in] prefix_len 子网掩码位数
         */
        std::shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;

        /**
         * @brief 获取该地址的网段
         * @param[in] prefix_len 子网掩码位数
         */
        std::shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) override;

        /**
         * @brief 获取子网掩码地址
         * @param[in] prefix_len 子网掩码位数
         */
        std::shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;

        /**
         * @brief 返回端口号
         */
        uint32_t getPort() const override;

        /**
         * @brief 设置端口号
         */
        void setPort(uint16_t port) override;

    private:
        sockaddr_in m_address{}; ///< IPv4地址结构体
    };

    /**
     * @brief IPv6地址
     */
    class IPv6Address final : public IPAddress
    {
    public:
        /**
         * @brief 通过IPv6地址字符串构造IPv6Address
         * @param[in] address IPv6 地址字符串
         * @param[in] port 端口号
         */
        static std::shared_ptr<IPv6Address> Create(const char* address, uint16_t port = 0);

        /**
         * @brief 无参构造函数
         */
        IPv6Address();

        /**
         * @brief 通过sockaddr_in6构造IPv6Address
         * @param[in] address `sockaddr_in6` 结构体
         */
        explicit IPv6Address(const sockaddr_in6& address);

        /**
         * @brief 通过IPv6二进制地址构造IPv6Address
         * @param[in] address IPv6 二进制地址
         * @param port 端口号
         */
        explicit IPv6Address(const uint8_t address[16], uint16_t port = 0);

        /**
         * @brief 返回sockaddr指针,只读
         */
        const sockaddr* getSockAddr() const override;

        /**
         * @brief 返回sockaddr指针,读写
         */
        sockaddr* getSockAddr() override;

        /**
         * @brief 返回sockaddr的长度
         */
        socklen_t getSockAddrLen() const override;

        /**
         * @brief 可读性输出地址
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& insert(std::ostream& os) const override;

        /**
         * @brief 获取该地址的广播地址
         * @param[in] prefix_len 子网掩码位数
         */
        std::shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;

        /**
         * @brief 获取该地址的网段
         * @param[in] prefix_len 子网掩码位数
         */
        std::shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) override;

        /**
         * @brief 获取子网掩码地址
         * @param[in] prefix_len 子网掩码位数
         */
        std::shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;

        /**
         * @brief 返回端口号
         */
        uint32_t getPort() const override;

        /**
         * @brief 设置端口号
         */
        void setPort(uint16_t port) override;

    private:
        sockaddr_in6 m_address{}; ///< IPv6 地址结构体
    };

    /**
     * @brief UnixSocket地址
     */
    class UnixAddress final : public Address
    {
    public:
        /**
         * @brief 无参构造函数
         */
        UnixAddress();

        /**
         * @brief 通过路径构造UnixAddress
         * @param[in] path Unix Socket 路径（长度小于 `UNIX_PATH_MAX`）
         */
        explicit UnixAddress(const std::string& path);

        /**
         * @brief 返回sockaddr指针,只读
         */
        const sockaddr* getSockAddr() const override;

        /**
         * @brief 返回sockaddr指针,读写
         */
        sockaddr* getSockAddr() override;

        /**
         * @brief 返回sockaddr的长度
         */
        socklen_t getSockAddrLen() const override;

        /**
         * @brief 可读性输出地址
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& insert(std::ostream& os) const override;

        /**
         * @brief 设置网络地址长度
         */
        void setAddrLen(uint32_t len);

        /**
         * @brief 获取地址
         */
        std::string getPath() const;

    private:
        sockaddr_un m_address{}; ///< Unix 地址结构体
        socklen_t m_length; ///< 地址长度
    };


    /**
     * @brief 未知地址
     */
    class UnknownAddress final : public Address
    {
    public:
        /**
         * @brief 使用协议族创建未知地址
         * @param[in] family 协议族（如 `AF_INET`, `AF_INET6`）
         */
        explicit UnknownAddress(int family);
        /**
         * @brief 使用sockaddr创建未知地址
         * @param[in] addr `sockaddr` 结构体
         */
        explicit UnknownAddress(const sockaddr& addr);

        /**
         * @brief 返回sockaddr指针,只读
         */
        const sockaddr* getSockAddr() const override;

        /**
         * @brief 返回sockaddr指针,读写
         */
        sockaddr* getSockAddr() override;

        /**
         * @brief 返回sockaddr的长度
         */
        socklen_t getSockAddrLen() const override;

        /**
         * @brief 可读性输出地址
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& insert(std::ostream& os) const override;

    private:
        sockaddr m_address{}; ///< 未知类型地址的结构体
    };

    /**
     * @brief 输出流操作符重载
     * @param os 输出流
     * @param address 地址对象
     * @return std::ostream& 输出流
     */
    std::ostream& operator<<(std::ostream& os, const Address& address);
}

#endif
