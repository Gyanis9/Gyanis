/**
 * @file Router.h
 * @brief HTTP 路由器，支持路由匹配、中间件链和协程处理器。
 * @details 基于 HTTP 方法和 URI 模式匹配（支持 :param 路径参数）选择对应处理器，
 *          并支持多个中间件包装处理器，形成处理链。
 */

#ifndef ROUTER_H
#define ROUTER_H

#include "HttpMessage.h"
#include "Core/Task.hpp"

#include <functional>
#include <regex>
#include <string>
#include <vector>

namespace Net::Http
{
    /**
     * @brief 请求处理器类型，接收 HttpRequest，返回协程任务（最终产出 HttpResponse）。
     */
    using RequestHandler = std::function<Core::Task<HttpResponse>(HttpRequest)>;

    /**
     * @brief 中间件类型，接收一个 RequestHandler，返回一个新的 RequestHandler（包装后的处理器）。
     * @details 中间件可在执行实际处理器前后进行日志、鉴权、修改请求/响应等操作。
     */
    using Middleware = std::function<RequestHandler(RequestHandler)>;

    /**
     * @brief HTTP 路由器类。
     * @details 管理路由表（方法+路径模式）和全局中间件链。
     *          匹配时根据请求的方法和 URI 找到对应的处理器，然后将中间件链从后向前依次包装处理器，最终返回可调用的处理器。
     *          支持路径参数模式，如 "/users/:id"，匹配时捕获参数值（但不提供参数提取能力，仅匹配）。
     */
    class Router
    {
    public:
        /**
         * @brief 添加一条路由规则。
         * @param method HTTP 方法（Get, Post 等）
         * @param path   URI 路径模式，支持 :param 动态片段（如 "/user/:id"）
         * @param handler 该路由对应的请求处理器
         */
        void addRoute(HttpMethod method, std::string path, RequestHandler handler);

        /**
         * @brief 添加一个全局中间件。
         * @param mw 中间件函数，将被存储在中间件列表中。
         * @note 中间件添加的顺序影响最终包装顺序：先添加的中间件在外层，后添加的在内层。
         */
        void addMiddleware(Middleware mw);

        /**
         * @brief 根据请求方法和方法匹配路由，返回经过中间件链包装后的处理器。
         * @param method HTTP 方法
         * @param uri    请求 URI 路径
         * @return 若匹配成功，返回可调用的 RequestHandler；若未匹配返回 nullptr。
         */
        RequestHandler match(const HttpMethod method, const std::string &uri);

    private:
        /**
         * @brief 检查请求 URI 是否匹配路径模式。
         * @param pattern 路由模式（可能包含 :param 动态段）
         * @param uri     实际请求 URI
         * @return 匹配返回 true，否则 false。
         */
        static bool matchPath(const std::string &pattern, const std::string &uri);

        /**
         * @brief 路由表内部条目。
         */
        struct RouteEntry
        {
            HttpMethod method;      ///< HTTP 方法
            std::string path;       ///< URI 路径模式
            RequestHandler handler; ///< 处理器

            RouteEntry(const HttpMethod m, std::string p, RequestHandler h);
        };

        std::vector<RouteEntry> m_routes;      ///< 路由表
        std::vector<Middleware> m_middlewares; ///< 全局中间件列表
    };
}

#endif
