/**
 * @file Http.h
 * @brief HTTP定义结构体封装
 * @date 2025-03-16
 */
#ifndef HTTP_H
#define HTTP_H
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>

namespace Gyanis::net::http
{
    /**
     * @brief HTTP方法枚举
     */
    enum class HttpMethod
    {
        DELETE = 0, GET = 1, HEAD = 2, POST = 3, PUT = 4, CONNECT = 5, OPTIONS = 6, TRACE = 7, COPY = 8, LOCK = 9,
        MKCOL = 10,
        MOVE = 11, PROPFIND = 12, PROPPATCH = 13, SEARCH = 14, UNLOCK = 15, BIND = 16, REBIND = 17, UNBIND = 18,
        ACL = 19,
        REPORT = 20, MKACTIVITY = 21, CHECKOUT = 22, MERGE = 23, MSEARCH = 24, NOTIFY = 25, SUBSCRIBE = 26,
        UNSUBSCRIBE = 27,
        PATCH = 28, PURGE = 29, MKCALENDAR = 30, LINK = 31, UNLINK = 32, SOURCE = 33, INVALID_METHOD = 34
    };

    /**
     * @brief HTTP状态码枚举
     */
    enum class HttpStatus
    {
        CONTINUE = 100, SWITCHING_PROTOCOLS = 101, PROCESSING = 102, OK = 200, CREATED = 201, ACCEPTED = 202,
        NON_AUTHORITATIVE_INFORMATION = 203, NO_CONTENT = 204, RESET_CONTENT = 205, PARTIAL_CONTENT = 206,
        MULTI_STATUS = 207, ALREADY_REPORTED = 208, IM_USED = 226, MULTIPLE_CHOICES = 300, MOVED_PERMANENTLY = 301,
        FOUND = 302, SEE_OTHER = 303, NOT_MODIFIED = 304, USE_PROXY = 305, TEMPORARY_REDIRECT = 307,
        PERMANENT_REDIRECT = 308, BAD_REQUEST = 400, UNAUTHORIZED = 401, PAYMENT_REQUIRED = 402, FORBIDDEN = 403,
        NOT_FOUND = 404, METHOD_NOT_ALLOWED = 405, NOT_ACCEPTABLE = 406, PROXY_AUTHENTICATION_REQUIRED = 407,
        REQUEST_TIMEOUT = 408, CONFLICT = 409, GONE = 410, LENGTH_REQUIRED = 411, PRECONDITION_FAILED = 412,
        PAYLOAD_TOO_LARGE = 413, URI_TOO_LONG = 414, UNSUPPORTED_MEDIA_TYPE = 415, RANGE_NOT_SATISFIABLE = 416,
        EXPECTATION_FAILED = 417, MISDIRECTED_REQUEST = 421, UNPROCESSABLE_ENTITY = 422, LOCKED = 423,
        FAILED_DEPENDENCY = 424, UPGRADE_REQUIRED = 426, PRECONDITION_REQUIRED = 428, TOO_MANY_REQUESTS = 429,
        REQUEST_HEADER_FIELDS_TOO_LARGE = 431, UNAVAILABLE_FOR_LEGAL_REASONS = 451, INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501, BAD_GATEWAY = 502, SERVICE_UNAVAILABLE = 503, GATEWAY_TIMEOUT = 504,
        HTTP_VERSION_NOT_SUPPORTED = 505, VARIANT_ALSO_NEGOTIATES = 506, INSUFFICIENT_STORAGE = 507,
        LOOP_DETECTED = 508, NOT_EXTENDED = 510, NETWORK_AUTHENTICATION_REQUIRED = 511,
    };

    /**
     * @brief 将字符串转换为 HTTP 方法枚举
     */
    HttpMethod StringToHttpMethod(const std::string& method);

    /**
     * @brief 将字符数组转换为 HTTP 方法枚举
     */
    HttpMethod CharsToHttpMethod(const char* method);

    /**
     * @brief 将 HTTP 方法枚举转换为字符串
     */
    const char* HttpMethodToString(const HttpMethod& method);

    /**
     * @brief 将 HTTP 状态码枚举转换为字符串
     */
    const char* HttpStatusToString(const HttpStatus& status);

    /**
     * @brief 比较两个字符串，忽略大小写
     */
    struct CaseInsensitiveLess
    {
        bool operator()(const std::string& s1, const std::string& s2) const;
    };

    /**
     * @brief 从 Map 中获取指定键的值并进行类型转换
     * @param map Map 类型的数据结构
     * @param key 键名
     * @param value 输出值
     * @param defaultValue 转换失败时返回的默认值
     * @return bool 是否成功转换
     */
    template <typename MapType, typename T>
    bool checkGetAs(const MapType& map, const std::string& key, const T& value, const T& defaultValue = T())
    {
        auto it = map.find(key);
        if (it == map.end())
        {
            value = defaultValue;
            return false;
        }
        try
        {
            value = boost::lexical_cast<T>(it->second);
            return true;
        }
        catch (...)
        {
            value = defaultValue;
        }
        return false;
    }

    /**
     * @brief 从 Map 中获取指定键的值并进行类型转换
     * @param map Map 类型的数据结构
     * @param key 键名
     * @param defaultValue 转换失败时返回的默认值
     * @return T 转换后的值，或默认值
     */
    template <typename MapType, typename T>
    T getAs(const MapType& map, const std::string& key, const T& defaultValue = T())
    {
        auto it = map.find(key);
        if (it == map.end())
        {
            return defaultValue;
        }
        try
        {
            return boost::lexical_cast<T>(it->second);
        }
        catch (...)
        {
        }
        return defaultValue;
    }

    class HttpResponse;

    /**
     * @brief HTTP 请求类
     */
    class HttpRequest
    {
    public:
        using MapType = std::unordered_map<std::string, std::string>;

        /**
         * @brief 构造函数
         * @param version HTTP 协议版本，默认值为 0x11
         * @param close 是否关闭连接，默认为 true
         */
        explicit HttpRequest(uint8_t version = 0x11, bool close = true);

        /**
         * @brief 创建一个新的 HttpRequest 对象
         */
        std::shared_ptr<HttpResponse> createResponse() const;

        /**
         * @brief 获取请求方法
         */
        HttpMethod getMethod() const;

        /**
         * @brief 获取 HTTP 协议版本
         */
        uint8_t getVersion() const;

        /**
         * @brief 获取请求路径
         */
        const std::string& getPath() const;

        /**
         * @brief 获取查询参数
         */
        const std::string& getQuery() const;

        /**
         * @brief 获取请求体内容
         */
        const std::string& getBody() const;

        /**
         * @brief 获取请求头
         */
        const MapType& getHeaders() const;

        /**
         * @brief 获取请求参数
         */
        const MapType& getParams() const;

        /**
         * @brief 获取请求的 cookies
         */
        const MapType& getCookies() const;

        /**
         * @brief 设置 HTTP 请求方法
         */
        void setMethod(const HttpMethod& method);

        /**
         * @brief 设置 HTTP 协议版本
         */
        void setVersion(uint8_t version);

        /**
         * @brief 设置请求路径
         */
        void setPath(const std::string& path);

        /**
         * @brief 设置查询字符串
         */
        void setQuery(const std::string& query);

        /**
         * @brief 设置 URL 片段
         */
        void setFragment(const std::string& fragment);

        /**
         * @brief 设置请求体内容
         */
        void setBody(const std::string& body);

        /**
         * @brief 获取连接关闭标志
         */
        bool isClose() const;

        /**
         * @brief 设置连接关闭标志
         */
        void setClose(bool close);

        /**
         * @brief 获取 WebSocket 是否开启
         */
        bool isWebSocket() const;

        /**
         * @brief 设置 WebSocket 标志
         */
        void setWebSocket(bool websocket);

        /**
         * @brief 设置请求头部
         */
        void setHeaders(const MapType& headers);

        /**
         * @brief 设置请求参数
         */
        void setParams(const MapType& params);

        /**
         * @brief 设置请求 cookies
         */
        void setCookies(const MapType& cookies);

        /**
         * @brief 获取请求头部字段的值
         * @param key 请求头字段的键名
         * @param def 默认值，默认为空字符串
         */
        std::string getHeader(const std::string& key, const std::string& def = "") const;

        /**
         * @brief 获取请求参数的值
         * @param key 查询参数的键名
         * @param def 默认值，默认为空字符串
         */
        std::string getParam(const std::string& key, const std::string& def = "");

        /**
         * @brief 获取请求 cookies 的值
         * @param key cookie 的键名
         * @param def 默认值，默认为空字符串
         */
        std::string getCookie(const std::string& key, const std::string& def = "");

        /**
         * @brief 设置请求头部字段的值
         * @param key 请求头字段的键名
         * @param value 请求头字段的值
         */
        void setHeader(const std::string& key, const std::string& value);

        /**
         * @brief 设置请求参数的值
         * @param key 查询参数的键名
         * @param value 查询参数的值
         */
        void setParam(const std::string& key, const std::string& value);

        /**
         * @brief 设置请求 cookies 的值
         * @param key cookie 的键名
         * @param value cookie 的值
         */
        void setCookie(const std::string& key, const std::string& value);

        /**
         * @brief 删除请求头部字段
         * @param key 请求头字段的键名
         */
        void deleteHeader(const std::string& key);

        /**
         * @brief 删除请求参数
         * @param key 查询参数的键名
         */
        void deleteParam(const std::string& key);

        /**
         * @brief 删除请求 cookies
         * @param key cookie 的键名
         */
        void deleteCookie(const std::string& key);

        /**
         * @brief 检查请求头部是否包含指定字段
         * @param key 请求头字段的键名
         * @param value 可选参数，如果字段存在，则返回字段的值
         */
        bool hasHeader(const std::string& key, std::string* value = nullptr);

        /**
         * @brief 检查请求参数是否包含指定字段
         * @param key 查询参数的键名
         * @param value 可选参数，如果参数存在，则返回字段的值
         */
        bool hasParam(const std::string& key, std::string* value = nullptr);

        /**
         * @brief 检查请求 cookies 是否包含指定字段
         * @param key cookie 的键名
         * @param value 可选参数，如果 cookie 存在，则返回字段的值
         */
        bool hasCookie(const std::string& key, std::string* value = nullptr);

        /**
         * @brief 检查并获取请求头字段的值，转换为指定类型
         * @param key 请求头字段的键名
         * @param value 请求头字段的值，转换为指定类型
         * @param defaultValue 默认值，默认为 T()
         */
        template <typename T>
        bool checkGetHeaderAs(const std::string& key, std::string& value, const T& defaultValue = T())
        {
            return checkGetAs<MapType>(m_headers, key, value, defaultValue);
        }

        /**
         * @brief 获取请求头字段的值，转换为指定类型
         * @param key 请求头字段的键名
         * @param defaultValue 默认值，默认为 T()
         */
        template <typename T>
        T getHeaderAs(const std::string& key, const T& defaultValue = T())
        {
            return getAs<MapType>(m_headers, key, defaultValue);
        }

        /**
         * @brief 检查并获取查询参数的值，转换为指定类型
         * @param key 查询参数的键名
         * @param value 查询参数的值，转换为指定类型
         * @param def 默认值，默认为 T()，如果字段不存在则返回默认值
         */
        template <typename T>
        bool checkParamAs(const std::string& key, T& value, const T& def = T())
        {
            initQueryParam();
            initBodyParam();
            return checkGetAs(m_params, key, value, def);
        }

        /**
         * @brief 获取查询参数的值，转换为指定类型
         * @param key 查询参数的键名
         * @param def 默认值，默认为 T()，如果字段不存在则返回默认值
         */
        template <typename T>
        T getParamAs(const std::string& key, const T& def = T())
        {
            initQueryParam();
            initBodyParam();
            return getAs(m_params, key, def);
        }

        /**
         * @brief 检查并获取 Cookie 的值，转换为指定类型
         * @param key cookie 的键名
         * @param value cookie 的值，转换为指定类型
         * @param def 默认值，默认为 T()，如果字段不存在则返回默认值
         */
        template <typename T>
        bool checkGetCookieAs(const std::string& key, T& value, const T& def = T())
        {
            initCookies();
            return checkGetAs(m_cookies, key, value, def);
        }

        /**
         * @brief 获取 Cookie 的值，转换为指定类型
         * @param key cookie 的键名
         * @param def 默认值，默认为 T()，如果字段不存在则返回默认值
         */
        template <typename T>
        T getCookieAs(const std::string& key, const T& def = T())
        {
            initCookies();
            return getAs(m_cookies, key, def);
        }

        /**
         * @brief 将 HTTP 请求的详细信息输出到流中
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& dump(std::ostream& os) const;

        /**
         * @brief 将 HTTP 请求转换为字符串
         */
        std::string toString() const;

        /**
         * @brief 初始化 HTTP 请求
         */
        void init();

        /**
         * @brief 初始化 HTTP 请求参数
         */
        void initParam();

        /**
         * @brief 初始化查询参数
         */
        void initQueryParam();

        /**
         * @brief 初始化请求体参数
         */
        void initBodyParam();

        /**
         * @brief 初始化 cookies
         */
        void initCookies();

    private:
        HttpMethod m_method; ///< HTTP 请求方法，例如 GET、POST 等
        uint8_t m_version; ///< HTTP 协议版本，通常为 0x11 表示 HTTP/1.1
        bool m_close; ///< 是否在响应后关闭连接
        bool m_websocket; ///< 是否启用 WebSocket 协议
        uint8_t m_parserParamFlag; ///< 标记 HTTP 请求是否解析了查询参数、请求体参数和 cookies
        std::string m_path; ///< 请求的路径部分，例如 `/index.html`
        std::string m_query; ///< URL 查询字符串部分，例如 `?key1=value1&key2=value2`
        std::string m_fragment; ///< URL 片段部分，例如 `#section1`
        std::string m_body; ///< 请求体内容，通常用于 POST 或 PUT 请求
        MapType m_headers; ///< 请求头部，包含 HTTP 请求的头部信息
        MapType m_params; ///< 请求参数，通常由 URL 查询字符串或请求体中的键值对组成
        MapType m_cookies; ///< 请求 cookies，包含所有通过 Cookie 传递的数据
    };

    class HttpResponse
    {
    public:
        using MapType = std::unordered_map<std::string, std::string>;

        /**
         * @brief 构造 HTTP 响应对象
         * @param version HTTP 协议版本，默认为 0x11（HTTP/1.1）
         * @param close 是否在响应结束后关闭连接，默认为 true
         */
        explicit HttpResponse(uint8_t version = 0x11, bool close = true);

        /**
         * @brief 获取 HTTP 响应状态码
         */
        HttpStatus getStatus() const;

        /**
         * @brief 获取 HTTP 协议版本
         */
        uint8_t getVersion() const;

        /**
         * @brief 获取 HTTP 响应体内容
         */
        const std::string& getBody() const;

        /**
         * @brief 获取 HTTP 响应状态描述
         */
        const std::string& getReason() const;

        /**
         * @brief 获取 HTTP 响应头部
         */
        const MapType& getHeaders() const;

        /**
         * @brief 设置 HTTP 响应状态码
         */
        void setStatus(HttpStatus value);

        /**
         * @brief 设置 HTTP 协议版本
         */
        void setVersion(uint8_t value);

        /**
         * @brief 设置 HTTP 响应体内容
         */
        void setBody(const std::string& value);

        /**
         * @brief 设置 HTTP 响应状态描述
         */
        void setReason(const std::string& value);

        /**
         * @brief 设置 HTTP 响应头部
         */
        void setHeaders(const MapType& value);

        /**
         * @brief 获取连接关闭标志
         */
        bool isClose() const;

        /**
         * @brief 设置连接关闭标志
         */
        void setClose(bool value);

        /**
         * @brief 获取 WebSocket 是否启用
         */
        bool isWebsocket() const;

        /**
         * @brief 设置 WebSocket 是否启用
         */
        void setWebsocket(bool value);

        /**
         * @brief 获取指定响应头部字段的值
         * @param key 响应头部字段的键名
         * @param def 默认值，如果字段不存在则返回默认值
         */
        std::string getHeader(const std::string& key, const std::string& def = "") const;

        /**
         * @brief 设置响应头部字段的值
         * @param key 响应头部字段的键名
         * @param value 响应头部字段的值
         */
        void setHeader(const std::string& key, const std::string& value);

        /**
         * @brief 删除指定响应头部字段
         * @param key 响应头部字段的键名
         */
        void delHeader(const std::string& key);

        /**
         * @brief 检查并获取响应头部字段的值，并将其转换为指定类型
         * @param key 响应头部字段的键名
         * @param value 返回转换后的值
         * @param def 默认值，默认为类型 T()
         */
        template <typename T>
        bool checkGetHeaderAs(const std::string& key, T& value, const T& def = T())
        {
            return checkGetAs(m_headers, key, value, def);
        }

        /**
         * @brief 获取响应头部字段的值，并将其转换为指定类型
         * @param key 响应头部字段的键名
         * @param def 默认值，默认为类型 T()
         */
        template <typename T>
        T getHeaderAs(const std::string& key, const T& def = T())
        {
            return getAs(m_headers, key, def);
        }

        /**
         * @brief 将 HTTP 响应的详细信息输出到流中
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& dump(std::ostream& os) const;

        /**
         * @brief 将 HTTP 响应转换为字符串
         */
        std::string toString() const;

        /**
         * @brief 设置重定向 URI
         */
        void setRedirect(const std::string& uri);

        /**
         * @brief 设置 Cookie
         * @param key cookie 的键名
         * @param value cookie 的值
         * @param expired cookie 的过期时间
         * @param path cookie 的有效路径
         * @param domain cookie 的域名
         * @param secure 是否使用 secure 标志（仅在 HTTPS 下传输）
         */
        void setCookie(const std::string& key, const std::string& value,
                       time_t expired = 0, const std::string& path = "",
                       const std::string& domain = "", bool secure = false);

    private:
        HttpStatus m_status; ///< HTTP 响应状态码
        uint8_t m_version; ///< HTTP 协议版本
        bool m_close; ///< 是否在响应后关闭连接
        bool m_websocket; ///< 是否启用 WebSocket 协议
        std::string m_body; ///< HTTP 响应体内容
        std::string m_reason; ///< HTTP 响应状态描述
        MapType m_headers; ///< HTTP 响应头部
        std::vector<std::string> m_cookies; ///< HTTP 响应的 cookies
    };

    /**
     * @brief 将 HTTP 请求对象输出到流
     * @param os 输出流，通常是 `std::cout` 或其他输出流。
     * @param request HTTP 请求对象。
     */
    std::ostream& operator<<(std::ostream& os, const HttpRequest& request);

    /**
     * @brief 将 HTTP 响应对象输出到流
     * @param os 输出流，通常是 `std::cout` 或其他输出流。
     * @param response HTTP 响应对象。
     */
    std::ostream& operator<<(std::ostream& os, const HttpResponse& response);
}

#endif
