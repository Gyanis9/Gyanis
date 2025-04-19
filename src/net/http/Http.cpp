#include "net/http/Http.h"
#include "base/Utils.h"

namespace Gyanis::net::http
{
    HttpMethod StringToHttpMethod(const std::string& method)
    {
        if (strcmp("DELETE", method.c_str()) == 0) { return HttpMethod::DELETE; }
        if (strcmp("GET", method.c_str()) == 0) { return HttpMethod::GET; }
        if (strcmp("HEAD", method.c_str()) == 0) { return HttpMethod::HEAD; }
        if (strcmp("POST", method.c_str()) == 0) { return HttpMethod::POST; }
        if (strcmp("PUT", method.c_str()) == 0) { return HttpMethod::PUT; }
        if (strcmp("CONNECT", method.c_str()) == 0) { return HttpMethod::CONNECT; }
        if (strcmp("OPTIONS", method.c_str()) == 0) { return HttpMethod::OPTIONS; }
        if (strcmp("TRACE", method.c_str()) == 0) { return HttpMethod::TRACE; }
        if (strcmp("COPY", method.c_str()) == 0) { return HttpMethod::COPY; }
        if (strcmp("LOCK", method.c_str()) == 0) { return HttpMethod::LOCK; }
        if (strcmp("MKCOL", method.c_str()) == 0) { return HttpMethod::MKCOL; }
        if (strcmp("MOVE", method.c_str()) == 0) { return HttpMethod::MOVE; }
        if (strcmp("PROPFIND", method.c_str()) == 0) { return HttpMethod::PROPFIND; }
        if (strcmp("PROPPATCH", method.c_str()) == 0) { return HttpMethod::PROPPATCH; }
        if (strcmp("SEARCH", method.c_str()) == 0) { return HttpMethod::SEARCH; }
        if (strcmp("UNLOCK", method.c_str()) == 0) { return HttpMethod::UNLOCK; }
        if (strcmp("BIND", method.c_str()) == 0) { return HttpMethod::BIND; }
        if (strcmp("REBIND", method.c_str()) == 0) { return HttpMethod::REBIND; }
        if (strcmp("UNBIND", method.c_str()) == 0) { return HttpMethod::UNBIND; }
        if (strcmp("ACL", method.c_str()) == 0) { return HttpMethod::ACL; }
        if (strcmp("REPORT", method.c_str()) == 0) { return HttpMethod::REPORT; }
        if (strcmp("MKACTIVITY", method.c_str()) == 0) { return HttpMethod::MKACTIVITY; }
        if (strcmp("CHECKOUT", method.c_str()) == 0) { return HttpMethod::CHECKOUT; }
        if (strcmp("MERGE", method.c_str()) == 0) { return HttpMethod::MERGE; }
        if (strcmp("M-SEARCH", method.c_str()) == 0) { return HttpMethod::MSEARCH; }
        if (strcmp("NOTIFY", method.c_str()) == 0) { return HttpMethod::NOTIFY; }
        if (strcmp("SUBSCRIBE", method.c_str()) == 0) { return HttpMethod::SUBSCRIBE; }
        if (strcmp("UNSUBSCRIBE", method.c_str()) == 0) { return HttpMethod::UNSUBSCRIBE; }
        if (strcmp("PATCH", method.c_str()) == 0) { return HttpMethod::PATCH; }
        if (strcmp("PURGE", method.c_str()) == 0) { return HttpMethod::PURGE; }
        if (strcmp("MKCALENDAR", method.c_str()) == 0) { return HttpMethod::MKCALENDAR; }
        if (strcmp("LINK", method.c_str()) == 0) { return HttpMethod::LINK; }
        if (strcmp("UNLINK", method.c_str()) == 0) { return HttpMethod::UNLINK; }
        if (strcmp("SOURCE", method.c_str()) == 0) { return HttpMethod::SOURCE; }
        return HttpMethod::INVALID_METHOD;
    }

    HttpMethod CharsToHttpMethod(const char* method)
    {
        if (strncmp("DELETE", method, strlen("DELETE")) == 0) { return HttpMethod::DELETE; }
        if (strncmp("GET", method, strlen("GET")) == 0) { return HttpMethod::GET; }
        if (strncmp("HEAD", method, strlen("HEAD")) == 0) { return HttpMethod::HEAD; }
        if (strncmp("POST", method, strlen("POST")) == 0) { return HttpMethod::POST; }
        if (strncmp("PUT", method, strlen("PUT")) == 0) { return HttpMethod::PUT; }
        if (strncmp("CONNECT", method, strlen("CONNECT")) == 0) { return HttpMethod::CONNECT; }
        if (strncmp("OPTIONS", method, strlen("OPTIONS")) == 0) { return HttpMethod::OPTIONS; }
        if (strncmp("TRACE", method, strlen("TRACE")) == 0) { return HttpMethod::TRACE; }
        if (strncmp("COPY", method, strlen("COPY")) == 0) { return HttpMethod::COPY; }
        if (strncmp("LOCK", method, strlen("LOCK")) == 0) { return HttpMethod::LOCK; }
        if (strncmp("MKCOL", method, strlen("MKCOL")) == 0) { return HttpMethod::MKCOL; }
        if (strncmp("MOVE", method, strlen("MOVE")) == 0) { return HttpMethod::MOVE; }
        if (strncmp("PROPFIND", method, strlen("PROPFIND")) == 0) { return HttpMethod::PROPFIND; }
        if (strncmp("PROPPATCH", method, strlen("PROPPATCH")) == 0) { return HttpMethod::PROPPATCH; }
        if (strncmp("SEARCH", method, strlen("SEARCH")) == 0) { return HttpMethod::SEARCH; }
        if (strncmp("UNLOCK", method, strlen("UNLOCK")) == 0) { return HttpMethod::UNLOCK; }
        if (strncmp("BIND", method, strlen("BIND")) == 0) { return HttpMethod::BIND; }
        if (strncmp("REBIND", method, strlen("REBIND")) == 0) { return HttpMethod::REBIND; }
        if (strncmp("UNBIND", method, strlen("UNBIND")) == 0) { return HttpMethod::UNBIND; }
        if (strncmp("ACL", method, strlen("ACL")) == 0) { return HttpMethod::ACL; }
        if (strncmp("REPORT", method, strlen("REPORT")) == 0) { return HttpMethod::REPORT; }
        if (strncmp("MKACTIVITY", method, strlen("MKACTIVITY")) == 0) { return HttpMethod::MKACTIVITY; }
        if (strncmp("CHECKOUT", method, strlen("CHECKOUT")) == 0) { return HttpMethod::CHECKOUT; }
        if (strncmp("MERGE", method, strlen("MERGE")) == 0) { return HttpMethod::MERGE; }
        if (strncmp("M-SEARCH", method, strlen("M-SEARCH")) == 0) { return HttpMethod::MSEARCH; }
        if (strncmp("NOTIFY", method, strlen("NOTIFY")) == 0) { return HttpMethod::NOTIFY; }
        if (strncmp("SUBSCRIBE", method, strlen("SUBSCRIBE")) == 0) { return HttpMethod::SUBSCRIBE; }
        if (strncmp("UNSUBSCRIBE", method, strlen("UNSUBSCRIBE")) == 0) { return HttpMethod::UNSUBSCRIBE; }
        if (strncmp("PATCH", method, strlen("PATCH")) == 0) { return HttpMethod::PATCH; }
        if (strncmp("PURGE", method, strlen("PURGE")) == 0) { return HttpMethod::PURGE; }
        if (strncmp("MKCALENDAR", method, strlen("MKCALENDAR")) == 0) { return HttpMethod::MKCALENDAR; }
        if (strncmp("LINK", method, strlen("LINK")) == 0) { return HttpMethod::LINK; }
        if (strncmp("UNLINK", method, strlen("UNLINK")) == 0) { return HttpMethod::UNLINK; }
        if (strncmp("SOURCE", method, strlen("SOURCE")) == 0) { return HttpMethod::SOURCE; };
        return HttpMethod::INVALID_METHOD;
    }

    static const char* s_method_string[] = {
        "DELETE", "GET", "HEAD", "POST", "PUT", "CONNECT", "OPTIONS", "TRACE", "COPY", "LOCK", "MKCOL", "MOVE",
        "PROPFIND", "PROPPATCH", "SEARCH", "UNLOCK", "BIND", "REBIND", "UNBIND", "ACL", "REPORT", "MKACTIVITY",
        "CHECKOUT", "MERGE", "M-SEARCH", "NOTIFY", "SUBSCRIBE", "UNSUBSCRIBE", "PATCH", "PURGE", "MKCALENDAR", "LINK",
        "UNLINK", "SOURCE",
    };

    const char* HttpMethodToString(const HttpMethod& method)
    {
        const auto idx = static_cast<uint32_t>(method);
        if (idx >= std::size(s_method_string))
        {
            return "<unknown>";
        }
        return s_method_string[idx];
    }

    const char* HttpStatusToString(const HttpStatus& status)
    {
        switch (status)
        {
        case HttpStatus::CONTINUE: return "Continue";
        case HttpStatus::SWITCHING_PROTOCOLS: return "Switching Protocols";
        case HttpStatus::PROCESSING: return "Processing";
        case HttpStatus::OK: return "OK";
        case HttpStatus::CREATED: return "Created";
        case HttpStatus::ACCEPTED: return "Accepted";
        case HttpStatus::NON_AUTHORITATIVE_INFORMATION: return "Non-Authoritative Information";
        case HttpStatus::NO_CONTENT: return "No Content";
        case HttpStatus::RESET_CONTENT: return "Reset Content";
        case HttpStatus::PARTIAL_CONTENT: return "Partial Content";
        case HttpStatus::MULTI_STATUS: return "Multi-Status";
        case HttpStatus::ALREADY_REPORTED: return "Already Reported";
        case HttpStatus::IM_USED: return "IM Used";
        case HttpStatus::MULTIPLE_CHOICES: return "Multiple Choices";
        case HttpStatus::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatus::FOUND: return "Found";
        case HttpStatus::SEE_OTHER: return "See Other";
        case HttpStatus::NOT_MODIFIED: return "Not Modified";
        case HttpStatus::USE_PROXY: return "Use Proxy";
        case HttpStatus::TEMPORARY_REDIRECT: return "Temporary Redirect";
        case HttpStatus::PERMANENT_REDIRECT: return "Permanent Redirect";
        case HttpStatus::BAD_REQUEST: return "Bad Request";
        case HttpStatus::UNAUTHORIZED: return "Unauthorized";
        case HttpStatus::PAYMENT_REQUIRED: return "Payment Required";
        case HttpStatus::FORBIDDEN: return "Forbidden";
        case HttpStatus::NOT_FOUND: return "Not Found";
        case HttpStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatus::NOT_ACCEPTABLE: return "Not Acceptable";
        case HttpStatus::PROXY_AUTHENTICATION_REQUIRED: return "Proxy Authentication Required";
        case HttpStatus::REQUEST_TIMEOUT: return "Request Timeout";
        case HttpStatus::CONFLICT: return "Conflict";
        case HttpStatus::GONE: return "Gone";
        case HttpStatus::LENGTH_REQUIRED: return "Length Required";
        case HttpStatus::PRECONDITION_FAILED: return "Precondition Failed";
        case HttpStatus::PAYLOAD_TOO_LARGE: return "Payload Too Large";
        case HttpStatus::URI_TOO_LONG: return "URI Too Long";
        case HttpStatus::UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
        case HttpStatus::RANGE_NOT_SATISFIABLE: return "Range Not Satisfiable";
        case HttpStatus::EXPECTATION_FAILED: return "Expectation Failed";
        case HttpStatus::MISDIRECTED_REQUEST: return "Misdirected Request";
        case HttpStatus::UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
        case HttpStatus::LOCKED: return "Locked";
        case HttpStatus::FAILED_DEPENDENCY: return "Failed Dependency";
        case HttpStatus::UPGRADE_REQUIRED: return "Upgrade Required";
        case HttpStatus::PRECONDITION_REQUIRED: return "Precondition Required";
        case HttpStatus::TOO_MANY_REQUESTS: return "Too Many Requests";
        case HttpStatus::REQUEST_HEADER_FIELDS_TOO_LARGE: return "Request Header Fields Too Large";
        case HttpStatus::UNAVAILABLE_FOR_LEGAL_REASONS: return "Unavailable For Legal Reasons";
        case HttpStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatus::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatus::BAD_GATEWAY: return "Bad Gateway";
        case HttpStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
        case HttpStatus::GATEWAY_TIMEOUT: return "Gateway Timeout";
        case HttpStatus::HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
        case HttpStatus::VARIANT_ALSO_NEGOTIATES: return "Variant Also Negotiates";
        case HttpStatus::INSUFFICIENT_STORAGE: return "Insufficient Storage";
        case HttpStatus::LOOP_DETECTED: return "Loop Detected";
        case HttpStatus::NOT_EXTENDED: return "Not Extended";
        case HttpStatus::NETWORK_AUTHENTICATION_REQUIRED: return "Network Authentication Required";
        default:
            return "<unknown>";
        }
    }

    bool CaseInsensitiveLess::operator()(const std::string& s1
                                         , const std::string& s2) const
    {
        return strcasecmp(s1.c_str(), s2.c_str()) < 0;
    }

    HttpRequest::HttpRequest(const uint8_t version, const bool close): m_method(HttpMethod::GET),
                                                                       m_version(version),
                                                                       m_close(close),
                                                                       m_websocket(false),
                                                                       m_parserParamFlag(0),
                                                                       m_path("/")
    {
    }

    std::shared_ptr<HttpResponse> HttpRequest::createResponse() const
    {
        auto result = std::make_shared<HttpResponse>(getVersion(), isClose());
        return result;
    }

    HttpMethod HttpRequest::getMethod() const
    {
        return m_method;
    }

    uint8_t HttpRequest::getVersion() const
    {
        return m_version;
    }

    const std::string& HttpRequest::getPath() const
    {
        return m_path;
    }

    const std::string& HttpRequest::getQuery() const
    {
        return m_query;
    }

    const std::string& HttpRequest::getBody() const
    {
        return m_body;
    }

    const HttpRequest::MapType& HttpRequest::getHeaders() const
    {
        return m_headers;
    }

    const HttpRequest::MapType& HttpRequest::getParams() const
    {
        return m_params;
    }

    const HttpRequest::MapType& HttpRequest::getCookies() const
    {
        return m_cookies;
    }

    void HttpRequest::setMethod(const HttpMethod& method)
    {
        m_method = method;
    }

    void HttpRequest::setVersion(const uint8_t version)
    {
        m_version = version;
    }

    void HttpRequest::setPath(const std::string& path)
    {
        m_path = path;
    }

    void HttpRequest::setQuery(const std::string& query)
    {
        m_query = query;
    }

    void HttpRequest::setFragment(const std::string& fragment)
    {
        m_fragment = fragment;
    }

    void HttpRequest::setBody(const std::string& body)
    {
        m_body = body;
    }

    bool HttpRequest::isClose() const
    {
        return m_close;
    }

    void HttpRequest::setClose(const bool close)
    {
        m_close = close;
    }

    bool HttpRequest::isWebSocket() const
    {
        return m_websocket;
    }

    void HttpRequest::setWebSocket(const bool websocket)
    {
        m_websocket = websocket;
    }

    void HttpRequest::setHeaders(const MapType& headers)
    {
        m_headers = headers;
    }

    void HttpRequest::setParams(const MapType& params)
    {
        m_params = params;
    }

    void HttpRequest::setCookies(const MapType& cookies)
    {
        m_cookies = cookies;
    }

    std::string HttpRequest::getHeader(const std::string& key, const std::string& def) const
    {
        const auto it = m_headers.find(key);
        return it != m_headers.end() ? it->second : def;
    }

    std::string HttpRequest::getParam(const std::string& key, const std::string& def)
    {
        initQueryParam();
        initBodyParam();
        const auto it = m_headers.find(key);
        return it != m_headers.end() ? it->second : def;
    }

    std::string HttpRequest::getCookie(const std::string& key, const std::string& def)
    {
        initCookies();
        const auto it = m_cookies.find(key);
        return it != m_cookies.end() ? it->second : def;
    }

    void HttpRequest::setHeader(const std::string& key, const std::string& value)
    {
        m_headers[key] = value;
    }

    void HttpRequest::setParam(const std::string& key, const std::string& value)
    {
        m_params[key] = value;
    }

    void HttpRequest::setCookie(const std::string& key, const std::string& value)
    {
        m_cookies[key] = value;
    }

    void HttpRequest::deleteHeader(const std::string& key)
    {
        m_headers.erase(key);
    }

    void HttpRequest::deleteParam(const std::string& key)
    {
        m_params.erase(key);
    }

    void HttpRequest::deleteCookie(const std::string& key)
    {
        m_cookies.erase(key);
    }

    bool HttpRequest::hasHeader(const std::string& key, std::string* value)
    {
        const auto it = m_headers.find(key);
        if (it == m_headers.end())
        {
            return false;
        }
        if (value)
        {
            *value = it->second;
        }
        return true;
    }

    bool HttpRequest::hasParam(const std::string& key, std::string* value)
    {
        initQueryParam();
        initBodyParam();
        const auto it = m_params.find(key);
        if (it == m_params.end())
        {
            return false;
        }
        if (value)
        {
            *value = it->second;
        }
        return true;
    }

    bool HttpRequest::hasCookie(const std::string& key, std::string* value)
    {
        initCookies();
        const auto it = m_cookies.find(key);
        if (it == m_cookies.end())
        {
            return false;
        }
        if (value)
        {
            *value = it->second;
        }
        return true;
    }

    std::ostream& HttpRequest::dump(std::ostream& os) const
    {
        os << HttpMethodToString(m_method) << " "
            << m_path
            << (m_query.empty() ? "" : "?")
            << m_query
            << (m_fragment.empty() ? "" : "#")
            << m_fragment
            << " HTTP/"
            << static_cast<uint32_t>(m_version >> 4)
            << "."
            << static_cast<uint32_t>(m_version & 0x0F)
            << "\r\n";
        if (!m_websocket)
        {
            os << "Connection: " << (m_close ? "Close" : "Keep-Alive") << "\r\n";
        }
        for (const auto& [fst, snd] : m_headers)
        {
            if (!m_websocket && strcasecmp(fst.c_str(), "Connection") == 0)
            {
                continue;
            }
            os << fst << ": " << snd << "\r\n";
        }

        if (!m_body.empty())
        {
            os << "Content-Length: " << m_body.size() << "\r\n\r\n"
                << m_body;
        }
        else
        {
            os << "\r\n";
        }
        return os;
    }

    std::string HttpRequest::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    void HttpRequest::init()
    {
        const std::string conn = getHeader("Connection");
        if (!conn.empty())
        {
            if (strcasecmp(conn.c_str(), "Keep-Alive") == 0)
            {
                m_close = false;
            }
            else
            {
                m_close = true;
            }
        }
    }

    void HttpRequest::initParam()
    {
        initQueryParam();
        initBodyParam();
        initCookies();
    }

    void HttpRequest::initQueryParam()
    {
        if (m_parserParamFlag & 0x1)
        {
            return;
        }

        size_t pos = 0;
        const size_t query_len = m_query.length();

        while (pos < query_len)
        {
            // 查找键值对的 '=' 和 '&' 分隔符
            const size_t key_start = pos;
            const size_t equal_pos = m_query.find('=', key_start);
            if (equal_pos == std::string::npos) { break; }

            const size_t key_end = equal_pos;
            const size_t value_start = equal_pos + 1;
            size_t amp_pos = m_query.find('&', value_start);

            if (amp_pos == std::string::npos)
            {
                amp_pos = query_len;
            }

            std::string key = m_query.substr(key_start, key_end - key_start);
            std::string value = base::UrlDecode(m_query.substr(value_start, amp_pos - value_start));
            m_params.insert(std::make_pair(key, value));

            // 更新 pos
            pos = amp_pos + 1; // 跳过 '&' 或到查询字符串结尾
        }

        // 设置标志，表示已解析参数
        m_parserParamFlag |= 0x1;
    }


    void HttpRequest::initBodyParam()
    {
        if (m_parserParamFlag & 0x2)
        {
            return;
        }

        const std::string content_type = getHeader("Content-Type");
        if (strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr)
        {
            m_parserParamFlag |= 0x2;
            return;
        }

        size_t pos = 0;
        const size_t body_len = m_body.length();

        while (pos < body_len)
        {
            // 查找 '=' 和 '&' 的位置
            const size_t key_start = pos;
            const size_t equal_pos = m_body.find('=', key_start);
            if (equal_pos == std::string::npos) { break; }

            const size_t key_end = equal_pos;
            const size_t value_start = equal_pos + 1;
            size_t amp_pos = m_body.find('&', value_start);

            // 如果没有找到 '&'，将值取到 body 末尾
            if (amp_pos == std::string::npos)
            {
                amp_pos = body_len;
            }

            // 提取并解码 key 和 value
            std::string key = m_body.substr(key_start, key_end - key_start);
            std::string value = base::UrlDecode(m_body.substr(value_start, amp_pos - value_start));
            m_params.insert(std::make_pair(key, value));

            // 更新 pos
            pos = amp_pos + 1; // 跳过 '&' 或直到 body 末尾
        }

        m_parserParamFlag |= 0x2; // 设置标志，表示已解析 body 参数
    }


    void HttpRequest::initCookies()
    {
        if (m_parserParamFlag & 0x4)
        {
            return;
        }

        std::string cookie = getHeader("Cookie");
        if (cookie.empty())
        {
            m_parserParamFlag |= 0x4;
            return;
        }

        size_t pos = 0;
        const size_t cookie_len = cookie.length();

        while (pos < cookie_len)
        {
            // 查找 '=' 和 ';' 的位置
            const size_t key_start = pos;
            const size_t equal_pos = cookie.find('=', key_start);
            if (equal_pos == std::string::npos) { break; }

            const size_t key_end = equal_pos;
            const size_t value_start = equal_pos + 1;
            size_t semicolon_pos = cookie.find(';', value_start);

            // 如果没有找到 ';'，则将值取到 cookie 字符串末尾
            if (semicolon_pos == std::string::npos)
            {
                semicolon_pos = cookie_len;
            }

            // 提取并解码 key 和 value
            std::string key = base::Trim(cookie.substr(key_start, key_end - key_start));
            std::string value = base::UrlDecode(cookie.substr(value_start, semicolon_pos - value_start));
            m_cookies.insert(std::make_pair(key, value));

            // 更新 pos
            pos = semicolon_pos + 1; // 跳过 ';' 或直到 cookie 字符串的末尾
        }

        m_parserParamFlag |= 0x4; // 设置标志，表示已解析 cookies
    }


    HttpResponse::HttpResponse(const uint8_t version, const bool close): m_status(HttpStatus::OK),
                                                                         m_version(version),
                                                                         m_close(close),
                                                                         m_websocket(false)
    {
    }

    HttpStatus HttpResponse::getStatus() const
    {
        return m_status;
    }

    uint8_t HttpResponse::getVersion() const
    {
        return m_version;
    }

    const std::string& HttpResponse::getBody() const
    {
        return m_body;
    }

    const std::string& HttpResponse::getReason() const
    {
        return m_reason;
    }

    const HttpResponse::MapType& HttpResponse::getHeaders() const
    {
        return m_headers;
    }

    void HttpResponse::setStatus(const HttpStatus value)
    {
        m_status = value;
    }

    void HttpResponse::setVersion(const uint8_t value)
    {
        m_version = value;
    }

    void HttpResponse::setBody(const std::string& value)
    {
        m_body = value;
    }

    void HttpResponse::setReason(const std::string& value)
    {
        m_reason = value;
    }

    void HttpResponse::setHeaders(const MapType& value)
    {
        m_headers = value;
    }

    bool HttpResponse::isClose() const
    {
        return m_close;
    }

    void HttpResponse::setClose(const bool value)
    {
        m_close = value;
    }

    bool HttpResponse::isWebsocket() const
    {
        return m_websocket;
    }

    void HttpResponse::setWebsocket(const bool value)
    {
        m_websocket = value;
    }

    std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const
    {
        const auto it = m_headers.find(key);
        return it != m_headers.end() ? it->second : def;
    }

    void HttpResponse::setHeader(const std::string& key, const std::string& value)
    {
        m_headers[key] = value;
    }

    void HttpResponse::delHeader(const std::string& key)
    {
        m_headers.erase(key);
    }

    std::ostream& HttpResponse::dump(std::ostream& os) const
    {
        os << "HTTP/"
            << static_cast<uint32_t>(m_version >> 4)
            << "."
            << static_cast<uint32_t>(m_version & 0x0F)
            << " "
            << static_cast<uint32_t>(m_status)
            << " "
            << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
            << "\r\n";

        for (const auto& [fst, snd] : m_headers)
        {
            if (!m_websocket && strcasecmp(fst.c_str(), "Connection") == 0)
            {
                continue;
            }
            os << fst << ": " << snd << "\r\n";
        }
        for (auto& i : m_cookies)
        {
            os << "Set-Cookie: " << i << "\r\n";
        }
        if (!m_websocket)
        {
            os << "Connection: " << (m_close ? "Close" : "Keep-Alive") << "\r\n";
        }
        if (!m_body.empty())
        {
            os << "Content-Length: " << m_body.size() << "\r\n\r\n"
                << m_body;
        }
        else
        {
            os << "\r\n";
        }
        return os;
    }

    std::string HttpResponse::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    void HttpResponse::setRedirect(const std::string& uri)
    {
        m_status = HttpStatus::FOUND;
        setHeader("Location", uri);
    }

    void HttpResponse::setCookie(const std::string& key, const std::string& value, const time_t expired,
                                 const std::string& path, const std::string& domain, const bool secure)
    {
        std::stringstream ss;
        ss << key << "=" << value;
        if (expired > 0)
        {
            ss << ";expires=" << base::Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
        }
        if (!domain.empty())
        {
            ss << ";domain=" << domain;
        }
        if (!path.empty())
        {
            ss << ";path=" << path;
        }
        if (secure)
        {
            ss << ";secure";
        }
        m_cookies.push_back(ss.str());
    }

    std::ostream& operator<<(std::ostream& os, const HttpRequest& request)
    {
        return request.dump(os);
    }

    std::ostream& operator<<(std::ostream& os, const HttpResponse& response)
    {
        return response.dump(os);
    }
}
