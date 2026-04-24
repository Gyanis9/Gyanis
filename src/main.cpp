// #include "Core/Awaitables.hpp"
//
// #include <iostream>
// #include <memory>
// #include <string>
//
// using namespace Core;
//
// // 简单的 Echo 处理协程
// Task<void> handleClient(const socket_t client_fd, const TaskPriority priority)
// {
//     char buffer[4096];
//     // 设置 socket 选项
//     setNoDelay(client_fd, true);
//
//     while (true)
//     {
//         // 检查取消
//         if (co_await schedule(priority); false)
//         {
//         } // 占位，实际取消检查可在每个挂起点进行
//         // 实际取消检查可通过 cancellation_token 实现，此处省略简化
//
//         // 异步读取
//         co_await asyncRead(client_fd);
//         const int n = ::recv(client_fd, buffer, sizeof(buffer), 0);
//         if (n <= 0)
//         {
//             break; // 连接关闭或错误
//         }
//
//         // 模拟计算（通过调度切换优先级）
//         co_await schedule(priority);
//
//         // 异步写入（echo）
//         co_await asyncWrite(client_fd);
//         if (const int sent = ::send(client_fd, buffer, n, 0); sent != n)
//         {
//             break;
//         }
//     }
//     closeSocket(client_fd);
//     std::cout << "Client disconnected\n";
// }
//
// // 接受连接循环
// Task<void> acceptLoop(const socket_t listen_fd, ExecutionContext &ctx)
// {
//     while (true)
//     {
//         co_await asyncRead(listen_fd);
//
//         // Edge-triggered 模式下需耗尽 accept 队列以防漏掉连接
//         while (true)
//         {
//             sockaddr_in addr{};
//             socklen_t len = sizeof(addr);
//             const socket_t client = ::accept(listen_fd, reinterpret_cast<sockaddr *>(&addr), &len);
//             if (client == INVALID_SOCKET_VAL)
//             {
// #ifdef _WIN32
//                 if (wouldBlock() || WSAGetLastError() == WSAEINTR)
//                 {
//                     break; // 本次无可处理连接
//                 }
// #else
//                 if (wouldBlock() || errno == EINTR)
//                 {
//                     break;
//                 }
// #endif
//                 break; // 致命错误，退出 accept 循环
//             }
//             setNonblocking(client, true);
//
//             // 根据客户端端口奇偶分配优先级（示例）
//             TaskPriority prio = (ntohs(addr.sin_port) % 2 == 0) ? TaskPriority::High : TaskPriority::Low;
//
//             char ip_str[INET_ADDRSTRLEN];
//             ::inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
//             std::cout << "New connection from " << ip_str
//                     << ":" << ntohs(addr.sin_port) << " priority=" << static_cast<int>(prio) << std::endl;
//
//             // 启动处理协程（自动继承执行上下文）
//             auto task = std::make_shared<Task<void>>(
//                 handleClient(client, prio).withExecutionContext(ctx));
//             ctx.scheduler()->schedule([task]() mutable
//             {
//                 task->get();
//             }, prio);
//         }
//     }
// }
//
// int main()
// {
//     initNetwork();
//
//     // 创建线程池（4个工作线程）
//     PriorityThreadPool thread_pool(4);
//     IoScheduler scheduler(thread_pool);
//     ExecutionContext ctx(&scheduler, &thread_pool);
//
//     scheduler.start();
//
//     // 创建监听 socket
//     socket_t listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
//     if (listen_fd == INVALID_SOCKET_VAL)
//     {
//         std::cerr << "socket failed\n";
//         return 1;
//     }
//     setNonblocking(listen_fd, true);
//     setReuseAddress(listen_fd, true);
//
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = INADDR_ANY;
//     addr.sin_port = htons(8080);
//     if (::bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR_VAL)
//     {
//         std::cerr << "bind failed\n";
//         closeSocket(listen_fd);
//         return 1;
//     }
//     if (::listen(listen_fd, SOMAXCONN) == SOCKET_ERROR_VAL)
//     {
//         std::cerr << "listen failed\n";
//         closeSocket(listen_fd);
//         return 1;
//     }
//
//     std::cout << "Echo server listening on port 8080\n";
//
//     // 启动接受循环
//     auto acceptor = std::make_shared<Task<void>>(acceptLoop(listen_fd, ctx));
//     ctx.scheduler()->schedule([acceptor]() mutable
//     {
//         acceptor->get();
//     }, TaskPriority::Critical);
//
//     std::cout << "Press Enter to shutdown...\n";
//     std::cin.get();
//
//     // 优雅关闭
//     closeSocket(listen_fd);
//     scheduler.stop();
//     thread_pool.shutdown();
//
//     cleanupNetwork();
//     return 0;
// }


#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>
#include "Net/EventLoopGroup.h"
#include "Net/HttpServer.h"
#include "Net/Router.h"

using namespace Core;
using namespace Net;
using namespace Net::Http;

static std::atomic<bool> g_stopRequested{false};

extern "C" void signalHandler(int)
{
    g_stopRequested = true;
}


int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    initNetwork();

    const EventLoopGroup loopGroup(1, 4); // 1 I/O 线程，4 工作线程
    loopGroup.start();
    auto ctx = loopGroup.getContext(0);

    const auto router = std::make_shared<Router>();

    // 中间件：记录每个请求
    router->addMiddleware([](RequestHandler next)
    {
        return [next = std::move(next)](HttpRequest req) -> Task<HttpResponse>
        {
            const auto start = steady_clock::now();
            auto resp = co_await next(req);
            const auto end = steady_clock::now();
            std::cout << "[" << methodString(req.method) << "] " << req.uri
                    << " -> " << resp.statusCode
                    << " (" << std::chrono::duration_cast<milliseconds>(end - start).count() << "ms)\n";
            co_return resp;
        };
    });

    router->addRoute(HttpMethod::Get, "/", [](HttpRequest req) -> Task<HttpResponse>
    {
        HttpResponse res;
        res.setBody("Hello from production async HTTP server!");
        co_return res;
    });

    router->addRoute(HttpMethod::Get, "/user/:id", [](HttpRequest req) -> Task<HttpResponse>
    {
        HttpResponse res;
        res.setBody(std::format("User ID: {}", req.uri));
        co_return res;
    });

    const HttpServer server(ctx, IpEndpoint(IpAddress::parse("0.0.0.0").value(), 8080), router);
    server.start();

    std::cout << "HTTP server running on 0.0.0.0:8080" << std::endl;
    while (!g_stopRequested)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Shutting down..." << std::endl;
    server.stop();
    loopGroup.stop();
    cleanupNetwork();
    return 0;
}
