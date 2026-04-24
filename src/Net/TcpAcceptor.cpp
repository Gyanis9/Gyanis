// ReSharper disable CppExpressionWithoutSideEffects
#include "TcpAcceptor.h"

namespace Net
{
    TcpAcceptor::TcpAcceptor(Core::ExecutionContext &ctx, const IpEndpoint &endpoint, bool reusePort) : m_ctx(ctx)
    {
        // 根据地址族创建 IPv4 或 IPv6 TCP socket
        m_listenSocket = Socket(endpoint.address().isV4() ? IpAddress::Family::V4 : IpAddress::Family::V6,SOCK_STREAM);
        m_listenSocket.setReuseAddr(true);   // 允许地址重用
        m_listenSocket.setNonBlocking(true); // 非阻塞模式
        if (!m_listenSocket.bind(endpoint))
        {
            throw std::system_error(Socket::lastError(), std::generic_category(), "bind failed");
        }
        if (!m_listenSocket.listen())
        {
            throw std::system_error(Socket::lastError(), std::generic_category(), "listen failed");
        }
    }

    Core::socket_t TcpAcceptor::nativeHandle() const noexcept
    {
        return m_listenSocket.nativeHandle();
    }

    bool TcpAcceptor::isValid() const noexcept
    {
        return m_listenSocket.isValid();
    }

    std::optional<Socket> TcpAcceptor::nativeAccept() const noexcept
    {
        return m_listenSocket.accept();
    }

    Core::Task<TcpConnection> TcpAcceptor::asyncAccept(Core::CancellationToken cancel) const
    {
        while (true)
        {
            if (auto client = m_listenSocket.accept())
            {
                client->setNonBlocking(true); // 新连接也设为非阻塞
                co_return TcpConnection(std::move(*client));
            }
            // 当 accept 失败且不是 would-block 时，抛出错误
            if (!Core::wouldBlock())
            {
                const auto ec = Socket::lastError();
                throw std::system_error(ec, std::system_category(), "accept failed");
            }
            // 等待监听 socket 可读（新连接到达）
            co_await Core::asyncRead(m_listenSocket.nativeHandle());
        }
    }

    void TcpAcceptor::close() noexcept
    {
        m_listenSocket.close();
    }
}
