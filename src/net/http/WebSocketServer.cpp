#include "net/http/WebSocketServer.h"

namespace Gyanis::net::http
{
    static auto g_logger = LOG_NAME("system");

    WSServer::WSServer(core::IOManager* worker, core::IOManager* io_worker, core::IOManager* accept_worker) : TcpServer(
        worker, io_worker, accept_worker)
    {
        m_dispatch = std::make_shared<WSServletDispatch>();
        m_type = "ws";
    }

    std::shared_ptr<WSServletDispatch> WSServer::getWSServletDispatch() const
    {
        return m_dispatch;
    }

    void WSServer::setWSServletDispatch(const std::shared_ptr<WSServletDispatch>& value)
    {
        m_dispatch = value;
    }

    void WSServer::handleClient(const std::shared_ptr<Socket>& client)
    {
        LOG_DEBUG(g_logger)
            << "Handling client connection. "
            << "Client details: " << client->toString();
        const auto session = std::make_shared<WSSession>(client);
        do
        {
            const auto header = session->handleShake();
            if (!header)
            {
                LOG_DEBUG(g_logger)
                    << "WSServer::handleClient() failed to handle handshake. "
                    << "An error occurred while processing the WebSocket handshake.";
                break;
            }
            const auto servlet = m_dispatch->getWSServlet(header->getPath());
            if (!servlet)
            {
                LOG_DEBUG(g_logger)
                    << "No matching WSServlet found. "
                    << "The requested servlet could not be matched to a valid handler.";
                break;
            }
            int result = servlet->onConnect(header, session);
            if (result)
            {
                LOG_DEBUG(g_logger)
                    << "onConnect() returned. "
                    << "Return value: " << result;
                break;
            }
            while (true)
            {
                auto msg = session->recvMessage();
                if (!msg)
                {
                    break;
                }
                result = servlet->handle(header, msg, session);
                if (result)
                {
                    LOG_DEBUG(g_logger)
                        << "Handle method returned. "
                        << "Result: " << result;
                    break;
                }
            }
            servlet->onClose(header, session);
        }
        while (false);
        session->close();
    }
}
