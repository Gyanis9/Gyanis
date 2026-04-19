#include "core/IOManager.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>

#if defined(__linux__)
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif

#include "base/Log.h"
#include "base/Macro.h"

namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");

    namespace
    {
        [[nodiscard]] constexpr bool IsInvalidFd(const int fd)
        {
            return fd < 0;
        }

        [[nodiscard]] constexpr bool IsSupportedEvent(const IOManager::Event event)
        {
            return event == IOManager::READ || event == IOManager::WRITE;
        }

        [[nodiscard]] size_t GrowContextSize(const size_t current_size, const size_t required_index)
        {
            const size_t min_size = required_index + 1;
            if (current_size >= min_size)
            {
                return current_size;
            }

            const size_t doubled_size = current_size == 0 ? 32 : current_size * 2;
            return std::max(doubled_size, min_size);
        }

#if defined(__linux__)
        [[nodiscard]] const char *EpollCtlToString(const int op)
        {
            switch (op)
            {
                case EPOLL_CTL_ADD:
                    return "EPOLL_CTL_ADD";
                case EPOLL_CTL_MOD:
                    return "EPOLL_CTL_MOD";
                case EPOLL_CTL_DEL:
                    return "EPOLL_CTL_DEL";
                default:
                    return "EPOLL_CTL_UNKNOWN";
            }
        }

        [[nodiscard]] std::string EpollEventsToString(const uint32_t events)
        {
            if (events == 0)
            {
                return "0";
            }

            std::string result;
            auto append_flag = [&result, &events](const uint32_t flag, const char *name)
            {
                if ((events & flag) == 0)
                {
                    return;
                }
                if (!result.empty())
                {
                    result += "|";
                }
                result += name;
            };

            append_flag(EPOLLIN, "EPOLLIN");
            append_flag(EPOLLPRI, "EPOLLPRI");
            append_flag(EPOLLOUT, "EPOLLOUT");
            append_flag(EPOLLRDNORM, "EPOLLRDNORM");
            append_flag(EPOLLRDBAND, "EPOLLRDBAND");
            append_flag(EPOLLWRNORM, "EPOLLWRNORM");
            append_flag(EPOLLWRBAND, "EPOLLWRBAND");
            append_flag(EPOLLMSG, "EPOLLMSG");
            append_flag(EPOLLERR, "EPOLLERR");
            append_flag(EPOLLHUP, "EPOLLHUP");
            append_flag(EPOLLRDHUP, "EPOLLRDHUP");
            append_flag(EPOLLONESHOT, "EPOLLONESHOT");
            append_flag(EPOLLET, "EPOLLET");

            return result;
        }
#endif
    }

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(const Event event)
    {
        switch (event)
        {
            case READ:
                return read;
            case WRITE:
                return write;
            default:
                ASSERT_MSG(false, "Assertion failed in FdContext::getContext().");
        }
        LOG_FATAL(g_logger) << "FdContext::getContext - Failed to get context for event " << event;
        throw std::invalid_argument("getContext invalid event");
    }

    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(const Event event)
    {
        ASSERT(events & event);
        events = static_cast<Event>(events & ~event);

        auto &ctx = getContext(event);
        ASSERT(ctx.scheduler != nullptr);
        if (ctx.cb != nullptr)
        {
            ctx.scheduler->schedule(std::move(ctx.cb));
        }
        else
        {
            ctx.scheduler->schedule(std::move(ctx.fiber));
        }
        resetContext(ctx);
    }

    IOManager::IOManager(const size_t threadCount, const std::string &name)
        : Scheduler(threadCount, name)
    {
#if defined(__linux__)
        m_epFd = epoll_create1(0);
        if (m_epFd < 0)
        {
            LOG_FATAL(g_logger)
                << "[IO管理器] 创建 epoll 失败，errno=" << errno << "，错误=" << strerror(errno);
            throw std::runtime_error("IOManager: epoll_create1 failed");
        }

        m_wakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (m_wakeFd < 0)
        {
            LOG_FATAL(g_logger)
                << "[IO管理器] 创建 eventfd 失败，errno=" << errno << "，错误=" << strerror(errno);
            ::close(m_epFd);
            m_epFd = -1;
            throw std::runtime_error("IOManager: eventfd failed");
        }

        epoll_event wake_event{};
        wake_event.events  = EPOLLIN | EPOLLET;
        wake_event.data.fd = m_wakeFd;
        if (epoll_ctl(m_epFd, EPOLL_CTL_ADD, m_wakeFd, &wake_event) != 0)
        {
            LOG_FATAL(g_logger)
                << "[IO管理器] 注册唤醒事件失败，errno=" << errno << "，错误=" << strerror(errno);
            ::close(m_wakeFd);
            ::close(m_epFd);
            m_wakeFd = -1;
            m_epFd   = -1;
            throw std::runtime_error("IOManager: epoll_ctl add wake fd failed");
        }
#else
        LOG_WARN(g_logger)
            << "[IO管理器] 当前平台未提供 epoll/eventfd，进入回退模式（仅支持定时器与调度）。";
#endif

        contextResize(32);
        start();
    }

    IOManager::~IOManager()
    {
        stop();

#if defined(__linux__)
        if (m_epFd >= 0)
        {
            ::close(m_epFd);
            m_epFd = -1;
        }
        if (m_wakeFd >= 0)
        {
            ::close(m_wakeFd);
            m_wakeFd = -1;
        }
#endif

        for (const auto &fd_context: m_fdContexts)
        {
            delete fd_context;
        }
        m_fdContexts.clear();
    }

    int IOManager::addEvent(const int fd, const Event event, std::function<void()> callback)
    {
        if (UNLIKELY(IsInvalidFd(fd) || !IsSupportedEvent(event)))
        {
            LOG_ERROR(g_logger)
                << "[IO管理器] addEvent 参数非法，fd=" << fd << "，event=" << event;
            return -1;
        }

#if !defined(__linux__)
        (void) callback;
        LOG_ERROR(g_logger)
            << "[IO管理器] Windows 回退模式暂不支持文件描述符事件注册，fd=" << fd << "，event=" << event;
        return -1;
#else
        FdContext *fd_context = nullptr;
        {
            std::shared_lock lock(m_mutex);
            if (static_cast<size_t>(fd) < m_fdContexts.size())
            {
                fd_context = m_fdContexts[fd];
            }
        }

        if (fd_context == nullptr)
        {
            std::unique_lock lock(m_mutex);
            if (static_cast<size_t>(fd) >= m_fdContexts.size())
            {
                contextResize(GrowContextSize(m_fdContexts.size(), static_cast<size_t>(fd)));
            }
            fd_context = m_fdContexts[fd];
        }

        std::scoped_lock lock(fd_context->mutex);
        if (UNLIKELY(fd_context->events & event))
        {
            LOG_ERROR(g_logger)
                << "[IO管理器] addEvent 重复注册事件，fd=" << fd
                << "，event=" << event
                << "，当前事件掩码=" << fd_context->events;
            return -1;
        }

        const auto op = fd_context->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event add_event{};
        add_event.events   = static_cast<uint32_t>(EPOLLET | fd_context->events | event);
        add_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, op, fd, &add_event) != 0)
        {
            LOG_ERROR(g_logger)
                << "[IO管理器] addEvent 调用 epoll_ctl 失败，epfd=" << m_epFd
                << "，操作=" << EpollCtlToString(op)
                << "，fd=" << fd
                << "，事件=" << EpollEventsToString(add_event.events)
                << "，errno=" << errno << "，错误=" << strerror(errno)
                << "，已有事件=" << EpollEventsToString(fd_context->events);
            return -1;
        }

        ++m_pendingEventCount;
        fd_context->events = static_cast<Event>(fd_context->events | event);

        auto &event_ctx = fd_context->getContext(event);
        ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
        event_ctx.scheduler = Scheduler::GetThis();
        if (callback)
        {
            event_ctx.cb.swap(callback);
        }
        else
        {
            event_ctx.fiber = Fiber::GetThis();
            ASSERT_MSG(event_ctx.fiber->getState() == Fiber::EXEC, "IOManager::addEvent() state not is EXEC");
        }

        return 0;
#endif
    }

    bool IOManager::delEvent(const int fd, const Event event)
    {
        if (UNLIKELY(IsInvalidFd(fd) || !IsSupportedEvent(event)))
        {
            return false;
        }

#if !defined(__linux__)
        LOG_WARN(g_logger)
            << "[IO管理器] Windows 回退模式暂不支持 delEvent，fd=" << fd << "，event=" << event;
        return false;
#else
        FdContext *fd_context = nullptr;
        {
            std::shared_lock lock(m_mutex);
            if (static_cast<size_t>(fd) >= m_fdContexts.size())
            {
                return false;
            }
            fd_context = m_fdContexts[fd];
        }

        std::scoped_lock lock(fd_context->mutex);
        if (UNLIKELY(!(fd_context->events & event)))
        {
            return false;
        }

        const auto new_event = static_cast<Event>(fd_context->events & ~event);
        const int op         = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event del_event{};
        del_event.events   = static_cast<uint32_t>(EPOLLET | new_event);
        del_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, op, fd, &del_event) != 0)
        {
            LOG_ERROR(g_logger)
                << "[IO管理器] delEvent 调用 epoll_ctl 失败，epfd=" << m_epFd
                << "，操作=" << EpollCtlToString(op)
                << "，fd=" << fd
                << "，事件=" << EpollEventsToString(del_event.events)
                << "，errno=" << errno << "，错误=" << strerror(errno);
            return false;
        }

        --m_pendingEventCount;
        fd_context->events = new_event;
        auto &event_ctx = fd_context->getContext(event);
        FdContext::resetContext(event_ctx);
        return true;
#endif
    }

    bool IOManager::cancelEvent(const int fd, const Event event)
    {
        if (UNLIKELY(IsInvalidFd(fd) || !IsSupportedEvent(event)))
        {
            return false;
        }

#if !defined(__linux__)
        LOG_WARN(g_logger)
            << "[IO管理器] Windows 回退模式暂不支持 cancelEvent，fd=" << fd << "，event=" << event;
        return false;
#else
        FdContext *fd_context = nullptr;
        {
            std::shared_lock lock(m_mutex);
            if (static_cast<size_t>(fd) >= m_fdContexts.size())
            {
                return false;
            }
            fd_context = m_fdContexts[fd];
        }

        std::scoped_lock lock(fd_context->mutex);
        if (UNLIKELY(!(fd_context->events & event)))
        {
            return false;
        }

        const auto new_event = static_cast<Event>(fd_context->events & ~event);
        const int op         = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event cancel_event{};
        cancel_event.events   = static_cast<uint32_t>(EPOLLET | new_event);
        cancel_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, op, fd, &cancel_event) != 0)
        {
            LOG_ERROR(g_logger)
                << "[IO管理器] cancelEvent 调用 epoll_ctl 失败，epfd=" << m_epFd
                << "，操作=" << EpollCtlToString(op)
                << "，fd=" << fd
                << "，事件=" << EpollEventsToString(cancel_event.events)
                << "，errno=" << errno << "，错误=" << strerror(errno);
            return false;
        }

        fd_context->triggerEvent(event);
        --m_pendingEventCount;
        return true;
#endif
    }

    bool IOManager::cancelAll(const int fd)
    {
        if (UNLIKELY(IsInvalidFd(fd)))
        {
            return false;
        }

#if !defined(__linux__)
        LOG_WARN(g_logger)
            << "[IO管理器] Windows 回退模式暂不支持 cancelAll，fd=" << fd;
        return false;
#else
        FdContext *fd_context = nullptr;
        {
            std::shared_lock lock(m_mutex);
            if (static_cast<size_t>(fd) >= m_fdContexts.size())
            {
                return false;
            }
            fd_context = m_fdContexts[fd];
        }

        std::scoped_lock lock(fd_context->mutex);
        if (!fd_context->events)
        {
            return false;
        }

        epoll_event cancel_event{};
        cancel_event.events   = 0;
        cancel_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, &cancel_event) != 0)
        {
            LOG_ERROR(g_logger)
                << "[IO管理器] cancelAll 调用 epoll_ctl 失败，epfd=" << m_epFd
                << "，操作=" << EpollCtlToString(EPOLL_CTL_DEL)
                << "，fd=" << fd
                << "，事件=" << EpollEventsToString(cancel_event.events)
                << "，errno=" << errno << "，错误=" << strerror(errno);
            return false;
        }

        if (fd_context->events & READ)
        {
            fd_context->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fd_context->events & WRITE)
        {
            fd_context->triggerEvent(WRITE);
            --m_pendingEventCount;
        }
        ASSERT(fd_context->events == 0);
        return true;
#endif
    }

    void IOManager::tickle()
    {
        if (!hasIdleThread())
        {
            return;
        }

#if defined(__linux__)
        constexpr uint64_t value = 1;
        const auto ret = eventfd_write(m_wakeFd, value);
        if (ret != 0 && errno != EAGAIN)
        {
            LOG_ERROR(g_logger)
                << "[IO管理器] 唤醒写入失败，wake_fd=" << m_wakeFd
                << "，errno=" << errno << "，错误=" << strerror(errno);
        }
#else
        {
            std::lock_guard lock(m_wakeMutex);
            m_tickleRequested = true;
        }
        m_wakeCv.notify_all();
#endif
    }

    bool IOManager::stopping()
    {
        std::chrono::milliseconds timeout = std::chrono::milliseconds::zero();
        return stopping(timeout);
    }

    void IOManager::idle()
    {
#if defined(__linux__)
        constexpr int MAX_EVENTS = 1024;
        std::vector<epoll_event> events(static_cast<size_t>(MAX_EVENTS));
        while (true)
        {
            auto next_timeout = std::chrono::milliseconds::zero();
            if (LIKELY(stopping(next_timeout)))
            {
                LOG_INFO(g_logger)
                    << "[IO管理器] idle 检测到停止条件，调度线程准备退出。";
                break;
            }

            int rt = 0;
            do
            {
                static constexpr std::chrono::milliseconds MAX_TIMEOUT{3000};
                if (next_timeout != std::chrono::milliseconds::max())
                {
                    next_timeout = next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                } else
                {
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epFd, events.data(), MAX_EVENTS, static_cast<int>(next_timeout.count()));
                if (rt < 0 && errno == EINTR)
                {
                    continue;
                }
                break;
            } while (true);

            std::vector<std::function<void()> > callbacks;
            ListExpiredCb(callbacks);
            if (!callbacks.empty())
            {
                schedule(callbacks.begin(), callbacks.end());
            }

            for (int i = 0; i < rt; ++i)
            {
                auto &event = events[i];
                if (event.data.fd == m_wakeFd)
                {
                    uint64_t value = 0;
                    while (eventfd_read(m_wakeFd, &value) > 0)
                    {
                    }
                    continue;
                }
                auto *fd_context = static_cast<FdContext *>(event.data.ptr);
                std::scoped_lock lock(fd_context->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_context->events;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }
                if ((fd_context->events & real_events) == NONE)
                {
                    continue;
                }

                const int left_events = fd_context->events & ~real_events;
                const int op          = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events          = static_cast<uint32_t>(EPOLLET | left_events);
                if (epoll_ctl(m_epFd, op, fd_context->fd, &event) != 0)
                {
                    LOG_ERROR(g_logger)
                        << "[IO管理器] idle 更新事件失败，fd=" << fd_context->fd
                        << "，操作=" << EpollCtlToString(op)
                        << "，事件=" << EpollEventsToString(event.events)
                        << "，errno=" << errno << "，错误=" << strerror(errno);
                    continue;
                }

                if (real_events & READ)
                {
                    fd_context->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    fd_context->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            Fiber::Yield();
        }
#else
        while (true)
        {
            auto next_timeout = std::chrono::milliseconds::zero();
            if (LIKELY(stopping(next_timeout)))
            {
                LOG_INFO(g_logger)
                    << "[IO管理器] Windows 回退模式检测到停止条件，调度线程准备退出。";
                break;
            }

            std::vector<std::function<void()> > callbacks;
            ListExpiredCb(callbacks);
            if (!callbacks.empty())
            {
                schedule(callbacks.begin(), callbacks.end());
                Fiber::Yield();
                continue;
            }

            static constexpr std::chrono::milliseconds MAX_TIMEOUT{3000};
            const auto wait_timeout = next_timeout == std::chrono::milliseconds::max()
                                          ? MAX_TIMEOUT
                                          : std::min(next_timeout, MAX_TIMEOUT);

            std::unique_lock lock(m_wakeMutex);
            m_wakeCv.wait_for(lock, wait_timeout, [this]()
            {
                return m_tickleRequested;
            });
            m_tickleRequested = false;
            lock.unlock();

            Fiber::Yield();
        }
#endif
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }

    void IOManager::contextResize(const size_t size)
    {
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                m_fdContexts[i]     = new FdContext;
                m_fdContexts[i]->fd = static_cast<int>(i);
            }
        }
    }

    bool IOManager::stopping(std::chrono::milliseconds &timeout)
    {
        timeout = getNextTimer();
        return timeout == std::chrono::milliseconds::max() && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }
}
