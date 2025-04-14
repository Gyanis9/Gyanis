#include "core/Scheduler.h"
#include "base/Utils.h"
#include "base/Macro.h"
#include "core/Hook.h"
#include "base/Log.h"


namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");
    static thread_local Scheduler* current_scheduler = nullptr;

    Scheduler::Scheduler(const size_t threadCount, std::string name) : m_threadCount(threadCount),
                                                                       m_name(std::move(name))
    {
    }

    Scheduler::~Scheduler() = default;

    const std::string& Scheduler::getName() const
    {
        return m_name;
    }

    void Scheduler::start()
    {
        std::scoped_lock lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads.emplace_back(std::make_unique<std::thread>(&Scheduler::run, this));
        }
    }

    void Scheduler::stop()
    {
        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }

        std::vector<std::unique_ptr<std::thread>> threads;
        {
            std::unique_lock lock(m_mutex);
            threads.swap(m_threads);
        }
        for (const auto& thread : threads)
        {
            thread->join();
        }
    }

    void Scheduler::tickle()
    {
        LOG_DEBUG(g_logger)
            << "Scheduler::tickle - Tickle event triggered. "
            << " | Status: Normal";
    }

    void Scheduler::run()
    {
        set_hook_enable(true);
        setThis();
        const auto& idle_fiber = std::make_shared<Fiber>([this] { this->idle(); });
        std::shared_ptr<Fiber> cb_fiber = nullptr;
        FiberAndThread ft;
        while (true)
        {
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                std::lock_guard lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    if (it->thread != -1 && it->thread != base::GetThreadID())
                    {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    ASSERT(it->fiber || it->cb);
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC)
                    {
                        ++it;
                        continue;
                    }

                    ft = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
                tickle_me |= it != m_fibers.end();
            }
            if (tickle_me)
            {
                tickle();
            }
            if (ft.fiber && ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
            {
                ft.fiber->resume();
                --m_activeThreadCount;
                if (ft.fiber->getState() == Fiber::READY)
                {
                    schedule(ft.fiber);
                }
                else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
                {
                    ft.fiber->setHold();
                }
                ft.reset();
            }
            else if (ft.cb)
            {
                if (cb_fiber)
                {
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    cb_fiber = std::make_shared<Fiber>(ft.cb);
                }
                ft.reset();
                cb_fiber->resume();
                --m_activeThreadCount;
                if (cb_fiber->getState() == Fiber::READY)
                {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
                {
                    cb_fiber->reset(nullptr);
                }
                else
                {
                    cb_fiber->setHold();
                    cb_fiber.reset();
                }
            }
            else
            {
                if (is_active)
                {
                    --m_activeThreadCount;
                    continue;
                }
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    LOG_DEBUG(g_logger)
                        << "Scheduler::run - Idle fiber execution completed. "
                        << " | Status: Normal";
                    break;
                }
                ++m_idleThreadCount;
                idle_fiber->resume();
                --m_idleThreadCount;
                if (idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT)
                {
                    idle_fiber->setHold();
                }
            }
        }
    }

    bool Scheduler::stopping()
    {
        std::shared_lock lock(m_mutex);
        return m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle()
    {
        LOG_DEBUG(g_logger)
            << "Scheduler::idle - Idle state reached. "
            << " | Status: Normal";
        while (!stopping())
        {
            Fiber::Yield();
        }
    }

    void Scheduler::setThis()
    {
        current_scheduler = this;
    }

    bool Scheduler::hasIdleThread() const
    {
        return m_idleThreadCount > 0;
    }

    std::string Scheduler::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    Scheduler* Scheduler::GetThis()
    {
        return current_scheduler;
    }

    Scheduler::FiberAndThread::FiberAndThread(std::shared_ptr<Fiber> fiber, const int thr): fiber(std::move(fiber)),
        thread(thr)
    {
    }

    Scheduler::FiberAndThread::FiberAndThread(std::shared_ptr<Fiber>* f, const int thr): thread(thr)
    {
        fiber.swap(*f);
    }

    Scheduler::FiberAndThread::FiberAndThread(std::function<void()> fiber, const int thr): cb(std::move(fiber)),
        thread(thr)
    {
    }

    Scheduler::FiberAndThread::FiberAndThread(std::function<void()>* fiber, const int thr): thread(thr)
    {
        cb.swap(*fiber);
    }

    Scheduler::FiberAndThread::FiberAndThread(): thread(-1)
    {
    }

    void Scheduler::FiberAndThread::reset()
    {
        fiber = nullptr;
        cb = nullptr;
        thread = -1;
    }

    void Scheduler::switchTo(const int thread)
    {
        ASSERT(Scheduler::GetThis() != nullptr);
        if (GetThis() == this)
        {
            if (thread == -1 || thread == base::GetThreadID())
            {
                return;
            }
        }
        schedule(Fiber::GetThis(), thread);
        Fiber::Yield();
    }

    std::ostream& Scheduler::dump(std::ostream& os) const
    {
        os << "[Scheduler Info - "
            << "Name: " << m_name
            << " | Total Threads: " << m_threadCount
            << " | Active Threads: " << m_activeThreadCount
            << " | Idle Threads: " << m_idleThreadCount
            << " | Stopping: " << m_stopping
            << " ]";
        return os;
    }

    FiberSemaphore::FiberSemaphore(const size_t initial_concurrency) : m_concurrency(initial_concurrency)
    {
    }

    FiberSemaphore::~FiberSemaphore()
    {
        ASSERT(m_waiters.empty());
    }

    bool FiberSemaphore::tryWait()
    {
        ASSERT(Scheduler::GetThis());
        {
            std::scoped_lock lock(m_mutex);
            if (m_concurrency > 0u)
            {
                --m_concurrency;
                return true;
            }
            return false;
        }
    }

    void FiberSemaphore::wait()
    {
        ASSERT(Scheduler::GetThis());
        {
            std::scoped_lock lock(m_mutex);
            if (m_concurrency > 0u)
            {
                --m_concurrency;
                return;
            }
            m_waiters.emplace_back(Scheduler::GetThis(), Fiber::GetThis());
        }
        Fiber::Yield();
    }

    void FiberSemaphore::notify()
    {
        std::scoped_lock lock(m_mutex);
        if (!m_waiters.empty())
        {
            const auto [first, second] = m_waiters.front();
            m_waiters.pop_front();
            first->schedule(second);
        }
        else
        {
            ++m_concurrency;
        }
    }

    size_t FiberSemaphore::getConcurrency() const
    {
        return m_concurrency;
    }

    void FiberSemaphore::reset()
    {
        m_concurrency = 0;
    }

    SchedulerSwitcher::SchedulerSwitcher(Scheduler* target)
    {
        m_caller = Scheduler::GetThis();
        if (target)
        {
            target->switchTo();
        }
    }

    SchedulerSwitcher::~SchedulerSwitcher()
    {
        if (m_caller)
        {
            m_caller->switchTo();
        }
    }
}
