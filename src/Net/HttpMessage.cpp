#include "HttpMessage.h"

#include <algorithm>

namespace Net
{
    std::string_view Http::methodString(const HttpMethod m)
    {
        switch (m)
        {
            case HttpMethod::Get: return "GET";
            case HttpMethod::Post: return "POST";
            case HttpMethod::Put: return "PUT";
            case HttpMethod::Delete: return "DELETE";
            case HttpMethod::Head: return "HEAD";
            case HttpMethod::Options: return "OPTIONS";
            case HttpMethod::Patch: return "PATCH";
            case HttpMethod::Connect: return "CONNECT";
            case HttpMethod::Trace: return "TRACE";
        }
        return "UNKNOWN";
    }

    std::optional<Http::HttpMethod> Http::methodFromString(const std::string_view s)
    {
        if (s == "GET")
            return HttpMethod::Get;
        if (s == "POST")
            return HttpMethod::Post;
        if (s == "PUT")
            return HttpMethod::Put;
        if (s == "DELETE")
            return HttpMethod::Delete;
        if (s == "HEAD")
            return HttpMethod::Head;
        if (s == "OPTIONS")
            return HttpMethod::Options;
        if (s == "PATCH")
            return HttpMethod::Patch;
        if (s == "CONNECT")
            return HttpMethod::Connect;
        if (s == "TRACE")
            return HttpMethod::Trace;
        return std::nullopt;
    }

    void Http::HttpHeaderMap::set(const std::string &key, std::string value)
    {
        std::string lower = key;
        std::ranges::transform(lower, lower.begin(), ::tolower);
        m_map[std::move(lower)] = std::move(value);
    }

    std::optional<std::string_view> Http::HttpHeaderMap::get(const std::string &key) const
    {
        std::string lower = key;
        std::ranges::transform(lower, lower.begin(), ::tolower);
        if (const auto it = m_map.find(lower); it != m_map.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    bool Http::HttpHeaderMap::contains(const std::string &key) const
    {
        std::string lower = key;
        std::ranges::transform(lower, lower.begin(), ::tolower);
        return m_map.contains(lower);
    }

    auto Http::HttpHeaderMap::begin() const
    {
        return m_map.begin();
    }

    auto Http::HttpHeaderMap::end() const
    {
        return m_map.end();
    }

    Http::HttpResponse::HttpResponse()
    {
        setStatus(200);
    }

    void Http::HttpResponse::setStatus(const int code)
    {
        statusCode = code;
        switch (code)
        {
            case 200: statusMessage = "OK";
                break;
            case 201: statusMessage = "Created";
                break;
            case 204: statusMessage = "No Content";
                break;
            case 400: statusMessage = "Bad Request";
                break;
            case 404: statusMessage = "Not Found";
                break;
            case 500: statusMessage = "Internal Server Error";
                break;
            default: statusMessage = "Unknown";
                break;
        }
    }

    void Http::HttpResponse::setBody(std::string b)
    {
        body = std::move(b);
        headers.set("Content-Length", std::to_string(body.size()));
    }
} // Net
