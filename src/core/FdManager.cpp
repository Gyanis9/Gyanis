#include "core/FdManager.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <utility>

#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#endif

#include "base/Log.h"

namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");

    namespace
    {
#if defined(_WIN32)
#ifndef SO_RCVTIMEO
#define SO_RCVTIMEO 0x1006
#endif
#ifndef SO_SNDTIMEO
#define SO_SNDTIMEO 0x1005
#endif
#endif

        [[nodiscard]] constexpr bool IsInvalidFd(const int fd)
        {
            return std::cmp_less(fd, 0);
        }

        [[nodiscard]] constexpr bool IsReceiveTimeoutType(const int type)
        {
            return type == SO_RCVTIMEO;
        }

        [[nodiscard]] constexpr bool IsSendTimeoutType(const int type)
        {
            return type == SO_SNDTIMEO;
        }

        [[nodiscard]] size_t CalculateGrowth(const size_t current, const int fd)
        {
            const size_t required = static_cast<size_t>(fd) + 1;
            if (current >= required)
            {
                return current;
            }

            const size_t expanded = current == 0 ? 64 : current + current / 2;
            return std::max(expanded, required);
        }

        [[nodiscard]] bool IsSocketDescriptor(const int fd)
        {
#if defined(_WIN32)
            (void) fd;
            return false;
#else
            struct stat fd_stat{};
            if (::fstat(fd, &fd_stat) != 0)
            {
                return false;
            }
            return S_ISSOCK(fd_stat.st_mode);
#endif
        }

        bool MarkSystemNonBlocking(const int fd)
        {
#if defined(_WIN32)
            (void) fd;
            return false;
#else
            const int flags = ::fcntl(fd, F_GETFL, 0);
            if (flags < 0)
            {
                return false;
            }
            if ((flags & O_NONBLOCK) == 0)
            {
                if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0)
                {
                    return false;
                }
            }
            return true;
#endif
        }
    }

    FdContext::FdContext(const int fd) : m_isInit(false)
                                         , m_isSocket(false)
                                         , m_sysNonblock(false)
                                         , m_userNonblock(false)
                                         , m_isClosed(false)
                                         , m_fd(fd)
                                         , m_recvTimeout(std::chrono::milliseconds::max())
                                         , m_sendTimeout(std::chrono::milliseconds::max())
    {
        static_cast<void>(init());
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
        if (IsReceiveTimeoutType(type))
        {
            m_recvTimeout = std::chrono::milliseconds(value);
            return;
        }
        if (!IsSendTimeoutType(type))
        {
            LOG_WARN(g_logger)
                << "[文件描述符管理] setTimeout 收到未知超时类型，type=" << type
                << "，按发送超时处理。";
        }
        m_sendTimeout = std::chrono::milliseconds(value);
    }

    std::chrono::milliseconds FdContext::getTimeout(const int type) const
    {
        if (IsReceiveTimeoutType(type))
        {
            return m_recvTimeout;
        }
        if (!IsSendTimeoutType(type))
        {
            LOG_WARN(g_logger)
                << "[文件描述符管理] getTimeout 收到未知超时类型，type=" << type
                << "，返回发送超时。";
        }
        return m_sendTimeout;
    }

    std::string FdContext::toString() const
    {
        std::stringstream ss;
        ss << "fd=" << m_fd
           << " | 是否Socket=" << m_isSocket
           << " | 是否已初始化=" << m_isInit
           << " | 系统非阻塞=" << m_sysNonblock
           << " | 用户非阻塞=" << m_userNonblock
           << " | 是否关闭=" << m_isClosed
           << " | 接收超时(ms)=" << m_recvTimeout.count()
           << " | 发送超时(ms)=" << m_sendTimeout.count();
        return ss.str();
    }

    bool FdContext::init()
    {
        if (IsInvalidFd(m_fd))
        {
            m_isInit       = false;
            m_isSocket     = false;
            m_sysNonblock  = false;
            m_userNonblock = false;
            m_isClosed     = false;
            return false;
        }

        m_recvTimeout = std::chrono::milliseconds::max();
        m_sendTimeout = std::chrono::milliseconds::max();

#if defined(_WIN32)
        // Windows 下任意整数 fd 可能触发 CRT invalid-parameter 终止，
        // 这里采用安全回退策略：不做系统探测，只维护逻辑状态。
        m_isInit   = true;
        m_isSocket = false;
#else
        struct stat fd_stat{};
        if (::fstat(m_fd, &fd_stat) == -1)
        {
            m_isInit   = false;
            m_isSocket = false;
        }
        else
        {
            m_isInit   = true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode) && IsSocketDescriptor(m_fd);
        }
#endif

        if (m_isSocket)
        {
            m_sysNonblock = MarkSystemNonBlocking(m_fd);
            if (!m_sysNonblock)
            {
                LOG_WARN(g_logger)
                    << "[文件描述符管理] 设置系统非阻塞失败，fd=" << m_fd
                    << "，errno=" << errno
                    << "，错误=" << std::strerror(errno);
            }
        }
        else
        {
            m_sysNonblock = false;
        }

        m_userNonblock = false;
        m_isClosed     = false;
        return m_isInit;
    }

    FdManager::FdManager()
    {
        m_datas.resize(64);
    }

    FdManager::~FdManager() = default;

    std::shared_ptr<FdContext> FdManager::get(int fd, const bool auto_create)
    {
        if (IsInvalidFd(fd))
        {
            return nullptr;
        }

        std::shared_lock lock1(m_mutex);
        if (std::cmp_less(fd, m_datas.size()))
        {
            if (m_datas[fd] || !auto_create)
            {
                return m_datas[fd];
            }
        }
        else if (!auto_create)
        {
            LOG_ERROR(g_logger)
                << "[文件描述符管理] 获取上下文失败：fd 越界且不允许自动创建，fd=" << fd
                << "，当前容量=" << m_datas.size();
            return nullptr;
        }

        lock1.unlock();
        std::unique_lock lock2(m_mutex);

        if (std::cmp_greater_equal(fd, m_datas.size()))
        {
            m_datas.resize(CalculateGrowth(m_datas.size(), fd));
        }

        if (!m_datas[fd])
        {
            m_datas[fd] = std::make_shared<FdContext>(fd);
        }
        return m_datas[fd];
    }

    void FdManager::del(const int fd)
    {
        if (IsInvalidFd(fd))
        {
            return;
        }

        std::unique_lock lock1(m_mutex);
        if (std::cmp_greater_equal(fd, m_datas.size()))
        {
            return;
        }
        m_datas[fd].reset();
    }
}
