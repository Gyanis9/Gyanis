/**
 * @file SocketStream.h
 * @brief 套接字流模块封装
 * @date 2025-03-17
 */
#ifndef SOCKETSTREAM_H
#define SOCKETSTREAM_H
#include "net/stream/Stream.h"
#include "net/Socket.h"

namespace Gyanis::net::stream
{
    /**
     * @brief 套接字流类
     */
    class SocketStream : public Stream
    {
    public:
        /**
         * @brief 构造函数
         * @param[in] socket 套接字对象
         * @param[in] owner 是否拥有该套接字的所有权，默认为 `true`
         * @note 如果 `owner` 为 `true`，则 `SocketStream` 会在析构时关闭该套接字
         */
        explicit SocketStream(const std::shared_ptr<Socket>& socket, bool owner = true);

        /**
         * @brief 析构函数
         */
        ~SocketStream() override;

        /**
         * @brief 从套接字流中读取数据
         * @param[out] buffer 用于存储读取数据的缓冲区
         * @param[in] length 要读取的字节数
         * @return 返回实际读取的字节数，失败时返回负值
         */
        long read(void* buffer, size_t length) override;

        /**
         * @brief 从套接字流中读取数据到 ByteArray
         * @param[out] buffer `ByteArray` 对象，用于存储读取的数据
         * @param[in] length 要读取的字节数
         * @return 返回实际读取的字节数，失败时返回负值
         */
        long read(const std::shared_ptr<ByteArray>& buffer, size_t length) override;

        /**
         * @brief 向套接字流中写入数据
         * @param[in] buffer 数据缓冲区
         * @param[in] length 要写入的字节数
         * @return 返回实际写入的字节数，失败时返回负值
         */
        long write(const void* buffer, size_t length) override;

        /**
         * @brief 向套接字流中写入数据来自 ByteArray
         * @param[in] buffer `ByteArray` 对象，包含要写入的数据
         * @param[in] length 要写入的字节数
         * @return 返回实际写入的字节数，失败时返回负值
         */
        long write(const std::shared_ptr<ByteArray>& buffer, size_t length) override;

        /**
         * @brief 关闭套接字流
         */

        void close() override;

        /**
         * @brief 获取底层套接字
         */
        [[nodiscard]] std::shared_ptr<Socket> getSocket() const;

        /**
         * @brief 判断套接字是否已连接
         */
        [[nodiscard]] bool isConnected() const;

        /**
         * @brief 获取远程地址
         */
        [[nodiscard]] std::shared_ptr<Address> getRemoteAddress() const;

        /**
         * @brief 获取本地地址
         */
        [[nodiscard]] std::shared_ptr<Address> getLocalAddress() const;

        /**
         * @brief 获取远程地址的字符串表示
         */
        [[nodiscard]] std::string getRemoteAddressString() const;

        /**
         * @brief 获取本地地址的字符串表示
         */
        [[nodiscard]] std::string getLocalAddressString() const;

    protected:
        std::shared_ptr<Socket> m_socket = nullptr; ///< 底层套接字对象
        bool m_owner; ///< 是否拥有套接字的所有权，决定是否在析构时关闭套接字
    };
}

#endif
