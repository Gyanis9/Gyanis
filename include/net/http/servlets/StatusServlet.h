/**
* @file StatusServlet.h
 * @brief 该文件定义了 `StatusServlet` 类，继承自 `Servlet` 类，用于处理 HTTP 请求中的状态相关操作
 * @date 2025-04-03
 */

#ifndef STATUSSERVLET_H
#define STATUSSERVLET_H

#include "Servlet.h"

namespace Gyanis::net::http
{
    /**
     * @class StatusServlet
     * @brief 该类继承自 `Servlet`，用于处理与系统状态相关的 HTTP 请求
     */
    class StatusServlet final : public Servlet
    {
    public:
        /**
         * @brief 构造函数，初始化 `StatusServlet` 对象
         */
        StatusServlet();

        /**
         * @brief 处理 HTTP 请求并生成响应
         * @param request 当前的 HTTP 请求对象，包含请求的详细信息
         * @param response 用于生成 HTTP 响应的对象
         * @param session 当前的 HTTP 会话对象，包含会话级别的信息
         * @return 返回一个状态码，通常是 `HTTP` 状态码，表示请求是否成功
         */
        int32_t handle(const std::shared_ptr<HttpRequest>& request,
                       const std::shared_ptr<HttpResponse>& response,
                       const std::shared_ptr<HttpSession>& session) override;
    };
}

#endif
