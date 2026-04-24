/**
 * @file TCPServer.h
 * @brief TCP 服务器框架，包含连接管理与协程驱动的客户处理。
 * @details 提供 TcpServer 和 ConnectionManager 类，支持异步接受连接、限制最大连接数、管理连接生命周期。
 *          用户提供客户端处理器（ClientHandler），在协程中处理每个连接。
 */

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Core/Awaitables.hpp"

#include "TcpAcceptor.h"
#include "TcpConnection.h"

#include <mutex>
#include <unordered_set>

namespace Net
{
    /**
     * @brief 连接管理器，线程安全地跟踪当前活动的 TcpConnection 对象。
     */
    class ConnectionManager
    {
    public:
        /**
         * @brief 添加一个连接到管理器。
         * @param conn 连接对象指针（非空）。
         */
        void add(TcpConnection *conn);

        /**
         * @brief 从管理器中移除一个连接。
         * @param conn 要移除的连接指针。
         */
        void remove(TcpConnection *conn);

        /**
         * @brief 关闭所有已管理的连接，并清空集合。
         * @note 调用每个连接的 close() 方法，然后清空内部集合。
         */
        void closeAll();

        /**
         * @brief 获取当前管理的连接数量。
         * @return 连接数。
         */
        [[nodiscard]] size_t count() const;

    private:
        mutable std::mutex m_mutex;                        ///< 保护内部集合的互斥锁
        std::unordered_set<TcpConnection *> m_connections; ///< 活动的连接指针集合
    };

    /**
     * @brief TCP 服务器类，封装监听端口、接受连接、执行用户业务逻辑。
     * @details 内部使用 TcpAcceptor 接受新连接，为每个连接创建独立的协程任务，
     *          任务在指定的 ExecutionContext 上调度，且受最大连接数限制。
     *          用户通过 ClientHandler 回调处理每个连接的生命周期。
     */
    class TcpServer
    {
    public:
        /**
         * @brief 客户端处理器类型：接收 TcpConnection，返回协程任务（通常为 void）。
         */
        using ClientHandler = std::function<Core::Task<void>(TcpConnection)>;

        /**
         * @brief 构造 TCP 服务器。
         * @param ctx           执行上下文（事件循环），所有异步操作在此上调度。
         * @param endpoint      监听端点（IP + 端口）。
         * @param handler       客户端处理函数（协程）。
         * @param maxConnections 最大并发连接数，超出时新连接会被直接关闭。
         * @param connMgr       可选的外部 ConnectionManager；若为空则使用内部默认管理器。
         */
        TcpServer(Core::ExecutionContext &ctx, const IpEndpoint &endpoint,
                  ClientHandler handler, size_t maxConnections = 10000,
                  ConnectionManager *connMgr = nullptr);

        /**
         * @brief 启动服务器，开始接受连接。
         * @details 启动一个协程任务运行 acceptLoop，将此任务提交到执行上下文调度器。
         */
        void start() const;

        /**
         * @brief 停止服务器，关闭监听 socket 并关闭所有活动连接。
         */
        void stop();

    public:
        /**
         * @brief 接受循环协程，持续接受新连接并创建客户端处理任务。
         * @return Core::Task<void> 协程任务，退出时表示服务器停止（如 acceptor 被关闭）。
         */
        Core::Task<void> acceptLoop() const;

        Core::ExecutionContext &m_ctx;      ///< 执行上下文引用
        TcpAcceptor m_acceptor;             ///< TCP 接受器
        ClientHandler m_handler;            ///< 用户客户端处理器
        size_t m_maxConnections;            ///< 最大并发连接数限制
        ConnectionManager m_defaultConnMgr; ///< 默认连接管理器（内部）
        ConnectionManager *m_connMgr;       ///< 连接管理器指针（外部或默认）
    };

    /**
     * @brief 辅助函数，将 TcpServer::acceptLoop 包装为协程任务，并绑定执行上下文。
     * @param server TCP 服务器指针
     * @return 协程任务对象
     */
    inline Core::Task<void> acceptLoopTask(const TcpServer *server)
    {
        auto t = server->acceptLoop();
        co_await std::move(t).withExecutionContext(server->m_ctx);
    }
}

#endif
