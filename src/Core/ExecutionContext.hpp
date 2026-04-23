/**
 * @file ExecutionContext.hpp
 * @brief 协程执行上下文对象声明。
 * @details 定义 ExecutionContext 类，封装 I/O 调度器和线程池等依赖，
 *          通过依赖注入与协程关联，为协程提供所需的运行时服务。
 */

#ifndef EXECUTIONCONTEXT_HPP
#define EXECUTIONCONTEXT_HPP

#include "ThreadPool.hpp"

namespace Core
{
    // 前向声明
    class IoScheduler;

    /**
     * @class ExecutionContext
     * @brief 协程执行上下文。
     *
     * @details 封装调度器、线程池等协程运行所需的依赖。
     *          采用依赖注入方式，通常在协程的 promise 中保存指向
     *          ExecutionContext 的指针，协程通过它获取调度器和线程池服务。
     */
    class ExecutionContext
    {
    public:
        /**
         * @brief 构造执行上下文，设置调度器和线程池。
         * @param scheduler I/O 调度器指针（非空，生命周期由外部管理）。
         * @param pool 优先级线程池指针（非空，生命周期由外部管理）。
         */
        ExecutionContext(IoScheduler *scheduler, PriorityThreadPool *pool)
            : m_scheduler(scheduler), m_thread_pool(pool)
        {
        }

        /**
         * @brief 获取关联的 I/O 调度器。
         * @return IoScheduler 指针，可能为 nullptr 如果未设置。
         */
        IoScheduler *scheduler() const
        {
            return m_scheduler;
        }

        /**
         * @brief 获取关联的优先级线程池。
         * @return PriorityThreadPool 指针，可能为 nullptr 如果未设置。
         */
        PriorityThreadPool *threadPool() const
        {
            return m_thread_pool;
        }

    private:
        IoScheduler *m_scheduler;          ///< I/O 调度器指针（外部管理生命周期）
        PriorityThreadPool *m_thread_pool; ///< 优先级线程池指针（外部管理生命周期）
    };
}

#endif
