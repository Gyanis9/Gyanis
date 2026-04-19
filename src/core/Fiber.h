/**
 * @file Fiber.h
 * @brief 协程模块封装
 * @date 2025-03-12
 */
#ifndef FIBER_H
#define FIBER_H
#include <boost/coroutine2/coroutine.hpp>
#include <memory>


namespace Gyanis::core
{
    /**
     * @brief Fiber 类，表示一个协程实例
     */
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        using CoroType = boost::coroutines2::asymmetric_coroutine<void>;

        enum State
        {
            INIT, HOLD, EXEC, TERM, READY, EXCEPT
        }; /// 协程状态枚举，表示协程的不同状态

        /**
         * @brief 构造函数
         */
        explicit Fiber(std::function<void()> callback, uint32_t stackSize = 0);

        /**
         * @brief 析构函数
         */
        ~Fiber();

        /**
         * @brief 恢复协程的执行
         */
        void resume();

        /**
         * @brief 挂起当前协程
         */
        void yield() const;

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
         * @brief 重置协程
         */
        void reset(std::function<void()> callback);

        /**
         * @brief 设置当前协程实例
         */
        static void SetThis(Fiber* fiber);

        /**
         * @brief 获取当前协程实例
         */
        static std::shared_ptr<Fiber> GetThis();

        /**
         * @brief 挂起当前协程并切换到其他协程
         */
        static void Yield();

        /**
         * @brief 协程的入口函数
         */
        static void Entry(CoroType::pull_type& yield);

    private:
        std::unique_ptr<CoroType::push_type> m_coro = nullptr; ///< 协程的推送类型，用于控制协程的执行
        std::function<void()> m_callback; ///< 协程的回调函数
        uint32_t m_stackSize; ///< 协程的栈大小
        State m_state = INIT; ///< 当前协程的状态
        CoroType::pull_type* m_yield = nullptr; ///< 用于控制协程的拉取类型
    };
}

#endif
