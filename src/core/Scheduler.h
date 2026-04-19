/**
 * @file Scheduler.h
 * @brief 协程调度器模块封装
 * @date 2025-03-12
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <vector>
#include <thread>
#include <mutex>
#include <list>
#include <queue>
#include <shared_mutex>
#include <atomic>

#include "../base/Mutex.h"
#include "Fiber.h"
#include "base/NonCopyable.h"

namespace Gyanis::core
{
    /**
     * @brief 协程调度器类
     */
    class Scheduler
    {
    public:
        /**
         * @brief 构造函数
         */
        explicit Scheduler(size_t threadCount = 1, std::string name = "");

        /**
         * @brief 析构函数
         */
        virtual ~Scheduler();

        /**
         * @brief 返回协程调度器名称
         */
        [[nodiscard]] const std::string& getName() const;

        /**
         * @brief 启动调度器
         */
        void start();

        /**
         * @brief 停止调度器
         */
        void stop();

        /**
         * @brief 调度一个协程或回调函数
         * @param fc 协程或回调函数
         * @param thread 指定调度的线程（默认为 -1，表示不指定线程）
         *               默认为 -1 表示由调度器选择一个合适的线程。
         */
        template <class FiberOrCb>
        void schedule(const FiberOrCb& fc, int thread = -1)
        {
            bool need_tickle = false;
            {
                std::lock_guard lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread); // 调用内部调度方法
            }

            if (need_tickle)
            {
                tickle(); // 唤醒调度器，可能导致任务被调度执行
            }
        }

        /**
         * @brief 调度一系列协程或回调函数
         * @param begin 迭代器指向任务列表的开始
         * @param end 迭代器指向任务列表的结束
         *            批量调度一组任务，适用于批量添加任务的场景。
         */
        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false;
            {
                std::lock_guard lock(m_mutex);
                while (begin != end)
                {
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle; // 批量调度任务
                    ++begin;
                }
            }
            if (need_tickle) // 如果需要唤醒调度器
            {
                tickle(); // 唤醒调度器
            }
        }

        /**
         * @brief 切换到指定线程执行任务
         * @param thread 指定线程（默认为 -1，表示不指定线程）
         *               在某些任务需要指定线程执行时使用。
         */
        void switchTo(int thread = -1);

        /**
         * @brief 输出调度器信息
         */
        std::ostream& dump(std::ostream& os) const;

        /**
         * @brief 输出信息
         */
        std::string toString() const;

    protected:
        /**
         * @brief 唤醒调度器
         */
        virtual void tickle();

        /**
         * @brief 执行调度操作
         */
        void run();

        /**
         * @brief 判断是否停止调度器
         */
        virtual bool stopping();

        /**
         * @brief 空闲时的操作
         */
        virtual void idle();

        /**
         * @brief 设置当前调度器实例
         */
        void setThis();

        /**
         * @brief 判断是否有空闲线程
         */
        bool hasIdleThread() const;

    private:
        /**
         * @brief 内部调度方法
         * @param fc 协程或回调函数
         * @param thread 指定线程
         * @return `true` 如果任务调度队列为空且需要唤醒其他线程；`false` 否则
         */
        template <class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread)
        {
            const bool need_tickle = m_fibers.empty();
            if (const FiberAndThread ft(fc, thread); ft.fiber || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    public:
        /**
         * @brief 获取当前调度器实例
         */
        static Scheduler* GetThis();

    private:
        /**
         * @brief 内部结构，表示一个协程与线程的组合
         */
        struct FiberAndThread
        {
            std::shared_ptr<Fiber> fiber = nullptr; ///< 协程实例
            std::function<void()> cb = nullptr; ///< 回调函数
            int thread; ///< 指定的线程
            /**
             * @brief 构造函数：初始化协程和线程信息
             */
            FiberAndThread(std::shared_ptr<Fiber> fiber, int thr);

            /**
             * @brief 构造函数：初始化协程指针和线程信息
             */
            FiberAndThread(std::shared_ptr<Fiber>* f, int thr);

            /**
             * @brief 构造函数：初始化回调函数和线程信息
             */
            FiberAndThread(std::function<void()> fiber, int thr);

            /**
             * @brief 构造函数：初始化回调函数指针和线程信息
             */
            FiberAndThread(std::function<void()>* fiber, int thr);

            /**
             * @brief 默认构造函数：设置线程为 -1
             */
            FiberAndThread();

            /**
             * @brief 重置函数：清空协程、回调和线程信息
             */
            void reset();
        };

    protected:
        size_t m_threadCount = 0; ///< 线程池大小
        std::atomic<size_t> m_activeThreadCount{0}; ///< 当前活动线程数量
        std::atomic<size_t> m_idleThreadCount{0}; ///< 当前空闲线程数量
        bool m_stopping = true; ///< 是否停止调度器

    private:
        std::string m_name; ///< 协程调度器名称
        mutable std::shared_mutex m_mutex; ///< 保护调度器全局数据的共享互斥锁
        std::vector<std::unique_ptr<std::thread>> m_threads; ///< 线程池
        std::list<FiberAndThread> m_fibers; ///< 待执行的协程队列
    };


    /**
     * @brief 协程信号量类
     */
    class FiberSemaphore : NonCopyable
    {
    public:
        /// 类型定义：互斥量类型，使用自旋锁保证线程安全
        using MutexType = base::Spinlock;

        /**
         * @brief 构造函数
         * @param initial_concurrency 初始化的并发度（默认为 0），表示信号量的初始计数值。
         */
        explicit FiberSemaphore(size_t initial_concurrency = 0);

        /**
         * @brief 析构函数
         */
        ~FiberSemaphore();

        /**
         * @brief 尝试等待信号量
         */
        bool tryWait();

        /**
         * @brief 等待信号量
         */
        void wait();

        /**
         * @brief 通知信号量
         */
        void notify();

        /**
         * @brief 获取当前信号量的并发度（计数）
         */
        [[nodiscard]] size_t getConcurrency() const;

        /**
         * @brief 重置信号量的并发度
         */
        void reset();

    private:
        MutexType m_mutex; ///< 保护信号量的互斥量，确保线程安全
        std::list<std::pair<Scheduler*, std::shared_ptr<Fiber>>> m_waiters; ///< 等待信号量的协程队列
        size_t m_concurrency; ///< 当前信号量的计数，表示可用的并发数量
    };


    /**
     * @brief 调度器切换类
     */
    class SchedulerSwitcher : public NonCopyable
    {
    public:
        /**
         * @brief 构造函数
         * @param target 目标调度器，指定要切换到的调度器。
         */
        explicit SchedulerSwitcher(Scheduler* target = nullptr);

        /**
         * @brief 析构函数
         */
        ~SchedulerSwitcher();

    private:
        Scheduler* m_caller; ///< 保存切换前的调度器指针
    };
}


#endif
