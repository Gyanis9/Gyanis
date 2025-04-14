#include "net/http/servlets/Servlet.h"

#include <mutex>
#include <utility>
#include <fnmatch.h>

namespace Gyanis::net::http
{
    Servlet::Servlet(std::string name): m_name(std::move(name))
    {
    }

    const std::string& Servlet::getName() const
    {
        return m_name;
    }

    FunctionServlet::FunctionServlet(callback cb): Servlet("FunctionServlet"), m_cb(std::move(cb))
    {
    }

    int32_t FunctionServlet::handle(const std::shared_ptr<HttpRequest>& request,
                                    const std::shared_ptr<HttpResponse>& response,
                                    const std::shared_ptr<HttpSession>& session)
    {
        return m_cb(request, response, session);
    }

    HoldServletCreator::HoldServletCreator(const std::shared_ptr<Servlet>& servlet): m_servlet(servlet)
    {
    }

    std::shared_ptr<Servlet> HoldServletCreator::get() const
    {
        return m_servlet;
    }

    std::string HoldServletCreator::getName() const
    {
        return m_servlet->getName();
    }

    ServletDispatch::ServletDispatch(): Servlet("ServletDispatch"),
                                        m_default(std::make_shared<NotFoundServlet>("Gyanis/1.0.0"))
    {
    }

    int32_t ServletDispatch::handle(const std::shared_ptr<HttpRequest>& request,
                                    const std::shared_ptr<HttpResponse>& response,
                                    const std::shared_ptr<HttpSession>& session)
    {
        if (const auto servlet = getMatchServlet(request->getPath()))
        {
            servlet->handle(request, response, session);
        }
        return 0;
    }

    void ServletDispatch::addServlet(const std::string& uri, const std::shared_ptr<Servlet>& servlet)
    {
        std::unique_lock lock(m_mutex);
        m_servlets[uri] = std::make_shared<HoldServletCreator>(servlet);
    }

    void ServletDispatch::addServlet(const std::string& uri, const FunctionServlet::callback& cb)
    {
        std::unique_lock lock(m_mutex);
        m_servlets[uri] = std::make_shared<HoldServletCreator>(std::make_shared<FunctionServlet>(cb));
    }

    void ServletDispatch::addGlobalServlet(const std::string& uri, const std::shared_ptr<Servlet>& servlet)
    {
        std::unique_lock lock(m_mutex);
        for (auto it = m_dispatchers.begin(); it != m_dispatchers.end(); ++it)
        {
            if (it->first == uri)
            {
                m_dispatchers.erase(it);
                break;
            }
        }
        m_dispatchers.emplace_back(uri, std::make_shared<HoldServletCreator>(servlet));
    }

    void ServletDispatch::addGlobalServlet(const std::string& uri, const FunctionServlet::callback& cb)
    {
        return addGlobalServlet(uri, std::make_shared<FunctionServlet>(cb));
    }

    void ServletDispatch::addServletCreator(const std::string& uri, const std::shared_ptr<IServletCreator>& servlet)
    {
        std::unique_lock lock(m_mutex);
        m_servlets[uri] = servlet;
    }

    void ServletDispatch::addGlobalServletCreator(const std::string& uri,
                                                  const std::shared_ptr<IServletCreator>& servlet)
    {
        std::unique_lock lock(m_mutex);
        for (auto it = m_dispatchers.begin(); it != m_dispatchers.end(); ++it)
        {
            if (it->first == uri)
            {
                m_dispatchers.erase(it);
                break;
            }
        }
        m_dispatchers.emplace_back(uri, servlet);
    }

    void ServletDispatch::delServlet(const std::string& uri)
    {
        std::unique_lock lock(m_mutex);
        m_servlets.erase(uri);
    }

    void ServletDispatch::delGlobalServlet(const std::string& uri)
    {
        std::unique_lock lock(m_mutex);
        for (auto it = m_dispatchers.begin(); it != m_dispatchers.end(); ++it)
        {
            if (it->first == uri)
            {
                m_dispatchers.erase(it);
                break;
            }
        }
    }

    std::shared_ptr<Servlet> ServletDispatch::getDefaultServlet() const
    {
        return m_default;
    }

    void ServletDispatch::setDefaultServlet(const std::shared_ptr<Servlet>& servlet)
    {
        m_default = servlet;
    }

    std::shared_ptr<Servlet> ServletDispatch::getServlet(const std::string& uri) const
    {
        std::shared_lock lock(m_mutex);
        const auto it = m_servlets.find(uri);
        return it == m_servlets.end() ? nullptr : it->second->get();
    }

    std::shared_ptr<Servlet> ServletDispatch::getGlobalServlet(const std::string& uri) const
    {
        std::unique_lock lock(m_mutex);
        for (const auto& [fst, snd] : m_dispatchers)
        {
            if (fst == uri)
            {
                return snd->get();
            }
        }
        return nullptr;
    }

    std::shared_ptr<Servlet> ServletDispatch::getMatchServlet(const std::string& uri) const
    {
        std::shared_lock lock(m_mutex);
        if (const auto it = m_servlets.find(uri); it != m_servlets.end())
        {
            return it->second->get();
        }
        for (const auto& [fst, snd] : m_dispatchers)
        {
            if (!fnmatch(fst.c_str(), uri.c_str(), 0))
            {
                return snd->get();
            }
        }
        return m_default;
    }

    void ServletDispatch::listAllServletCreator(
        std::unordered_map<std::string, std::shared_ptr<IServletCreator>>& servlets) const
    {
        std::shared_lock lock(m_mutex);
        for (const auto& [fst, snd] : m_servlets)
        {
            servlets[fst] = snd;
        }
    }

    void ServletDispatch::listGlobalServletCreator(
        std::unordered_map<std::string, std::shared_ptr<IServletCreator>>& servlets)
    {
        std::shared_lock lock(m_mutex);
        for (auto& [fst, snd] : m_dispatchers)
        {
            servlets[fst] = snd;
        }
    }

    NotFoundServlet::NotFoundServlet(std::string name): Servlet("NotFoundServlet"), m_name(std::move(name))
    {
        m_content = "<html><head><title>404 Not Found</title></head>"
            "<body><center><h1>404 Not Found</h1></center>"
            "<hr><center>Gyanis/1.0</center></body></html>";
    }

    int32_t NotFoundServlet::handle(const std::shared_ptr<HttpRequest>& request,
                                    const std::shared_ptr<HttpResponse>& response,
                                    const std::shared_ptr<HttpSession>& session)
    {
        response->setStatus(HttpStatus::NOT_FOUND);
        response->setHeader("Server", "Gyanis/1.0.0");
        response->setHeader("Content-Type", "text/html");
        response->setBody(m_content);
        return 0;
    }
}
