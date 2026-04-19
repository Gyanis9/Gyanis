#include "core/Fiber.h"

#include <atomic>
#include <utility>

#include "base/Log.h"
#include "base/Macro.h"

namespace Gyanis::core
{
    namespace
    {
        static auto g_logger = LOG_NAME("system");

        static thread_local Fiber *current_fiber = nullptr;

        class FiberContextGuard
        {
        public:
            explicit FiberContextGuard(Fiber *fiber) : m_previous(current_fiber)
            {
                current_fiber = fiber;
            }

            ~FiberContextGuard()
            {
                current_fiber = m_previous;
            }

        private:
            Fiber *m_previous = nullptr;
        };
    }

    Fiber::Task::Task(const Handle handle) noexcept : m_handle(handle)
    {
    }

    Fiber::Task::Task(Task &&other) noexcept : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    Fiber::Task &Fiber::Task::operator=(Task &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        destroy();
        m_handle = other.m_handle;
        other.m_handle = nullptr;
        return *this;
    }

    Fiber::Task::~Task()
    {
        destroy();
    }

    bool Fiber::Task::done() const noexcept
    {
        return !m_handle || m_handle.done();
    }

    bool Fiber::Task::valid() const noexcept
    {
        return static_cast<bool>(m_handle);
    }

    void Fiber::Task::resume() const
    {
        if (m_handle && !m_handle.done())
        {
            m_handle.resume();
        }
    }

    void Fiber::Task::destroy()
    {
        if (m_handle)
        {
            m_handle.destroy();
            m_handle = nullptr;
        }
    }

    std::exception_ptr Fiber::Task::getException() const noexcept
    {
        if (!m_handle)
        {
            return nullptr;
        }
        return m_handle.promise().exception;
    }

    Fiber::Task Fiber::Task::promise_type::get_return_object() noexcept
    {
        return Task{Handle::from_promise(*this)};
    }

    std::suspend_always Fiber::Task::promise_type::initial_suspend() const noexcept
    {
        return {};
    }

    std::suspend_always Fiber::Task::promise_type::final_suspend() const noexcept
    {
        return {};
    }

    void Fiber::Task::promise_type::unhandled_exception() noexcept
    {
        exception = std::current_exception();
    }

    void Fiber::Task::promise_type::return_void() const noexcept
    {
    }

    bool Fiber::SuspendAwaitable::await_ready() const noexcept
    {
        return false;
    }

    void Fiber::SuspendAwaitable::await_suspend(const std::coroutine_handle<>) const noexcept
    {
        if (current_fiber && current_fiber->m_state == Fiber::EXEC)
        {
            current_fiber->m_state = Fiber::HOLD;
        }
    }

    void Fiber::SuspendAwaitable::await_resume() const noexcept
    {
    }

    Fiber::Task Fiber::WrapLegacyCallback(Callback callback)
    {
        if (callback)
        {
            callback();
        }
        co_return;
    }

    Fiber::Fiber(Callback callback, const uint32_t stackSize)
        : m_legacyCallback(std::move(callback)),
          m_stackSize(stackSize == 0 ? 128 * 1024U : stackSize)
    {
        m_coroutineCallback = [cb = m_legacyCallback]() mutable
        {
            return WrapLegacyCallback(std::move(cb));
        };
        rebuildTask();
    }

    Fiber::Fiber(CoroutineCallback callback, const uint32_t stackSize)
        : m_coroutineCallback(std::move(callback)),
          m_stackSize(stackSize == 0 ? 128 * 1024U : stackSize)
    {
        rebuildTask();
    }

    Fiber::~Fiber() = default;

    void Fiber::resume()
    {
        if (m_state == EXEC)
        {
            LOG_ERROR(g_logger) << "[协程] resume 失败：当前协程正在执行中。";
            return;
        }
        if (!m_task.valid())
        {
            LOG_WARN(g_logger) << "[协程] resume 失败：协程任务未初始化。";
            m_state = TERM;
            return;
        }
        if (m_task.done())
        {
            if (m_state != TERM && m_state != EXCEPT)
            {
                m_state = TERM;
            }
            return;
        }

        FiberContextGuard guard(this);
        m_state = EXEC;
        m_task.resume();

        if (m_task.done())
        {
            if (const auto ex = m_task.getException())
            {
                try
                {
                    std::rethrow_exception(ex);
                } catch (const std::exception &e)
                {
                    LOG_ERROR(g_logger) << "[协程] 执行异常：" << e.what();
                } catch (...)
                {
                    LOG_ERROR(g_logger) << "[协程] 执行异常：发生未知异常。";
                }
                m_state = EXCEPT;
            }
            else
            {
                m_state = TERM;
            }
            return;
        }

        if (m_state == EXEC)
        {
            m_state = HOLD;
        }
    }

    void Fiber::yield()
    {
        if (m_state == EXEC)
        {
            m_state = HOLD;
        }

        static std::atomic_flag warned = ATOMIC_FLAG_INIT;
        if (!warned.test_and_set())
        {
            LOG_WARN(g_logger)
                << "[协程] 兼容接口 Fiber::yield/Fiber::Yield 不会触发真正挂起。"
                << "请在 C++20 协程回调中使用 co_await Fiber::Suspend()。";
        }
    }

    Fiber::State Fiber::getState() const
    {
        return m_state;
    }

    void Fiber::setHold()
    {
        m_state = HOLD;
    }

    void Fiber::setReady()
    {
        m_state = READY;
    }

    void Fiber::reset(Callback callback)
    {
        ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        m_legacyCallback = std::move(callback);
        m_coroutineCallback = [cb = m_legacyCallback]() mutable
        {
            return WrapLegacyCallback(std::move(cb));
        };
        rebuildTask();
    }

    void Fiber::reset(CoroutineCallback callback)
    {
        ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        m_legacyCallback = nullptr;
        m_coroutineCallback = std::move(callback);
        rebuildTask();
    }

    void Fiber::SetThis(Fiber *fiber)
    {
        current_fiber = fiber;
    }

    std::shared_ptr<Fiber> Fiber::GetThis()
    {
        if (current_fiber)
        {
            return current_fiber->shared_from_this();
        }
        LOG_WARN(g_logger) << "[协程] 获取当前协程失败：当前线程不存在活跃 Fiber。";
        return nullptr;
    }

    void Fiber::Yield()
    {
        if (const auto fiber = GetThis())
        {
            fiber->yield();
        }
    }

    Fiber::SuspendAwaitable Fiber::Suspend()
    {
        return {};
    }

    void Fiber::rebuildTask()
    {
        if (!m_coroutineCallback)
        {
            LOG_WARN(g_logger) << "[协程] 重建任务时回调为空，协程将直接终止。";
            m_task.destroy();
            m_state = TERM;
            return;
        }

        m_task.destroy();
        m_task = m_coroutineCallback();
        m_state = INIT;
    }
}
