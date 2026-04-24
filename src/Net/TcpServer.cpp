#include "TcpServer.h"

namespace Net
{
    inline void ConnectionManager::add(TcpConnection *conn)
    {
        std::lock_guard lock(m_mutex);
        m_connections.insert(conn);
    }

    void ConnectionManager::remove(TcpConnection *conn)
    {
        std::lock_guard lock(m_mutex);
        m_connections.erase(conn);
    }

    void ConnectionManager::closeAll()
    {
        std::lock_guard lock(m_mutex);
        for (auto *c: m_connections)
        {
            c->close();
        }
        m_connections.clear();
    }

    size_t ConnectionManager::count() const
    {
        std::lock_guard lock(m_mutex);
        return m_connections.size();
    }

    TcpServer::TcpServer(Core::ExecutionContext &ctx, const IpEndpoint &endpoint, ClientHandler handler, size_t maxConnections, ConnectionManager *connMgr) : m_ctx(ctx),
        m_acceptor(ctx, endpoint), m_handler(std::move(handler)),
        m_maxConnections(maxConnections),
        m_connMgr(connMgr ? connMgr : &m_defaultConnMgr)
    {
    }

    /**
 * @brief TcpServer::start 的实现（定义在类外）。
 * @details 启动 acceptLoop 协程，并以 fire-and-forget 方式调度。
 */
    inline void TcpServer::start() const
    {
        auto acceptTask = acceptLoopTask(this);
        acceptTask = std::move(acceptTask).withExecutionContext(m_ctx);
        auto handle = acceptTask.fireAndForget();
        m_ctx.scheduler()->schedule([handle]() mutable
        {
            handle.resume();
        }, Core::TaskPriority::High);
    }

    void TcpServer::stop()
    {
        m_acceptor.close();
        m_connMgr->closeAll();
    }

    Core::Task<void> TcpServer::acceptLoop() const
    {
        while (true)
        {
            try
            {
                auto acceptTask = m_acceptor.asyncAccept();
                auto conn = co_await std::move(acceptTask).withExecutionContext(m_ctx);
                if (m_connMgr->count() >= m_maxConnections)
                {
                    conn.close();
                    continue;
                }
                {
                    auto handlerTask = [this](TcpConnection conn) -> Core::Task<void>
                    {
                        m_connMgr->add(&conn);
                        try
                        {
                            co_await m_handler(std::move(conn));
                        } catch (...)
                        {
                        }
                        m_connMgr->remove(&conn);
                    }(std::move(conn));
                    handlerTask = std::move(handlerTask).withExecutionContext(m_ctx);
                    auto handle = handlerTask.fireAndForget();
                    m_ctx.scheduler()->schedule([handle]() mutable
                    {
                        handle.resume();
                    }, Core::TaskPriority::High);
                }
            } catch (const std::exception &e)
            {
                break;
            }
        }
    }
} // Net
