/**
 * @file Servlet.h
 * @brief Servlet模块封装
 * @date 2025-03-18
 */
#ifndef SERVLET_H
#define SERVLET_H

#include <functional>
#include <shared_mutex>
#include <utility>

#include "../../../base/Utils.h"
#include "../HttpSession.h"

namespace Gyanis::net::http
{
    /**
     * @brief Servlet 基类
     */
    class Servlet
    {
    public:
        /**
         * @brief 构造 Servlet 对象
         */
        explicit Servlet(std::string name);

        /**
         * @brief 析构 Servlet 对象
         */
        virtual ~Servlet() = default;

        /**
         * @brief 处理 HTTP 请求
         * @param request HTTP 请求对象
         * @param response HTTP 响应对象
         * @param session 当前会话对象
         * @return int32_t 返回处理结果，通常为 HTTP 响应的状态码
         */
        virtual int32_t handle(const std::shared_ptr<HttpRequest>& request,
                               const std::shared_ptr<HttpResponse>& response,
                               const std::shared_ptr<HttpSession>& session) = 0;

        /**
         * @brief 返回Servlet名称
         */
        [[nodiscard]] const std::string& getName() const;

    protected:
        std::string m_name; ///< 名称
    };

    /**
     * @brief 基于回调函数的 Servlet 实现
     */
    class FunctionServlet final : public Servlet
    {
    public:
        using callback = std::function<int32_t(const std::shared_ptr<HttpRequest>& request,
                                               const std::shared_ptr<HttpResponse>& response,
                                               const std::shared_ptr<HttpSession>& session)>;

        /**
         * @brief 构造 FunctionServlet 对象
         */
        explicit FunctionServlet(callback cb);

        /**
         * @brief 处理 HTTP 请求
         * @param request HTTP 请求对象
         * @param response HTTP 响应对象
         * @param session 当前会话对象
         * @return int32_t 返回处理结果，通常为 HTTP 响应的状态码
         */
        int32_t handle(const std::shared_ptr<HttpRequest>& request, const std::shared_ptr<HttpResponse>& response,
                       const std::shared_ptr<HttpSession>& session) override;

    private:
        callback m_cb; ///< 存储处理 HTTP 请求的回调函数。
    };

    /**
     * @brief Servlet 创建器接口
     */
    class IServletCreator
    {
    public:
        /**
         * @brief 析构 IServletCreator 对象
         */
        virtual ~IServletCreator() = default;

        /**
         * @brief 获取新的 Servlet 实例
         */
        [[nodiscard]] virtual std::shared_ptr<Servlet> get() const = 0;

        /**
         * @brief 获取新的 Servlet名称
         */
        [[nodiscard]] virtual std::string getName() const = 0;
    };

    /**
     * @brief 持有 Servlet 实例的创建器
     */
    class HoldServletCreator final : public IServletCreator
    {
    public:
        /**
         * @brief 构造 HoldServletCreator 对象
         */
        explicit HoldServletCreator(const std::shared_ptr<Servlet>& servlet);

        /**
         * @brief 获取持有的 Servlet 实例
         */
        [[nodiscard]] std::shared_ptr<Servlet> get() const override;

        /**
         * @brief 获取持有的 Servlet 名称
         */
        [[nodiscard]] std::string getName() const override;

    private:
        std::shared_ptr<Servlet> m_servlet; ///< 持有的 Servlet 实例
    };

    /**
     * @brief 泛型 Servlet 创建器
     */
    template <typename T>
    class ServletCreator final : public IServletCreator
    {
    public:
        ServletCreator() = default;

        /**
         * @brief 获取新的 Servlet 实例
         */
        [[nodiscard]] std::shared_ptr<Servlet> get() const override
        {
            return std::make_shared<Servlet>(std::make_shared<T>());
        }

        /**
         * @brief 获取持有的 Servlet 名称
         */
        [[nodiscard]] std::string getName() const override
        {
            return base::TypeToName<T>();
        }
    };

    /**
     * @brief Servlet 调度器
     */
    class ServletDispatch : public Servlet
    {
    public:
        /**
         * @brief 构造 Servlet 调度器
         */
        ServletDispatch();

        /**
         * @brief 处理 HTTP 请求
         * @param request HTTP 请求对象
         * @param response HTTP 响应对象
         * @param session 当前会话对象
         * @return int32_t 返回处理结果，通常为 HTTP 响应的状态码
         */
        int32_t handle(const std::shared_ptr<HttpRequest>& request, const std::shared_ptr<HttpResponse>& response,
                       const std::shared_ptr<HttpSession>& session) override;

        /**
         * @brief 添加 Servlet 路由
         * @param uri 请求 URI
         * @param servlet 要添加的 Servlet 实例
         */
        void addServlet(const std::string& uri, const std::shared_ptr<Servlet>& servlet);

        /**
         * @brief 添加基于回调的 Servlet 路由
         * @param uri 请求 URI
         * @param cb 回调函数，用于处理 HTTP 请求
         */
        void addServlet(const std::string& uri, const FunctionServlet::callback& cb);

        /**
         * @brief 添加全局 Servlet 路由
         * @param uri 请求 URI
         * @param servlet 要添加的全局 Servlet 实例
         */
        void addGlobalServlet(const std::string& uri, const std::shared_ptr<Servlet>& servlet);

        /**
         * @brief 添加基于回调的全局 Servlet 路由
         * @param uri 请求 URI
         * @param cb 回调函数，用于处理 HTTP 请求
         */
        void addGlobalServlet(const std::string& uri, const FunctionServlet::callback& cb);

        /**
         * @brief 添加 Servlet 创建器
         * @param uri 请求 URI
         * @param servlet 创建 Servlet 实例的创建器
         */
        void addServletCreator(const std::string& uri, const std::shared_ptr<IServletCreator>& servlet);

        /**
         * @brief 添加全局 Servlet 创建器
         * @param uri 请求 URI
         * @param servlet 创建全局 Servlet 实例的创建器
         */
        void addGlobalServletCreator(const std::string& uri, const std::shared_ptr<IServletCreator>& servlet);

        /**
         * @brief 添加模板类型的 Servlet 创建器
         * @param uri 请求 URI
         */
        template <typename T>
        void addServletCreator(const std::string& uri)
        {
            addServletCreator(uri, std::make_shared<ServletCreator<T>>());
        }

        /**
         * @brief 添加模板类型的全局 Servlet 创建器
         * @param uri 请求 URI
         */
        template <typename T>
        void addGlobalServletCreator(const std::string& uri)
        {
            addGlobalServletCreator(uri, std::make_shared<ServletCreator<T>>());
        }

        /**
         * @brief 删除 Servlet 路由
         * @param uri 请求 URI
         */
        void delServlet(const std::string& uri);

        /**
         * @brief 删除全局 Servlet 路由
         * @param uri 请求 URI
         */
        void delGlobalServlet(const std::string& uri);

        /**
         * @brief 获取默认 Servlet
         */
        std::shared_ptr<Servlet> getDefaultServlet() const;

        /**
         * @brief 设置默认 Servlet
         */
        void setDefaultServlet(const std::shared_ptr<Servlet>& servlet);

        /**
         * @brief 获取指定 URI 对应的 Servlet
         */
        std::shared_ptr<Servlet> getServlet(const std::string& uri) const;

        /**
         * @brief 获取指定 URI 对应的全局 Servlet
         */
        std::shared_ptr<Servlet> getGlobalServlet(const std::string& uri) const;

        /**
         * @brief 获取与 URI 最匹配的 Servlet
         */
        std::shared_ptr<Servlet> getMatchServlet(const std::string& uri) const;

        /**
         * @brief 列出所有 Servlet 创建器
         */
        void listAllServletCreator(std::unordered_map<std::string, std::shared_ptr<IServletCreator>>& servlets) const;

        /**
         * @brief 列出所有全局 Servlet 创建器
         */
        void listGlobalServletCreator(std::unordered_map<std::string, std::shared_ptr<IServletCreator>>& servlets);

    private:
        mutable std::shared_mutex m_mutex; ///< 用于保护 Servlet 路由和创建器的线程安全互斥锁
        std::unordered_map<std::string, std::shared_ptr<IServletCreator>> m_servlets; ///< 存储 URI 到 Servlet 创建器的映射
        std::vector<std::pair<std::string, std::shared_ptr<IServletCreator>>> m_dispatchers;
        ///< 存储 URI 到 Servlet 创建器的顺序映射
        std::shared_ptr<Servlet> m_default = nullptr; ///< 默认 Servlet，用于处理未匹配到的请求
    };

    /**
     * @brief 404 错误响应 Servlet
     */
    class NotFoundServlet final : public Servlet
    {
    public:
        /**
         * @brief 构造 NotFoundServlet 对象
         */
        explicit NotFoundServlet(std::string name);

        /**
         * @brief 处理 404 错误请求
         * @param request HTTP 请求对象
         * @param response HTTP 响应对象
         * @param session 当前会话对象
         * @return int32_t 返回 HTTP 状态码 404
         */
        int32_t handle(const std::shared_ptr<HttpRequest>& request, const std::shared_ptr<HttpResponse>& response,
                       const std::shared_ptr<HttpSession>& session) override;

    private:
        std::string m_name;
        std::string m_content; ///< 404 错误页面的内容
    };
}

#endif
