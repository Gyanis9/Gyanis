/**
 * @file HttpServer.h
 * @brief HTTP 服务器，基于 TcpServer 和 HTTP 协议解析/序列化，集成路由与中间件。
 * @details 为每个 TCP 连接创建独立会话，解析 HTTP 请求，通过 Router 匹配处理器，
 *          序列化响应并发送，支持 Keep-Alive 连接管理。
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "Router.h"
#include "TcpServer.h"

#include <memory>

namespace Net::Http
{
    /**
     * @brief HTTP 服务器类。
     * @details 内部使用 TcpServer 作为底层 TCP 服务，为每个连接启动协程会话。
     *          每个会话循环执行：读取数据、解析请求、路由分发、发送响应。
     *          支持 HTTP/1.0 和 HTTP/1.1 Keep-Alive 语义。
     */
    class HttpServer
    {
    public:
        /**
         * @brief 构造 HTTP 服务器。
         * @param ctx      执行上下文（事件循环）。
         * @param endpoint 监听端点（IP + 端口）。
         * @param router   路由表（共享指针，可多个服务器共用）。
         * @param maxConns 最大并发连接数，超出时新连接被关闭。
         */
        HttpServer(Core::ExecutionContext &ctx, const IpEndpoint &endpoint,
                   std::shared_ptr<Router> router, size_t maxConns = 10000);

        /**
         * @brief 启动服务器，开始接受连接。
         */
        void start() const;

        /**
         * @brief 停止服务器，关闭监听 socket 及所有活动连接。
         */
        void stop() const;

    private:
        /**
         * @brief 处理单个客户端连接的会话协程。
         * @param conn 已建立的 TCP 连接。
         * @return Core::Task<void>
         * @details 循环执行：
         *          1. 异步读取数据到内部缓冲区；
         *          2. 尝试解析一个或多个请求（while 循环）；
         *          3. 对每个请求，通过 Router 匹配处理器并调用；
         *          4. 序列化响应并通过连接发送；
         *          5. 根据 HTTP 版本和 Connection 头判断是否保持连接。
         */
        Core::Task<void> handleSession(TcpConnection conn) const;

        Core::ExecutionContext &m_ctx;          ///< 执行上下文引用
        std::shared_ptr<Router> m_router;       ///< 路由表（共享）
        std::unique_ptr<TcpServer> m_tcpServer; ///< TCP 服务器实例
    };
}

#endif
