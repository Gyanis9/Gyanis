#include "HttpServer.h"
#include "HttpParser.h"
#include "HttpSerializer.h"

namespace Net
{
    Http::HttpServer::HttpServer(Core::ExecutionContext &ctx, const IpEndpoint &endpoint, std::shared_ptr<Router> router, size_t maxConns) : m_ctx(ctx), m_router(std::move(router))
    {
        m_tcpServer = std::make_unique<TcpServer>(ctx, endpoint,
                                                  [this](TcpConnection conn) -> Core::Task<void>
                                                  {
                                                      co_await handleSession(std::move(conn));
                                                  }, maxConns);
    }

    void Http::HttpServer::start() const
    {
        m_tcpServer->start();
    }

    void Http::HttpServer::stop() const
    {
        m_tcpServer->stop();
    }

    Core::Task<void> Http::HttpServer::handleSession(TcpConnection conn) const
    {
        HttpRequestParser parser;
        HttpResponseSerializer serializer;

        while (true)
        {
            // 异步读取数据
            try
            {
                if (size_t n = co_await conn.asyncReadSome(m_ctx); n == 0)
                {
                    break;
                }
            } catch (...)
            {
                break;
            }

            // 尝试解析请求（可能已解析多个）
            while (true)
            {
                auto reqOpt = parser.parse(conn.inputBuffer());
                if (!reqOpt)
                {
                    break;
                }

                HttpRequest req = std::move(*reqOpt);
                HttpResponse res;

                // 执行中间件链与路由
                if (auto handler = m_router->match(req.method, req.uri))
                {
                    try
                    {
                        res = co_await handler(req);
                    } catch (const std::exception &e)
                    {
                        res.setStatus(500);
                        res.setBody(std::string("Internal Server Error: ") + e.what());
                    }
                } else
                {
                    res.setStatus(404);
                    res.setBody("Not Found");
                }

                // 序列化并发送
                Buffer output;
                serializer.serialize(res, output);
                co_await conn.asyncWriteBuffer(m_ctx, output);

                // 判断是否保持连接
                bool keepAlive = (req.httpMajor == 1 && req.httpMinor == 1);
                if (auto connHeader = req.headers.get("connection"))
                {
                    std::string lower(*connHeader);
                    std::ranges::transform(lower, lower.begin(), ::tolower);
                    if (lower == "close")
                    {
                        keepAlive = false;
                    }
                }
                if (!keepAlive)
                {
                    conn.close();
                    co_return;
                }
            }
        }
        conn.close();
    }
}
