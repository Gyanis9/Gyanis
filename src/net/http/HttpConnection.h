/**
 * @file HttpConnection.h
 * @brief HTTP 连接和连接池管理
 * @date 2025-03-18
 */
#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <atomic>
#include <list>
#include <mutex>
#include <utility>

#include "net/http/HttpParser.h"
#include "net/stream/SocketStream.h"
#include "net/http/Uri.h"

namespace Gyanis::net::http
{
    /**
     * @brief HTTP 请求结果结构体
     */
    struct HttpResult
    {
        /**
         * @brief 错误类型枚举
         */
        enum class Error
        {
            OK = 0, ///< 正常
            INVALID_URL = 1, ///< 非法 URL
            INVALID_HOST = 2, ///< 无法解析 HOST
            CONNECT_FAIL = 3, ///< 连接失败
            SEND_CLOSE_BY_PEER = 4, ///< 连接被对端关闭
            SEND_SOCKET_ERROR = 5, ///< 发送请求产生 Socket 错误
            TIMEOUT = 6, ///< 超时
            CREATE_SOCKET_ERROR = 7, ///< 创建 Socket 失败
            POOL_GET_CONNECTION = 8, ///< 从连接池中取连接失败
            POOL_INVALID_CONNECTION = 9, ///< 无效的连接
        };

        /**
         * @brief 构造 HTTP 请求结果对象
         * @param result 请求执行结果的状态码
         * @param response HTTP 响应对象
         * @param error 错误信息字符串
         */
        HttpResult(int result, const std::shared_ptr<HttpResponse>& response, std::string error);

        /**
         * @brief 将请求结果转换为字符串
         */
        [[nodiscard]] std::string toString() const;

        int result; ///< 请求执行结果的状态码 
        std::shared_ptr<HttpResponse> response; ///< 对应的 HTTP 响应对象 
        std::string error; ///< 错误信息字符串 
    };

    class HttpConnectionPool;

    /**
     * @brief HTTP 连接类
     */
    class HttpConnection : public stream::SocketStream
    {
        friend class HttpConnectionPool;

    public:
        /**
         * @brief 构造 HTTP 连接对象
         * @param sock 与服务器的套接字连接 
         * @param owner 是否拥有该套接字的所有权（默认为 `true`） 
         */
        explicit HttpConnection(const std::shared_ptr<Socket>& sock, bool owner = true);

        /**
         * @brief 析构 HTTP 连接对象
         */
        ~HttpConnection() override;

        /**
         * @brief 接收 HTTP 响应
         */
        std::shared_ptr<HttpResponse> recvResponse();

        /**
         * @brief 发送 HTTP 请求
         */
        long sendRequest(const std::shared_ptr<HttpRequest>& request);

        /**
         * @brief 执行 GET 请求
         * @param url 请求的 URL 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        static std::shared_ptr<HttpResult> DoGet(const std::string& url, uint64_t timeout_ms,
                                                 const std::unordered_map<std::string, std::string>& headers = {},
                                                 const std::string& body = "");

        /**
         * @brief 执行 GET 请求
         * @param uri 请求的 URI 对象 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        static std::shared_ptr<HttpResult> DoGet(const std::shared_ptr<Uri>& uri, uint64_t timeout_ms,
                                                 const std::unordered_map<std::string, std::string>& headers = {},
                                                 const std::string& body = "");

        /**
         * @brief 执行 POST 请求
         * @param url 请求的 URL 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        static std::shared_ptr<HttpResult> DoPost(const std::string& url, uint64_t timeout_ms,
                                                  const std::unordered_map<std::string, std::string>& headers = {},
                                                  const std::string& body = "");

        /**
         * @brief 执行 POST 请求
         *
         * @param uri 请求的 URI 对象 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容 
         * @return std::shared_ptr<HttpResult> 返回 HTTP 请求结果 
         */
        static std::shared_ptr<HttpResult> DoPost(const std::shared_ptr<Uri>& uri
                                                  , uint64_t timeout_ms
                                                  , const std::unordered_map<std::string, std::string>& headers = {}
                                                  , const std::string& body = "");

        /**
         * @brief 执行 HTTP 请求
         * @param method 请求方法（GET、POST 等） 
         * @param url 请求的 URL 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        static std::shared_ptr<HttpResult> DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms,
                                                     const std::unordered_map<std::string, std::string>& headers = {},
                                                     const std::string& body = "");

        /**
         * @brief 执行 HTTP 请求
         * @param method 请求方法（GET、POST 等） 
         * @param uri 请求的 URI 对象 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        static std::shared_ptr<HttpResult> DoRequest(HttpMethod method
                                                     , const std::shared_ptr<Uri>& uri
                                                     , uint64_t timeout_ms
                                                     , const std::unordered_map<std::string, std::string>& headers = {}
                                                     , const std::string& body = "");

        /**
         * @brief 执行 HTTP 请求
         * @param req 要发送的 HTTP 请求对象 
         * @param uri 请求的 URI 对象 
         * @param timeout_ms 请求超时时间（毫秒）
         */
        static std::shared_ptr<HttpResult> DoRequest(const std::shared_ptr<HttpRequest>& req
                                                     , const std::shared_ptr<Uri>& uri
                                                     , uint64_t timeout_ms);

    private:
        uint64_t m_createTime = 0; ///< 连接创建时间 
        uint64_t m_request = 0; ///< 当前请求的时间戳 
    };

    /**
     * @brief HTTP 连接池类
     */
    class HttpConnectionPool
    {
    public:
        /**
         * @brief 构造 HTTP 连接池对象
         *
         * @param host 服务器主机 
         * @param vhost 虚拟主机 
         * @param port 端口号 
         * @param is_https 是否是 HTTPS 连接 
         * @param max_size 最大连接池大小 
         * @param max_alive_time 最大连接存活时间 
         * @param max_request 每个连接的最大请求次数 
         */
        HttpConnectionPool(std::string host
                           , std::string vhost
                           , uint32_t port
                           , bool is_https
                           , uint32_t max_size
                           , uint32_t max_alive_time
                           , uint32_t max_request);

        /**
         * @brief 获取一个连接
         */
        std::shared_ptr<HttpConnection> getConnection();

        /**
         * @brief 执行 GET 请求
         * @param url 请求的 URL 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        std::shared_ptr<HttpResult> doGet(const std::string& url
                                          , uint64_t timeout_ms
                                          , const std::unordered_map<std::string, std::string>& headers = {}
                                          , const std::string& body = "");

        /**
         * @brief 执行 GET 请求
         * @param uri 请求的 URI 对象 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        std::shared_ptr<HttpResult> doGet(const std::shared_ptr<Uri>& uri
                                          , uint64_t timeout_ms
                                          , const std::unordered_map<std::string, std::string>& headers = {}
                                          , const std::string& body = "");

        /**
         * @brief 执行 POST 请求
         * @param url 请求的 URL 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        std::shared_ptr<HttpResult> doPost(const std::string& url
                                           , uint64_t timeout_ms
                                           , const std::unordered_map<std::string, std::string>& headers = {}
                                           , const std::string& body = "");

        /**
         * @brief 执行 POST 请求
         * @param uri 请求的 URI 对象 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        std::shared_ptr<HttpResult> doPost(const std::shared_ptr<Uri>& uri
                                           , uint64_t timeout_ms
                                           , const std::unordered_map<std::string, std::string>& headers = {}
                                           , const std::string& body = "");

        /**
         * @brief 执行 HTTP 请求
         * @param method 请求方法（GET、POST 等） 
         * @param url 请求的 URL 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        std::shared_ptr<HttpResult> doRequest(HttpMethod method
                                              , const std::string& url
                                              , uint64_t timeout_ms
                                              , const std::unordered_map<std::string, std::string>& headers = {}
                                              , const std::string& body = "");

        /**
         * @brief 执行 HTTP 请求
         * @param method 请求方法（GET、POST 等） 
         * @param uri 请求的 URI 对象 
         * @param timeout_ms 请求超时时间（毫秒） 
         * @param headers 可选的 HTTP 请求头 
         * @param body 请求体内容
         */
        std::shared_ptr<HttpResult> doRequest(HttpMethod method
                                              , const std::shared_ptr<Uri>& uri
                                              , uint64_t timeout_ms
                                              , const std::unordered_map<std::string, std::string>& headers = {}
                                              , const std::string& body = "");

        /**
         * @brief 执行 HTTP 请求
         *
         * @param request 要发送的 HTTP 请求对象
         * @param timeout_ms 请求超时时间（毫秒） 
         * @return std::shared_ptr<HttpResult> 返回 HTTP 请求结果 
         */
        std::shared_ptr<HttpResult> doRequest(const std::shared_ptr<HttpRequest>& request
                                              , uint64_t timeout_ms);

    private:
        /**
         * @brief 释放连接
         *
         * @param ptr HTTP 连接对象 
         * @param pool 连接池对象 
         */
        static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

    public:
        /**
         * @brief 创建 HTTP 连接池
         *
         * @param uri 请求的 URI 
         * @param vhost 虚拟主机名 
         * @param max_size 最大连接池大小 
         * @param max_alive_time 连接池中连接的最大生存时间 
         * @param max_request 每个连接最大请求数 
         * @return std::shared_ptr<HttpConnectionPool> 返回新创建的连接池实例 
         */
        static std::shared_ptr<HttpConnectionPool> Create(const std::string& uri, const std::string& vhost,
                                                          uint32_t max_size, uint32_t max_alive_time,
                                                          uint32_t max_request);

    private:
        std::string m_host; ///< 服务器主机名 
        std::string m_vhost; ///< 虚拟主机名 
        uint32_t m_port; ///< 服务器端口号 
        uint32_t m_maxSize; ///< 最大连接池大小 
        uint32_t m_maxAliveTime; ///< 连接池连接的最大生存时间 
        uint32_t m_maxRequest; ///< 每个连接的最大请求数 
        bool m_isHttps; ///< 是否使用 HTTPS 连接 
        std::mutex m_mutex; ///< 保护连接池线程安全的互斥锁 
        std::list<HttpConnection*> m_connections; ///< 存储连接池中的所有连接 
        std::atomic<int32_t> m_total{0}; ///< 当前总连接数 
    };
}

#endif
