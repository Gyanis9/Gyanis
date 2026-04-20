/**
 * @file ConfigServlet.h
 * @brief 配置管理 Servlet
 * @date 2025-03-28
 */
#ifndef CONFIGSERVLET_H
#define CONFIGSERVLET_H
#include "Servlet.h"

namespace Gyanis::net::http
{
    /**
     * @brief 配置管理 Servlet
     */
    class ConfigServlet final : public Servlet
    {
    public:
        /**
         * @brief 构造 ConfigServlet 对象
         */
        ConfigServlet();

        /**
         * @brief 处理 HTTP 请求
         * @param request HTTP 请求对象，包含了客户端发送的请求数据
         * @param response HTTP 响应对象，用于返回结果给客户端
         * @param session 当前 HTTP 会话对象，包含会话信息
         * @return int32_t 返回 HTTP 响应的状态码，通常为处理结果的状态码（例如 200 表示成功）
         */
        int32_t handle(const std::shared_ptr<HttpRequest>& request, const std::shared_ptr<HttpResponse>& response,
                       const std::shared_ptr<HttpSession>& session) override;
    };
}

#endif
