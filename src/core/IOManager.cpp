#include "core/IOManager.h"
#include <sys/epoll.h>
#include "base/Macro.h"
#include <sys/eventfd.h>
#include "base/Log.h"

namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");

    enum EpollCtlOp
    {
    };

    static std::ostream& operator<<(std::ostream& os, const EpollCtlOp& op)
    {
        switch (static_cast<int>(op))
        {
#define XX(ctl) \
case ctl: \
return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << static_cast<int>(op);
        }
#undef XX
    }

    static std::ostream& operator<<(std::ostream& os, const EPOLL_EVENTS events)
    {
        if (!events)
        {
            return os << "0";
        }
        bool first = true;
#define XX(E) \
if(events & E) { \
if(!first) { \
os << "|"; \
} \
os << #E; \
first = false; \
}
        XX(EPOLLIN);
        XX(EPOLLPRI);
        XX(EPOLLOUT);
        XX(EPOLLRDNORM);
        XX(EPOLLRDBAND);
        XX(EPOLLWRNORM);
        XX(EPOLLWRBAND);
        XX(EPOLLMSG);
        XX(EPOLLERR);
        XX(EPOLLHUP);
        XX(EPOLLRDHUP);
        XX(EPOLLONESHOT);
        XX(EPOLLET);
#undef XX
        return os;
    }


    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(const Event event)
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

    void IOManager::FdContext::resetContext(EventContext& ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(const Event event)
    {
        ASSERT(events&event);
        events = static_cast<Event>(events & ~event);
        auto& [scheduler, fiber, cb] = getContext(event);
        if (cb != nullptr)
        {
            scheduler->schedule(&cb);
        }
        else
        {
            scheduler->schedule(&fiber);
        }
        scheduler = nullptr;
    }

    IOManager::IOManager(const size_t threadCount, const std::string& name): Scheduler(threadCount, name),
                                                                             m_epFd(epoll_create(5000))
    {
        m_wakeFd = eventfd(0, EFD_NONBLOCK);
        epoll_event wakeEvent = {};
        wakeEvent.events = EPOLLIN | EPOLLET;
        wakeEvent.data.fd = m_wakeFd;
        epoll_ctl(m_epFd, EPOLL_CTL_ADD, m_wakeFd, &wakeEvent);
        contextResize(32);
        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_epFd);
        close(m_wakeFd);
        for (const auto& m_fdContext : m_fdContexts)
        {
            delete m_fdContext;
        }
    }

    int IOManager::addEvent(const int fd, const Event event, std::function<void()> callback)
    {
        FdContext* fd_context = nullptr;
        std::shared_lock lock1(m_mutex);
        if (static_cast<int>(m_fdContexts.size()) > fd)
        {
            fd_context = m_fdContexts[fd];
            lock1.unlock();
        }
        else
        {
            lock1.unlock();
            std::unique_lock lock2(m_mutex);
            contextResize(static_cast<size_t>(fd * 1.5));
            fd_context = m_fdContexts[fd];
        }
        std::scoped_lock lock3(fd_context->mutex);
        if (UNLIKELY(fd_context->events&event))
        {
            LOG_ERROR(g_logger)
                << "IOManager::addEvent - Assertion failed during addEvent operation. "
                << "File Descriptor (fd): " << fd
                << " | Event: " << event
                << " | fd_context.events: " << fd_context->events
                << " | Status: Invalid";
            ASSERT(!(fd_context->events&event));
        }

        const auto op = fd_context->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event add_event{};
        add_event.events = EPOLLET | fd_context->events | event;
        add_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, op, fd, &add_event))
        {
            LOG_ERROR(g_logger)
                << "IOManager::addEvent - epoll_ctl failed. "
                << "Epoll File Descriptor (epFd): " << m_epFd
                << " | Operation: " << static_cast<EpollCtlOp>(op)
                << " | File Descriptor (fd): " << fd
                << " | Event mask: " << static_cast<EPOLL_EVENTS>(add_event.events)
                << " | Error: (" << errno << ") " << strerror(errno)
                << " | fd_context.events: " << static_cast<EPOLL_EVENTS>(fd_context->events)
                << " | Status: Invalid";
            return -1;
        }
        ++m_pendingEventCount;
        fd_context->events = static_cast<Event>(fd_context->events | event);
        auto& [scheduler, fiber, cb] = fd_context->getContext(event);
        ASSERT(!scheduler&&!fiber&&!cb);
        scheduler = Scheduler::GetThis();
        if (callback)
        {
            cb.swap(callback);
        }
        else
        {
            fiber = Fiber::GetThis();
            ASSERT_MSG(fiber->getState() == Fiber::EXEC, "IOManager::addEvent() state not is EXEC");
        }
        return 0;
    }

    bool IOManager::delEvent(const int fd, const Event event)
    {
        std::shared_lock lock1(m_mutex);
        if (static_cast<int>(m_fdContexts.size()) <= fd)
        {
            return false;
        }
        auto* fd_context = m_fdContexts[fd];
        lock1.unlock();
        std::scoped_lock lock2(fd_context->mutex);
        if (UNLIKELY(!(fd_context->events&event)))
        {
            return false;
        }
        const auto new_event = static_cast<Event>(fd_context->events & ~event);
        const int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event del_event{};
        del_event.events = EPOLLET | new_event;
        del_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, op, fd, &del_event))
        {
            LOG_ERROR(g_logger)
                << "IOManager::delEvent - epoll_ctl failed. "
                << "Epoll File Descriptor (epFd): " << m_epFd
                << " | Operation: " << static_cast<EpollCtlOp>(op)
                << " | File Descriptor (fd): " << fd
                << " | Event mask: " << static_cast<EPOLL_EVENTS>(del_event.events)
                << " | Error: (" << errno << ") " << strerror(errno)
                << " | Status: Invalid";
            return false;
        }
        --m_pendingEventCount;
        fd_context->events = new_event;
        auto& event_ctx = fd_context->getContext(event);
        FdContext::resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(const int fd, const Event event)
    {
        std::shared_lock lock1(m_mutex);
        if (static_cast<int>(m_fdContexts.size()) <= fd)
        {
            return false;
        }
        auto* fd_context = m_fdContexts[fd];
        lock1.unlock();
        std::scoped_lock lock2(fd_context->mutex);
        if (UNLIKELY(!(fd_context->events&event)))
        {
            return false;
        }
        const auto new_event = static_cast<Event>(fd_context->events & ~event);
        const int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event cancel_event{};
        cancel_event.events = EPOLLET | new_event;
        cancel_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, op, fd, &cancel_event))
        {
            LOG_ERROR(g_logger)
                << "IOManager::cancelEvent - epoll_ctl failed. "
                << "Epoll File Descriptor (epFd): " << m_epFd
                << " | Operation: " << static_cast<EpollCtlOp>(op)
                << " | File Descriptor (fd): " << fd
                << " | Event mask: " << static_cast<EPOLL_EVENTS>(cancel_event.events)
                << " | Error: (" << errno << ") " << strerror(errno)
                << " | Status: Invalid";
            return false;
        }

        fd_context->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(const int fd)
    {
        std::shared_lock lock1(m_mutex);
        if (static_cast<int>(m_fdContexts.size()) <= fd)
        {
            return false;
        }
        auto* fd_context = m_fdContexts[fd];
        lock1.unlock();
        std::scoped_lock lock2(fd_context->mutex);
        if (!fd_context->events)
        {
            return false;
        }
        epoll_event cancel_event{};
        cancel_event.events = 0;
        cancel_event.data.ptr = fd_context;
        if (epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, &cancel_event))
        {
            LOG_ERROR(g_logger)
                << "IOManager::cancelAll - epoll_ctl failed. "
                << "Epoll File Descriptor (epFd): " << m_epFd
                << " | Operation: EPOLL_CTL_DEL (" << static_cast<EpollCtlOp>(EPOLL_CTL_DEL) << ")"
                << " | File Descriptor (fd): " << fd
                << " | Event mask: " << static_cast<EPOLL_EVENTS>(cancel_event.events)
                << " | Error: (" << errno << ") " << strerror(errno)
                << " | Status: Invalid";
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
        ASSERT(fd_context->events==0);
        return true;
    }

    void IOManager::tickle()
    {
        if (!hasIdleThread())
        {
            return;
        }
        constexpr uint64_t value = 1;
        const auto ret = eventfd_write(m_wakeFd, value);
        ASSERT(ret ==0);
    }

    bool IOManager::stopping()
    {
        std::chrono::milliseconds timeout = std::chrono::milliseconds::zero();
        return stopping(timeout);
    }

    void IOManager::idle()
    {
        constexpr uint64_t MAX_EVENTS = 1024;
        std::vector<epoll_event> events(MAX_EVENTS);
        while (true)
        {
            auto next_timeout = std::chrono::milliseconds::zero();
            if (LIKELY(stopping(next_timeout)))
            {
                LOG_INFO(g_logger)
                    << "IOManager::idle - IOManager idle timeout reached. "
                    << "Stopping and exiting. "
                    << " | Status: Normal";
                break;
            }
            int rt = 0;
            do
            {
                static constexpr std::chrono::milliseconds MAX_TIMEOUT{3000};
                if (next_timeout != std::chrono::milliseconds::max())
                {
                    next_timeout = next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epFd, events.data(), MAX_EVENTS, static_cast<int>(next_timeout.count()));
                if (rt < 0 && errno == EINTR)
                {
                    continue;
                }
                break;
            }
            while (true);
            std::vector<std::function<void()>> callbacks;
            ListExpiredCb(callbacks);
            if (!callbacks.empty())
            {
                schedule(callbacks.begin(), callbacks.end());
                callbacks.clear();
            }
            for (int i = 0; i < rt; ++i)
            {
                auto& event = events[i];
                if (event.data.fd == m_wakeFd)
                {
                    uint64_t value = 0;
                    while (eventfd_read(m_wakeFd, &value) > 0)
                    {
                    }
                    continue;
                }
                auto* fd_context = static_cast<FdContext*>(event.data.ptr);
                std::scoped_lock lock1(fd_context->mutex);
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
                const int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;
                if (epoll_ctl(m_epFd, op, fd_context->fd, &event))
                {
                    LOG_ERROR(g_logger)
                        << "IOManager::idle - epoll_ctl operation failed. "
                        << "An error occurred while managing the IO events. "
                        << " | Status: Invalid";
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
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = static_cast<int>(i);
            }
        }
    }

    bool IOManager::stopping(std::chrono::milliseconds& timeout)
    {
        timeout = getNextTimer();
        return timeout == std::chrono::milliseconds::max() && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    IOManager* IOManager::GetThis()
    {
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }
}
