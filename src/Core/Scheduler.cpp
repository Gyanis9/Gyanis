#include "Scheduler.hpp"

namespace Core
{
    IoScheduler::IoScheduler(PriorityThreadPool &thread_pool) : m_thread_pool(thread_pool),
                                                                m_poller(createPoller()),
                                                                m_timer_service(createTimerService()),
                                                                m_running(false)
    {
        // 将 timer notifier fd 注册到 poller
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
            // 向 Poller 注册时，将 fd 指针化作为 user_data 传递
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
                    // 定时器事件：处理到期定时器
                    m_timer_service->processExpired();
                    continue;
                }
                // 根据 fd 查找回调
                socket_t fd = static_cast<socket_t>(reinterpret_cast<intptr_t>(res.user_data));
                IoCallback cb;
                void *ud = nullptr;
                {
                    std::lock_guard lock(m_mutex);
                    if (auto it = m_callbacks.find(fd); it != m_callbacks.end())
                    {
                        cb = it->second.callback;
                        ud = it->second.user_data;
                    }
                }
                if (cb)
                {
                    // 以高优先级在线程池执行回调
                    m_thread_pool.submit(TaskPriority::High, [cb = std::move(cb), events = res.events, ud]
                    {
                        cb(events, ud);
                    });
                }
            }
        }
    }
}
