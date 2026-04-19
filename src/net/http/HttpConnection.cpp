#include "net/http/HttpConnection.h"
#include "base/Log.h"
#include "../stream/ZlibStream.h"
#include <functional>
#include <utility>

namespace Gyanis::net::http
{
    static auto g_logger = LOG_NAME("system");

    HttpResult::HttpResult(const int result, const std::shared_ptr<HttpResponse>& response, std::string error):
        result(result),
        response(response), error(std::move(error))
    {
    }

    std::string HttpResult::toString() const
    {
        std::stringstream ss;
        ss << "[HttpResult Information: "
            << "Result: " << result
            << " | Error: " << error
            << " | Response: " << (response ? response->toString() : "nullptr")
            << "]";
        return ss.str();
    }

    HttpConnection::HttpConnection(const std::shared_ptr<Socket>& sock, const bool owner): SocketStream(sock, owner)
    {
    }

    HttpConnection::~HttpConnection()
    {
        LOG_DEBUG(g_logger)
            << "HttpConnection destructor called. "
            << "The HttpConnection object is being destroyed.";
    }

    std::shared_ptr<HttpResponse> HttpConnection::recvResponse()
    {
        const auto parser = std::make_shared<HttpResponseParser>();
        const uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
        std::vector<char> buffer(buff_size);
        char* data = buffer.data();
        int offset = 0;
        do
        {
            auto len = read(data + offset, buff_size - offset);
            if (len <= 0)
            {
                close();
                return nullptr;
            }
            len += offset;
            data[len] = '\0';
            const size_t nparse = parser->execute(data, len, false);
            if (parser->hasError())
            {
                close();
                return nullptr;
            }
            offset = len - nparse;
            if (offset == static_cast<int>(buff_size))
            {
                close();
                return nullptr;
            }
            if (parser->isFinished())
            {
                break;
            }
        }
        while (true);
        auto& client_parser = parser->getParser();
        std::string body;
        if (client_parser.chunked)
        {
            int len = offset;
            do
            {
                bool begin = true;
                do
                {
                    if (!begin || len == 0)
                    {
                        const auto rt = read(data + len, buff_size - len);
                        if (rt <= 0)
                        {
                            close();
                            return nullptr;
                        }
                        len += rt;
                    }
                    data[len] = '\0';
                    const size_t nparse = parser->execute(data, len, true);
                    if (parser->hasError())
                    {
                        close();
                        return nullptr;
                    }
                    len -= nparse;
                    if (len == static_cast<int>(buff_size))
                    {
                        close();
                        return nullptr;
                    }
                    begin = false;
                }
                while (!parser->isFinished());

                if (client_parser.content_len + 2 <= len)
                {
                    body.append(data, client_parser.content_len);
                    memmove(data, data + client_parser.content_len + 2
                            , len - client_parser.content_len - 2);
                    len -= client_parser.content_len + 2;
                }
                else
                {
                    body.append(data, len);
                    int left = client_parser.content_len - len + 2;
                    while (left > 0)
                    {
                        const auto rt = read(data, left > static_cast<int>(buff_size)
                                                       ? static_cast<int>(buff_size)
                                                       : left);
                        if (rt <= 0)
                        {
                            close();
                            return nullptr;
                        }
                        body.append(data, rt);
                        left -= rt;
                    }
                    body.resize(body.size() - 2);
                    len = 0;
                }
            }
            while (!client_parser.chunks_done);
        }
        else
        {
            if (auto length = parser->getContentLength(); length > 0)
            {
                body.resize(length);

                uint64_t len = 0;
                if (length >= static_cast<size_t>(offset))
                {
                    memcpy(&body[0], data, offset);
                    len = offset;
                }
                else
                {
                    memcpy(&body[0], data, length);
                    len = length;
                }
                length -= offset;
                if (length > 0)
                {
                    if (readFixSize(&body[len], length) <= 0)
                    {
                        close();
                        return nullptr;
                    }
                }
            }
        }
        if (!body.empty())
        {
            const auto content_encoding = parser->getData()->getHeader("content-encoding");
            LOG_DEBUG(g_logger)
                << "Content encoding: " << content_encoding
                << " | Body size: " << body.size();
            if (strcasecmp(content_encoding.c_str(), "gzip") == 0)
            {
                const auto zs = stream::ZlibStream::CreateGzip(false);
                zs->write(body.c_str(), body.size());
                zs->flush();
                zs->getResult().swap(body);
            }
            else if (strcasecmp(content_encoding.c_str(), "deflate") == 0)
            {
                const auto zs = stream::ZlibStream::CreateDeflate(false);
                zs->write(body.c_str(), body.size());
                zs->flush();
                zs->getResult().swap(body);
            }
            parser->getData()->setBody(body);
        }
        return parser->getData();
    }

    long HttpConnection::sendRequest(const std::shared_ptr<HttpRequest>& request)
    {
        std::stringstream ss;
        ss << *request;
        const std::string data = ss.str();
        return writeFixSize(data.c_str(), data.size());
    }

    std::shared_ptr<HttpResult> HttpConnection::DoGet(const std::string& url, const uint64_t timeout_ms,
                                                      const std::unordered_map<std::string, std::string>& headers,
                                                      const std::string& body)
    {
        const auto uri = Uri::Create(url);
        if (!uri)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::INVALID_URL)
                                                , nullptr, "invalid url: " + url);
        }
        return DoGet(uri, timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnection::DoGet(const std::shared_ptr<Uri>& uri, const uint64_t timeout_ms,
                                                      const std::unordered_map<std::string, std::string>& headers,
                                                      const std::string& body)
    {
        return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnection::DoPost(const std::string& url, const uint64_t timeout_ms,
                                                       const std::unordered_map<std::string, std::string>& headers,
                                                       const std::string& body)
    {
        const auto uri = Uri::Create(url);
        if (!uri)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::INVALID_URL)
                                                , nullptr, "invalid url: " + url);
        }
        return DoPost(uri, timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnection::DoPost(const std::shared_ptr<Uri>& uri, const uint64_t timeout_ms,
                                                       const std::unordered_map<std::string, std::string>& headers,
                                                       const std::string& body)
    {
        return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnection::DoRequest(const HttpMethod method, const std::string& url,
                                                          const uint64_t timeout_ms,
                                                          const std::unordered_map<std::string, std::string>& headers,
                                                          const std::string& body)
    {
        const auto uri = Uri::Create(url);
        if (!uri)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::INVALID_URL)
                                                , nullptr, "invalid url: " + url);
        }
        return DoRequest(method, uri, timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnection::DoRequest(const HttpMethod method, const std::shared_ptr<Uri>& uri,
                                                          const uint64_t timeout_ms,
                                                          const std::unordered_map<std::string, std::string>& headers,
                                                          const std::string& body)
    {
        const auto req = std::make_shared<HttpRequest>();
        req->setPath(uri->getPath());
        req->setQuery(uri->getQuery());
        req->setFragment(uri->getFragment());
        req->setMethod(method);
        bool has_host = false;
        for (const auto& [fst, snd] : headers)
        {
            if (strcasecmp(fst.c_str(), "Connection") == 0)
            {
                if (strcasecmp(snd.c_str(), "Keep-Alive") == 0)
                {
                    req->setClose(false);
                }
                continue;
            }

            if (!has_host && strcasecmp(fst.c_str(), "Host") == 0)
            {
                has_host = !snd.empty();
            }

            req->setHeader(fst, snd);
        }
        if (!has_host)
        {
            req->setHeader("Host", uri->getHost());
        }
        req->setBody(body);
        return DoRequest(req, uri, timeout_ms);
    }

    std::shared_ptr<HttpResult> HttpConnection::DoRequest(const std::shared_ptr<HttpRequest>& req,
                                                          const std::shared_ptr<Uri>& uri,
                                                          const uint64_t timeout_ms)
    {
        const bool is_ssl = uri->getScheme() == "https";
        const auto address = uri->createAddress();
        if (!address)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::INVALID_HOST)
                                                , nullptr, "invalid host: " + uri->getHost());
        }
        auto sock = is_ssl ? SSLSocket::CreateTCP(address) : Socket::CreateTCP(address);
        if (!sock)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::CREATE_SOCKET_ERROR)
                                                , nullptr, "create socket fail: " + address->toString()
                                                + " errno=" + std::to_string(errno)
                                                + " errStr=" + std::string(strerror(errno)));
        }
        if (!sock->connect(address, std::chrono::milliseconds::max().count()))
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::CONNECT_FAIL)
                                                , nullptr, "connect fail: " + address->toString());
        }
        sock->setRecvTimeout(timeout_ms);
        const auto conn = std::make_shared<HttpConnection>(sock);
        const auto rt = conn->sendRequest(req);
        if (rt == 0)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::SEND_CLOSE_BY_PEER)
                                                , nullptr, "send request closed by peer: " + address->toString());
        }
        if (rt < 0)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::SEND_SOCKET_ERROR)
                                                , nullptr, "send request socket error errno=" + std::to_string(errno)
                                                + " errStr=" + std::string(strerror(errno)));
        }
        auto rsp = conn->recvResponse();
        if (!rsp)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::TIMEOUT)
                                                , nullptr, "recv response timeout: " + address->toString()
                                                + " timeout_ms:" + std::to_string(timeout_ms));
        }
        return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::OK), rsp, "ok");
    }

    HttpConnectionPool::HttpConnectionPool(std::string host, std::string vhost, const uint32_t port,
                                           const bool is_https, const uint32_t max_size, const uint32_t max_alive_time,
                                           const uint32_t max_request) : m_host(std::move(host))
                                                                         , m_vhost(std::move(vhost))
                                                                         , m_port(port ? port : (is_https ? 443 : 80))
                                                                         , m_maxSize(max_size)
                                                                         , m_maxAliveTime(max_alive_time)
                                                                         , m_maxRequest(max_request)
                                                                         , m_isHttps(is_https)
    {
    }

    std::shared_ptr<HttpConnection> HttpConnectionPool::getConnection()
    {
        const uint64_t now_ms = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::vector<HttpConnection*> invalid_conns;
        HttpConnection* ptr = nullptr;
        std::unique_lock lock(m_mutex);
        while (!m_connections.empty())
        {
            auto conn = *m_connections.begin();
            m_connections.pop_front();
            if (!conn->isConnected())
            {
                invalid_conns.push_back(conn);
                continue;
            }
            if (conn->m_createTime + m_maxAliveTime > now_ms)
            {
                invalid_conns.push_back(conn);
                continue;
            }
            ptr = conn;
            break;
        }
        lock.unlock();
        for (const auto i : invalid_conns)
        {
            delete i;
        }
        m_total -= invalid_conns.size();

        if (!ptr)
        {
            const auto address = Address::LookupAnyIPAddress(m_host);
            if (!address)
            {
                LOG_ERROR(g_logger) << "get address failed:" << m_host;
                return nullptr;
            }
            address->setPort(m_port);
            const auto sock = m_isHttps ? SSLSocket::CreateTCP(address) : Socket::CreateTCP(address);
            // const auto sock = Socket::CreateTCP(address);
            if (!sock)
            {
                LOG_ERROR(g_logger) << "create socket fail";
                return nullptr;
            }
            if (!sock->connect(address, std::chrono::milliseconds::max().count()))
            {
                LOG_ERROR(g_logger) << "sock connect fail";
                return nullptr;
            }

            ptr = new HttpConnection(sock);
            ++m_total;
        }
        return {
            ptr, [this](auto&& PH1) { HttpConnectionPool::ReleasePtr(std::forward<decltype(PH1)>(PH1), this); }
        };
    }

    std::shared_ptr<HttpResult> HttpConnectionPool::doGet(const std::string& url, const uint64_t timeout_ms,
                                                          const std::unordered_map<std::string, std::string>& headers,
                                                          const std::string& body)
    {
        return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnectionPool::doGet(const std::shared_ptr<Uri>& uri, const uint64_t timeout_ms,
                                                          const std::unordered_map<std::string, std::string>& headers,
                                                          const std::string& body)
    {
        std::stringstream ss;
        ss << uri->getPath()
            << (uri->getQuery().empty() ? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty() ? "" : "#")
            << uri->getFragment();
        return doGet(ss.str(), timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnectionPool::doPost(const std::string& url, uint64_t timeout_ms,
                                                           const std::unordered_map<std::string, std::string>& headers,
                                                           const std::string& body)
    {
        return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnectionPool::doPost(const std::shared_ptr<Uri>& uri, const uint64_t timeout_ms,
                                                           const std::unordered_map<std::string, std::string>& headers,
                                                           const std::string& body)
    {
        std::stringstream ss;
        ss << uri->getPath()
            << (uri->getQuery().empty() ? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty() ? "" : "#")
            << uri->getFragment();
        return doPost(ss.str(), timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnectionPool::doRequest(const HttpMethod method, const std::string& url,
                                                              const uint64_t timeout_ms,
                                                              const std::unordered_map<std::string, std::string>&
                                                              headers,
                                                              const std::string& body)
    {
        const auto req = std::make_shared<HttpRequest>();
        req->setPath(url);
        req->setMethod(method);
        req->setClose(false);
        bool has_host = false;
        for (const auto& [fst, snd] : headers)
        {
            if (strcasecmp(fst.c_str(), "Connection") == 0)
            {
                if (strcasecmp(snd.c_str(), "Keep-Alive") == 0)
                {
                    req->setClose(false);
                }
                continue;
            }

            if (!has_host && strcasecmp(fst.c_str(), "Host") == 0)
            {
                has_host = !snd.empty();
            }

            req->setHeader(fst, snd);
        }
        if (!has_host)
        {
            if (m_vhost.empty())
            {
                req->setHeader("Host", m_host);
            }
            else
            {
                req->setHeader("Host", m_vhost);
            }
        }
        req->setBody(body);
        return doRequest(req, timeout_ms);
    }

    std::shared_ptr<HttpResult> HttpConnectionPool::doRequest(const HttpMethod method, const std::shared_ptr<Uri>& uri,
                                                              const uint64_t timeout_ms,
                                                              const std::unordered_map<std::string, std::string>&
                                                              headers,
                                                              const std::string& body)
    {
        std::stringstream ss;
        ss << uri->getPath()
            << (uri->getQuery().empty() ? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty() ? "" : "#")
            << uri->getFragment();
        return doRequest(method, ss.str(), timeout_ms, headers, body);
    }

    std::shared_ptr<HttpResult> HttpConnectionPool::doRequest(const std::shared_ptr<HttpRequest>& request,
                                                              const uint64_t timeout_ms)
    {
        const auto conn = getConnection();
        if (!conn)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::POOL_GET_CONNECTION)
                                                , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
        }
        const auto sock = conn->getSocket();
        if (!sock)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::POOL_INVALID_CONNECTION)
                                                , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
        }
        sock->setRecvTimeout(timeout_ms);
        const auto rt = conn->sendRequest(request);
        if (rt == 0)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::SEND_CLOSE_BY_PEER)
                                                , nullptr,
                                                "send request closed by peer: " + sock->getRemoteAddress()->toString());
        }
        if (rt < 0)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::SEND_SOCKET_ERROR)
                                                , nullptr, "send request socket error errno=" + std::to_string(errno)
                                                + " errStr=" + std::string(strerror(errno)));
        }
        auto rsp = conn->recvResponse();
        if (!rsp)
        {
            return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::TIMEOUT)
                                                , nullptr,
                                                "recv response timeout: " + sock->getRemoteAddress()->toString()
                                                + " timeout_ms:" + std::to_string(timeout_ms));
        }
        return std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::OK), rsp, "ok");
    }

    void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool)
    {
        ++ptr->m_request;
        if (!ptr->isConnected()
            || ptr->m_createTime + pool->m_maxAliveTime >= static_cast<uint64_t>(
                std::chrono::high_resolution_clock::to_time_t(
                    std::chrono::high_resolution_clock::now()))
            || ptr->m_request >= pool->m_maxRequest)
        {
            delete ptr;
            --pool->m_total;
            return;
        }
        std::scoped_lock lock(pool->m_mutex);
        pool->m_connections.push_back(ptr);
    }

    std::shared_ptr<HttpConnectionPool> HttpConnectionPool::Create(const std::string& uri, const std::string& vhost,
                                                                   uint32_t max_size, uint32_t max_alive_time,
                                                                   uint32_t max_request)
    {
        const auto turi = Uri::Create(uri);
        if (!turi)
        {
            LOG_ERROR(g_logger)
            << "URI creation failed. "
            << "An error occurred while creating the URI.";
        }
        return std::make_shared<HttpConnectionPool>(turi->getHost()
                                                    , vhost, turi->getPort(), turi->getScheme() == "https"
                                                    , max_size, max_alive_time, max_request);
    }
}
