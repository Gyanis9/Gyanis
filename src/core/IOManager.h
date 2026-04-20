/**
 * @file IOManager.h
 * @brief IO协程调度器模块封装
 * @date 2025-03-13
 */
#ifndef IOMANAGER_H
#define IOMANAGER_H
#include "Scheduler.h"
#include "base/Timer.h"

namespace Gyanis::core
{
    /**
     * @brief IO事件类型枚举
     */
    class IOManager final : public Scheduler, public base::TimerManager
    {
    public:
        enum Event { NONE = 0x0, READ = 0x1, WRITE = 0x4 }; ///< IO事件的类型：读取、写入等

    private:
        /**
         * @brief 文件描述符上下文
         */
        struct FdContext
        {
            /**
             * @brief 事件上下文
             */
            struct EventContext
            {
                Scheduler* scheduler = nullptr; ///< 关联的调度器
                std::shared_ptr<Fiber> fiber = nullptr; ///< 关联的协程对象
                std::function<void()> cb = nullptr; ///< 事件触发时执行的回调函数
            };

            /**
             * @brief 获取指定事件的上下文
             */
            EventContext& getContext(Event event);

            /**
             * @brief 重置事件上下文
             */
            static void resetContext(EventContext& ctx);

            /**
             * @brief 触发指定事件
             * @param event 要触发的事件类型（READ 或 WRITE） 
             */
            void triggerEvent(Event event);

            EventContext read; ///< 读取事件的上下文
            EventContext write; ///< 写入事件的上下文
            int fd = 0; ///< 文件描述符
            Event events = NONE; ///< 当前文件描述符的事件类型
            std::mutex mutex; ///< 保护事件上下文的互斥锁
        };

    public:
        /**
         * @brief 构造函数
         */
        explicit IOManager(size_t threadCount = 1, const std::string& name = "");

        /**
         * @brief 析构函数
         */
        ~IOManager() override;

        /**
         * @brief 添加 IO 事件
         */
        int addEvent(int fd, Event event, std::function<void()> callback = nullptr);

        /**
         * @brief 删除 IO 事件
         */
        bool delEvent(int fd, Event event);

        /**
         * @brief 取消 IO 事件
         */
        bool cancelEvent(int fd, Event event);

        /**
         * @brief 取消所有 IO 事件
         */
        bool cancelAll(int fd);

        /**
         * @brief 获取当前 IO 管理器实例
         */
        static IOManager* GetThis();

    protected:
        /**
         * @brief 唤醒 IO 管理器
         */
        void tickle() override;

        /**
         * @brief 判断是否停止 IO 管理器
         */
        bool stopping() override;

        /**
         * @brief 空闲时的操作
         */
        void idle() override;

        /**
         * @brief 在定时器插入到队列时执行
         */
        void onTimerInsertedAtFront() override;

        /**
         * @brief 调整 IO 管理器的上下文大小
         */
        void contextResize(size_t size);

        /**
         * @brief 判断是否停止 IO 管理器，并等待指定超时
         */
        bool stopping(std::chrono::milliseconds& timeout);

    private:
        int m_epFd = 0; ///< epoll 文件描述符
        int m_wakeFd = 0; ///< 唤醒文件描述符
        std::atomic<size_t> m_pendingEventCount{0}; ///< 当前挂起的事件数量
        std::shared_mutex m_mutex; ///< 保护文件描述符上下文的互斥锁
        std::vector<FdContext*> m_fdContexts; ///< 存储文件描述符上下文
    };
}

#endif
