/**
* @file HttpServer.h
 * @brief HTTP 服务器封装
 * @date 2025-03-18
 */
#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include "net/web/TcpServer.h"
#include "servlets/Servlet.h"

namespace Gyanis::net::http
{
    /**
     * @brief HTTP 服务器类
     */
    class HttpServer : public web::TcpServer
    {
    public:
        /**
         * @brief 构造 HTTP 服务器对象
         * @param keepAlive 是否启用 HTTP 长连接（默认为 `false`）
         * @param worker 工作线程池（默认为 `core::IOManager::GetThis()`）
         * @param io_worker IO 操作的线程池（默认为 `core::IOManager::GetThis()`）
         * @param accept_worker 接收连接的线程池（默认为 `core::IOManager::GetThis()`）
         */
        explicit HttpServer(bool keepAlive = false, core::IOManager* worker = core::IOManager::GetThis(),
                            core::IOManager* io_worker = core::IOManager::GetThis(),
                            core::IOManager* accept_worker = core::IOManager::GetThis());

        /**
         * @brief 获取 Servlet 调度器
         */
        std::shared_ptr<ServletDispatch> getServletDispatch() const { return m_dispatch; }

        /**
         * @brief 设置 Servlet 调度器
         */
        void setServletDispatch(const std::shared_ptr<ServletDispatch>& dispatch) { m_dispatch = dispatch; }

        /**
         * @brief 设置服务器名称
         */
        void setName(const std::string& value) override;

    protected:
        /**
         * @brief 处理客户端连接
         * @param client 客户端套接字对象，表示与客户端的连接
         */
        void handleClient(const std::shared_ptr<Socket>& client) override;

    private:
        bool m_isKeepAlive; ///< 是否启用 HTTP 长连接（默认为 `false`）
        std::shared_ptr<ServletDispatch> m_dispatch; ///< 存储 HTTP 路由调度器，用于请求路由
    };
}

#endif
