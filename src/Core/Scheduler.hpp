/**
 * @file Scheduler.hpp
 * @brief I/O 调度器与事件分发实现。
 * @details 封装 I/O 多路复用（Poller）和定时器（TimerService），
 *          在一个独立的 I/O 线程中运行事件循环，将就绪的 I/O 事件
 *          和定时器回调分发到关联的线程池（PriorityThreadPool）中以
 *          指定优先级执行。提供 watch/unwatch 管理 I/O 监听，
 *          以及延迟调度功能。
 */

#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "ExecutionContext.hpp"
#include "Poller.hpp"
#include "ThreadPool.hpp"
#include "Timer.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Core
{
    /**
     * @brief I/O 调度器。
     */
    class IoScheduler
    {
    public:
        /**
         * @brief I/O 事件回调类型。
         * @param events 就绪的事件集合。
         * @param user_data 注册时关联的用户数据指针。
         */
        using IoCallback = std::function<void(PollEvent, void *user_data)>;

        /**
         * @brief 构造 I/O 调度器。
         * @param thread_pool 用于执行回调的优先级线程池（引用，生命周期需外部保证）。
         * @throw std::system_error 如果创建 Poller 或 TimerService 失败。
         *
         * @details 创建平台相关的 Poller 和 TimerService，并将定时器的通知描述符
         *          注册到 Poller 中，以便在定时器到期时被唤醒。
         */
        explicit IoScheduler(PriorityThreadPool &thread_pool);

        /**
         * @brief 测试用构造器：注入预先创建的 Poller 和 TimerService。
         * @param thread_pool 线程池引用。
         * @param poller 已创建的 Poller 实例。
         * @param timer_service 已创建的 TimerService 实例。
         */
        IoScheduler(PriorityThreadPool &thread_pool, std::unique_ptr<Poller> poller, std::unique_ptr<TimerService> timer_service);

        /**
         * @brief 析构函数，停止 I/O 线程并释放资源。
         */
        ~IoScheduler();

        /**
         * @brief 启动 I/O 线程。
         */
        void start();

        /**
         * @brief 停止 I/O 线程。
         * @details 设置停止标志，唤醒 poll() 阻塞，并等待 I/O 线程结束。
         */
        void stop();

        /**
         * @brief 向关联的线程池提交一个任务。
         * @param task 任务函数。
         * @param priority 执行优先级，默认 Normal。
         */
        void schedule(std::function<void()> task, TaskPriority priority) const;

        /**
         * @brief 注册 I/O 事件监听。
         * @param fd 套接字描述符。
         * @param events 感兴趣的事件掩码（可组合）。
         * @param callback 事件触发时的回调，将在线程池中高优先级执行。
         * @param user_data 用户数据指针，会原样传递给回调（默认为 nullptr）。
         */
        void watch(socket_t fd, PollEvent events, IoCallback callback, void *user_data = nullptr);

        /**
         * @brief 移除 I/O 事件监听。
         * @param fd 要移除的套接字描述符。
         */
        void unwatch(socket_t fd);

        /**
         * @brief 延迟调度任务。
         * @param delay 延迟时长。
         * @param task 要执行的任务。
         * @param priority 任务优先级。
         *
         * @details 使用 TimerService 创建一次性定时器，到期后将任务
         *          以指定优先级提交到线程池。
         */
        void postDelayed(milliseconds delay, std::function<void()> task, TaskPriority priority) const;

        /**
         * @brief 获取执行上下文（供协程使用）。
         * @return 包含当前调度器和线程池的 ExecutionContext。
         */
        ExecutionContext getExecutionContext();

    private:
        /**
         * @struct CallbackEntry
         * @brief 内部结构，存储 I/O 回调及相关数据。
         */
        struct CallbackEntry
        {
            IoCallback callback; ///< 用户注册的回调函数。
            PollEvent events;    ///< 当前监听的事件掩码。
            void *user_data;     ///< 用户数据指针。
        };

        /**
         * @brief I/O 线程主循环。
         */
        void ioLoop();

        PriorityThreadPool &m_thread_pool;                       ///< 关联的线程池引用。
        std::unique_ptr<Poller> m_poller;                        ///< I/O 复用器实例（平台相关）。
        std::unique_ptr<TimerService> m_timer_service;           ///< 定时器服务实例（平台相关）。
        std::atomic<bool> m_running;                             ///< 运行标志（原子操作）。
        std::thread m_io_thread;                                 ///< I/O 工作线程。
        std::mutex m_mutex;                                      ///< 互斥锁，保护 m_callbacks。
        std::unordered_map<socket_t, CallbackEntry> m_callbacks; ///< 记录 fd 到回调的映射。
    };
}

#endif //SCHEDULER_HPP
