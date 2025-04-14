#include "net/http/WebSocketConnection.h"
#include "../../../include/base/HashUtils.h"

namespace Gyanis::net::http
{
    WSConnection::WSConnection(const std::shared_ptr<Socket>& sock, const bool owner) : HttpConnection(sock, owner)
    {
    }

    std::pair<std::shared_ptr<HttpResult>, std::shared_ptr<WSConnection>>
    WSConnection::Create(const std::string& url, const uint64_t timeout_ms,
                         const std::unordered_map<std::string, std::string>& headers)
    {
        const auto uri = Uri::Create(url);
        if (!uri)
        {
            return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::INVALID_URL),
                                                               nullptr, "invalid url:" + url), nullptr);
        }
        return Create(uri, timeout_ms, headers);
    }

    std::pair<std::shared_ptr<HttpResult>, std::shared_ptr<WSConnection>>
    WSConnection::Create(const std::shared_ptr<Uri>& uri, const uint64_t timeout_ms,
                         const std::unordered_map<std::string, std::string>& headers)
    {
        auto address = uri->createAddress();
        if (!address)
        {
            return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::INVALID_HOST),
                                                               nullptr,
                                                               "invalid host: " + uri->getHost()), nullptr);
        }
        auto sock = Socket::CreateTCP(address);
        if (!sock)
        {
            return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::CREATE_SOCKET_ERROR),
                                                               nullptr,
                                                               "create socket fail: " + address->toString()
                                                               + " errno=" + std::to_string(errno)
                                                               + " errstr=" + std::string(strerror(errno))), nullptr);
        }
        if (!sock->connect(address, std::chrono::milliseconds::max().count()))
        {
            return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::CONNECT_FAIL),
                                                               nullptr,
                                                               "connect fail: " + address->toString()), nullptr);
        }
        sock->setRecvTimeout(timeout_ms);
        auto conn = std::make_shared<WSConnection>(sock);

        auto request = std::make_shared<HttpRequest>();
        request->setPath(uri->getPath());
        request->setQuery(uri->getQuery());
        request->setFragment(uri->getFragment());
        request->setMethod(HttpMethod::GET);
        bool has_host = false;
        bool has_conn = false;
        for (const auto& [fst, snd] : headers)
        {
            if (strcasecmp(fst.c_str(), "connection") == 0)
            {
                has_conn = true;
            }
            else if (!has_host && strcasecmp(fst.c_str(), "host") == 0)
            {
                has_host = !snd.empty();
            }

            request->setHeader(fst, snd);
        }
        request->setWebSocket(true);
        if (!has_conn)
        {
            request->setHeader("connection", "Upgrade");
        }
        request->setHeader("Upgrade", "websocket");
        request->setHeader("Sec-WebSocket-Version", "13");
        request->setHeader("Sec-WebSocket-Key", base::base64encode(base::random_string(16)));
        if (!has_host)
        {
            request->setHeader("Host", uri->getHost());
        }

        const auto result = conn->sendRequest(request);
        if (result == 0)
        {
            return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::SEND_CLOSE_BY_PEER),
                                                               nullptr,
                                                               "send request closed by peer: " + address->toString()),
                                  nullptr);
        }
        if (result < 0)
        {
            return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::SEND_SOCKET_ERROR),
                                                               nullptr,
                                                               "send request socket error errno=" +
                                                               std::to_string(errno)
                                                               + " errstr=" + std::string(strerror(errno))), nullptr);
        }
        auto response = conn->recvResponse();
        if (!response)
        {
            return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::TIMEOUT), nullptr,
                                                               "recv response timeout: " + address->toString()
                                                               + " timeout_ms:" + std::to_string(timeout_ms)), nullptr);
        }

        if (response->getStatus() != HttpStatus::SWITCHING_PROTOCOLS)
        {
            return std::make_pair(
                std::make_shared<HttpResult>(50, response, "not websocket server " + address->toString()),
                nullptr);
        }
        return std::make_pair(std::make_shared<HttpResult>(static_cast<int>(HttpResult::Error::OK), response, "ok"),
                              conn);
    }

    std::shared_ptr<WSFrameMessage> WSConnection::recvMessage()
    {
        return WSRecvMessage(this, true);
    }

    int32_t WSConnection::sendMessage(const std::shared_ptr<WSFrameMessage>& msg, const bool fin)
    {
        return WSSendMessage(this, msg, true, fin);
    }

    int32_t WSConnection::sendMessage(const std::string& msg, int32_t opcode, const bool fin)
    {
        return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), true, fin);
    }

    int32_t WSConnection::ping()
    {
        return WSPing(this);
    }

    int32_t WSConnection::pong()
    {
        return WSPong(this);
    }
}
