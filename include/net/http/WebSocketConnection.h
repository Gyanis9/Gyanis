/**
 * @file WebSocketConnection.h
 * @brief WebSocket连接模块封装
 * @date 2025-04-03
 */

#ifndef WEBSOCKETCONNECTION_H
#define WEBSOCKETCONNECTION_H

#include "net/http/HttpConnection.h"
#include "net/http/WebSocketSession.h"

namespace Gyanis::net::http
{
    /**
     * @brief WebSocket 连接类，继承自 `HttpConnection`，封装了与 WebSocket 服务器的连接管理和数据通信
     */
    class WSConnection : public HttpConnection
    {
    public:
        /**
         * @brief 构造函数，初始化 WebSocket 连接
         * @param sock WebSocket 连接的套接字 
         * @param owner 是否拥有该套接字，默认为 true 
         */
        explicit WSConnection(const std::shared_ptr<Socket>& sock, bool owner = true);

        /**
         * @brief 创建一个 WebSocket 连接对象，使用 URL 进行连接 
         * @param url WebSocket 服务器的 URL 
         * @param timeout_ms 连接超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头
         */
        static std::pair<std::shared_ptr<HttpResult>, std::shared_ptr<WSConnection>>
        Create(const std::string& url, uint64_t timeout_ms,
               const std::unordered_map<std::string, std::string>& headers = {});

        /**
         * @brief 创建一个 WebSocket 连接对象，使用 URI 进行连接 
         * @param uri WebSocket 服务器的 URI 
         * @param timeout_ms 连接超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头
         */
        static std::pair<std::shared_ptr<HttpResult>, std::shared_ptr<WSConnection>>
        Create(const std::shared_ptr<Uri>& uri, uint64_t timeout_ms,
               const std::unordered_map<std::string, std::string>& headers = {});

        /**
         * @brief 接收 WebSocket 消息
         */
        std::shared_ptr<WSFrameMessage> recvMessage();

        /**
         * @brief 发送 WebSocket 消息 
         * @param msg WebSocket 消息对象 
         * @param fin 是否为最后一帧，默认为 true 
         * @return 返回发送操作的状态码 
         */
        int32_t sendMessage(const std::shared_ptr<WSFrameMessage>& msg, bool fin = true);

        /**
         * @brief 发送字符串形式的 WebSocket 消息 
         * @param msg 消息内容字符串 
         * @param opcode 操作码，默认为文本帧（`TEXT_FRAME`） 
         * @param fin 是否为最后一帧，默认为 true 
         * @return 返回发送操作的状态码 
         */
        int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);

        /**
         * @brief 发送 PING 帧，用于检测 WebSocket 连接是否存活 
         * @return 返回发送操作的状态码 
         */
        int32_t ping();

        /**
         * @brief 发送 PONG 帧，回应 WebSocket 服务器的 PING 帧 
         * @return 返回发送操作的状态码 
         */
        int32_t pong();
    };
}

#endif
