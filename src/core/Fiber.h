#ifndef FIBER_H
#define FIBER_H

#include <coroutine>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>

namespace Gyanis::core
{
    /**
     * @brief Fiber 类，表示一个协程实例（基于 C++20 协程）
     */
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        enum State
        {
            INIT, HOLD, EXEC, TERM, READY, EXCEPT
        }; ///< 协程状态枚举，表示协程的不同状态

        class Task
        {
        public:
            struct promise_type
            {
                std::exception_ptr exception = nullptr;

                [[nodiscard]] Task get_return_object() noexcept;
                [[nodiscard]] std::suspend_always initial_suspend() const noexcept;
                [[nodiscard]] std::suspend_always final_suspend() const noexcept;
                void unhandled_exception() noexcept;
                void return_void() const noexcept;
            };

            using Handle = std::coroutine_handle<promise_type>;

            explicit Task(Handle handle = nullptr) noexcept;
            Task(Task &&other) noexcept;
            Task &operator=(Task &&other) noexcept;
            Task(const Task &) = delete;
            Task &operator=(const Task &) = delete;
            ~Task();

            [[nodiscard]] bool done() const noexcept;
            [[nodiscard]] bool valid() const noexcept;
            void resume() const;
            void destroy();
            [[nodiscard]] std::exception_ptr getException() const noexcept;

        private:
            Handle m_handle = nullptr;
        };

        struct SuspendAwaitable
        {
            [[nodiscard]] bool await_ready() const noexcept;
            void await_suspend(std::coroutine_handle<>) const noexcept;
            void await_resume() const noexcept;
        };

        using Callback = std::function<void()>;
        using CoroutineCallback = std::function<Task()>;

        /**
         * @brief 构造函数（兼容旧回调签名）
         */
        explicit Fiber(Callback callback, uint32_t stackSize = 0);

        /**
         * @brief 构造函数（推荐：C++20 协程回调）
         */
        explicit Fiber(CoroutineCallback callback, uint32_t stackSize = 0);

        /**
         * @brief 析构函数
         */
        ~Fiber();

        /**
         * @brief 恢复协程的执行
         */
        void resume();

        /**
         * @brief 旧接口：标记当前协程为 HOLD（不执行真正挂起）
         */
        void yield();

        /**
         * @brief 获取当前协程的状态
         */
        [[nodiscard]] State getState() const;

        /**
         * @brief 将协程状态设置为 HOLD（挂起）
         */
        void setHold();

        /**
         * @brief 将协程状态设置为 READY（就绪）
         */
        void setReady();

        /**
         * @brief 重置协程（兼容旧回调签名）
         */
        void reset(Callback callback);

        /**
         * @brief 重置协程（推荐：C++20 协程回调）
         */
        void reset(CoroutineCallback callback);

        /**
         * @brief 设置当前协程实例
         */
        static void SetThis(Fiber *fiber);

        /**
         * @brief 获取当前协程实例
         */
        static std::shared_ptr<Fiber> GetThis();

        /**
         * @brief 旧接口：兼容层调用，建议改为 co_await Suspend()
         */
        static void Yield();

        /**
         * @brief C++20 挂起接口：在协程回调中使用 co_await Fiber::Suspend()
         */
        [[nodiscard]] static SuspendAwaitable Suspend();

    private:
        static Task WrapLegacyCallback(Callback callback);
        void rebuildTask();

    private:
        Task              m_task;
        Callback          m_legacyCallback;
        CoroutineCallback m_coroutineCallback;
        uint32_t          m_stackSize = 0;
        State             m_state     = INIT;
    };
}

#endif
