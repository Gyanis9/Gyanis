#include "ThreadPool.hpp"

namespace Core
{
    bool PrioritizedTask::operator<(const PrioritizedTask &other) const
    {
        if (priority != other.priority)
        {
            // 优先级数值越小越优先，所以使用大于号让 lower numeric 排在前面
            return static_cast<int>(priority) > static_cast<int>(other.priority);
        }
        // 同优先级按入队时间排序，早入队的优先
        return enqueue_time > other.enqueue_time;
    }

    PriorityThreadPool::PriorityThreadPool(const size_t num_threads) : m_stop(false)
    {
        for (size_t i = 0; i < num_threads; ++i)
        {
            m_workers.emplace_back([this]
            {
                workerLoop();
            });
        }
    }

    PriorityThreadPool::~PriorityThreadPool()
    {
        shutdown();
    }

    void PriorityThreadPool::shutdown()
    {
        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto &worker: m_workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    void PriorityThreadPool::workerLoop()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [this]
                {
                    return m_stop || !m_tasks.empty();
                });
                if (m_stop && m_tasks.empty())
                {
                    return;
                }
                task = std::move(m_tasks.top().func);
                m_tasks.pop();
            }
            task();
        }
    }
}
