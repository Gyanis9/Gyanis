/**
 * @file Cancellation.hpp
 * @brief 协程取消令牌与回调封装。
 * @details 基于 C++20 的 std::stop_token 机制，
 *          提供线程安全的协作式取消支持，用于协程或异步任务。
 */

#ifndef CANCELLATION_HPP
#define CANCELLATION_HPP

#include <functional>
#include <stop_token>

namespace Core
{
    /**
     * @brief 取消令牌类型，用于查询是否已请求取消。
     * @see std::stop_token
     */
    using CancellationToken = std::stop_token;

    /**
     * @brief 取消源类型，用于发起取消请求。
     * @see std::stop_source
     */
    using CancellationSource = std::stop_source;

    /**
     * @class CancellationCallback
     * @brief 取消注册辅助类（RAII），在销毁时自动注销回调。
     * @details 通过封装 std::stop_callback，在对象构造时向取消令牌注册一个回调，
     *          当取消请求发生或对象析构时自动注销回调。该类不可拷贝、不可移动，
     *          确保回调的生命周期与对象严格绑定。
     *
     * @note 典型用法：在需要响应取消的代码块中创建此对象，
     *       传入一个取消令牌和需要执行的回调函数（如设置标志、唤醒等待等）。
     */
    class CancellationCallback
    {
    public:
        /**
         * @brief 构造一个 CancellationCallback 并注册取消回调。
         * @tparam F 可调用对象类型，需可转换为 std::function<void()>。
         * @param token 用于注册回调的取消令牌（CancellationToken 即 std::stop_token）。
         * @param f 取消时执行的回调函数，签名为 void()。
         */
        template<typename F>
        CancellationCallback(CancellationToken token, F &&f)
            : m_callback(std::move(token), std::forward<F>(f))
        {
        }

        /// @brief 禁止拷贝构造。
        CancellationCallback(const CancellationCallback &) = delete;

        /// @brief 禁止拷贝赋值。
        CancellationCallback &operator=(const CancellationCallback &) = delete;

        /// @brief 禁止移动构造。
        CancellationCallback(CancellationCallback &&) = delete;

        /// @brief 禁止移动赋值。
        CancellationCallback &operator=(CancellationCallback &&) = delete;

    private:
        /**
         * @brief 内部持有的 std::stop_callback 对象，管理回调的注册与注销。
         * @details 使用 std::function<void()> 作为回调包装类型，允许存储任意可调用对象。
         */
        std::stop_callback<std::function<void()> > m_callback;
    };
}

#endif
