/**
 * @file Awaitables.hpp
 * @brief 协程 awaiter 与 TaskPromise await_transform 适配实现。
 */

#ifndef AWAITABLES_H
#define AWAITABLES_H

#include "PlatformCompat.hpp"
#include "Scheduler.hpp"
#include "Task.hpp"

#include <future>
#include <thread>

namespace Core
{
    /**
     * @brief 将协程恢复调度到线程池的 awaiter。
     */
    struct ScheduleAwaiter
    {
        TaskPriority priority;

        /**
         * @brief 调度 awaiter 总是进入挂起流程。
         * @return bool 始终返回 false。
         */
        bool await_ready() const noexcept
        {
            return false;
        }

        /**
         * @brief 挂起协程并提交恢复任务到调度器。
         * @tparam Promise 协程 promise 类型。
         * @param h 协程句柄。
         */
        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> h)
        {
            auto *ctx = h.promise().executionContext();
            if (!ctx)
            {
                throw std::runtime_error("ScheduleAwaiter: no execution context");
            }
            ctx->scheduler()->schedule([h]
            {
                h.resume();
            }, priority);
        }

        /**
         * @brief await 恢复后无返回值。
         */
        void await_resume() const noexcept
        {
        }
    };

    /**
     * @brief 构造调度 awaiter。
     * @param priority 恢复时任务优先级。
     * @return ScheduleAwaiter 调度 awaiter。
     */
    inline auto schedule(const TaskPriority priority = TaskPriority::Normal)
    {
        return ScheduleAwaiter{priority};
    }

    /**
     * @brief 等待 socket I/O 事件并恢复协程的 awaiter。
     */
    struct IoAwaiter
    {
        socket_t fd;
        PollEvent event;

        /**
         * @brief I/O awaiter 默认挂起。
         * @return bool 始终返回 false。
         */
        bool await_ready() const noexcept
        {
            return false;
        }

        /**
         * @brief 注册事件监听并在触发时恢复协程。
         * @tparam Promise 协程 promise 类型。
         * @param h 协程句柄。
         */
        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> h)
        {
            auto *ctx = h.promise().executionContext();
            if (!ctx)
            {
                throw std::runtime_error("IoAwaiter: no execution context");
            }
            // 注册一次性 watch：触发后立即移除，防止 level-triggered 重复触发
            ctx->scheduler()->watch(fd, event, [h, fd_local = fd, sched = ctx->scheduler()](PollEvent pe, void *)
            {
                sched->unwatch(fd_local);
                if (!h.done())
                {
                    sched->schedule([h]
                    {
                        h.resume();
                    }, TaskPriority::High);
                }
            });
        }

        /**
         * @brief await 恢复后无返回值。
         */
        void await_resume() const noexcept
        {
        }
    };

    /**
     * @brief 创建“等待可读事件” awaiter。
     * @param fd 目标描述符。
     * @return IoAwaiter I/O awaiter。
     */
    inline IoAwaiter asyncRead(const socket_t fd)
    {
        return {fd, PollEvent::Read};
    }

    /**
     * @brief 创建“等待可写事件” awaiter。
     * @param fd 目标描述符。
     * @return IoAwaiter I/O awaiter。
     */
    inline IoAwaiter asyncWrite(const socket_t fd)
    {
        return {fd, PollEvent::Write};
    }

    /**
     * @brief 在延迟到期后恢复协程的 awaiter。
     */
    struct TimeoutAwaiter
    {
        milliseconds delay;

        /**
         * @brief 超时 awaiter 默认挂起。
         * @return bool 始终返回 false。
         */
        bool await_ready() const noexcept
        {
            return false;
        }

        /**
         * @brief 向调度器提交延迟恢复任务。
         * @tparam Promise 协程 promise 类型。
         * @param h 协程句柄。
         */
        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> h)
        {
            auto *ctx = h.promise().executionContext();
            if (!ctx)
            {
                throw std::runtime_error("TimeoutAwaiter: no execution context");
            }
            ctx->scheduler()->postDelayed(delay, [h]
            {
                h.resume();
            }, h.promise().priority());
        }

        /**
         * @brief await 恢复后无返回值。
         */
        void await_resume() const noexcept
        {
        }
    };

    /**
     * @brief 创建超时 awaiter。
     * @param ms 延迟时长。
     * @return TimeoutAwaiter 超时 awaiter。
     */
    inline TimeoutAwaiter timeout(const milliseconds ms)
    {
        return {ms};
    }

    /**
     * @brief 等待 std::future 完成并恢复协程的 awaiter。
     * @tparam T future 返回值类型。
     */
    template<typename T>
    struct FutureAwaiter
    {
        std::shared_future<T> future;

        /**
         * @brief 从 std::future 构造（自动转为 shared_future）。
         */
        FutureAwaiter(std::future<T> &&f) : future(std::move(f).share())
        {
        }

        /**
         * @brief 检查 future 是否已经就绪。
         * @return bool 已就绪返回 true。
         */
        bool await_ready() const
        {
            return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        /**
         * @brief 在后台线程等待 future 完成后恢复协程。
         * @tparam Promise 协程 promise 类型。
         * @param h 协程句柄。
         */
        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> h)
        {
            auto *ctx = h.promise().executionContext();
            if (!ctx)
            {
                throw std::runtime_error("FutureAwaiter: no execution context");
            }
            // 分离线程等待 future（shared_future 可安全多线程访问）
            auto f = future;
            std::thread([f, h, ctx]() mutable
            {
                f.wait();
                ctx->scheduler()->schedule([h]
                {
                    h.resume();
                }, TaskPriority::Normal);
            }).detach();
        }

        /**
         * @brief 获取 future 的最终结果。
         * @return T future 结果值。
         */
        T await_resume()
        {
            return future.get();
        }
    };

    /**
     * @brief TaskPromise 对同类型 Task 的 await_transform。
     * @tparam T 当前 promise 返回类型。
     * @param task 子任务对象。
     * @return auto 绑定执行上下文后的任务对象。
     */
    template<typename T>
    auto TaskPromise<T>::awaitTransform(Task<T> &&task)
    {
        // 确保子任务继承当前协程的执行上下文
        if (m_exec_ctx && !task.m_coro.promise().executionContext())
        {
            task.m_coro.promise().setExecutionContext(m_exec_ctx);
        }
        return std::move(task);
    }

    /**
     * @brief TaskPromise 对调度 awaiter 的转换。
     * @tparam T 当前 promise 返回类型。
     * @param awaiter 调度 awaiter。
     * @return auto 原 awaiter。
     */
    template<typename T>
    auto TaskPromise<T>::awaitTransform(ScheduleAwaiter awaiter) const
    {
        return awaiter;
    }

    /**
     * @brief TaskPromise 对 I/O awaiter 的转换。
     * @tparam T 当前 promise 返回类型。
     * @param awaiter I/O awaiter。
     * @return auto 原 awaiter。
     */
    template<typename T>
    auto TaskPromise<T>::awaitTransform(IoAwaiter awaiter) const
    {
        return awaiter;
    }

    /**
     * @brief TaskPromise 对超时 awaiter 的转换。
     * @tparam T 当前 promise 返回类型。
     * @param awaiter 超时 awaiter。
     * @return auto 原 awaiter。
     */
    template<typename T>
    auto TaskPromise<T>::awaitTransform(TimeoutAwaiter awaiter) const
    {
        return awaiter;
    }

    /**
     * @brief TaskPromise 对 future awaiter 的转换。
     * @tparam T 当前 promise 返回类型。
     * @tparam U future 返回类型。
     * @param awaiter future awaiter。
     * @return auto 原 awaiter。
     */
    template<typename T>
    template<typename U>
    auto TaskPromise<T>::awaitTransform(FutureAwaiter<U> awaiter)
    {
        return awaiter;
    }

    // ========== TaskPromise<void> await_transform 实现 ==========

    /**
     * @brief TaskPromise 对异类型 Task 的 await_transform。
     * @tparam T 当前 promise 返回类型。
     * @tparam U 子任务返回类型。
     * @param task 子任务对象。
     * @return auto 绑定执行上下文后的任务对象。
     */
    template<typename T>
    template<typename U>
    auto TaskPromise<T>::awaitTransform(Task<U> &&task)
    {
        if (m_exec_ctx && !task.m_coro.promise().executionContext())
        {
            task.m_coro.promise().setExecutionContext(m_exec_ctx);
        }
        return std::move(task);
    }

    inline auto TaskPromise<void>::awaitTransform(Task<void> &&task) const
    {
        if (m_exec_ctx && !task.m_coro.promise().executionContext())
        {
            task.m_coro.promise().setExecutionContext(m_exec_ctx);
        }
        return std::move(task);
    }

    inline auto TaskPromise<void>::awaitTransform(const ScheduleAwaiter awaiter) const
    {
        return awaiter;
    }

    inline auto TaskPromise<void>::awaitTransform(const IoAwaiter awaiter) const
    {
        return awaiter;
    }

    inline auto TaskPromise<void>::awaitTransform(const TimeoutAwaiter awaiter) const
    {
        return awaiter;
    }

    template<typename U>
    inline auto TaskPromise<void>::awaitTransform(FutureAwaiter<U> awaiter)
    {
        return awaiter;
    }

    template<typename U>
    inline auto TaskPromise<void>::awaitTransform(Task<U> &&task)
    {
        if (m_exec_ctx && !task.m_coro.promise().executionContext())
        {
            task.m_coro.promise().setExecutionContext(m_exec_ctx);
        }
        return std::move(task);
    }
}


#endif
