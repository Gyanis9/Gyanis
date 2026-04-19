/**
 * @file WebSocketServlet.h
 * @brief WsServlet模块封装
 * @date 2025-04-03
 */

#ifndef WEBSOCKETSERVLET_H
#define WEBSOCKETSERVLET_H

#include "servlets/Servlet.h"
#include "net/http/WebSocketSession.h"

namespace Gyanis::net::http
{
    /**
     * @class WSServlet
     * @brief WebSocket Servlet 基类，用于处理 WebSocket 连接的握手、消息收发和关闭
     */
    class WSServlet : public Servlet
    {
    public:
        /**
         * @brief 构造函数，初始化 WebSocket Servlet
         */
        explicit WSServlet(const std::string& name)
            : Servlet(name)
        {
        }

        /**
         * @brief 析构函数，销毁 WebSocket Servlet 对象 
         */
        ~WSServlet() override = default;

        /**
         * @brief 处理 WebSocket 请求，通常用于 WebSocket 握手
         * @param request HTTP 请求对象 
         * @param response HTTP 响应对象 
         * @param session 当前的 HTTP 会话对象 
         * @return 返回处理状态，0 表示成功，其他值表示错误 
         */
        int32_t handle(const std::shared_ptr<HttpRequest>& request, const std::shared_ptr<HttpResponse>& response,
                       const std::shared_ptr<HttpSession>& session) override
        {
            return 0;
        }

        /**
         * @brief WebSocket 连接时的回调接口，具体操作由派生类实现 
         * @param header HTTP 请求头信息 
         * @param session 当前的 WebSocket 会话对象 
         * @return 返回连接处理的状态 
         */
        virtual int32_t
        onConnect(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSSession>& session) = 0;

        /**
         * @brief WebSocket 关闭时的回调接口，具体操作由派生类实现 
         * @param header HTTP 请求头信息 
         * @param session 当前的 WebSocket 会话对象 
         * @return 返回关闭处理的状态 
         */
        virtual int32_t
        onClose(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSSession>& session) = 0;

        /**
         * @brief 处理 WebSocket 消息的回调接口，具体操作由派生类实现 
         * @param header HTTP 请求头信息 
         * @param msg WebSocket 帧消息对象 
         * @param session 当前的 WebSocket 会话对象 
         * @return 返回消息处理的状态 
         */
        virtual int32_t handle(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSFrameMessage>& msg,
                               const std::shared_ptr<WSSession>& session) = 0;
    };

    /**
     * @class FunctionWSServlet
     * @brief WebSocket Servlet 的函数实现类，通过回调函数处理 WebSocket 连接、关闭和消息 
     *        该类允许用户自定义处理 WebSocket 连接、关闭以及消息接收的逻辑 
     */
    class FunctionWSServlet : public WSServlet
    {
    public:
        using on_connect_cb = std::function<int32_t(const std::shared_ptr<HttpRequest>& header,
                                                    const std::shared_ptr<WSSession>& session)>;

        using on_close_cb = std::function<int32_t(const std::shared_ptr<HttpRequest>& header,
                                                  const std::shared_ptr<WSSession>& session)>;

        using callback = std::function<int32_t(const std::shared_ptr<HttpRequest>& header,
                                               const std::shared_ptr<WSFrameMessage>& msg,
                                               const std::shared_ptr<WSSession>& session)>;

        /**
         * @brief 构造函数，初始化 WebSocket Servlet 
         * @param cb 处理 WebSocket 消息的回调函数 
         * @param connect_cb 处理 WebSocket 连接的回调函数，默认为 nullptr 
         * @param close_cb 处理 WebSocket 关闭的回调函数，默认为 nullptr 
         */
        explicit FunctionWSServlet(callback cb, on_connect_cb connect_cb = nullptr, on_close_cb close_cb = nullptr);

        /**
         * @brief 处理 WebSocket 连接请求 
         * @param header HTTP 请求头信息 
         * @param session 当前的 WebSocket 会话对象 
         * @return 返回连接处理的状态 
         */
        int32_t
        onConnect(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSSession>& session) override;

        /**
         * @brief 处理 WebSocket 关闭请求 
         * @param header HTTP 请求头信息 
         * @param session 当前的 WebSocket 会话对象 
         * @return 返回关闭处理的状态 
         */
        int32_t onClose(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSSession>& session) override;

        /**
         * @brief 处理 WebSocket 消息 
         * @param header HTTP 请求头信息 
         * @param msg WebSocket 帧消息对象 
         * @param session 当前的 WebSocket 会话对象 
         * @return 返回消息处理的状态 
         */
        int32_t handle(const std::shared_ptr<HttpRequest>& header, const std::shared_ptr<WSFrameMessage>& msg,
                       const std::shared_ptr<WSSession>& session) override;

    protected:
        callback m_callback; ///< 处理 WebSocket 消息的回调函数
        on_connect_cb m_onConnect; ///< 处理 WebSocket 连接的回调函数
        on_close_cb m_onClose; ///< 处理 WebSocket 关闭的回调函数
    };

    /**
     * @class WSServletDispatch
     * @brief WebSocket Servlet 分发器类，用于管理和调度多个 WebSocket Servlet 
     *        它允许将请求分发到适当的 `FunctionWSServlet` 实例 
     */
    class WSServletDispatch : public ServletDispatch
    {
    public:
        WSServletDispatch();

        /**
         * @brief 添加一个 WebSocket Servlet 到分发器 
         * @param uri WebSocket 请求的 URI 
         * @param cb 处理 WebSocket 消息的回调函数 
         * @param connect_cb 处理 WebSocket 连接的回调函数，默认为 nullptr 
         * @param close_cb 处理 WebSocket 关闭的回调函数，默认为 nullptr 
         */
        void addServletSelf(const std::string& uri, const FunctionWSServlet::callback& cb,
                            const FunctionWSServlet::on_connect_cb& connect_cb = nullptr,
                            const FunctionWSServlet::on_close_cb& close_cb = nullptr);

        /**
         * @brief 添加一个通配符 WebSocket Servlet 到分发器 
         * @param uri WebSocket 请求的 URI 
         * @param cb 处理 WebSocket 消息的回调函数 
         * @param connect_cb 处理 WebSocket 连接的回调函数，默认为 nullptr 
         * @param close_cb 处理 WebSocket 关闭的回调函数，默认为 nullptr 
         */
        void addGlobServletSelf(const std::string& uri, const FunctionWSServlet::callback& cb,
                                const FunctionWSServlet::on_connect_cb& connect_cb = nullptr,
                                const FunctionWSServlet::on_close_cb& close_cb = nullptr);

        /**
         * @brief 根据 URI 获取相应的 WebSocket Servlet 
         * @param uri WebSocket 请求的 URI 
         * @return 返回对应 URI 的 WebSocket Servlet 
         */
        std::shared_ptr<WSServlet> getWSServlet(const std::string& uri) const;
    };
}

#endif
