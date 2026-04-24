/**
 * @file TLS.h
 * @brief TLS/SSL 加密传输封装，基于 OpenSSL，支持异步握手和读写。
 */

#ifndef TLS_H
#define TLS_H

#include "TcpConnection.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace Net
{
    /**
     * @brief TLS 上下文管理类，管理 SSL_CTX 对象。
     * @details 负责初始化 OpenSSL 库，创建 SSL_CTX，支持服务器或客户端模式。
     *          上下文可被多个 TlsConnection 共享。
     */
    class TlsContext
    {
    public:
        /**
         * @brief 构造 TLS 上下文。
         * @param serverMode true 表示服务器模式，false 表示客户端模式。
         * @throw std::runtime_error 如果 SSL_CTX 创建失败。
         */
        explicit TlsContext(bool serverMode);

        /**
         * @brief 析构函数，释放 SSL_CTX。
         */
        ~TlsContext();

        /**
         * @brief 获取原生 SSL_CTX 句柄。
         * @return SSL_CTX* 指针。
         */
        SSL_CTX *nativeHandle() const noexcept;

    private:
        SSL_CTX *m_ctx; ///< OpenSSL SSL_CTX 对象
    };

    /**
     * @brief TLS 连接类，对 TcpConnection 进行加密封装。
     * @details 使用内存 BIO 将 OpenSSL 的 I/O 与底层 TCP 异步 I/O 解耦，
     *          提供协程风格的异步握手、读写操作。
     */
    class TlsConnection
    {
    public:
        /**
         * @brief 构造 TLS 连接。
         * @param tcpConn 已建立的 TCP 连接（将被移动）。
         * @param ctx     TLS 上下文（共享）。
         * @param serverMode true 表示服务器端（接受连接），false 表示客户端（发起连接）。
         */
        TlsConnection(TcpConnection &&tcpConn, const TlsContext &ctx, const bool serverMode = true);

        /**
         * @brief 析构函数，释放 SSL 对象。
         */
        ~TlsConnection();

        /**
         * @brief 异步执行 TLS 握手。
         * @param ctx 执行上下文（用于异步 I/O）。
         * @param cancel 取消令牌。
         * @return Core::Task<void> 握手完成时恢复。
         * @throw std::runtime_error 如果握手失败。
         */
        Core::Task<void> asyncHandshake(Core::ExecutionContext &ctx, Core::CancellationToken cancel = {});

        /**
         * @brief 异步读取解密后的数据。
         * @param ctx 执行上下文。
         * @param buf 用户缓冲区。
         * @param len 缓冲区大小。
         * @param cancel 取消令牌。
         * @return 实际读取的字节数，0 表示连接关闭。
         */
        Core::Task<size_t> asyncRead(Core::ExecutionContext &ctx, void *buf, size_t len, Core::CancellationToken cancel = {});

        /**
         * @brief 异步写入数据（加密后发送）。
         * @param ctx 执行上下文。
         * @param buf 用户数据缓冲区。
         * @param len 数据长度。
         * @param cancel 取消令牌。
         * @return 实际写入的字节数（通常等于 len）。
         * @throw std::runtime_error 写入错误。
         */
        Core::Task<size_t> asyncWrite(Core::ExecutionContext &ctx, const void *buf, size_t len, Core::CancellationToken cancel = {});

    private:
        /**
         * @brief 从底层 TCP 读取加密数据并喂给 SSL 读 BIO。
         * @param ctx 执行上下文。
         * @param cancel 取消令牌。
         */
        Core::Task<void> pumpRead(Core::ExecutionContext &ctx, Core::CancellationToken cancel);

        /**
         * @brief 将 SSL 写 BIO 中的加密数据发送到 TCP。
         * @param ctx 执行上下文。
         * @param cancel 取消令牌。
         */
        Core::Task<void> flushWrite(Core::ExecutionContext &ctx, Core::CancellationToken cancel) const;

        TcpConnection m_tcp; ///< 底层 TCP 连接
        SSL *m_ssl;          ///< OpenSSL SSL 对象
        BIO *m_rbio;         ///< 读内存 BIO（输入加密数据）
        BIO *m_wbio;         ///< 写内存 BIO（输出加密数据）
    };
}

#endif
