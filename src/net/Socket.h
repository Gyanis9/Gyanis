/**
 * @file Socket.h
 * @brief 网络socket模块封装
 * @date 2025-03-14
 */
#ifndef SOCKET_H
#define SOCKET_H
#include <openssl/ssl.h>
#include "net/Address.h"
#include "net/Socket.h"

namespace Gyanis::net
{
    class Socket : public std::enable_shared_from_this<Socket>
    {
    public:
        /**
          * @brief 套接字类型枚举
          */
        enum Type
        {
            TCP = SOCK_STREAM, ///< TCP 套接字类型
            UDP = SOCK_DGRAM ///< UDP 套接字类型
        };

        /**
         * @brief 套接字协议族枚举
         */
        enum Family
        {
            IPv4 = AF_INET, ///< IPv4 协议族
            IPv6 = AF_INET6, ///< IPv6 协议族
            UNIX = AF_UNIX ///< Unix 套接字协议族
        };

        /**
         * @brief 创建一个 TCP 套接字并绑定到指定地址
         * @param[in] address 地址对象，指定绑定的地址
         * @return 返回创建的 `Socket` 实例
         */
        static std::shared_ptr<Socket> CreateTCP(const std::shared_ptr<Address>& address);

        /**
         * @brief 创建一个 UDP 套接字并绑定到指定地址
         * @param[in] address 地址对象，指定绑定的地址
         * @return 返回创建的 `Socket` 实例
         */
        static std::shared_ptr<Socket> CreateUDP(const std::shared_ptr<Address>& address);

        /**
         * @brief 创建一个 TCP 套接字
         */
        static std::shared_ptr<Socket> CreateTCPSocket();

        /**
         * @brief 创建一个 UDP 套接字
         */
        static std::shared_ptr<Socket> CreateUDPSocket();

        /**
         * @brief 创建一个 IPv6 的 TCP 套接字
         */
        static std::shared_ptr<Socket> CreateTCPSocket6();

        /**
         * @brief 创建一个 IPv6 的 UDP 套接字
         */
        static std::shared_ptr<Socket> CreateUDPSocket6();

        /**
         * @brief 创建一个 Unix 域套接字的 TCP 套接字
         */
        static std::shared_ptr<Socket> CreateUnixTCPSocket();

        /**
         * @brief 创建一个 Unix 域套接字的 UDP 套接字
         */
        static std::shared_ptr<Socket> CreateUnixUDPSocket();

        /**
         * @brief 构造函数
         * @param[in] family 协议族（如 `AF_INET`）
         * @param[in] type 套接字类型（如 `SOCK_STREAM`）
         * @param[in] protocol 协议类型（如 `IPPROTO_TCP`）
         */
        explicit Socket(int family, int type, int protocol);

        /**
         * @brief 析构函数
         */
        virtual ~Socket();

        /**
         * @brief 获取发送超时时间
         */
        int64_t getSendTimeout() const;

        /**
         * @brief 设置发送超时时间
         */
        void setSendTimeout(int64_t timeout);

        /**
         * @brief 获取接收超时时间
         */
        int64_t getRecvTimeout() const;

        /**
         * @brief 设置接收超时时间
         * @param[in] timeout 超时时间（毫秒）
         */
        void setRecvTimeout(int64_t timeout);

        /**
         * @brief 获取套接字选项
         * @param[in] level 套接字选项的级别
         * @param[in] option 套接字选项
         * @param[out] result 保存选项的结果
         * @param[in,out] len 选项的长度
         * @return 是否成功获取选项
         */
        bool getOption(int level, int option, void* result, socklen_t* len) const;

        /**
         * @brief 获取指定类型的套接字选项
         * @param[in] level 套接字选项的级别
         * @param[in] option 套接字选项
         * @param[out] result 保存选项的结果
         * @return 是否成功获取选项
         */
        template <typename T>
        bool getOption(const int level, const int option, T& result)
        {
            socklen_t len = sizeof(T);
            return getOption(level, option, &result, &len);
        }

        /**
         * @brief 设置套接字选项
         * @param[in] level 套接字选项的级别
         * @param[in] option 套接字选项
         * @param[in] result 选项值
         * @param[in] len 选项的长度
         * @return 是否成功设置选项
         */
        bool setOption(int level, int option, const void* result, socklen_t len) const;

        /**
         * @brief 设置指定类型的套接字选项
         * @param[in] level 套接字选项的级别
         * @param[in] option 套接字选项
         * @param[in] value 选项值
         * @return 是否成功设置选项
         */
        template <typename T>
        bool setOption(const int level, const int option, const T& value)
        {
            return setOption(level, option, &value, sizeof(T));
        }

        /**
         * @brief 接受连接请求
         */
        virtual std::shared_ptr<Socket> accept();

        /**
         * @brief 绑定套接字到指定地址
         * @param[in] address 要绑定的地址
         * @return 是否成功绑定
         */
        virtual bool bind(const std::shared_ptr<Address>& address);

        /**
         * @brief 连接到指定地址
         * @param[in] address 目标地址
         * @param[in] timeout_ms 超时时间（毫秒）
         * @return 是否成功连接
         */
        virtual bool connect(const std::shared_ptr<Address>& address,
                             uint64_t timeout_ms);

        /**
         * @brief 重新连接
         * @param[in] timeout_ms 超时时间（毫秒）
         * @return 是否成功重新连接
         */
        virtual bool reconnect(uint64_t timeout_ms);

        /**
         * @brief 监听连接请求
         * @param[in] backlog 队列中等待的最大连接数
         * @return 是否成功监听
         */
        virtual bool listen(int backlog) const;

        /**
         * @brief 关闭套接字
         */
        bool close();

        /**
         * @brief 发送数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 数据长度
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        virtual long send(const void* buffer, size_t length, int flags) const;

        /**
         * @brief 发送多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        virtual long send(iovec* buffer, size_t len, int flags) const;

        /**
         * @brief 向指定地址发送数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 数据长度
         * @param[in] address 目标地址
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        virtual long sendTo(const void* buffer, size_t length, const std::shared_ptr<Address>& address,
                            int flags) const;

        /**
         * @brief 向指定地址发送多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] address 目标地址
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        virtual long sendTo(iovec* buffer, size_t len, const std::shared_ptr<Address>& address,
                            int flags) const;

        /**
         * @brief 接收数据
         * @param[in] buffer 接收数据缓冲区
         * @param[in] length 缓冲区大小
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        virtual long recv(void* buffer, size_t length, int flags) const;

        /**
         * @brief 接收多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        virtual long recv(iovec* buffer, size_t len, int flags) const;

        /**
         * @brief 从指定地址接收数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 缓冲区大小
         * @param[in] address 来源地址
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        virtual long recvFrom(void* buffer, size_t length, const std::shared_ptr<Address>& address,
                              int flags) const;

        /**
         * @brief 从指定地址接收多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] address 来源地址
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        virtual long recvFrom(iovec* buffer, size_t len, const std::shared_ptr<Address>& address, int flags) const;

        /**
         * @brief 获取远程地址
         */
        std::shared_ptr<Address> getRemoteAddress();

        /**
         * @brief 获取本地地址
         */
        std::shared_ptr<Address> getLocalAddress();

        /**
         * @brief 获取协议族
         */
        int getFamily() const;

        /**
         * @brief 获取套接字类型
         */
        int getType() const;

        /**
         * @brief 获取协议类型
         */
        int getProtocol() const;

        /**
         * @brief 判断套接字是否已连接
         */
        bool isConnected() const;

        /**
         * @brief 判断套接字是否有效
         */
        bool isValid() const;

        /**
         * @brief 获取套接字错误码
         */
        int getError() const;

        /**
         * @brief 转储套接字信息
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        virtual std::ostream& dump(std::ostream& os) const;

        /**
         * @brief 获取套接字信息字符串
         */
        virtual std::string toString() const;

        /**
         * @brief 获取底层套接字描述符
         */
        int getSocket() const;

        /**
         * @brief 取消读操作
         */
        bool cancelRead() const;

        /**
         * @brief 取消写操作
         */
        bool cancelWrite() const;

        /**
         * @brief 取消接受操作
         */
        bool cancelAccept() const;

        /**
         * @brief 取消所有操作
         */
        bool cancelAll() const;

    protected:
        /**
         * @brief 初始化套接字
         */
        void initSock();

        /**
         * @brief 创建新的套接字
         */
        void newSock();

        /**
         * @brief 初始化套接字
         *
         * @param sock 套接字描述符
         * @return 是否成功初始化
         */
        virtual bool init(int sock);

        int m_sock; ///< 套接字描述符
        int m_family; ///< 协议族
        int m_type; ///< 套接字类型
        int m_protocol; ///< 协议类型
        bool m_isConnected; ///< 是否已连接
        std::shared_ptr<Address> m_localAddress = nullptr; ///< 本地地址
        std::shared_ptr<Address> m_remoteAddress = nullptr; ///< 远程地址
    };

    /**
     * @brief SSL/TLS 套接字
     */
    class SSLSocket final : public Socket
    {
    public:
        /**
         * @brief 创建一个 TCP SSL 套接字并绑定到指定地址
         * @param[in] address 地址对象，指定绑定的地址
         * @return 返回创建的 `SSLSocket` 实例
         */
        static std::shared_ptr<SSLSocket> CreateTCP(const std::shared_ptr<Address>& address);

        /**
         * @brief 创建一个 TCP SSL 套接字
         * @return 返回创建的 `SSLSocket` 实例
         */
        static std::shared_ptr<SSLSocket> CreateTCPSocket();

        /**
         * @brief 创建一个 IPv6 的 TCP SSL 套接字
         * @return 返回创建的 `SSLSocket` 实例
         */
        static std::shared_ptr<SSLSocket> CreateTCPSocket6();

        /**
         * @brief 构造函数
         * @param[in] family 协议族（如 `AF_INET`）
         * @param[in] type 套接字类型（如 `SOCK_STREAM`）
         * @param[in] protocol 协议类型（如 `IPPROTO_TCP`）
         */
        SSLSocket(int family, int type, int protocol = 0);

        /**
         * @brief 接受 SSL/TLS 连接
         * @return 返回接受的 `Socket` 实例
         */
        std::shared_ptr<Socket> accept() override;

        /**
         * @brief 绑定 SSL/TLS 套接字到指定地址
         * @param[in] address 地址对象
         * @return 是否成功绑定
         */
        bool bind(const std::shared_ptr<Address>& address) override;

        /**
         * @brief 连接到指定地址并启用 SSL/TLS
         * @param[in] address 目标地址
         * @param[in] timeout_ms 超时时间（毫秒）
         * @return 是否成功连接
         */
        bool connect(const std::shared_ptr<Address>& address, uint64_t timeout_ms) override;

        /**
         * @brief 监听 SSL/TLS 连接请求
         * @param[in] backlog 队列中等待的最大连接数
         * @return 是否成功监听
         */
        bool listen(int backlog) const override;

        /**
         * @brief 发送加密数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 数据长度
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        long send(const void* buffer, size_t length, int flags) const override;

        /**
         * @brief 发送多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        long send(iovec* buffer, size_t len, int flags) const override;

        /**
         * @brief 向指定地址发送加密数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 数据长度
         * @param[in] address 目标地址
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        long sendTo(const void* buffer, size_t length, const std::shared_ptr<Address>& address,
                    int flags) const override;

        /**
         * @brief 向指定地址发送多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] address 目标地址
         * @param[in] flags 发送标志
         * @return 发送的字节数
         */
        long sendTo(iovec* buffer, size_t len, const std::shared_ptr<Address>& address, int flags) const override;

        /**
         * @brief 接收加密数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 缓冲区大小
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        long recv(void* buffer, size_t length, int flags) const override;

        /**
         * @brief 接收多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        long recv(iovec* buffer, size_t len, int flags) const override;

        /**
         * @brief 从指定地址接收加密数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 缓冲区大小
         * @param[in] address 来源地址
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        long recvFrom(void* buffer, size_t length, const std::shared_ptr<Address>& address, int flags) const override;

        /**
         * @brief 从指定地址接收多个数据块
         * @param[in] buffer 数据块
         * @param[in] len 数据块数量
         * @param[in] address 来源地址
         * @param[in] flags 接收标志
         * @return 接收的字节数
         */
        long recvFrom(iovec* buffer, size_t len, const std::shared_ptr<Address>& address, int flags) const override;

        /**
         * @brief 加载 SSL/TLS 证书
         * @param[in] cert_file 证书文件路径
         * @param[in] key_file 私钥文件路径
         * @return 是否加载成功
         */
        bool loadCertificates(const std::string& cert_file, const std::string& key_file);

        /**
         * @brief 转储 SSL/TLS 套接字信息
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& dump(std::ostream& os) const override;

    protected:
        /**
         * @brief 初始化 SSL/TLS 套接字
         * @param sock 套接字描述符
         * @return 是否成功初始化
         */
        bool init(int sock) override;

    private:
        std::shared_ptr<SSL_CTX> m_ctx; ///< OpenSSL SSL 上下文
        std::shared_ptr<SSL> m_ssl; ///< OpenSSL SSL 连接
    };
}

#endif
