/**
 * @file TcpAcceptor.h
 * @brief TCP 异步连接接收器，封装监听 socket 并提供协程风格 accept。
 * @details 结合 ExecutionContext 和事件循环，实现非阻塞 accept 操作。
 *          创建时自动绑定并监听指定端点，支持取消令牌。
 */

#ifndef TCPACCEPTOR_H
#define TCPACCEPTOR_H

#include "TcpConnection.h"
#include "Core/Awaitables.hpp"

namespace Net
{
    /**
     * @brief TCP 异步接受器类。
     * @details 管理一个监听 socket，能够以协程方式异步接受客户端连接。
     *          使用 ExecutionContext 驱动事件循环，支持外部取消。
     */
    class TcpAcceptor
    {
    public:
        /**
         * @brief 构造一个 TcpAcceptor 并开始监听。
         * @param ctx 关联的执行上下文（事件循环）。
         * @param endpoint 本地监听端点（IP + 端口）。
         * @param reusePort 是否开启 SO_REUSEPORT（当前未使用，预留）。
         * @throw std::system_error 如果 socket 创建、绑定或监听失败。
         */
        TcpAcceptor(Core::ExecutionContext &ctx, const IpEndpoint &endpoint, bool reusePort = false);

        /**
         * @brief 获取原生监听 socket 句柄。
         * @return socket_t 原生句柄。
         */
        [[nodiscard]] Core::socket_t nativeHandle() const noexcept;

        /**
         * @brief 判断监听 socket 是否有效。
         * @return true 有效，false 无效。
         */
        [[nodiscard]] bool isValid() const noexcept;

        /**
         * @brief 立即尝试接受一个连接（非阻塞）。
         * @return 如果没有就绪连接则返回 std::nullopt，否则返回已连接的 Socket 对象。
         */
        [[nodiscard]] std::optional<Socket> nativeAccept() const noexcept;

        /**
         * @brief 协程方式异步接受一个客户端连接。
         * @param cancel 可选的取消令牌，用于提前终止等待。
         * @return Core::Task<TcpConnection> 协程任务，恢复时产生已连接的 TcpConnection。
         */
        Core::Task<TcpConnection> asyncAccept(Core::CancellationToken cancel = {}) const;

        /**
         * @brief 关闭监听 socket，释放资源。
         */
        void close() noexcept;

    private:
        Core::ExecutionContext &m_ctx; ///< 关联的执行上下文（用于调度，当前未直接使用但保留）
        Socket m_listenSocket;         ///< 监听 socket 对象
    };
}

#endif // TCPACCEPTOR_H
