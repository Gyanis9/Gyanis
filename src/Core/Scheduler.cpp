#include "Scheduler.hpp"

namespace Core
{
    IoScheduler::IoScheduler(PriorityThreadPool &thread_pool)
        : IoScheduler(thread_pool, createPoller(), createTimerService())
    {
    }

    IoScheduler::IoScheduler(PriorityThreadPool &thread_pool, std::unique_ptr<Poller> poller, std::unique_ptr<TimerService> timer_service)
        : m_thread_pool(thread_pool),
          m_poller(std::move(poller)),
          m_timer_service(std::move(timer_service)),
          m_running(false)
    {
        if (const socket_t timer_fd = m_timer_service->getNotifierFd(); timer_fd != INVALID_SOCKET_VAL)
        {
            m_poller->add(timer_fd, PollEvent::Read, this);
        }
    }

    IoScheduler::~IoScheduler()
    {
        stop();
    }

    void IoScheduler::start()
    {
        m_running = true;
        m_io_thread = std::thread([this]
        {
            ioLoop();
        });
    }

    void IoScheduler::stop()
    {
        if (m_running.exchange(false))
        {
            m_poller->wake();
            if (m_io_thread.joinable())
            {
                m_io_thread.join();
            }
        }
    }

    void IoScheduler::schedule(std::function<void()> task, const TaskPriority priority) const
    {
        m_thread_pool.submit(priority, std::move(task));
    }

    void IoScheduler::watch(const socket_t fd, const PollEvent events, IoCallback callback, void *user_data)
    {
        std::lock_guard lock(m_mutex);
        if (const auto it = m_callbacks.find(fd); it == m_callbacks.end())
        {
            CallbackEntry entry{std::move(callback), events, user_data};
            m_callbacks[fd] = std::move(entry);
            m_poller->add(fd, events, reinterpret_cast<void *>(fd));
        } else
        {
            it->second.callback = std::move(callback);
            it->second.events = events;
            it->second.user_data = user_data;
            m_poller->modify(fd, events, reinterpret_cast<void *>(fd));
        }
    }

    void IoScheduler::unwatch(const socket_t fd)
    {
        std::lock_guard lock(m_mutex);
        if (m_callbacks.erase(fd))
        {
            m_poller->remove(fd);
        }
    }

    void IoScheduler::postDelayed(const milliseconds delay, std::function<void()> task, TaskPriority priority) const
    {
        m_timer_service->scheduleAfter(delay, [this, task = std::move(task), priority]
        {
            schedule(std::move(task), priority);
        });
    }

    ExecutionContext IoScheduler::getExecutionContext()
    {
        return ExecutionContext(this, &m_thread_pool);
    }

    void IoScheduler::ioLoop()
    {
        while (m_running)
        {
            for (auto results = m_poller->poll(100); const auto &res: results)
            {
                if (res.user_data == this)
                {
                    m_timer_service->processExpired();
                    continue;
                }
                socket_t fd = static_cast<socket_t>(reinterpret_cast<intptr_t>(res.user_data));
                IoCallback cb;
                void *ud = nullptr;
                {
                    std::lock_guard lock(m_mutex);
                    if (auto it = m_callbacks.find(fd); it != m_callbacks.end())
                    {
                        cb = it->second.callback;
                        ud = it->second.user_data;
                        m_callbacks.erase(it);
                        m_poller->remove(fd);
                    }
                }
                if (cb)
                {
                    m_thread_pool.submit(TaskPriority::High, [cb = std::move(cb), events = res.events, ud]
                    {
                        cb(events, ud);
                    });
                }
            }
        }
    }
}
