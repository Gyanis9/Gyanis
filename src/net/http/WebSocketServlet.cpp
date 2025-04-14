#include "net/http/WebSocketServlet.h"

namespace Gyanis::net::http
{
    FunctionWSServlet::FunctionWSServlet(callback cb, on_connect_cb connect_cb,
                                         on_close_cb close_cb) : WSServlet("FunctionWSServlet"),
                                                                 m_callback(std::move(cb)),
                                                                 m_onConnect(std::move(connect_cb)),
                                                                 m_onClose(std::move(close_cb))
    {
    }

    int32_t
    FunctionWSServlet::onConnect(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSSession>& session)
    {
        if (m_onConnect)
        {
            return m_onConnect(header, session);
        }
        return 0;
    }

    int32_t
    FunctionWSServlet::onClose(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSSession>& session)
    {
        if (m_onClose)
        {
            return m_onClose(header, session);
        }
        return 0;
    }

    int32_t
    FunctionWSServlet::handle(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSFrameMessage>& msg,
                              const std::shared_ptr<WSSession>& session)
    {
        if (m_callback)
        {
            return m_callback(header, msg, session);
        }
        return 0;
    }

    WSServletDispatch::WSServletDispatch()
    {
        m_name = "WSServletDispatch";
    }

    void WSServletDispatch::addServletSelf(const std::string& uri, const FunctionWSServlet::callback& cb,
                                           const FunctionWSServlet::on_connect_cb& connect_cb,
                                           const FunctionWSServlet::on_close_cb& close_cb)
    {
        addGlobalServlet(uri, std::make_shared<FunctionWSServlet>(cb, connect_cb, close_cb));
    }

    void WSServletDispatch::addGlobServletSelf(const std::string& uri, const FunctionWSServlet::callback& cb,
                                               const FunctionWSServlet::on_connect_cb& connect_cb,
                                               const FunctionWSServlet::on_close_cb& close_cb)
    {
        addGlobalServlet(uri, std::make_shared<FunctionWSServlet>(cb, connect_cb, close_cb));
    }

    std::shared_ptr<WSServlet> WSServletDispatch::getWSServlet(const std::string& uri) const
    {
        const auto servlet = getMatchServlet(uri);
        return std::dynamic_pointer_cast<WSServlet>(servlet);
    }
}
