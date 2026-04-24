#include "HttpParser.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>

namespace Net
{
    std::optional<Http::HttpRequest> Http::HttpRequestParser::parse(Buffer &buffer)
    {
        while (true)
        {
            switch (m_state)
            {
                case State::RequestLine:
                    if (!parseRequestLine(buffer))
                    {
                        return std::nullopt;
                    }
                    m_state = State::Headers;
                    break;
                case State::Headers:
                    if (!parseHeaders(buffer))
                    {
                        return std::nullopt;
                    }
                    // 判断 body 解析模式
                    if (m_request.headers.contains("transfer-encoding") &&
                        *m_request.headers.get("transfer-encoding") == "chunked")
                    {
                        m_state = State::BodyChunked;
                        m_chunkSize = 0;
                        m_chunkState = ChunkState::Size;
                    } else if (m_request.headers.contains("content-length"))
                    {
                        const auto val = m_request.headers.get("content-length");
                        if (auto [ptr, ec] = std::from_chars(val->data(), val->data() + val->size(), m_contentLength); ec != std::errc())
                        {
                            m_state = State::Error;
                            return std::nullopt;
                        }
                        m_state = State::Body;
                    } else
                    {
                        // 无 body
                        m_state = State::Complete;
                        return finalize();
                    }
                    break;
                case State::Body:
                    if (!parseBody(buffer))
                    {
                        return std::nullopt;
                    }
                    return finalize();
                case State::BodyChunked:
                    if (!parseChunkedBody(buffer))
                    {
                        return std::nullopt;
                    }
                    return finalize();
                case State::Complete:
                case State::Error:
                    return std::nullopt;
            }
        }
    }

    bool Http::HttpRequestParser::parseRequestLine(Buffer &buf)
    {
        auto line = extractLine(buf);
        if (!line)
        {
            return false;
        }
        // 手动解析方法 URI 版本，避免 istringstream 不健壮
        const size_t pos1 = line->find(' ');
        if (pos1 == std::string_view::npos)
        {
            m_state = State::Error;
            return false;
        }
        const std::string_view methodStr = line->substr(0, pos1);
        const size_t pos2 = line->find(' ', pos1 + 1);
        if (pos2 == std::string_view::npos)
        {
            m_state = State::Error;
            return false;
        }
        const std::string_view uri = line->substr(pos1 + 1, pos2 - pos1 - 1);
        const std::string_view version = line->substr(pos2 + 1);

        const auto method = methodFromString(methodStr);
        if (!method)
        {
            m_state = State::Error;
            return false;
        }
        m_request.method = *method;
        m_request.uri = std::string(uri);
        if (version == "HTTP/1.0")
        {
            m_request.httpMajor = 1;
            m_request.httpMinor = 0;
        } else if (version == "HTTP/1.1")
        {
            m_request.httpMajor = 1;
            m_request.httpMinor = 1;
        } else
        {
            m_state = State::Error;
            return false;
        }
        return true;
    }

    bool Http::HttpRequestParser::parseHeaders(Buffer &buf)
    {
        while (true)
        {
            auto line = extractLine(buf);
            if (!line)
            {
                return false;
            }
            if (line->empty())
            {
                return true; // 头部结束
            }
            const auto colon = line->find(':');
            if (colon == std::string_view::npos)
            {
                m_state = State::Error;
                return false;
            }
            std::string key(line->substr(0, colon));
            std::string value(line->substr(colon + 1));
            // ltrim / rtrim
            const auto isSpace = [](char c)
            {
                return c == ' ' || c == '\t';
            };
            value.erase(0, std::distance(value.begin(), std::ranges::find_if_not(value, isSpace)));
            value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
            m_request.headers.set(std::move(key), std::move(value));
        }
    }

    bool Http::HttpRequestParser::parseBody(Buffer &buf)
    {
        if (buf.readableSize() < m_contentLength)
        {
            return false;
        }
        m_request.body.resize(m_contentLength);
        buf.read(m_request.body.data(), m_contentLength);
        return true;
    }

    bool Http::HttpRequestParser::parseChunkedBody(Buffer &buf)
    {
        while (true)
        {
            switch (m_chunkState)
            {
                case ChunkState::Size:
                {
                    const auto line = extractLine(buf);
                    if (!line)
                    {
                        return false;
                    }
                    // 分块大小可能包含扩展，但此处简化只读取十六进制数字
                    const char *start = line->data();
                    char *end = nullptr;
                    m_chunkSize = std::strtoul(start, &end, 16);
                    if (end == start)
                    {
                        m_state = State::Error;
                        return false;
                    }
                    if (m_chunkSize == 0)
                    {
                        m_chunkState = ChunkState::Trailer;
                    } else
                    {
                        m_chunkState = ChunkState::Data;
                    }
                    break;
                }
                case ChunkState::Data:
                {
                    if (buf.readableSize() < m_chunkSize + 2)
                    {
                        return false; // +2 for \r\n
                    }
                    m_request.body.append(reinterpret_cast<const char *>(buf.peek().data()), m_chunkSize);
                    buf.skip(m_chunkSize);
                    // 读取 \r\n
                    if (buf.readableSize() < 2)
                    {
                        return false;
                    }
                    buf.skip(2);
                    m_chunkState = ChunkState::Size;
                    break;
                }
                case ChunkState::Trailer:
                {
                    // 读取尾部空行 \r\n，忽略扩展尾部
                    const auto line = extractLine(buf);
                    if (!line)
                    {
                        return false;
                    }
                    if (line->empty())
                    {
                        m_chunkState = ChunkState::End;
                        return true;
                    }
                    // 否则是尾部头，忽略
                    break;
                }
                case ChunkState::End:
                    return true;
            }
        }
    }

    std::optional<std::string_view> Http::HttpRequestParser::extractLine(Buffer &buf)
    {
        const auto pos = buf.find("\r\n");
        if (!pos)
        {
            return std::nullopt;
        }
        const size_t len = *pos;
        if (len == 0)
        {
            buf.skip(2); // 消费空行
            return std::string_view{};
        }
        // 提取一行，不包括 \r\n
        std::string line(len, '\0');
        buf.read(line.data(), len);
        buf.skip(2);
        // 将 line 存入成员变量，以延长生命周期，返回视图
        m_lineBuffer = std::move(line);
        return std::string_view(m_lineBuffer);
    }

    std::optional<Http::HttpRequest> Http::HttpRequestParser::finalize()
    {
        auto req = std::move(m_request);
        reset();
        return req;
    }

    void Http::HttpRequestParser::reset()
    {
        m_request = HttpRequest{};
        m_state = State::RequestLine;
        m_contentLength = 0;
        m_chunkSize = 0;
        m_chunkState = ChunkState::Size;
        m_lineBuffer.clear();
    }
}
