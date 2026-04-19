#include <mutex>

#include "base/Timer.h"
#include "base/Log.h"


namespace Gyanis::base
{
    Timer::Timer(const uint64_t                  id,
                 const std::chrono::milliseconds interval,
                 std::function<void()>           callback,
                 const bool                      recurring) : id(id),
                                         expired(std::chrono::high_resolution_clock::now() + interval),
                                         interval(interval),
                                         callback(std::move(callback)),
                                         recurring(recurring)
    {
    }

    bool Timer::Comparator::operator()(const std::shared_ptr<Timer> &lhs, const std::shared_ptr<Timer> &rhs) const
    {
        return lhs->expired < rhs->expired;
    }

    TimerManager::TimerManager() : m_last_check_time(std::chrono::high_resolution_clock::now())
    {
    }

    TimerManager::~TimerManager() = default;

    uint64_t TimerManager::addTimer(const uint64_t interval, const std::function<void()> &callback, bool recurring)
    {
        const auto timer = std::make_shared<Timer>(++cur_id, std::chrono::milliseconds(interval), callback, recurring);
        return addTimerInternal(timer);
    }

    uint64_t TimerManager::addConditionTimer(const uint64_t      interval, std::function<void()> callback,
                                             std::weak_ptr<void> weak_cond, bool                 recurring)
    {
        auto wrapped = [condition = std::move(weak_cond), cb = std::move(callback)]
        {
            if (std::shared_ptr<void> temp = condition.lock())
            {
                cb();
            }
        };
        const auto timer = std::make_shared<Timer>(++cur_id, std::chrono::milliseconds(interval), wrapped, recurring);
        return addTimerInternal(timer);
    }

    bool TimerManager::cancel(const uint64_t id)
    {
        std::unique_lock lock(m_mutex);
        const auto       it = m_timer_map.find(id);
        if (it == m_timer_map.end())
        {
            return false;
        }
        m_timers.erase(it->second);
        m_timer_map.erase(it);
        return true;
    }

    bool TimerManager::refresh(const uint64_t id)
    {
        const auto it = m_timer_map.find(id);
        if (it == m_timer_map.end())
            return false;

        const auto timer = *(it->second);
        m_timers.erase(it->second);
        timer->expired    = std::chrono::high_resolution_clock::now() + timer->interval;
        const auto new_it = m_timers.insert(timer);
        m_timer_map[id]   = new_it;

        if (new_it == m_timers.begin() && (!m_tickled))
        {
            m_tickled = true;
            onTimerInsertedAtFront();
        }
        return true;
    }

    bool TimerManager::reset(const uint64_t id, const uint64_t interval, const bool from_now)
    {
        std::unique_lock lock(m_mutex);
        const auto       it = m_timer_map.find(id);
        if (it == m_timer_map.end())
            return false;

        const auto timer = *(it->second);
        m_timers.erase(it->second);

        if (from_now)
        {
            timer->expired = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(interval);
        } else
        {
            timer->expired += std::chrono::milliseconds(interval);
        }

        const auto new_it = m_timers.insert(timer);
        m_timer_map[id]   = new_it;

        if (new_it == m_timers.begin() && (!m_tickled))
        {
            m_tickled = true;
            onTimerInsertedAtFront();
        }
        return true;
    }

    std::chrono::milliseconds TimerManager::getNextTimer()
    {
        std::shared_lock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty())
            return std::chrono::milliseconds::max();
        const auto now = std::chrono::high_resolution_clock::now();
        if (const auto duration = (*m_timers.begin())->expired - now; duration > std::chrono::milliseconds(0))
            return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        return std::chrono::milliseconds::zero();
    }

    void TimerManager::ListExpiredCb(std::vector<std::function<void()> > &callbacks)
    {
        checkTimeRollback();
        const auto now = std::chrono::high_resolution_clock::now();

        std::unique_lock lock(m_mutex);
        while (!m_timers.empty())
        {
            auto it    = m_timers.begin();
            auto timer = *it;

            if (timer->expired > now)
                break;

            callbacks.push_back(timer->callback);
            m_timers.erase(it);

            if (timer->recurring)
            {
                timer->expired         = now + timer->interval;
                const auto new_it      = m_timers.insert(timer);
                m_timer_map[timer->id] = new_it;
            } else
            {
                m_timer_map.erase(timer->id);
            }
        }
    }

    bool TimerManager::hasTimer()
    {
        std::shared_lock lock(m_mutex);
        return !m_timers.empty();
    }

    void TimerManager::handleTimeRollback()
    {
        const auto                           now = std::chrono::high_resolution_clock::now();
        std::vector<std::shared_ptr<Timer> > affected_timers;

        for (auto it = m_timers.begin(); it != m_timers.end();)
        {
            if (auto timer = *it; timer->expired < now)
            {
                affected_timers.push_back(timer);
                it = m_timers.erase(it);
                m_timer_map.erase(timer->id);
            } else
            {
                ++it;
            }
        }

        for (auto &timer: affected_timers)
        {
            timer->expired         = now + timer->interval;
            auto new_it            = m_timers.insert(timer);
            m_timer_map[timer->id] = new_it;

            if (new_it == m_timers.begin() && (!m_tickled))
            {
                m_tickled = true;
                onTimerInsertedAtFront();
            }
        }
    }

    void TimerManager::checkTimeRollback()
    {
        std::unique_lock lock(m_mutex);
        const auto       current = std::chrono::high_resolution_clock::now();
        if (current < m_last_check_time)
        {
            handleTimeRollback();
        }
        m_last_check_time = current;
    }

    uint64_t TimerManager::addTimerInternal(const std::shared_ptr<Timer> &timer)
    {
        std::unique_lock lock(m_mutex);
        const auto       it    = m_timers.insert(timer);
        m_timer_map[timer->id] = it;
        if (it == m_timers.begin() && (!m_tickled))
        {
            m_tickled = true;
            onTimerInsertedAtFront();
        }
        return timer->id;
    }
}
