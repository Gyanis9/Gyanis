/**
 * @file address.h
 * @brief 跨平台 IP 地址与网络端点封装（IPv4/IPv6）
 * @details 提供 IpAddress 和 IpEndpoint 类，支持地址解析、格式化、比较、哈希及系统 socket 结构转换。
 *          内部使用 IPv6 映射地址（::ffff:x.x.x.x）统一存储 IPv4 地址。
 */

#ifndef ADDRESS_H
#define ADDRESS_H

#include <algorithm>
#include <compare>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

// 跨平台 socket 头文件处理
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# pragma comment(lib, "ws2_32.lib")
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

namespace Net
{
    // -------------------------------------------------------------------
    // 内部辅助：跨平台 inet_pton / inet_ntop (Windows 可能缺失)
    // -------------------------------------------------------------------
    namespace detail
    {
        /**
         * @brief 跨平台将字符串 IP 地址转换为二进制结构
         * @param af 地址族 (AF_INET 或 AF_INET6)
         * @param src 输入的字符串地址（如 "192.168.1.1" 或 "::1"）
         * @param dst 输出缓冲区，指向 in_addr 或 in6_addr
         * @return 成功返回1，输入无效返回0，地址族不支持返回 -1
         */
        int safe_inet_pton(int af, const char *src, void *dst);

        /**
         * @brief 跨平台将二进制地址转换为字符串
         * @param af 地址族 (AF_INET 或 AF_INET6)
         * @param src 指向 in_addr 或 in6_addr 的指针
         * @param dst 输出缓冲区
         * @param size 缓冲区大小
         * @return 成功返回 dst 指针，失败返回 nullptr
         */
        const char *safe_inet_ntop(int af, const void *src, char *dst, socklen_t size);
    } // namespace detail

    // -------------------------------------------------------------------
    // IP 地址类 (IPv4 / IPv6)
    // 内部统一使用 IPv6 映射地址存储 IPv4 (::ffff:x.x.x.x)
    // -------------------------------------------------------------------

    /**
     * @brief IP 地址类，支持 IPv4 和 IPv6。
     * @details 内部存储为 IPv6 格式，IPv4 被映射为 ::ffff:0:0/96 范围内的地址。
     *          提供构造、解析、比较、格式化及系统 sockaddr 互转功能。
     */
    class IpAddress
    {
    public:
        /**
         * @brief 地址族枚举。
         */
        enum class Family : int
        {
            None = AF_UNSPEC, ///< 未指定/无效地址
            V4   = AF_INET,   ///< IPv4
            V6   = AF_INET6   ///< IPv6
        };

        /**
         * @brief 默认构造函数，构造一个无效地址。
         */
        constexpr IpAddress() noexcept
            : m_family(Family::None)
        {
            m_addr6 = in6_addr{};
        }

        /**
         * @brief 从 IPv4 原始结构构造地址。
         * @param addr4 IPv4 地址结构 (in_addr)
         */
        explicit IpAddress(const in_addr &addr4) noexcept;

        /**
         * @brief 从 IPv6 原始结构构造地址。
         * @param addr6 IPv6 地址结构 (in6_addr)
         * @note 若 addr6 本身是 IPv4 映射地址，则自动识别为 IPv4 地址。
         */
        explicit IpAddress(const in6_addr &addr6) noexcept;

        /**
         * @brief 从 sockaddr 结构构造地址（支持 AF_INET/AF_INET6）。
         * @param sa sockaddr 指针，若为 nullptr 则构造无效地址。
         */
        explicit IpAddress(const sockaddr *sa) noexcept;

        /**
         * @brief 从字符串解析 IP 地址。
         * @param addr 字符串地址，如 "192.168.1.1" 或 "::1"
         * @return 解析成功返回 IpAddress，失败返回 std::nullopt。
         */
        static std::optional<IpAddress> parse(std::string_view addr);

        /**
         * @brief 获取地址族。
         * @return Family 枚举值。
         */
        [[nodiscard]] Family family() const noexcept;

        /**
         * @brief 判断是否为有效地址。
         * @return true 表示有效，false 表示无效。
         */
        [[nodiscard]] bool isValid() const noexcept;

        /**
         * @brief 判断是否为 IPv4 地址（包括映射形式）。
         * @return true 表示 IPv4。
         */
        [[nodiscard]] bool isV4() const noexcept;

        /**
         * @brief 判断是否为 IPv6 地址（非映射地址）。
         * @return true 表示 IPv6。
         */
        [[nodiscard]] bool isV6() const noexcept;

        /**
         * @brief 将地址转换为字符串（点分十进制或十六进制冒号格式）。
         * @return 地址字符串，无效地址返回空字符串。
         */
        [[nodiscard]] std::string toString() const;

        /**
         * @brief 获取 IPv4 原始结构（仅当 isV4() 为 true 时有效）。
         * @return in_addr 结构，若地址不是 IPv4 则返回全零地址。
         */
        [[nodiscard]] in_addr toV4() const noexcept;

        /**
         * @brief 获取 IPv6 原始结构（对于 IPv4 地址返回映射后的 IPv6 表示）。
         * @return in6_addr 结构。
         */
        [[nodiscard]] const in6_addr &toV6() const noexcept;

        /**
         * @brief 将地址填充到 sockaddr_storage 中，并绑定端口。
         * @param port 端口号（主机字节序），默认为 0。
         * @return sockaddr_storage 结构。
         */
        [[nodiscard]] sockaddr_storage toSockaddrStorage(uint16_t port = 0) const noexcept;

        /**
         * @brief 从 sockaddr_storage 构造地址。
         * @param ss sockaddr_storage 结构。
         * @return 成功返回 IpAddress，失败返回 std::nullopt。
         */
        static std::optional<IpAddress> fromSockaddr(const sockaddr_storage &ss) noexcept;

        /**
         * @brief 三路比较运算符（基于字节序）。
         * @param lhs 左操作数
         * @param rhs 右操作数
         * @return 比较结果（std::strong_ordering）
         */
        friend auto operator<=>(const IpAddress &lhs, const IpAddress &rhs) noexcept
        {
            if (lhs.m_family != rhs.m_family)
            {
                return static_cast<int>(lhs.m_family) <=> static_cast<int>(rhs.m_family);
            }
            if (lhs.m_family == Family::None)
                return std::strong_ordering::equal;
            return std::memcmp(&lhs.m_addr6, &rhs.m_addr6, sizeof(in6_addr)) <=> 0;
        }

        /**
         * @brief 相等比较运算符。
         */
        friend bool operator==(const IpAddress &lhs, const IpAddress &rhs) noexcept
        {
            return (lhs <=> rhs) == 0;
        }

        // 支持 std::hash
        friend struct std::hash<IpAddress>;

    private:
        /**
         * @brief 检查当前 IPv6 地址是否为 IPv4 映射地址（::ffff:0:0/96）。
         * @return true 表示为映射地址。
         */
        bool isV4Mapped() const noexcept;

        Family m_family;  ///< 地址族
        in6_addr m_addr6; ///< 存储 IPv6 地址(IPv4 采用映射方式)
    };

    // -------------------------------------------------------------------
    // 网络端点 (IP + 端口)
    // -------------------------------------------------------------------

    /**
     * @brief IP 网络端点，包含 IP 地址和端口号。
     * @details 提供构造、解析、格式化以及与 sockaddr 互转的功能。
     */
    class IpEndpoint
    {
    public:
        /**
         * @brief 默认构造函数，构造无效端点（地址无效，端口 0）。
         */
        constexpr IpEndpoint() noexcept
            : m_address(), m_port(0)
        {
        }

        /**
         * @brief 从 IpAddress 和端口构造端点。
         * @param addr IP 地址
         * @param port 端口号（主机字节序），默认为 0
         */
        explicit IpEndpoint(const IpAddress &addr, const uint16_t port = 0) noexcept;

        /**
         * @brief 从 sockaddr 结构构造端点。
         * @param sa sockaddr 指针，支持 AF_INET 和 AF_INET6。
         */
        explicit IpEndpoint(const sockaddr *sa) noexcept;

        /**
         * @brief 从字符串解析端点，支持格式：
         *        - IPv4: "192.168.1.1:8080"
         *        - IPv6: "[::1]:8080"
         * @param endpoint 待解析字符串
         * @return 解析成功返回 IpEndpoint，失败返回 std::nullopt。
         */
        static std::optional<IpEndpoint> parse(std::string_view endpoint);

        /**
         * @brief 获取 IP 地址（只读）。
         * @return IpAddress 引用。
         */
        [[nodiscard]] const IpAddress &address() const noexcept;

        /**
         * @brief 获取端口号（主机字节序）。
         * @return 端口号。
         */
        [[nodiscard]] uint16_t port() const noexcept;

        /**
         * @brief 设置 IP 地址。
         * @param addr 新的 IP 地址。
         */
        void setAddress(const IpAddress &addr) noexcept;

        /**
         * @brief 设置端口号（主机字节序）。
         * @param port 新的端口号。
         */
        void setPort(const uint16_t port) noexcept;

        /**
         * @brief 判断端点是否有效（地址有效）。
         * @return true 有效，false 无效。
         */
        [[nodiscard]] bool isValid() const noexcept;

        /**
         * @brief 将端点转换为 sockaddr_storage 结构。
         * @return sockaddr_storage 对象。
         */
        [[nodiscard]] sockaddr_storage toSockaddrStorage() const noexcept;

        /**
         * @brief 从 sockaddr_storage 构造端点。
         * @param ss sockaddr_storage 结构。
         * @return 成功返回 IpEndpoint，失败返回 std::nullopt。
         */
        static std::optional<IpEndpoint> fromSockaddr(const sockaddr_storage &ss) noexcept;

        /**
         * @brief 将端点转换为字符串（格式如 "192.168.1.1:8080" 或 "[::1]:8080"）。
         * @return 字符串表示，无效端点返回空字符串。
         */
        [[nodiscard]] std::string toString() const;

        /**
         * @brief 三路比较运算符（先比较地址，再比较端口）。
         */
        friend auto operator<=>(const IpEndpoint &lhs, const IpEndpoint &rhs) noexcept
        {
            if (const auto cmp = lhs.m_address <=> rhs.m_address; cmp != 0)
            {
                return cmp;
            }
            return lhs.m_port <=> rhs.m_port;
        }

        /**
         * @brief 相等比较运算符。
         */
        friend bool operator==(const IpEndpoint &lhs, const IpEndpoint &rhs) noexcept
        {
            return lhs.m_address == rhs.m_address && lhs.m_port == rhs.m_port;
        }

        // 支持 std::hash
        friend struct std::hash<IpEndpoint>;

    private:
        IpAddress m_address; ///< IP 地址
        uint16_t m_port;     ///< 端口号（主机字节序）
    };
} // namespace Net  // 注：原代码中写为 coro，这里修正为 Net

// -------------------------------------------------------------------
// 标准库 hash 支持
// -------------------------------------------------------------------
namespace std
{
    /**
     * @brief std::hash 特化，支持 IpAddress 作为 unordered_map 的键。
     */
    template<>
    struct hash<Net::IpAddress>
    {
        /**
         * @brief 计算 IpAddress 的哈希值。
         * @param addr IP 地址对象
         * @return 哈希值（size_t）
         */
        size_t operator()(const Net::IpAddress &addr) const noexcept
        {
            if (!addr.isValid())
            {
                return 0;
            }
            // 使用 in6_addr 的字节生成哈希
            const auto *bytes = reinterpret_cast<const uint8_t *>(&addr.m_addr6);
            size_t h = 0;
            for (size_t i = 0; i < sizeof(addr.m_addr6); ++i)
            {
                h = h * 31 + bytes[i];
            }
            // 混入地址族
            h = h * 31 + static_cast<int>(addr.m_family);
            return h;
        }
    };

    /**
     * @brief std::hash 特化，支持 IpEndpoint 作为 unordered_map 的键。
     */
    template<>
    struct hash<Net::IpEndpoint>
    {
        /**
         * @brief 计算 IpEndpoint 的哈希值。
         * @param ep 端点对象
         * @return 哈希值（size_t）
         */
        size_t operator()(const Net::IpEndpoint &ep) const noexcept
        {
            const size_t h1 = hash<Net::IpAddress>{}(ep.m_address);
            const size_t h2 = hash<uint16_t>{}(ep.m_port);
            return h1 ^ (h2 << 1);
        }
    };
}

// -------------------------------------------------------------------
// C++20 格式化支持 (std::formatter)
// -------------------------------------------------------------------

/**
 * @brief std::formatter 特化，支持 IpAddress 使用 std::format。
 */
template<>
struct std::formatter<Net::IpAddress> : std::formatter<std::string>
{
    /**
     * @brief 格式化 IpAddress 为字符串。
     * @param addr IP 地址对象
     * @param ctx 格式化上下文
     * @return 格式化后的输出迭代器
     */
    auto format(const Net::IpAddress &addr, std::format_context &ctx) const
    {
        return std::formatter<std::string>::format(addr.toString(), ctx);
    }
};

/**
 * @brief std::formatter 特化，支持 IpEndpoint 使用 std::format。
 */
template<>
struct std::formatter<Net::IpEndpoint> : std::formatter<std::string>
{
    /**
     * @brief 格式化 IpEndpoint 为字符串。
     * @param ep 端点对象
     * @param ctx 格式化上下文
     * @return 格式化后的输出迭代器
     */
    auto format(const Net::IpEndpoint &ep, std::format_context &ctx) const
    {
        return std::formatter<std::string>::format(ep.toString(), ctx);
    }
};

#endif // ADDRESS_H
