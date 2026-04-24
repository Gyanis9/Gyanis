#include "Tls.h"

#include <memory>
#include <openssl/bio.h>

namespace Net
{
    TlsContext::TlsContext(const bool serverMode)
    {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        const SSL_METHOD *method = serverMode ? TLS_server_method() : TLS_client_method();
        m_ctx = SSL_CTX_new(method);
        if (!m_ctx)
        {
            throw std::runtime_error("SSL_CTX_new failed");
        }
        SSL_CTX_set_mode(m_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    }

    TlsContext::~TlsContext()
    {
        if (m_ctx)
        {
            SSL_CTX_free(m_ctx);
        }
    }

    SSL_CTX *TlsContext::nativeHandle() const noexcept
    {
        return m_ctx;
    }

    TlsConnection::TlsConnection(TcpConnection &&tcpConn, const TlsContext &ctx, const bool serverMode) : m_tcp(std::move(tcpConn)), m_ssl(nullptr)
    {
        m_ssl = SSL_new(ctx.nativeHandle());
        if (serverMode)
        {
            SSL_set_accept_state(m_ssl);
        } else
        {
            SSL_set_connect_state(m_ssl);
        }

        // 使用内存 BIO 来处理读写，使 SSL 与底层 TCP 异步解耦
        m_rbio = BIO_new(BIO_s_mem());
        m_wbio = BIO_new(BIO_s_mem());
        SSL_set_bio(m_ssl, m_rbio, m_wbio);
    }

    TlsConnection::~TlsConnection()
    {
        if (m_ssl)
        {
            SSL_free(m_ssl);
        }
    }

    Core::Task<void> TlsConnection::asyncHandshake(Core::ExecutionContext &ctx, const Core::CancellationToken cancel)
    {
        while (true)
        {
            const int ret = SSL_do_handshake(m_ssl);
            if (ret == 1)
            {
                break;
            }
            if (const int err = SSL_get_error(m_ssl, ret); err == SSL_ERROR_WANT_READ)
            {
                // 需要从网络读取数据到 rbio
                co_await pumpRead(ctx, cancel);
            } else if (err == SSL_ERROR_WANT_WRITE)
            {
                // 需要将 wbio 数据发送到网络
                co_await flushWrite(ctx, cancel);
            } else
            {
                throw std::runtime_error("TLS handshake failed: " + std::to_string(ERR_get_error()));
            }
        }
    }

    Core::Task<size_t> TlsConnection::asyncRead(Core::ExecutionContext &ctx, void *buf, const size_t len, const Core::CancellationToken cancel)
    {
        while (true)
        {
            int n = SSL_read(m_ssl, buf, static_cast<int>(len));
            if (n > 0)
            {
                co_return n;
            }
            if (const int err = SSL_get_error(m_ssl, n); err == SSL_ERROR_WANT_READ)
            {
                co_await pumpRead(ctx, cancel);
            } else if (err == SSL_ERROR_WANT_WRITE)
            {
                co_await flushWrite(ctx, cancel);
            } else if (err == SSL_ERROR_ZERO_RETURN)
            {
                co_return 0;
            } else
            {
                throw std::runtime_error("SSL_read error");
            }
        }
    }

    Core::Task<size_t> TlsConnection::asyncWrite(Core::ExecutionContext &ctx, const void *buf, const size_t len, const Core::CancellationToken cancel)
    {
        size_t total = 0;
        while (total < len)
        {
            const int n = SSL_write(m_ssl, static_cast<const char *>(buf) + total, static_cast<int>(len - total));
            if (n > 0)
            {
                total += n;
                // 写入后尽快将加密数据送出
                co_await flushWrite(ctx, cancel);
                continue;
            }
            if (const int err = SSL_get_error(m_ssl, n); err == SSL_ERROR_WANT_WRITE)
            {
                co_await flushWrite(ctx, cancel);
            } else if (err == SSL_ERROR_WANT_READ)
            {
                co_await pumpRead(ctx, cancel);
            } else
            {
                throw std::runtime_error("SSL_write error");
            }
        }
        co_return total;
    }

    Core::Task<void> TlsConnection::pumpRead(Core::ExecutionContext &ctx, const Core::CancellationToken cancel)
    {
        char buf[4096];
        if (const size_t n = co_await m_tcp.asyncRead(ctx, buf, sizeof(buf), cancel); n > 0)
        {
            BIO_write(m_rbio, buf, n);
        } else if (n == 0)
        {
            // 对端关闭，也通知 BIO 没有更多数据
            BIO_set_mem_eof_return(m_rbio, 0);
        }
    }

    Core::Task<void> TlsConnection::flushWrite(Core::ExecutionContext &ctx, const Core::CancellationToken cancel) const
    {
        char buf[4096];
        int pending = BIO_pending(m_wbio);
        while (pending > 0)
        {
            // 注意：原代码中 “min” 应为 “std::min”，这里保持原样，注释提醒。
            int n = BIO_read(m_wbio, buf, min(pending, (int) sizeof(buf)));
            if (n <= 0)
            {
                break;
            }
            co_await m_tcp.asyncWrite(ctx, buf, n, cancel);
            pending = BIO_pending(m_wbio);
        }
    }
}
