/**
 * @file HttpSession.h
 * @brief HTTPSession封装
 * @date 2025-03-17
 */
#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include "Http.h"
#include "net/stream/SocketStream.h"

namespace Gyanis::net::http
{
    /**
     * @brief HTTP 会话类
     */
    class HttpSession : public stream::SocketStream
    {
    public:
        /**
         * @brief 构造 HTTP 会话对
         * @param socket 共享的套接字指针，表示与客户端的连接
         * @param owner 指示是否拥有该套接字的所有权，默认为 `true`，表示该类负责销毁套接字
         */
        explicit HttpSession(const std::shared_ptr<Socket>& socket, bool owner = true);

        /**
         * @brief 接收 HTTP 请求
         */
        std::shared_ptr<HttpRequest> recvRequest();

        /**
         * @brief 发送 HTTP 响应
         * @param response 要发送的 HTTP 响应对象
         * @return long 返回实际发送的字节数 如果发送成功，返回发送的字节数；如果发生错误，返回负值
         */
        long sendResponse(const std::shared_ptr<HttpResponse>& response);
    };
}

#endif
