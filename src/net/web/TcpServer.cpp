#include "net/web/TcpServer.h"
#include "base/Log.h"

#include <sstream>


namespace Gyanis::net::web
{
    static auto g_logger = LOG_NAME("system");
    auto g_tcp_server_read_timeout = base::Config::LookUp<uint64_t>("tcp_server.read_timeout",
                                                                    60 * 1000 * 2,
                                                                    "tcp server read timeout");

    TcpServer::TcpServer(core::IOManager* worker, core::IOManager* io_worker, core::IOManager* accept_worker):
        m_worker(worker), m_ioWorker(io_worker), m_acceptWorker(accept_worker),
        m_recvTimeout(g_tcp_server_read_timeout->getValue()), m_name("Gyanis/1.0.0"), m_isStop(true)
    {
    }

    TcpServer::~TcpServer()
    {
        for (const auto& i : m_sockets)
        {
            i->close();
        }
        m_sockets.clear();
    }

    bool TcpServer::bind(const std::shared_ptr<Address>& address, const bool ssl)
    {
        std::vector<std::shared_ptr<Address>> addresses;
        std::vector<std::shared_ptr<Address>> fails;
        addresses.push_back(address);
        return bind(addresses, fails, ssl);
    }

    bool TcpServer::bind(const std::vector<std::shared_ptr<Address>>& addresses,
                         std::vector<std::shared_ptr<Address>>& fails, const bool ssl)
    {
        m_ssl = ssl;
        for (auto& address : addresses)
        {
            auto sock = ssl ? SSLSocket::CreateTCP(address) : Socket::CreateTCP(address);
            if (!sock->bind(address))
            {
                LOG_ERROR(g_logger)
                    << "TCPServer::bind - failed. "
                    << "Error code: " << errno
                    << " | Error description: " << strerror(errno)
                    << " | Address: " << address->toString();
                fails.push_back(address);
                continue;
            }
            if (!sock->listen(SOMAXCONN))
            {
                LOG_ERROR(g_logger)
                    << "TCPServer::bind - failed. "
                    << "Error code: " << errno
                    << " | Error description: " << strerror(errno)
                    << " | Address: " << address->toString();
                fails.push_back(address);
                continue;
            }
            m_sockets.push_back(sock);
        }
        if (!fails.empty())
        {
            m_sockets.clear();
            return false;
        }
        return true;
    }

    bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file) const
    {
        return std::all_of(m_sockets.begin(), m_sockets.end(),
                           [&cert_file, &key_file](const auto& i)
                           {
                               if (const auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i))
                               {
                                   return ssl_socket->loadCertificates(cert_file, key_file);
                               }
                               return true; // 如果不是 SSLSocket 类型，返回 true，继续检查其他元素
                           });
    }

    bool TcpServer::start()
    {
        if (!m_isStop)
        {
            return true;
        }
        m_isStop = false;
        for (auto& sock : m_sockets)
        {
            m_acceptWorker->schedule([capture0 = shared_from_this(), sock] { capture0->startAccept(sock); });
        }
        return true;
    }

    void TcpServer::stop()
    {
        m_isStop = true;
        auto self = shared_from_this();
        m_acceptWorker->schedule([this]()-> void
        {
            for (const auto& sock : m_sockets)
            {
                if (const bool flag = sock->cancelAll(); !flag)
                {
                    LOG_ERROR(g_logger)
                        << "TCPServer::stop - failed. "
                        << "Not all socket events have been canceled.";
                }
                sock->close();
            }
            m_sockets.clear();
        });
    }

    std::chrono::milliseconds TcpServer::getRecvTimeout() const
    {
        return m_recvTimeout;
    }

    void TcpServer::setRecvTimeout(const std::chrono::milliseconds timeout)
    {
        m_recvTimeout = timeout;
    }

    bool TcpServer::isStop() const
    {
        return m_isStop;
    }

    std::shared_ptr<base::TcpServerConf> TcpServer::getConf() const
    {
        return m_conf;
    }

    void TcpServer::setConf(const std::shared_ptr<base::TcpServerConf>& value)
    {
        m_conf = value;
    }

    void TcpServer::setConf(const base::TcpServerConf& value)
    {
        m_conf = std::make_shared<base::TcpServerConf>(value);
    }

    std::string TcpServer::toString(const std::string& prefix)
    {
        std::stringstream ss;
        ss << prefix << "[Type: " << m_type
            << " | Name: " << m_name
            << " | SSL enabled: " << m_ssl
            << " | Worker: " << (m_worker ? m_worker->getName() : "N/A")
            << " | Accept Worker: " << (m_acceptWorker ? m_acceptWorker->getName() : "N/A")
            << " | Receive Timeout: " << m_recvTimeout.count() << " seconds]"
            << std::endl;
        const std::string pfx = prefix.empty() ? "    " : prefix;
        for (const auto& i : m_sockets)
        {
            ss << pfx << pfx << i->toString() << std::endl;
        }
        return ss.str();
    }

    std::vector<std::shared_ptr<Socket>> TcpServer::getSockets() const
    {
        return m_sockets;
    }

    std::string TcpServer::getName() const
    {
        return m_name;
    }

    void TcpServer::setName(const std::string& value)
    {
        m_name = value;
    }

    void TcpServer::handleClient(const std::shared_ptr<Socket>& client)
    {
        LOG_DEBUG(g_logger)
            << "TcpServer::handleClient - Handling client connection. "
            << "Client details: " << client->toString();
    }

    void TcpServer::startAccept(const std::shared_ptr<Socket>& sock)
    {
        while (!m_isStop)
        {
            // LOG_INFO(g_logger) << "TcpServer::start() Accept info:" << core::IOManager::GetThis()->toString();
            if (auto client = sock->accept(); client)
            {
                client->setRecvTimeout(m_recvTimeout.count());
                m_ioWorker->schedule([capture0 = shared_from_this(), client] { capture0->handleClient(client); });
            }
            else
            {
                LOG_ERROR(g_logger)
                    << "TcpServer::startAccept - failed. "
                    << "Error code: " << errno
                    << " | Error description: " << strerror(errno);
            }
        }
    }
}
