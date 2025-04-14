#include "net/http/WebSocketSession.h"
#include "base/HashUtils.h"
#include "base/Endian.h"

namespace Gyanis::net::http
{
    static auto g_logger = LOG_NAME("system");
    auto g_websocket_message_max_size
        = base::Config::LookUp<uint32_t>("websocket.message.max_size", 1024 * 1024 * 32,
                                         "websocket message max size");


    std::string WSFrameHead::toString() const
    {
        std::stringstream ss;
        ss << "[WebSocket Frame Header Information: "
            << "FIN: " << fin
            << " | RSV1: " << rsv1
            << " | RSV2: " << rsv2
            << " | RSV3: " << rsv3
            << " | Opcode: " << opcode
            << " | Mask: " << mask
            << " | Payload length: " << payload
            << "]";
        return ss.str();
    }

    WSFrameMessage::WSFrameMessage(const int opcode, std::string data) : m_opcode(opcode), m_data(std::move(data))
    {
    }

    int WSFrameMessage::getOpcode() const
    {
        return m_opcode;
    }

    void WSFrameMessage::setOpcode(const int value)
    {
        m_opcode = value;
    }

    const std::string& WSFrameMessage::getData() const
    {
        return m_data;
    }

    std::string& WSFrameMessage::getData()
    {
        return m_data;
    }

    void WSFrameMessage::setData(const std::string& value)
    {
        m_data = value;
    }

    WSSession::WSSession(const std::shared_ptr<Socket>& sock, const bool owner) : HttpSession(sock, owner)
    {
    }

    std::shared_ptr<HttpRequest> WSSession::handleShake()
    {
        std::shared_ptr<HttpRequest> request = nullptr;
        do
        {
            request = recvRequest();
            if (!request)
            {
                LOG_INFO(g_logger)
                    << "Invalid HTTP request received. "
                    << "Please verify the request format and try again.";
                break;
            }
            if (strcasecmp(request->getHeader("Upgrade").c_str(), "websocket") != 0)
            {
                LOG_INFO(g_logger)
                    << "HTTP header 'Upgrade' is not set to 'websocket. "
                    << "The connection upgrade request is invalid.";
                break;
            }
            if (strcasecmp(request->getHeader("Connection").c_str(), "Upgrade") != 0)
            {
                LOG_INFO(g_logger)
                << "HTTP header 'Connection' is not set to 'Upgrade'. "
                << "The connection header is invalid for a WebSocket handshake.";
                break;
            }
            if (request->getHeaderAs<int>("Sec-WebSocket-Version") != 13)
            {
                LOG_INFO(g_logger)
                    << "HTTP header 'Sec-WebSocket-Version' is not set to '13'. "
                    << "The WebSocket version is invalid for the handshake.";
                break;
            }
            const std::string key = request->getHeader("Sec-WebSocket-Key");
            if (key.empty())
            {
                LOG_INFO(g_logger)
                    << "HTTP header 'Sec-WebSocket-Key' is missing or null. "
                    << "The WebSocket key is required for a valid handshake.";
                break;
            }

            std::string value = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            value = base::base64encode(base::sha1sum(value));
            request->setWebSocket(true);

            const auto response = request->createResponse();
            response->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
            response->setWebsocket(true);
            response->setReason("Web Socket Protocol Handshake");
            response->setHeader("Upgrade", "websocket");
            response->setHeader("Connection", "Upgrade");
            response->setHeader("Sec-WebSocket-Accept", value);

            sendResponse(response);
            LOG_DEBUG(g_logger) << *request;
            LOG_DEBUG(g_logger) << *response;
            return request;
        }
        while (false);
        if (request)
        {
            LOG_INFO(g_logger) << *request;
        }
        return nullptr;
    }

    std::shared_ptr<WSFrameMessage> WSSession::recvMessage()
    {
        return WSRecvMessage(this, false);
    }

    int32_t WSSession::sendMessage(const std::shared_ptr<WSFrameMessage>& msg, const bool fin)
    {
        return WSSendMessage(this, msg, false, fin);
    }

    int32_t WSSession::sendMessage(const std::string& msg, int32_t opcode, const bool fin)
    {
        return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), false, fin);
    }

    int32_t WSSession::ping()
    {
        return WSPing(this);
    }

    int32_t WSSession::pong()
    {
        return WSPong(this);
    }

    bool WSSession::handleServerShake()
    {
        return false;
    }

    bool WSSession::handleClientShake()
    {
        return false;
    }

    std::shared_ptr<WSFrameMessage> WSRecvMessage(stream::Stream* stream, const bool client)
    {
        int opcode = 0;
        std::string data;
        int cur_len = 0;
        do
        {
            WSFrameHead ws_head{};
            if (stream->readFixSize(&ws_head, sizeof(ws_head)) <= 0)
            {
                break;
            }
            LOG_DEBUG(g_logger)
                << "WebSocket Frame Header: "
                << ws_head.toString();

            if (ws_head.opcode == WSFrameHead::PING)
            {
                LOG_INFO(g_logger) << "PING";
                if (WSPong(stream) <= 0)
                {
                    break;
                }
            }
            else if (ws_head.opcode == WSFrameHead::PONG)
            {
            }
            else if (ws_head.opcode == WSFrameHead::CONTINUE
                || ws_head.opcode == WSFrameHead::TEXT_FRAME
                || ws_head.opcode == WSFrameHead::BIN_FRAME)
            {
                if (!client && !ws_head.mask)
                {
                    LOG_INFO(g_logger)
                     << "WebSocket Frame Header 'mask' is not set to 1. "
                     << "The mask field should be set to 1 for a valid WebSocket frame.";
                    break;
                }
                uint64_t length = 0;
                if (ws_head.payload == 126)
                {
                    uint16_t len = 0;
                    if (stream->readFixSize(&len, sizeof(len)) <= 0)
                    {
                        break;
                    }
                    length = base::byteswapOnLittleEndian(len);
                }
                else if (ws_head.payload == 127)
                {
                    uint64_t len = 0;
                    if (stream->readFixSize(&len, sizeof(len)) <= 0)
                    {
                        break;
                    }
                    length = base::byteswapOnLittleEndian(len);
                }
                else
                {
                    length = ws_head.payload;
                }

                if (cur_len + length >= g_websocket_message_max_size->getValue())
                {
                    LOG_WARN(g_logger)
                       << "WebSocket Frame Message length exceeds maximum allowed. "
                       << "Max allowed length: " << g_websocket_message_max_size->getValue()
                       << " | Current length: " << (cur_len + length);

                    break;
                }

                char mask[4] = {};
                if (ws_head.mask)
                {
                    if (stream->readFixSize(mask, sizeof(mask)) <= 0)
                    {
                        break;
                    }
                }
                data.resize(cur_len + length);
                if (stream->readFixSize(&data[cur_len], length) <= 0)
                {
                    break;
                }
                if (ws_head.mask)
                {
                    for (int i = 0; i < static_cast<int>(length); ++i)
                    {
                        data[cur_len + i] ^= mask[i % 4];
                    }
                }
                cur_len += length;

                if (!opcode && ws_head.opcode != WSFrameHead::CONTINUE)
                {
                    opcode = ws_head.opcode;
                }

                if (ws_head.fin)
                {
                    LOG_DEBUG(g_logger) << data;
                    return std::make_shared<WSFrameMessage>(opcode, std::move(data));
                }
            }
            else
            {
                LOG_DEBUG(g_logger)
                    << "Invalid WebSocket opcode received. "
                    << "Opcode: " << ws_head.opcode;
            }
        }
        while (true);
        stream->close();
        return nullptr;
    }

    int32_t WSSendMessage(stream::Stream* stream, const std::shared_ptr<WSFrameMessage>& msg, const bool client,
                          const bool fin)
    {
        do
        {
            WSFrameHead ws_head = {};
            ws_head.fin = fin;
            ws_head.opcode = msg->getOpcode();
            ws_head.mask = client;
            const uint64_t size = msg->getData().size();
            if (size < 126)
            {
                ws_head.payload = size;
            }
            else if (size < 65536)
            {
                ws_head.payload = 126;
            }
            else
            {
                ws_head.payload = 127;
            }

            if (stream->writeFixSize(&ws_head, sizeof(ws_head)) <= 0)
            {
                break;
            }
            if (ws_head.payload == 126)
            {
                uint16_t len = size;
                len = base::byteswapOnLittleEndian(len);
                if (stream->writeFixSize(&len, sizeof(len)) <= 0)
                {
                    break;
                }
            }
            else if (ws_head.payload == 127)
            {
                uint64_t len = base::byteswapOnLittleEndian(size);
                if (stream->writeFixSize(&len, sizeof(len)) <= 0)
                {
                    break;
                }
            }
            if (client)
            {
                char mask[4];
                const uint32_t rand_value = rand();
                memcpy(mask, &rand_value, sizeof(mask));
                std::string& data = msg->getData();
                for (size_t i = 0; i < data.size(); ++i)
                {
                    data[i] ^= mask[i % 4];
                }

                if (stream->writeFixSize(mask, sizeof(mask)) <= 0)
                {
                    break;
                }
            }
            if (stream->writeFixSize(msg->getData().c_str(), size) <= 0)
            {
                break;
            }
            return size + sizeof(ws_head);
        }
        while (false);
        stream->close();
        return -1;
    }

    int32_t WSPing(stream::Stream* stream)
    {
        WSFrameHead ws_head = {};
        ws_head.fin = true;
        ws_head.opcode = WSFrameHead::PING;
        const auto result = stream->writeFixSize(&ws_head, sizeof(ws_head));
        if (result <= 0)
        {
            stream->close();
        }
        return result;
    }

    int32_t WSPong(stream::Stream* stream)
    {
        WSFrameHead ws_head = {};
        ws_head.fin = true;
        ws_head.opcode = WSFrameHead::PONG;
        const auto result = stream->writeFixSize(&ws_head, sizeof(ws_head));
        if (result <= 0)
        {
            stream->close();
        }
        return result;
    }
}
