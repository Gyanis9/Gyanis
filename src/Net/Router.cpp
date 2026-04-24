#include "Router.h"

namespace Net
{
    void Http::Router::addRoute(HttpMethod method, std::string path, RequestHandler handler)
    {
        m_routes.emplace_back(method, std::move(path), std::move(handler));
    }

    void Http::Router::addMiddleware(Middleware mw)
    {
        m_middlewares.push_back(std::move(mw));
    }

    Http::RequestHandler Http::Router::match(const HttpMethod method, const std::string &uri)
    {
        RequestHandler handler = nullptr;
        for (auto &[m, path, h]: m_routes)
        {
            if (m == method && matchPath(path, uri))
            {
                handler = h;
                break;
            }
        }
        if (!handler)
        {
            return nullptr;
        }
        // 包装中间件（从后向前）
        for (auto it = m_middlewares.rbegin(); it != m_middlewares.rend(); ++it)
        {
            handler = (*it)(std::move(handler));
        }
        return handler;
    }

    bool Http::Router::matchPath(const std::string &pattern, const std::string &uri)
    {
        // 支持 :param 路径参数，转换为正则匹配，简化版使用 exact 或 /:id 模式
        // 先将 pattern 转换为正则，例如 "/users/:id" -> "^/users/([^/]+)$"
        // 完整实现需要缓存编译过的正则，此处给出一个简单的手动实现：
        std::string regexStr = "^";
        size_t pos = 0;
        while (pos < pattern.size())
        {
            if (pattern[pos] == ':')
            {
                // 找到参数名直到 '/' 或结尾
                size_t end = pattern.find('/', pos);
                if (end == std::string::npos)
                {
                    end = pattern.size();
                }
                regexStr += "([^/]+)";
                pos = end;
            } else
            {
                // 普通字符，转义正则特殊字符
                if (pattern[pos] == '.' || pattern[pos] == '*' || pattern[pos] == '?' ||
                    pattern[pos] == '+' || pattern[pos] == '^' || pattern[pos] == '$' ||
                    pattern[pos] == '(' || pattern[pos] == ')' || pattern[pos] == '[' ||
                    pattern[pos] == ']' || pattern[pos] == '{' || pattern[pos] == '}' ||
                    pattern[pos] == '|' || pattern[pos] == '\\')
                {
                    regexStr += '\\';
                }
                regexStr += pattern[pos];
                ++pos;
            }
        }
        regexStr += "$";
        const std::regex re(regexStr, std::regex::optimize);
        return std::regex_match(uri, re);
    }

    Http::Router::RouteEntry::RouteEntry(const HttpMethod m, std::string p, RequestHandler h) : method(m), path(std::move(p)), handler(std::move(h))
    {
    }
}
