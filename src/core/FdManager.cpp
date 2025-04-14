#include <asm-generic/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mutex>

#include "core/FdManager.h"
#include "base/Log.h"
#include "core/Hook.h"

namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");

    FdContext::FdContext(const int fd) : m_isInit(false)
                                         , m_isSocket(false)
                                         , m_sysNonblock(false)
                                         , m_userNonblock(false)
                                         , m_isClosed(false)
                                         , m_fd(fd)
                                         , m_recvTimeout(std::chrono::milliseconds::max())
                                         , m_sendTimeout(std::chrono::milliseconds::max())
    {
        init();
    }

    FdContext::~FdContext() = default;

    bool FdContext::isInit() const
    {
        return m_isInit;
    }

    bool FdContext::isSocket() const
    {
        return m_isSocket;
    }

    bool FdContext::isClose() const
    {
        return m_isClosed;
    }

    void FdContext::setUserNonblock(const bool value)
    {
        m_userNonblock = value;
    }

    bool FdContext::getUserNonblock() const
    {
        return m_userNonblock;
    }

    void FdContext::setSysNonblock(const bool value)
    {
        m_sysNonblock = value;
    }

    bool FdContext::getSysNonblock() const
    {
        return m_sysNonblock;
    }

    void FdContext::setTimeout(const int type, const uint64_t value)
    {
        if (type == SO_RCVTIMEO)
        {
            m_recvTimeout = std::chrono::milliseconds(value);
        }
        else
        {
            m_sendTimeout = std::chrono::milliseconds(value);
        }
    }

    std::chrono::milliseconds FdContext::getTimeout(const int type) const
    {
        if (type == SO_RCVTIMEO)
        {
            return m_recvTimeout;
        }
        return m_sendTimeout;
    }

    std::string FdContext::toString() const
    {
        std::stringstream ss;
        ss << "File Descriptor (fd): " << m_fd
            << " | Is Socket: " << m_isSocket
            << " | Is Initialized: " << m_isInit
            << " | System Non-blocking: " << m_sysNonblock
            << " | User Non-blocking: " << m_userNonblock
            << " | Is Closed: " << m_isClosed
            << " | Receive Timeout: " << m_recvTimeout.count()
            << " | Send Timeout: " << m_sendTimeout.count();
        return ss.str();
    }

    bool FdContext::init()
    {
        m_recvTimeout = std::chrono::milliseconds::max();
        m_sendTimeout = std::chrono::milliseconds::max();
        struct stat fd_stat{};
        if (-1 == fstat(m_fd, &fd_stat))
        {
            m_isInit = false;
            m_isSocket = false;
        }
        else
        {
            m_isInit = true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }
        if (m_isSocket)
        {
            if (const int flags = fcntl_f(m_fd, F_GETFL, 0); !(flags & O_NONBLOCK))
            {
                fcntl_f(m_fd,F_SETFL, flags | O_NONBLOCK);
            }
            m_sysNonblock = true;
        }
        else
        {
            m_sysNonblock = false;
        }
        m_userNonblock = false;
        m_isClosed = false;
        return m_isInit;
    }

    FdManager::FdManager()
    {
        m_datas.resize(64);
    }

    FdManager::~FdManager() = default;

    std::shared_ptr<FdContext> FdManager::get(int fd, const bool auto_create)
    {
        if (fd == -1)
        {
            return nullptr;
        }
        std::shared_lock lock1(m_mutex);
        if (static_cast<int>(m_datas.size()) <= fd)
        {
            if (auto_create == false)
            {
                LOG_ERROR(g_logger) << "FdManager::get - Failed to retrieve file descriptor. "
                                    << "File descriptor (" << fd << ") is out of range. "
                                    << " | Status: Invalid";
                return nullptr;
            }
        }
        else
        {
            if (m_datas[fd] || !auto_create)
            {
                return m_datas[fd];
            }
        }
        lock1.unlock();
        std::unique_lock lock2(m_mutex);
        auto ctx = std::make_shared<FdContext>(fd);
        if (fd >= static_cast<int>(m_datas.size()))
        {
            m_datas.resize(static_cast<int>(fd * 1.5));
        }
        m_datas[fd] = ctx;
        return ctx;
    }

    void FdManager::del(const int fd)
    {
        std::unique_lock lock1(m_mutex);
        if (fd >= static_cast<int>(m_datas.size()))
        {
            return;
        }
        m_datas[fd].reset();
    }
}
