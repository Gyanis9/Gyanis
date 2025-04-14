#include "net/http/HttpSession.h"
#include "net/http/HttpParser.h"

namespace Gyanis::net::http
{
    HttpSession::HttpSession(const std::shared_ptr<Socket>& socket, const bool owner): SocketStream(socket, owner)
    {
    }

    std::shared_ptr<HttpRequest> HttpSession::recvRequest()
    {
        const auto parser = std::make_shared<HttpRequestParser>();
        const uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
        std::vector<char> buffer(buff_size);
        char* data = buffer.data();
        long offset = 0;

        // 循环读取数据直到解析完成
        while (true)
        {
            long len = read(data + offset, buff_size - offset);
            if (len <= 0) // 如果读取失败或连接关闭，返回 nullptr
            {
                close();
                return nullptr;
            }

            len += offset; // 更新已读取的长度
            const size_t nparse = parser->execute(data, len);

            if (parser->hasError()) // 如果解析出错，关闭连接并返回 nullptr
            {
                close();
                return nullptr;
            }

            offset = len - nparse; // 更新未解析的数据

            if (offset == static_cast<int>(buff_size)) // 缓冲区已满，连接关闭
            {
                close();
                return nullptr;
            }

            if (parser->isFinished()) // 如果解析完成，跳出循环
            {
                break;
            }
        }

        // 解析请求体内容（如果有）
        if (uint64_t content_length = parser->getContentLength(); content_length > 0)
        {
            std::string body;
            body.resize(content_length);

            uint64_t len = 0;
            if (content_length >= static_cast<uint64_t>(offset)) // 如果缓冲区中的数据足够
            {
                memcpy(&body[0], data, offset);
                len = offset;
            }
            else // 如果需要继续读取数据
            {
                memcpy(&body[0], data, content_length);
                len = content_length;
            }
            content_length -= offset;
            // 如果请求体内容没有完全读取，再从连接中读取
            if (content_length > 0)
            {
                if (const long read_len = readFixSize(&body[len], content_length); read_len <= 0)
                // 如果读取失败，关闭连接并返回 nullptr
                {
                    close();
                    return nullptr;
                }
            }

            parser->getData()->setBody(body); // 设置请求体
        }

        parser->getData()->init(); // 初始化请求数据

        return parser->getData(); // 返回解析后的请求数据
    }


    long HttpSession::sendResponse(const std::shared_ptr<HttpResponse>& response)
    {
        std::stringstream ss;
        ss << *response;
        const std::string data = ss.str();
        return writeFixSize(data.c_str(), data.size());
    }
}
