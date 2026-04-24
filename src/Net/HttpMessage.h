/**
 * @file HttpMessage.h
 * @brief HTTP 协议消息基础类型定义，包含请求、响应、头部和常用方法。
 */

#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Net::Http
{
    /**
     * @brief HTTP 请求方法枚举。
     */
    enum class HttpMethod
    {
        Get,     ///< GET 方法
        Post,    ///< POST 方法
        Put,     ///< PUT 方法
        Delete,  ///< DELETE 方法
        Head,    ///< HEAD 方法
        Options, ///< OPTIONS 方法
        Patch,   ///< PATCH 方法
        Connect, ///< CONNECT 方法
        Trace    ///< TRACE 方法
    };

    /**
     * @brief 将 HttpMethod 枚举转换为对应的 HTTP 方法字符串（大写）。
     * @param m 方法枚举值
     * @return 字符串视图，例如 "GET", "POST"
     */
    std::string_view methodString(HttpMethod m);

    /**
     * @brief 将 HTTP 方法字符串转换为对应的 HttpMethod 枚举。
     * @param s 方法字符串，例如 "GET", "POST"（大小写敏感，建议使用大写）
     * @return 成功返回 HttpMethod，失败返回 std::nullopt。
     */
    std::optional<HttpMethod> methodFromString(std::string_view s);

    /**
     * @brief HTTP 头部键值映射容器。
     * @details 键存储为小写形式，实现大小写不敏感查找。
     *          支持迭代器遍历，可用于序列化。
     */
    class HttpHeaderMap
    {
    public:
        /**
         * @brief 设置头部字段的值。
         * @param key 字段名（大小写不敏感，内部会转换为小写）。
         * @param value 字段值。
         */
        void set(const std::string &key, std::string value);

        /**
         * @brief 获取头部字段的值。
         * @param key 字段名（大小写不敏感）。
         * @return 包含字段值的 optional，如果不存在则返回 nullopt。
         */
        std::optional<std::string_view> get(const std::string &key) const;

        /**
         * @brief 判断是否存在指定头部字段。
         * @param key 字段名（大小写不敏感）。
         * @return true 存在，false 不存在。
         */
        bool contains(const std::string &key) const;

        /**
         * @brief 返回指向第一个元素的迭代器（用于范围 for 循环）。
         * @return 迭代器。
         */
        auto begin() const
        {
            return m_map.begin();
        }

        /**
         * @brief 返回指向末尾的迭代器。
         * @return 迭代器。
         */
        auto end() const
        {
            return m_map.end();
        }

    private:
        std::unordered_map<std::string, std::string> m_map; ///< 实际存储，键为小写形式。
    };

    /**
     * @brief HTTP 请求消息。
     * @details 包含方法、URI、HTTP 版本号、头部和消息体。
     *          未提供解析/序列化逻辑，仅作为数据容器。
     */
    class HttpRequest
    {
    public:
        HttpMethod method = HttpMethod::Get; ///< 请求方法，默认为 GET。
        std::string uri;                     ///< 请求 URI（通常以 '/' 开头）。
        int httpMajor = 1;                   ///< HTTP 主版本号，默认为 1。
        int httpMinor = 1;                   ///< HTTP 次版本号，默认为 1。
        HttpHeaderMap headers;               ///< 头部映射。
        std::string body;                    ///< 消息体（如 POST 数据）。
    };

    /**
     * @brief HTTP 响应消息。
     * @details 包含状态码、状态文本、头部和消息体。
     *          提供便捷方法设置常见状态码和 Content-Length。
     */
    class HttpResponse
    {
    public:
        int statusCode = 200;      ///< HTTP 状态码，默认为 200。
        std::string statusMessage; ///< 状态描述文本（如 "OK"）。
        HttpHeaderMap headers;     ///< 头部映射。
        std::string body;          ///< 响应消息体。

        /**
         * @brief 默认构造函数，状态码默认为 200，状态消息为 "OK"。
         */
        HttpResponse();

        /**
         * @brief 根据状态码设置状态消息。
         * @param code HTTP 状态码，支持常见值（200, 201, 204, 400, 404, 500），其他值设为 "Unknown"。
         */
        void setStatus(int code);

        /**
         * @brief 设置响应消息体并自动更新 Content-Length 头部。
         * @param b 消息体字符串。
         */
        void setBody(std::string b);
    };
}

#endif
