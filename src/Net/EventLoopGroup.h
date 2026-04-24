/**
 * @file EventLoopGroup.h
 * @brief 事件循环与事件循环组，封装线程池和 I/O 调度器。
 * @details EventLoop 提供单事件循环实例，内部组合了优先线程池和 I/O 调度器；
 *          EventLoopGroup 管理多个 EventLoop，可实现负载均衡（例如用于多 Reactor 模型）。
 */

#ifndef EVENTLOOPGROUP_H
#define EVENTLOOPGROUP_H

#include "Core/Scheduler.hpp"
#include "Core/ThreadPool.hpp"

#include <memory>
#include <vector>

namespace Net
{
    /**
     * @brief 事件循环类，管理一个线程池和一个 I/O 调度器。
     * @details 一个 EventLoop 实例代表一个独立的执行上下文，内部包含一个 PriorityThreadPool
     *          用于执行通用任务，以及一个 IoScheduler 用于事件驱动（如 epoll/IOCP）。
     *          通过 getContext() 可获取 ExecutionContext 用于协程调度。
     */
    class EventLoop
    {
    public:
        /**
         * @brief 构造事件循环。
         * @param poolThreads 线程池中的线程数量（每个循环独立线程池）。
         */
        explicit EventLoop(size_t poolThreads = 4);

        /**
         * @brief 启动事件循环（启动 I/O 调度器和线程池）。
         */
        void start() const;

        /**
         * @brief 停止事件循环。
         */
        void stop() const;

        /**
         * @brief 获取执行上下文，用于协程的调度和等待。
         * @return ExecutionContext 对象，包含调度器和线程池指针。
         */
        Core::ExecutionContext getContext() const;

    private:
        std::unique_ptr<Core::PriorityThreadPool> m_threadPool; ///< 优先级线程池
        std::unique_ptr<Core::IoScheduler> m_scheduler;         ///< I/O 调度器（事件循环核心）
    };

    /**
     * @brief 事件循环组，管理多个 EventLoop 实例。
     * @details 用于多线程/多核场景，可将不同连接或任务分配到不同 EventLoop 上，
     *          实现水平扩展。每个 EventLoop 拥有独立的线程池和 I/O 调度器。
     */
    class EventLoopGroup
    {
    public:
        /**
         * @brief 构造事件循环组。
         * @param numLoops          组内 EventLoop 的数量。
         * @param poolThreadsPerLoop 每个 EventLoop 内部的线程池大小。
         */
        EventLoopGroup(size_t numLoops, size_t poolThreadsPerLoop);

        /**
         * @brief 启动组内所有 EventLoop。
         */
        void start() const;

        /**
         * @brief 停止组内所有 EventLoop。
         */
        void stop() const;

        /**
         * @brief 根据索引获取指定 EventLoop 的执行上下文。
         * @param index 索引（0 <= index < size()）。
         * @return 对应 EventLoop 的 ExecutionContext。
         */
        Core::ExecutionContext getContext(size_t index) const;

        /**
         * @brief 返回组内 EventLoop 的数量。
         * @return size_t 数量。
         */
        size_t size() const noexcept;

    private:
        std::vector<std::unique_ptr<EventLoop> > m_loops; ///< EventLoop 实例列表
    };
}

#endif
