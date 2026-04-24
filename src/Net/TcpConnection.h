/**
 * @file TcpConnection.h
 * @brief TCP 连接封装，支持异步读写、缓冲区和分隔符读取。
 * @details 内部维护输入缓冲区（Buffer），提供基于协程的异步 I/O。
 *          支持取消操作，自动管理 socket 生命周期。
 */

#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "Buffer.h"
#include "Socket.h"
#include "Core/Awaitables.hpp"
#include "Core/Cancellation.hpp"
#include "Core/ExecutionContext.hpp"

#include <optional>

namespace Net
{
    /**
     * @brief TCP 连接类，表示一个已建立的 TCP 连接。
     */
    class TcpConnection
    {
    public:
        /**
         * @brief 默认构造函数，构造一个空连接（无效）。
         */
        TcpConnection() = default;

        /**
         * @brief 从 Socket 对象构造，接管所有权。
         * @param sock 有效的 Socket 对象（通常由 TcpAcceptor 或 connect 产生）。
         */
        explicit TcpConnection(Socket &&sock) noexcept;

        /**
         * @brief 析构函数，自动关闭连接。
         */
        ~TcpConnection();

        // 支持移动语义
        TcpConnection(TcpConnection &&) noexcept = default;

        TcpConnection &operator=(TcpConnection &&) noexcept = default;

        /**
         * @brief 主动关闭连接，释放 socket 资源。
         */
        void close() noexcept;

        /**
         * @brief 获取原生 socket 句柄。
         * @return socket_t 原生句柄。
         */
        [[nodiscard]] Core::socket_t nativeHandle() const noexcept;

        /**
         * @brief 判断连接是否有效。
         * @return true 表示有效，false 表示已关闭或未初始化。
         */
        [[nodiscard]] bool isValid() const noexcept;

        /**
         * @brief 异步从 socket 读取数据到内部输入缓冲区。
         * @param ctx 执行上下文（用于调度，但此实现中未直接使用，保持接口一致）。
         * @param cancel 取消令牌，可在等待期间提前中止操作。
         * @return Core::Task<size_t> 恢复时返回本次实际读取的字节数（可能小于请求的 BLOCK_SIZE），0 表示对端关闭连接。
         */
        Core::Task<size_t> asyncReadSome(Core::ExecutionContext &ctx, Core::CancellationToken cancel = {});

        /**
         * @brief 异步读取指定长度的数据到用户缓冲区。
         * @param ctx 执行上下文。
         * @param dest 用户缓冲区指针。
         * @param len 需要读取的字节数。
         * @param cancel 取消令牌。
         * @return Core::Task<size_t> 实际读取的字节数（可能小于 len，若遇到 EOF）。
         */
        Core::Task<size_t> asyncRead(Core::ExecutionContext &ctx, void *dest, size_t len, Core::CancellationToken cancel = {});

        /**
         * @brief 异步读取数据直到遇到指定分隔符（包含分隔符）。
         * @param ctx 执行上下文。
         * @param delimiter 分隔符字符串。
         * @param cancel 取消令牌。
         * @return Core::Task<std::string> 包含分隔符在内的字符串；若连接关闭而未遇到分隔符，则返回已读取的部分（或空字符串）。
         */
        Core::Task<std::string> asyncReadUntil(Core::ExecutionContext &ctx, std::string_view delimiter, Core::CancellationToken cancel = {});

        /**
         * @brief 异步发送数据。
         * @param ctx 执行上下文。
         * @param data 数据缓冲区指针。
         * @param len 要发送的字节数。
         * @param cancel 取消令牌。
         * @return Core::Task<size_t> 实际发送的字节数（通常等于 len，除非连接中断）。
         */
        Core::Task<size_t> asyncWrite(Core::ExecutionContext &ctx, const void *data, size_t len, Core::CancellationToken cancel = {}) const;

        /**
         * @brief 异步发送输出缓冲区的全部数据。
         * @param ctx 执行上下文。
         * @param output 输出缓冲区（内容会被消费，调用后其 readableSize 变为 0）。
         * @param cancel 取消令牌。
         * @return Core::Task<void> 完成后缓冲区清空。
         */
        Core::Task<void> asyncWriteBuffer(Core::ExecutionContext &ctx, Buffer &output, Core::CancellationToken cancel = {}) const;

        /**
         * @brief 获取连接的本地端点。
         * @return 成功返回 IpEndpoint，失败返回 std::nullopt。
         */
        [[nodiscard]] std::optional<IpEndpoint> localEndpoint() const noexcept;

        /**
         * @brief 获取连接的远端端点。
         * @return 成功返回 IpEndpoint，失败返回 std::nullopt。
         */
        [[nodiscard]] std::optional<IpEndpoint> remoteEndpoint() const noexcept;

        /**
         * @brief 获取输入缓冲区引用（允许外部直接操作）。
         * @return Buffer& 内部输入缓冲区，可用于预填充、检查或手动管理。
         */
        Buffer &inputBuffer() noexcept;

    private:
        Socket m_socket;  ///< 底层 socket 对象
        Buffer m_readBuf; ///< 输入缓冲区，存储从网络读取但尚未被应用层消费的数据
    };
}

#endif
