#include <chrono>
#include <sstream>
#include <utility>

#include "core/Scheduler.h"
#include "base/Log.h"
#include "base/Macro.h"
#include "base/Utils.h"

namespace Gyanis::core
{
    namespace
    {
        auto                    g_logger          = LOG_NAME("system");
        thread_local Scheduler *current_scheduler = nullptr;

        constexpr int  kAnyThread = -1;
        constexpr auto kIdleSleep = std::chrono::milliseconds(1);
    }

    Scheduler::Scheduler(const size_t threadCount, std::string name)
        : m_threadCount(threadCount == 0 ? 1 : threadCount),
          m_name(std::move(name))
    {
        if (threadCount == 0)
        {
            LOG_WARN(g_logger) << "[调度器] 线程数为 0，已自动调整为 1。";
        }
    }

    Scheduler::~Scheduler() = default;

    const std::string &Scheduler::getName() const
    {
        return m_name;
    }

    void Scheduler::start()
    {
        std::scoped_lock lock(m_mutex);
        if (!m_stopping)
        {
            LOG_WARN(g_logger) << "[调度器] 启动请求被忽略：调度器已在运行。";
            return;
        }

        m_stopping = false;
        m_threads.reserve(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads.emplace_back(std::make_unique<std::thread>(&Scheduler::run, this));
        }

        LOG_INFO(g_logger) << "[调度器] 启动完成：线程数=" << m_threadCount << "。";
    }

    void Scheduler::stop()
    {
        {
            std::scoped_lock lock(m_mutex);
            if (m_stopping && m_threads.empty())
            {
                return;
            }
            m_stopping = true;
        }

        for (size_t i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }

        std::vector<std::unique_ptr<std::thread> > threads;
        {
            std::unique_lock lock(m_mutex);
            threads.swap(m_threads);
        }

        for (const auto &thread: threads)
        {
            if (thread && thread->joinable())
            {
                thread->join();
            }
        }

        LOG_INFO(g_logger) << "[调度器] 停止完成。";
    }

    void Scheduler::tickle()
    {
        LOG_DEBUG(g_logger) << "[调度器] 收到唤醒信号。";
    }

    void Scheduler::run()
    {
        setThis();
        FiberAndThread ft;

        while (true)
        {
            ft.reset();
            bool tickle_me   = false;
            bool picked_task = false;

            {
                std::lock_guard lock(m_mutex);
                auto            it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    if (it->thread != kAnyThread && it->thread != base::GetThreadID())
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
                    picked_task = true;
                    break;
                }

                tickle_me = tickle_me || (it != m_fibers.end());
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
                } else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
                {
                    ft.fiber->setHold();
                }
                ft.reset();
                continue;
            }

            if (ft.cb)
            {
                auto callback_fiber = std::make_shared<Fiber>(Fiber::Callback(std::move(ft.cb)));
                ft.reset();

                callback_fiber->resume();
                --m_activeThreadCount;

                if (callback_fiber->getState() == Fiber::READY)
                {
                    schedule(callback_fiber);
                } else if (callback_fiber->getState() != Fiber::TERM && callback_fiber->getState() != Fiber::EXCEPT)
                {
                    callback_fiber->setHold();
                }
                continue;
            }

            if (picked_task)
            {
                --m_activeThreadCount;
                continue;
            }

            if (stopping())
            {
                LOG_DEBUG(g_logger) << "[调度器] 调度线程退出：检测到停止条件。";
                break;
            }

            ++m_idleThreadCount;
            idle();
            --m_idleThreadCount;
        }
    }

    bool Scheduler::stopping()
    {
        std::shared_lock lock(m_mutex);
        return m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle()
    {
        std::this_thread::sleep_for(kIdleSleep);
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

    Scheduler *Scheduler::GetThis()
    {
        return current_scheduler;
    }

    Scheduler::FiberAndThread::FiberAndThread(std::shared_ptr<Fiber> fiber, const int thr)
        : fiber(std::move(fiber)),
          thread(thr)
    {
    }

    Scheduler::FiberAndThread::FiberAndThread(std::shared_ptr<Fiber> *f, const int thr)
        : thread(thr)
    {
        fiber.swap(*f);
    }

    Scheduler::FiberAndThread::FiberAndThread(std::function<void()> callback, const int thr)
        : cb(std::move(callback)),
          thread(thr)
    {
    }

    Scheduler::FiberAndThread::FiberAndThread(std::function<void()> *callback, const int thr)
        : thread(thr)
    {
        cb.swap(*callback);
    }

    Scheduler::FiberAndThread::FiberAndThread()
        : thread(kAnyThread)
    {
    }

    void Scheduler::FiberAndThread::reset()
    {
        fiber  = nullptr;
        cb     = nullptr;
        thread = kAnyThread;
    }

    void Scheduler::switchTo(const int thread)
    {
        ASSERT(Scheduler::GetThis() != nullptr);

        if (GetThis() == this && (thread == kAnyThread || thread == base::GetThreadID()))
        {
            return;
        }

        auto fiber = Fiber::GetThis();
        if (!fiber)
        {
            LOG_WARN(g_logger) << "[调度器] switchTo 失败：当前线程没有可切换的协程。";
            return;
        }

        schedule(fiber, thread);
        Fiber::Yield();
    }

    std::ostream &Scheduler::dump(std::ostream &os) const
    {
        os << "[调度器] 名称=" << m_name
                << " 线程总数=" << m_threadCount
                << " 活动线程=" << m_activeThreadCount
                << " 空闲线程=" << m_idleThreadCount
                << " 停止标记=" << std::boolalpha << m_stopping;
        return os;
    }

    FiberSemaphore::FiberSemaphore(const size_t initial_concurrency)
        : m_concurrency(initial_concurrency)
    {
    }

    FiberSemaphore::~FiberSemaphore()
    {
        ASSERT(m_waiters.empty());
    }

    bool FiberSemaphore::tryWait()
    {
        ASSERT(Scheduler::GetThis());
        std::scoped_lock lock(m_mutex);
        if (m_concurrency > 0U)
        {
            --m_concurrency;
            return true;
        }
        return false;
    }

    void FiberSemaphore::wait()
    {
        ASSERT(Scheduler::GetThis());
        {
            std::scoped_lock lock(m_mutex);
            if (m_concurrency > 0U)
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
            const auto [scheduler, fiber] = m_waiters.front();
            m_waiters.pop_front();
            scheduler->schedule(fiber);
            return;
        }
        ++m_concurrency;
    }

    size_t FiberSemaphore::getConcurrency() const
    {
        return m_concurrency;
    }

    void FiberSemaphore::reset()
    {
        m_concurrency = 0;
    }

    SchedulerSwitcher::SchedulerSwitcher(Scheduler *target)
        : m_caller(Scheduler::GetThis())
    {
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
