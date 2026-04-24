/**
 * @file Task.hpp
 * @brief 协程 Task/Promise 核心类型定义。
 * @details 定义了 Task<T> 协程任务类型及其关联的 Promise 类型（TaskPromise<T>）。
 *          提供协程链式调用、优先级、取消令牌和执行上下文的便捷配置方法。
 *          支持自定义内存池分配（CoroutineMemoryPool）以及通过 await_transform
 *          集成调度器、I/O 等待、超时等异步操作。
 */

#ifndef TASK_HPP
#define TASK_HPP

#include "Cancellation.hpp"
#include "ExecutionContext.hpp"
#include "MemoryPool.hpp"

#include <cassert>
#include <coroutine>
#include <exception>
#include <thread>
#include <variant>

namespace Core
{
    // 前向声明
    class IoScheduler;
    struct ScheduleAwaiter;
    struct IoAwaiter;
    struct TimeoutAwaiter;

    template<typename T>
    struct FutureAwaiter;

    template<typename T>
    class Task;

    /**
     * @namespace detail
     * @brief 内部实现细节，包含自定义 Awaiter 等辅助结构。
     */
    namespace detail
    {
        /**
         * @struct FinalAwaiter
         * @brief 协程最终挂起时使用的 Awaiter，用于恢复调用方协程。
         * @details 在协程 final_suspend 时返回，负责将控制权交还给 continuation_ 句柄，
         *          若不存在挂起点则返回 noop_coroutine。
         */
        struct FinalAwaiter
        {
            /**
             * @brief 总是挂起，不直接 ready。
             */
            bool await_ready() const noexcept
            {
                return false;
            }

            /**
             * @brief 挂起当前协程，传递 continuation_ 继续执行。
             * @tparam Promise 协程 Promise 类型。
             * @param h 当前协程句柄。
             * @return 若存在 continuation_ 则返回其句柄，否则返回 std::noop_coroutine。
             */
            template<typename Promise>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept
            {
                auto &promise = h.promise();
                if (promise.isDetached())
                {
                    // 分离式 fire-and-forget：协程结束时自动销毁帧
                    h.destroy();
                    return std::noop_coroutine();
                }
                if (auto cont = promise.continuation())
                {
                    return cont;
                }
                return std::noop_coroutine();
            }

            /**
             * @brief 恢复执行后无操作。
             */
            void await_resume() const noexcept
            {
            }
        };
    }

    /**
     * @class TaskPromise
     * @brief 协程 Promise 类型，存储任务结果、异常、优先级、取消源及执行上下文。
     * @tparam T 协程最终返回的结果类型。
     * @details 负责协程生命周期管理、结果存储、异常捕获。
     *          通过自定义 operator new/delete 支持内存池分配。
     *          提供一系列 await_transform 重载，以便在协程中直接 co_await 各种异步对象。
     */
    template<typename T>
    class TaskPromise
    {
    public:
        /// @brief 结果类型的别名。
        using result_type = T;

        /**
         * @brief 构造函数，初始化结果状态为 std::monostate（空）。
         */
        TaskPromise() : m_result(std::monostate{})
        {
        }

        /**
         * @brief 获取与此 Promise 关联的 Task 对象。
         * @return Task<T> 实例。
         */
        Task<T> get_return_object();

        /**
         * @brief 初始挂起点（总是挂起）。
         * @return std::suspend_always 实例。
         */
        std::suspend_always initial_suspend() const noexcept
        {
            return {};
        }

        /**
         * @brief 最终挂起点，使用 FinalAwaiter 实现链式唤醒。
         * @return FinalAwaiter 实例。
         */
        detail::FinalAwaiter final_suspend() const noexcept
        {
            return {};
        }

        /**
         * @brief 协程正常返回时调用，存储返回值。
         * @tparam U 实际返回类型（需可转换为 T）。
         * @param value 返回值。
         */
        template<typename U>
        void return_value(U &&value)
        {
            m_result.template emplace<T>(std::forward<U>(value));
        }

        /**
         * @brief 协程抛出未捕获异常时调用。
         * @details 将当前异常指针存储到结果 variant 中。
         */
        void unhandled_exception()
        {
            m_result.template emplace<std::exception_ptr>(std::current_exception());
        }

        /**
         * @brief 自定义内存分配，使用 CoroutineMemoryPool。
         * @param size 分配大小。
         * @return 内存块指针。
         */
        void *operator new(const std::size_t size)
        {
            return CoroutineMemoryPool::instance().allocate(size);
        }

        /**
         * @brief 自定义内存释放，归还给 CoroutineMemoryPool。
         * @param ptr 内存块指针。
         * @param size 块大小。
         */
        void operator delete(void *ptr, const std::size_t size)
        {
            CoroutineMemoryPool::instance().deallocate(ptr, size);
        }

        /**
         * @brief 设置当前协程的 continuation 句柄（用于链式调用）。
         * @param cont 后续协程句柄。
         */
        void setContinuation(const std::coroutine_handle<> cont)
        {
            m_continuation = cont;
        }

        /**
         * @brief 设置任务优先级。
         * @param prio 优先级枚举值。
         */
        void setPriority(const TaskPriority prio)
        {
            m_priority = prio;
        }

        /**
         * @brief 设置取消源，用于协作式取消。
         * @param src 取消源指针（生命周期需外部保证）。
         */
        void setCancellationSource(CancellationSource *src)
        {
            m_cancel_source = src;
        }

        /**
         * @brief 设置执行上下文（包含调度器和线程池）。
         * @param ctx 执行上下文指针。
         */
        void setExecutionContext(ExecutionContext *ctx)
        {
            m_exec_ctx = ctx;
        }

        /**
         * @brief 获取当前任务优先级。
         * @return 优先级枚举值，默认 Normal。
         */
        TaskPriority priority() const
        {
            return m_priority;
        }

        /**
         * @brief 获取取消令牌，若未设置取消源则返回空令牌。
         * @return 关联的 CancellationToken。
         */
        CancellationToken cancellationToken() const
        {
            return m_cancel_source ? m_cancel_source->get_token() : CancellationToken{};
        }

        /**
         * @brief 获取执行上下文。
         * @return 指向关联 ExecutionContext 的指针，可能为 nullptr。
         */
        ExecutionContext *executionContext() const
        {
            return m_exec_ctx;
        }

        /**
         * @brief 获取 continuation 句柄（用于最终挂起时链式唤醒）。
         * @return 后续待恢复的协程句柄，为空则表示无 continuation。
         */
        std::coroutine_handle<> continuation() const
        {
            return m_continuation;
        }

        /**
         * @brief 标记 coroutine 为分离执行（fire-and-forget）。
         *        在 final_suspend 时自动销毁帧。
         */
        void setDetached() noexcept
        {
            m_detached = true;
        }

        /**
         * @brief 检查是否标记为分离执行。
         */
        bool isDetached() const noexcept
        {
            return m_detached;
        }

        // --- await_transform 重载（具体实现在 awaitables.hpp 中）---

        /**
         * @brief 允许 co_await 一个 Task<T>（右值），移动其内容以继续。
         * @param task 要等待的任务。
         * @return 内部 Awaiter 对象。
         */
        auto awaitTransform(Task<T> &&task);

        /**
         * @brief 支持 co_await 调度等待器。
         * @param awaiter ScheduleAwaiter 对象。
         * @return 内部 Awaiter 对象。
         */
        auto awaitTransform(ScheduleAwaiter awaiter) const;

        /**
         * @brief 支持 co_await I/O 等待器。
         * @param awaiter IoAwaiter 对象。
         * @return 内部 Awaiter 对象。
         */
        auto awaitTransform(IoAwaiter awaiter) const;

        /**
         * @brief 支持 co_await 超时等待器。
         * @param awaiter TimeoutAwaiter 对象。
         * @return 内部 Awaiter 对象。
         */
        auto awaitTransform(TimeoutAwaiter awaiter) const;

        /**
         * @brief 允许 co_await 一个 FutureAwaiter<U>。
         * @tparam U future 的结果类型。
         * @param awaiter FutureAwaiter 对象。
         * @return 内部 Awaiter 对象。
         */
        template<typename U>
        auto awaitTransform(FutureAwaiter<U> awaiter);

        /**
         * @brief 允许 co_await 其他类型的 Task<U>。
         * @tparam U 任务结果类型。
         * @param task 要等待的任务。
         * @return 内部 Awaiter 对象。
         */
        template<typename U>
        auto awaitTransform(Task<U> &&task);

        /**
         * @brief 获取协程最终结果。
         * @return 如果存储的是 T 则移动返回，若为异常则重新抛出，否则抛出 std::runtime_error。
         */
        T result()
        {
            if (auto *p = std::get_if<T>(&m_result))
            {
                return std::move(*p);
            } else if (auto *ep = std::get_if<std::exception_ptr>(&m_result))
            {
                std::rethrow_exception(*ep);
            }
            throw std::runtime_error("Task result not set");
        }

    private:
        std::variant<std::monostate, T, std::exception_ptr> m_result; ///< 结果存储：空/正常值/异常。
        std::coroutine_handle<> m_continuation;                       ///< 后续继续执行的协程句柄。
        TaskPriority m_priority = TaskPriority::Normal;               ///< 任务优先级。
        CancellationSource *m_cancel_source = nullptr;                ///< 取消源指针。
        ExecutionContext *m_exec_ctx = nullptr;                       ///< 执行上下文指针。
        bool m_detached = false;                                      ///< 分离执行标志（fire-and-forget）。

        template<typename U>
        friend class Task; ///< 允许 Task 访问私有成员。
    };

    /**
     * @class Task
     * @brief 协程任务类型，可作为 co_await 的目标。
     * @tparam T 任务返回值类型。
     */
    template<typename T>
    class Task
    {
    public:
        using promise_type = TaskPromise<T>; ///< 关联的 Promise 类型。
        friend class TaskPromise<T>;         ///< 允许 Promise 访问私有成员（用于 awaitTransform）。

        /**
         * @brief 默认构造，空任务。
         */
        Task() noexcept : m_coro(nullptr)
        {
        }

        /**
         * @brief 从协程句柄构造 Task。
         * @param h 协程句柄。
         */
        explicit Task(std::coroutine_handle<promise_type> h) : m_coro(h)
        {
        }

        /**
         * @brief 移动构造函数，转移协程所有权。
         * @param other 源 Task。
         */
        Task(Task &&other) noexcept : m_coro(std::exchange(other.m_coro, nullptr))
        {
        }

        /**
         * @brief 移动赋值运算符，释放当前任务并接管源任务。
         * @param other 源 Task。
         * @return *this。
         */
        Task &operator=(Task &&other) noexcept
        {
            if (this != &other)
            {
                if (m_coro)
                {
                    m_coro.destroy();
                }
                m_coro = std::exchange(other.m_coro, nullptr);
            }
            return *this;
        }

        /**
         * @brief 析构函数，若持有协程则销毁。
         */
        ~Task()
        {
            if (m_coro)
            {
                m_coro.destroy();
            }
        }

        /** @brief 禁止拷贝构造函数。 */
        Task(const Task &) = delete;

        /** @brief 禁止拷贝赋值运算符。 */
        Task &operator=(const Task &) = delete;

        /**
         * @brief 链式设置当前任务的优先级。
         * @param prio 目标优先级。
         * @return 移动后的自身（右值引用）。
         */
        Task &&withPriority(TaskPriority prio) &&
        {
            if (m_coro)
            {
                m_coro.promise().setPriority(prio);
            }
            return std::move(*this);
        }

        /**
         * @brief 链式设置当前任务的取消源。
         * @param src 取消源引用。
         * @return 移动后的自身（右值引用）。
         */
        Task &&withCancellation(CancellationSource &src) &&
        {
            if (m_coro)
            {
                m_coro.promise().setCancellationSource(&src);
            }
            return std::move(*this);
        }

        /**
         * @brief 链式设置当前任务的执行上下文。
         * @param ctx 执行上下文引用。
         * @return 移动后的自身（右值引用）。
         */
        Task &&withExecutionContext(ExecutionContext &ctx) &&
        {
            if (m_coro)
            {
                m_coro.promise().setExecutionContext(&ctx);
            }
            return std::move(*this);
        }

        /**
         * @brief 单次恢复协程执行。
         * @note 协程会从当前暂停点继续执行直到下一个暂停点或完成。
         *       适用于协程由外部调度器驱动的场景（如配合 I/O 事件循环）。
         */
        void resume()
        {
            if (m_coro && !m_coro.done())
            {
                m_coro.resume();
            }
        }

        /**
         * @brief 分离协程句柄，使 Task 析构时不再自动销毁协程帧。
         * @return 底层 coroutine_handle，调用者需负责最终销毁。
         * @note 用于协程由外部调度器异步驱动的场景，此时 Task 的生命周期
         *       可能短于协程。调用后此 Task 的 get() 将抛出异常。
         */
        std::coroutine_handle<promise_type> detachHandle()
        {
            auto h = m_coro;
            m_coro = nullptr;
            return h;
        }

        /**
         * @brief 分离协程句柄并标记为 fire-and-forget。
         *        协程在 final_suspend 时自动销毁帧，无需手动 destroy。
         * @return 底层 coroutine_handle。
         */
        std::coroutine_handle<promise_type> fireAndForget()
        {
            if (m_coro)
            {
                m_coro.promise().setDetached();
            }
            return detachHandle();
        }

        /**
         * @brief 阻塞方式获取任务结果（简易实现，生产环境应改用条件变量等）。
         * @return 协程返回的 T 类型结果。
         * @throw 如果任务为空，抛出 std::runtime_error；若协程以异常结束，则重新抛出该异常。
         * @note 此方法仅适用于不会在外部 awaiter 中挂起的同步协程。
         *       对于异步协程（如使用 co_await schedule/asyncRead/timeout），
         *       请使用 resume() 配合外部调度器驱动。
         */
        T get()
        {
            if (!m_coro)
            {
                throw std::runtime_error("Task empty");
            }
            auto &promise = m_coro.promise();
            while (!m_coro.done())
            {
                m_coro.resume();
                if (!m_coro.done())
                {
                    std::this_thread::yield();
                }
            }
            return promise.result();
        }

        /**
         * @brief 检测任务是否已完成（包括无协程）。
         * @return true 表示任务已结束。
         */
        bool done() const noexcept
        {
            return !m_coro || m_coro.done();
        }

        /**
         * @brief 请求取消当前任务（如果关联了取消源）。
         */
        void cancel()
        {
            if (m_coro)
            {
                if (auto *src = m_coro.promise().m_cancel_source)
                {
                    src->request_stop();
                }
            }
        }

        // --- Awaitable 接口，使 Task 可作为 co_await 的对象 ---

        /**
         * @brief 判断任务是否已就绪（完成）。
         * @return true 如果协程已结束。
         */
        bool await_ready() const noexcept
        {
            return m_coro.done();
        }

        /**
         * @brief 挂起当前协程，设置 continuation 并返回自身句柄以恢复执行。
         * @param h 当前协程句柄。
         * @return 当前 Task 的协程句柄。
         */
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept
        {
            m_coro.promise().setContinuation(h);
            return m_coro;
        }

        /**
         * @brief 恢复后获取任务结果。
         * @return 任务返回值。
         */
        T await_resume()
        {
            return m_coro.promise().result();
        }

        /**
         * @brief 允许通过 co_await 直接使用右值 Task（移动语义）。
         * @return 移动后的 Task 自身。
         */
        auto operator co_await() &&
        {
            return std::move(*this);
        }

    private:
        std::coroutine_handle<promise_type> m_coro; ///< 协程句柄，拥有生命周期。
    };

    // getReturnObject 的实现必须在 Task 定义之后
    template<typename T>
    Task<T> TaskPromise<T>::get_return_object()
    {
        return Task<T>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
    }

    /**
     * @brief void 类型协程的 Promise 特化。
     * @details 与泛型版本不同，不使用 std::variant 存储 void 值（非法），
     *          改用 std::exception_ptr 仅存储异常。提供 return_void() 方法
     *          以满足 void 协程的语义要求。
     */
    template<>
    class TaskPromise<void>
    {
    public:
        using result_type = void;

        TaskPromise() : m_result(std::monostate{})
        {
        }

        Task<void> get_return_object();

        std::suspend_always initial_suspend() const noexcept
        {
            return {};
        }

        detail::FinalAwaiter final_suspend() const noexcept
        {
            return {};
        }

        void return_void() const noexcept
        {
        }

        void unhandled_exception()
        {
            m_result = std::current_exception();
        }

        void *operator new(const std::size_t size)
        {
            return CoroutineMemoryPool::instance().allocate(size);
        }

        void operator delete(void *ptr, const std::size_t size)
        {
            CoroutineMemoryPool::instance().deallocate(ptr, size);
        }

        void setContinuation(const std::coroutine_handle<> cont)
        {
            m_continuation = cont;
        }

        void setPriority(const TaskPriority prio)
        {
            m_priority = prio;
        }

        void setCancellationSource(CancellationSource *src)
        {
            m_cancel_source = src;
        }

        void setExecutionContext(ExecutionContext *ctx)
        {
            m_exec_ctx = ctx;
        }

        TaskPriority priority() const
        {
            return m_priority;
        }

        CancellationToken cancellationToken() const
        {
            return m_cancel_source ? m_cancel_source->get_token() : CancellationToken{};
        }

        ExecutionContext *executionContext() const
        {
            return m_exec_ctx;
        }

        std::coroutine_handle<> continuation() const
        {
            return m_continuation;
        }

        void setDetached() noexcept
        {
            m_detached = true;
        }

        bool isDetached() const noexcept
        {
            return m_detached;
        }

        void result()
        {
            if (auto *ep = std::get_if<std::exception_ptr>(&m_result))
            {
                std::rethrow_exception(*ep);
            }
        }

        // --- await_transform 声明（实现在 Awaitables.hpp）---

        auto awaitTransform(Task<void> &&task) const;

        auto awaitTransform(ScheduleAwaiter awaiter) const;

        auto awaitTransform(IoAwaiter awaiter) const;

        auto awaitTransform(TimeoutAwaiter awaiter) const;

        template<typename U>
        auto awaitTransform(FutureAwaiter<U> awaiter);

        template<typename U>
        auto awaitTransform(Task<U> &&task);

    private:
        std::variant<std::monostate, std::exception_ptr> m_result;
        std::coroutine_handle<> m_continuation;
        TaskPriority m_priority = TaskPriority::Normal;
        CancellationSource *m_cancel_source = nullptr;
        ExecutionContext *m_exec_ctx = nullptr;
        bool m_detached = false;

        template<typename U>
        friend class Task;
    };

    inline Task<void> TaskPromise<void>::get_return_object()
    {
        return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
    }
}

#endif // TASK_HPP
