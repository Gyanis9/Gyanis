/**
 * @file WebSocketServer.h
 * @brief WSServer模块封装
 * @date 2025-04-03
 */

#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include "net/web/TcpServer.h"
#include "net/http/WebSocketServlet.h"

namespace Gyanis::net::http
{
    /**
     * @brief WebSocket 服务器类，继承自 `TcpServer`，用于管理 WebSocket 协议的连接和消息处理
     */
    class WSServer : public web::TcpServer
    {
    public:
        /**
         * @brief 构造函数，初始化 WebSocket 服务器
         * @param worker 工作线程 IO 管理器，默认为 `core::IOManager::GetThis()`
         * @param io_worker IO 工作线程 IO 管理器，默认为 `core::IOManager::GetThis()`
         * @param accept_worker 接收线程 IO 管理器，默认为 `core::IOManager::GetThis()`
         */
        explicit WSServer(core::IOManager* worker = core::IOManager::GetThis(),
                          core::IOManager* io_worker = core::IOManager::GetThis(),
                          core::IOManager* accept_worker = core::IOManager::GetThis());

        /**
         * @brief 获取 WebSocket 服务调度器
         * @return 返回一个 `WSServletDispatch` 对象的共享指针
         */
        std::shared_ptr<WSServletDispatch> getWSServletDispatch() const;

        /**
         * @brief 设置 WebSocket 服务调度器
         * @param value 设置的 `WSServletDispatch` 对象的共享指针
         */
        void setWSServletDispatch(const std::shared_ptr<WSServletDispatch>& value);

    protected:
        /**
         * @brief 处理 WebSocket 客户端连接
         * @param client 客户端连接的套接字对象
         */
        void handleClient(const std::shared_ptr<Socket>& client) override;

        std::shared_ptr<WSServletDispatch> m_dispatch; ///< WebSocket 服务调度器，用于管理 WebSocket 服务和请求
    };
}

#endif
