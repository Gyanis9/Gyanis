#include "net/http/HttpServer.h"
#include "base/Log.h"
#include "net/http/servlets/ConfigServlet.h"
#include "net/http/servlets/StatusServlet.h"

namespace Gyanis::net::http
{
    static auto g_logger = LOG_NAME("system");

    HttpServer::HttpServer(const bool keepAlive, core::IOManager* worker, core::IOManager* io_worker,
                           core::IOManager* accept_worker): TcpServer(worker, io_worker, accept_worker),
                                                            m_isKeepAlive(keepAlive),
                                                            m_dispatch(std::make_shared<ServletDispatch>())
    {
        m_type = "http";
        m_dispatch->addServlet("/_/status", std::make_shared<StatusServlet>());
        m_dispatch->addServlet("/_/config", std::make_shared<ConfigServlet>());
    }

    void HttpServer::setName(const std::string& value)
    {
        TcpServer::setName(value);
        m_dispatch->setDefaultServlet(std::make_shared<NotFoundServlet>(value));
    }

    void HttpServer::handleClient(const std::shared_ptr<Socket>& client)
    {
        LOG_DEBUG(g_logger)
            << "HttpServer::handleClient - Handling client connection. "
            << "Client details: " << client->toString();
        const auto session = std::make_shared<HttpSession>(client);
        do
        {
            const auto request = session->recvRequest();
            if (!request)
            {
                LOG_DEBUG(g_logger)
                    << "Failed to receive HTTP request. "
                    << "Error code: " << errno
                    << " | Error description: " << strerror(errno)
                    << " | Keep-Alive status: " << m_isKeepAlive;
                break;
            }

            auto response = std::make_shared<HttpResponse>(request->getVersion(), request->isClose() || !m_isKeepAlive);
            response->setHeader("Server", "Gyanis");
            m_dispatch->handle(request, response, session);
            session->sendResponse(response);

            if (!m_isKeepAlive || request->isClose())
            {
                break;
            }
        }
        while (true);
        session->close();
    }
}
