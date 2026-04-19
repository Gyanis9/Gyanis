/**
 * @file Worker.h
 * @brief 工作任务模块封装
 * @date 2025-04-03
 */

#ifndef WORKER_H
#define WORKER_H
#include "base/Singleton.h"
#include "../base/Log.h"
#include "IOManager.h"

namespace Gyanis::core
{
    /**
     * @brief 该类负责管理一组工作任务（工人）并调度它们的执行
     */
    class WorkerGroup : public std::enable_shared_from_this<WorkerGroup>, NonCopyable
    {
    public:
        /**
         * @brief WorkerGroup 构造函数，初始化 WorkerGroup 的批处理大小以及调度器
         */
        explicit WorkerGroup(uint32_t batch_size, Scheduler* s = Scheduler::GetThis());

        /**
         * @brief 析构函数，销毁 WorkerGroup 实例时进行清理
         */
        ~WorkerGroup();

        /**
         * @brief 调度任务，将传入的回调函数（任务）分配给指定的工作线程
         */
        void scheduler(const std::function<void()>& callback, int thread = -1);

        /**
         * @brief 等待所有任务完成。
         */
        void waitAll();

        /**
         * @brief 创建并返回一个 WorkerGroup 的智能指针
         * @param batch_size 批处理的大小，决定一次可以调度多少个任务
         * @param scheduler 调度器，默认为当前线程的调度器
         */
        static std::shared_ptr<WorkerGroup> Create(uint32_t batch_size, Scheduler* scheduler = Scheduler::GetThis());

    private:
        /**
         * @brief 执行工作任务的内部函数
         * @param callback 需要执行的任务
         */
        void doWork(const std::function<void()>& callback);

        uint32_t m_batchSize; ///< 批处理的大小，每次调度的任务数量
        bool m_finish; ///< 标记任务是否完成
        Scheduler* m_scheduler; ///< 所使用的调度器
        FiberSemaphore m_sem; ///< 用于同步的信号量
    };

    /**
     * @brief WorkerManager 类用于管理多个调度器，并提供调度任务的接口
     */
    class WorkerManager
    {
    public:
        /**
         * @brief WorkerManager 构造函数，初始化 WorkerManager。
         */
        WorkerManager();

        /**
         * @brief 向 WorkerManager 中添加一个调度器
         */
        void add(const std::shared_ptr<Scheduler>& scheduler);

        /**
         * @brief 根据名称获取指定的调度器
         */
        std::shared_ptr<Scheduler> get(const std::string& name);

        /**
         * @brief 根据名称获取指定的 IOManager 调度器
         */
        std::shared_ptr<IOManager> getAsIOManager(const std::string& name);

        /**
         * @brief 调度任务，将指定的任务调度到指定的线程上
         */
        template <typename FiberOrCb>
        void schedule(const std::string& name, FiberOrCb fc, int thread = -1)
        {
            if (auto s = get(name); s)
            {
                s->schedule(fc, thread);
            }
            else
            {
                static auto s_logger = LOG_NAME("system");
                LOG_ERROR(s_logger)
                    << "WorkerManager::schedule - Scheduler with name '" << name << "' does not exist. "
                    << " | Status: Invalid";
            }
        }

        /**
         * @brief 调度任务，将指定范围内的多个任务调度到指定线程上
         */
        template <class Iter>
        void schedule(const std::string& name, Iter begin, Iter end)
        {
            if (auto s = get(name))
            {
                s->schedule(begin, end);
            }
            else
            {
                static auto s_logger = LOG_NAME("system");
                LOG_ERROR(s_logger)
                    << "WorkerManager::schedule - Scheduler with name '" << name << "' does not exist. "
                    << " | Status: Invalid";
            }
        }

        /**
         * @brief 初始化 WorkerManager，执行必要的配置
         */
        bool init();

        /**
         * @brief 根据配置值初始化 WorkerManager
         */
        bool init(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& value);

        /**
         * @brief 停止所有调度器并清理资源。
         */
        void stop();

        /**
         * @brief 获取 WorkerManager 的停止状态
         */
        bool isStopped() const;

        /**
         * @brief 打印 WorkerManager 的状态
         */
        std::ostream& dump(std::ostream& os);

        /**
         * @brief 获取当前管理的调度器数量
         */
        uint32_t getCount() const;

    private:
        std::unordered_map<std::string, std::vector<std::shared_ptr<Scheduler>>> m_datas; ///< 存储调度器的数据
        bool m_stop; ///< 标记 WorkerManager 是否已经停止
    };

    // WorkerManager 的单例实例
    using WorkerMgr = Singleton<WorkerManager>;
}

#endif
