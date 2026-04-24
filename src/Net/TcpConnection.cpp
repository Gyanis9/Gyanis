#include "TcpConnection.h"

namespace Net
{
    TcpConnection::TcpConnection(Socket &&sock) noexcept : m_socket(std::move(sock))
    {
    }

    TcpConnection::~TcpConnection()
    {
        close();
    }

    void TcpConnection::close() noexcept
    {
        m_socket.close();
    }

    Core::socket_t TcpConnection::nativeHandle() const noexcept
    {
        return m_socket.nativeHandle();
    }

    bool TcpConnection::isValid() const noexcept
    {
        return m_socket.isValid();
    }

    Core::Task<size_t> TcpConnection::asyncReadSome(Core::ExecutionContext &ctx, const Core::CancellationToken cancel)
    {
        while (true)
        {
            if (cancel.stop_requested())
            {
                throw std::runtime_error("Cancelled");
            }

            auto span = m_readBuf.reservePrepare(Buffer::BLOCK_SIZE);
            ssize_t n = m_socket.recv(span.data(), span.size());
            if (n > 0)
            {
                m_readBuf.commit(n);
                co_return n;
            }
            if (n == 0)
            {
                co_return 0;
            }
            if (!Core::wouldBlock())
            {
                throw std::system_error(Socket::lastError(), std::generic_category(), "recv error");
            }
            co_await Core::asyncRead(m_socket.nativeHandle());
        }
    }

    Core::Task<size_t> TcpConnection::asyncRead(Core::ExecutionContext &ctx, void *dest, const size_t len, const Core::CancellationToken cancel)
    {
        size_t total = 0;
        while (total < len)
        {
            // 先从已有缓冲区消费
            if (m_readBuf.readableSize() > 0)
            {
                const size_t consumed = (std::min)(len - total, m_readBuf.readableSize());
                m_readBuf.read(static_cast<uint8_t *>(dest) + total, consumed);
                total += consumed;
                continue;
            }
            // 网络 I/O
            if (const auto n = co_await asyncReadSome(ctx, cancel); n == 0)
            {
                break; // EOF
            }
        }
        co_return total;
    }

    Core::Task<std::string> TcpConnection::asyncReadUntil(Core::ExecutionContext &ctx, const std::string_view delimiter, const Core::CancellationToken cancel)
    {
        while (true)
        {
            if (const auto pos = m_readBuf.find(delimiter))
            {
                const size_t len = *pos + delimiter.size();
                std::string result(len, '\0');
                m_readBuf.read(result.data(), len);
                co_return result;
            }
            if (const auto n = co_await asyncReadSome(ctx, cancel); n == 0)
            {
                co_return std::string{};
            }
        }
    }

    Core::Task<size_t> TcpConnection::asyncWrite(Core::ExecutionContext &ctx, const void *data, const size_t len, Core::CancellationToken cancel) const
    {
        size_t total = 0;
        while (total < len)
        {
            if (cancel.stop_requested())
            {
                throw std::runtime_error("Cancelled");
            }
            if (const ssize_t n = m_socket.send(static_cast<const uint8_t *>(data) + total, len - total); n >= 0)
            {
                total += n;
                continue;
            }
            if (!Core::wouldBlock())
            {
                throw std::system_error(Socket::lastError(), std::generic_category(), "send error");
            }
            co_await Core::asyncWrite(m_socket.nativeHandle());
        }
        co_return total;
    }

    Core::Task<void> TcpConnection::asyncWriteBuffer(Core::ExecutionContext &ctx, Buffer &output, const Core::CancellationToken cancel) const
    {
        while (output.readableSize() > 0)
        {
            auto view = output.peek();
            const size_t n = co_await asyncWrite(ctx, view.data(), view.size(), cancel);
            output.skip(n);
        }
    }

    std::optional<IpEndpoint> TcpConnection::localEndpoint() const noexcept
    {
        return m_socket.localEndpoint();
    }

    std::optional<IpEndpoint> TcpConnection::remoteEndpoint() const noexcept
    {
        return m_socket.remoteEndpoint();
    }

    Buffer &TcpConnection::inputBuffer() noexcept
    {
        return m_readBuf;
    }
}
