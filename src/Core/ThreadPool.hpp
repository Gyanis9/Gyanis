/**
 * @file ThreadPool.hpp
 * @brief 基于优先级队列的线程池实现。
 * @details 提供了一个支持任务优先级的线程池，任务按照优先级（数值越小优先级越高）
 *          和先进先出顺序执行。通过 std::future 获取任务结果，并支持优雅关闭。
 */

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include "PlatformCompat.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Core
{
    /**
     * @enum TaskPriority
     * @brief 任务优先级枚举，数值越小优先级越高。
     */
    enum class TaskPriority
    {
        Critical,  ///< 关键优先级（最高）
        High,      ///< 高优先级
        Normal,    ///< 普通优先级
        Low,       ///< 低优先级
        Background ///< 后台优先级（最低）
    };

    /**
     * @struct PrioritizedTask
     * @brief 优先级任务结构。
     * @details 包含任务函数、优先级和入队时间。
     *          比较规则：优先级数值越小越优先，同优先级按入队时间 FIFO。
     */
    struct PrioritizedTask
    {
        TaskPriority priority;                 ///< 任务优先级
        steady_clock::time_point enqueue_time; ///< 入队时间戳
        std::function<void()> func;            ///< 任务函数

        /**
         * @brief 比较运算符，用于优先队列排序。
         * @param other 另一个任务
         * @return true 表示当前任务优先级低于 other（排在后面）
         * @note 优先级数值越小越优先，同优先级时越早入队越优先。
         */
        bool operator<(const PrioritizedTask &other) const;
    };

    /**
     * @class PriorityThreadPool
     * @brief 基于优先队列的线程池。
     * @details 内部维护一组工作线程和一个优先级队列。
     *          用户可通过 submit() 提交带优先级的任务，返回 std::future 获取结果。
     *          调用 shutdown() 可优雅停止线程池，等待所有已提交任务完成后退出。
     */
    class PriorityThreadPool
    {
    public:
        /**
         * @brief 构造线程池并启动指定数量的工作线程。
         * @param num_threads 线程数量
         */
        explicit PriorityThreadPool(const size_t num_threads);

        /**
         * @brief 析构函数，自动调用 shutdown() 等待所有线程结束。
         */
        ~PriorityThreadPool();

        /**
         * @brief 提交一个任务到线程池。
         * @tparam F 可调用对象类型
         * @tparam Args 参数类型
         * @param priority 任务优先级
         * @param f 可调用对象
         * @param args 参数
         * @return 与任务结果关联的 std::future 对象
         * @throw std::runtime_error 如果线程池已停止
         */
        template<typename F, typename... Args>
        auto submit(const TaskPriority priority, F &&f, Args &&... args) -> std::future<std::invoke_result_t<F, Args...> >
        {
            using result_type = std::invoke_result_t<F, Args...>;
            // 使用 packaged_task 包装任务以便获取 future
            auto task = std::make_shared<std::packaged_task<result_type()> >(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

            std::future<result_type> res = task->get_future();
            {
                std::lock_guard lock(m_mutex);
                if (m_stop)
                {
                    throw std::runtime_error("submit on stopped ThreadPool");
                }
                m_tasks.push(PrioritizedTask{
                                 priority,
                                 steady_clock::now(),
                                 [task]()
                                 {
                                     (*task)();
                                 }
                             });
            }
            m_cv.notify_one();
            return res;
        }

        /**
         * @brief 优雅关闭线程池。
         * @details 设置停止标志，唤醒所有工作线程，并等待它们完成当前任务后退出。
         *          已入队但未执行的任务将在工作线程退出前被处理（若线程池已停止则不再接受新任务）。
         */
        void shutdown();

    private:
        /**
         * @brief 工作线程主循环。
         * @details 不断从优先级队列中取出任务并执行，直到线程池被停止且队列为空。
         */
        void workerLoop();

        std::vector<std::thread> m_workers;           ///< 工作线程列表
        std::priority_queue<PrioritizedTask> m_tasks; ///< 优先级任务队列
        std::mutex m_mutex;                           ///< 互斥锁，保护任务队列和停止标志
        std::condition_variable m_cv;                 ///< 条件变量，用于线程唤醒
        std::atomic<bool> m_stop;                     ///< 停止标志（原子变量）
    };
}

#endif
