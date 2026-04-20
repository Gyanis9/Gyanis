/**
 * @file FoxThread.h
 * @brief event线程模块封装
 * @date 2025-04-11
 */
#ifndef FOXTHREAD_H
#define FOXTHREAD_H
#include <functional>
#include <iosfwd>
#include <list>
#include <memory>
#include <shared_mutex>
#include <string>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <thread>

#include "base/Singleton.h"

namespace Gyanis::db
{
    class FoxThread;

    /**
     * @brief 线程接口类，用于定义线程管理和任务调度的基本操作
     */
    class IFoxThread
    {
    public:
        using callback = std::function<void ()>; ///< 定义回调类型，表示不带参数且不返回值的函数

        /**
         * @brief 虚拟析构函数
         */
        virtual ~IFoxThread() = default;

        /**
         * @brief 派发一个回调任务到线程
         * @param cb 回调函数
         */
        virtual bool dispatch(const callback& cb) = 0;

        /**
         * @brief 根据任务 ID 派发一个回调任务到线程
         * @param id 任务 ID
         * @param cb 回调函数
         */
        virtual bool dispatch(uint32_t id, const callback& cb) = 0;

        /**
         * @brief 批量派发回调任务到线程
         * @param cbs 回调函数列表
         */
        virtual bool batchDispatch(const std::vector<callback>& cbs) = 0;

        /**
         * @brief 向所有线程广播一个回调任务
         * @param cb 回调函数
         */
        virtual void broadcast(const callback& cb) = 0;

        /**
         * @brief 启动线程
         */
        virtual void start() = 0;

        /**
         * @brief 停止线程
         */
        virtual void stop() = 0;

        /**
         * @brief 等待线程完成任务
         */
        virtual void join() = 0;

        /**
         * @brief 打印线程信息
         * @param os 输出流
         */
        virtual void dump(std::ostream& os) = 0;

        /**
         * @brief 获取线程的总任务数
         * @return 任务总数
         */
        virtual uint64_t getTotal() = 0;
    };

    /**
     * @brief FoxThread 类，继承自 IFoxThread，用于实现线程管理和任务调度
     */
    class FoxThread final : public IFoxThread
    {
    public:
        using callback = std::function<void ()>; ///< 定义回调类型，表示不带参数且不返回值的函数
        using init_cb = std::function<void(FoxThread*)>; ///< 初始化回调类型，用于线程初始化时的操作

        /**
         * @brief 构造函数，初始化线程
         * @param name 线程名称
         * @param base libevent 事件循环基对象
         */
        explicit FoxThread(std::string  name = "", event_base* base = nullptr);

        /**
         * @brief 析构函数，释放线程资源
         */
        ~FoxThread() override;

        /**
         * @brief 获取当前线程对象
         */
        static FoxThread* GetThis();

        /**
         * @brief 获取当前线程的名称
         */
        static const std::string& GetFoxThreadName();

        /**
         * @brief 获取所有 FoxThread 的名称
         */
        static void GetAllFoxThreadName(std::unordered_map<uint64_t, std::string>& names);

        /**
         * @brief 设置当前线程为本线程
         */
        void setThis();

        /**
         * @brief 取消设置当前线程为本线程
         */
        static void unsetThis();

        /**
         * @brief 启动线程
         */
        void start() override;

        /**
         * @brief 派发回调任务到线程
         * @param cb 回调函数
         */
        bool dispatch(const callback& cb) override;

        /**
         * @brief 根据任务 ID 派发回调任务到线程
         * @param id 任务 ID
         * @param cb 回调函数
         */
        bool dispatch(uint32_t id, const callback& cb) override;

        /**
         * @brief 批量派发回调任务到线程
         * @param cbs 回调函数列表
         */
        bool batchDispatch(const std::vector<callback>& cbs) override;

        /**
         * @brief 向所有线程广播回调任务
         */
        void broadcast(const callback& cb) override;

        /**
         * @brief 等待线程完成任务
         */
        void join() override;

        /**
         * @brief 停止线程
         */
        void stop() override;

        /**
         * @brief 获取线程是否已启动
         */
        bool isStart() const;

        /**
         * @brief 获取 libevent 事件基对象
         */
        event_base* getBase() const;

        /**
         * @brief 获取线程的 ID
         */
        std::thread::id getId() const;

        /**
         * @brief 获取线程中的数据
         * @param name 数据名称
         */
        void* getData(const std::string& name);

        /**
         * @brief 获取线程中的特定类型的数据
         * @param name 数据名称
         */
        template <class T>
        T* getData(const std::string& name)
        {
            return static_cast<T*>(getData(name));
        }

        /**
         * @brief 设置线程中的数据
         * @param name 数据名称
         * @param value 数据指针
         */
        void setData(const std::string& name, void* value);

        /**
         * @brief 设置线程初始化回调函数
         * @param value 初始化回调函数
         */
        void setInitCb(const init_cb& value);

        /**
         * @brief 打印线程信息
         * @param os 输出流
         */
        void dump(std::ostream& os) override;

        /**
         * @brief 获取线程的总任务数
         */
        uint64_t getTotal() override;

    private:
        /**
         * @brief 线程的回调函数
         */
        void thread_cb();

        /**
         * @brief 事件回调函数，用于处理事件
         * @param sock 套接字
         * @param which 事件类型
         * @param args 事件参数
         */
        static void read_cb(evutil_socket_t sock, short which, void* args);

    private:
        evutil_socket_t m_read; ///< 读取套接字
        evutil_socket_t m_write; ///< 写入套接字
        event_base* m_base; ///< libevent 事件基对象
        event* m_event; ///< libevent 事件
        std::thread* m_thread; ///< 线程对象
        std::shared_mutex m_mutex; ///< 共享互斥锁，用于保护线程操作
        std::list<callback> m_callbacks; ///< 存储待处理的回调函数
        std::string m_name; ///< 线程名称
        init_cb m_initCb; ///< 初始化回调函数
        std::unordered_map<std::string, void*> m_datas; ///< 存储线程中的数据
        bool m_working; ///< 线程是否正在工作
        bool m_start; ///< 线程是否已启动
        uint64_t m_total; ///< 线程的总任务数
    };

    /**
     * @brief FoxThreadPool 类，继承自 IFoxThread，用于管理一个线程池
     */
    class FoxThreadPool final : public IFoxThread
    {
    public:
        /**
         * @brief 构造函数，初始化线程池
         * @param size 线程池大小
         * @param name 线程池名称
         * @param advance 是否启用高级模式
         */
        explicit FoxThreadPool(uint32_t size, const std::string& name = "", bool advance = false);

        /**
         * @brief 析构函数，释放线程池资源
         */
        ~FoxThreadPool() override;

        /**
         * @brief 启动线程池，启动所有线程
         */
        void start() override;

        /**
         * @brief 停止线程池，停止所有线程
         */
        void stop() override;

        /**
         * @brief 等待所有线程完成任务
         */
        void join() override;

        /**
         * @brief 派发回调任务到线程池中的一个线程
         * @param cb 回调函数
         */
        bool dispatch(const callback& cb) override;

        /**
         * @brief 根据任务 ID 派发回调任务到线程池中的一个线程
         * @param id 任务 ID
         * @param cb 回调函数
         */
        bool dispatch(uint32_t id, const callback& cb) override;

        /**
         * @brief 批量派发回调任务到线程池中的多个线程
         * @param cbs 回调函数列表
         */
        bool batchDispatch(const std::vector<callback>& cbs) override;

        /**
         * @brief 获取一个随机线程对象
         */
        FoxThread* getRandFoxThread();

        /**
         * @brief 设置线程池的初始化回调函数
         * @param value 初始化回调函数
         */
        void setInitCb(const FoxThread::init_cb& value);

        /**
         * @brief 打印线程池信息
         * @param os 输出流
         */
        void dump(std::ostream& os) override;

        /**
         * @brief 向所有线程广播回调任务
         * @param cb 回调函数
         */
        void broadcast(const callback& cb) override;

        /**
         * @brief 获取线程池的总任务数
         */
        uint64_t getTotal() override;

    private:
        /**
         * @brief 释放 FoxThread 线程对象
         * @param thread 线程对象
         */
        void releaseFoxThread(FoxThread* thread);

        /**
         * @brief 检查线程池中的线程状态，维护线程池
         */
        void check();

        /**
         * @brief 包装回调函数并分配给线程
         * @param thread 线程对象
         * @param cb 回调函数
         */
        static void wrapcb(const std::shared_ptr<FoxThread>& thread, const callback& cb);

    private:
        uint32_t m_size; ///< 线程池大小
        uint32_t m_cur; ///< 当前已使用线程数
        std::string m_name; ///< 线程池名称
        bool m_advance; ///< 是否启用高级模式
        bool m_start; ///< 线程池是否已启动
        std::shared_mutex m_mutex; ///< 共享互斥锁，用于线程安全
        std::list<callback> m_callbacks; ///< 存储待处理的回调函数
        std::vector<FoxThread*> m_threads; ///< 存储线程池中的所有线程
        std::list<FoxThread*> m_freeFoxThreads; ///< 存储空闲的线程
        FoxThread::init_cb m_initCb; ///< 线程池的初始化回调函数
        uint64_t m_total; ///< 线程池总任务数
    };


    /**
     * @brief FoxThreadManager 类，负责管理多个线程实例
     */
    class FoxThreadManager
    {
    public:
        using callback = IFoxThread::callback; ///< 定义回调类型，表示不带参数且不返回值的函数

        /**
         * @brief 派发回调任务到指定名称的线程
         * @param name 线程名称
         * @param cb 回调函数
         */
        void dispatch(const std::string& name,const callback& cb);

        /**
         * @brief 根据任务 ID 派发回调任务到指定名称的线程
         * @param name 线程名称
         * @param id 任务 ID
         * @param cb 回调函数
         */
        void dispatch(const std::string& name, uint32_t id, const callback& cb);

        /**
         * @brief 批量派发回调任务到指定名称的线程
         * @param name 线程名称
         * @param cbs 回调函数列表
         */
        void batchDispatch(const std::string& name, const std::vector<callback>& cbs);

        /**
         * @brief 向指定名称的线程广播回调任务
         * @param name 线程名称
         * @param cb 回调函数
         */
        void broadcast(const std::string& name, const callback& cb);

        /**
         * @brief 打印所有线程的状态信息
         * @param os 输出流
         */
        void dumpFoxThreadStatus(std::ostream& os);

        /**
         * @brief 初始化线程管理器
         */
        void init();

        /**
         * @brief 启动线程管理器中的所有线程
         */
        void start() const;

        /**
         * @brief 停止线程管理器中的所有线程
         */
        void stop() const;

        /**
         * @brief 获取指定名称的线程实例
         * @param name 线程名称
         * @return 线程实例的 shared_ptr
         */
        std::shared_ptr<IFoxThread> get(const std::string& name);

        /**
         * @brief 添加线程实例到管理器
         * @param name 线程名称
         * @param thread 线程实例的 shared_ptr
         */
        void add(const std::string& name, const std::shared_ptr<IFoxThread>& thread);

    private:
        std::unordered_map<std::string, std::shared_ptr<IFoxThread>> m_threads; ///< 存储线程实例的映射，按名称存储
    };


    ///单例模式，获取 FoxThreadManager 实例
    using FoxThreadMgr = Singleton<FoxThreadManager>;
}

#endif
