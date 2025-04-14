#include "net/stream/SocketStream.h"

namespace Gyanis::net::stream
{
    SocketStream::SocketStream(const std::shared_ptr<Socket>& socket, const bool owner): m_socket(socket),
        m_owner(owner)
    {
    }

    SocketStream::~SocketStream()
    {
        if (m_owner && m_socket)
        {
            m_socket->close();
        }
    }

    long SocketStream::read(void* buffer, const size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        return m_socket->recv(buffer, length, 0);
    }

    long SocketStream::read(const std::shared_ptr<ByteArray>& buffer, const size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovecs;
        buffer->getWriteBuffers(iovecs, length);
        const long result = m_socket->recv(&iovecs[0], iovecs.size(), 0);
        if (result > 0)
        {
            buffer->setPosition(buffer->getPosition() + result);
        }
        return result;
    }

    long SocketStream::write(const void* buffer, const size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        return m_socket->send(buffer, length, 0);
    }

    long SocketStream::write(const std::shared_ptr<ByteArray>& buffer, const size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovecs;
        buffer->getReadBuffers(iovecs, length);
        const long result = m_socket->send(&iovecs[0], iovecs.size(), 0);
        if (result > 0)
        {
            buffer->setPosition(buffer->getPosition() + result);
        }
        return result;
    }

    void SocketStream::close()
    {
        if (m_socket)
        {
            m_socket->close();
        }
    }

    std::shared_ptr<Socket> SocketStream::getSocket() const
    {
        return m_socket;
    }

    bool SocketStream::isConnected() const
    {
        return m_socket && m_socket->isConnected();
    }

    std::shared_ptr<Address> SocketStream::getRemoteAddress() const
    {
        if (m_socket)
        {
            return m_socket->getRemoteAddress();
        }
        return nullptr;
    }

    std::shared_ptr<Address> SocketStream::getLocalAddress() const
    {
        if (m_socket)
        {
            return m_socket->getLocalAddress();
        }
        return nullptr;
    }

    std::string SocketStream::getRemoteAddressString() const
    {
        if (const auto address = getRemoteAddress())
        {
            return address->toString();
        }
        return "";
    }

    std::string SocketStream::getLocalAddressString() const
    {
        if (const auto address = getLocalAddress())
        {
            return address->toString();
        }
        return "";
    }
}
