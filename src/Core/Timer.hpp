/**
 * @file Timer.hpp
 * @brief 跨平台定时器服务抽象与实现。
 * @details 提供跨平台的定时器服务，支持延迟执行回调。
 *          - Linux: 基于 timerfd，与 epoll 事件循环集成。
 *          - Windows: 基于 CreateTimerQueueTimer，并通过自连接 UDP socket 唤醒 IOCP 循环。
 *          通过工厂函数 createTimerService() 自动选择平台实现。
 */

#ifndef TIMER_HPP
#define TIMER_HPP

#include "PlatformCompat.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <queue>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/timerfd.h>
#include <unistd.h>
#endif

namespace Core
{
    /**
     * @class TimerService
     * @brief 跨平台定时器抽象基类。
     * @details 定义统一的定时器接口：延迟执行回调、获取通知描述符、处理到期事件。
     *          子类需要将定时器与平台的事件循环（Poller）集成。
     *          所有方法需保证线程安全。
     */
    class TimerService
    {
    public:
        /**
         * @brief 回调函数类型。
         */
        using Callback = std::function<void()>;

        virtual ~TimerService() = default;

        /**
         * @brief 在指定延迟后执行回调。
         * @param delay 延迟时长（毫秒）。
         * @param cb 到期时执行的回调函数。
         * @throw std::system_error 如果平台调度失败。
         */
        virtual void scheduleAfter(milliseconds delay, Callback cb) = 0;

        /**
         * @brief 获取可用于 Poller 监听的描述符。
         * @return 在 Linux 上返回 timerfd 描述符（可读表示定时器到期）；
         *         在 Windows 上返回用于唤醒 IOCP 的内部 socket（INVALID_SOCKET 表示无效）。
         */
        virtual socket_t getNotifierFd() const = 0;

        /**
         * @brief 处理定时器到期事件。
         */
        virtual void processExpired() = 0;
    };

    // -------------------------------------------------------------------
    // Linux: 基于 timerfd
    // -------------------------------------------------------------------
#ifdef __linux__
    /**
     * @class TimerFdService
     * @brief 基于 timerfd 的 Linux 定时器服务实现。
     * @details 使用 Linux 特有的 timerfd 机制，通过 epoll 监听可读事件触发回调。
     *          内部维护一个优先队列（最早到期优先），每次调度时重置 timerfd 的绝对超时时间。
     */
    class TimerFdService : public TimerService
    {
    public:
        /**
         * @brief 构造 TimerFdService 实例。
         * @throw std::system_error 若 timerfd_create 失败。
         * @details 创建 CLOCK_MONOTONIC 的 timerfd，设置为非阻塞且执行时关闭（CLOEXEC）。
         */
        TimerFdService()
        {
            fd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
            if (fd_ == -1)
            {
                throw std::system_error(errno, std::generic_category(), "timerfd_create");
            }
        }

        /**
         * @brief 析构函数，关闭 timerfd。
         */
        ~TimerFdService() override
        {
            if (fd_ != -1)
                ::close(fd_);
        }

        /**
         * @brief 调度延迟回调。
         * @param delay 延迟时长。
         * @param cb 到期回调。
         * @throw std::system_error 若 timerfd_settime 失败。
         * @details 将回调按到期时间插入优先队列，并更新 timerfd 的超时时间。
         */
        void schedule_after(milliseconds delay, Callback cb) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto expire_time = steady_clock::now() + delay;
            pending_.push({expire_time, std::move(cb)});
            update_timer();
        }

        /**
         * @brief 返回 timerfd 描述符。
         * @return fd_ 值。
         */
        socket_t get_notifier_fd() const override
        {
            return fd_;
        }

        /**
         * @brief 处理所有已到期的定时器。
         * @details 从 timerfd 读取到期次数以清除边缘触发就绪状态，
         *          然后从优先队列中取出所有不晚于当前时间的回调并执行。
         *          注意：回调在当前线程中同步执行，生产环境中可考虑提交到线程池。
         */
        void process_expired() override
        {
            // 读取到期次数（边缘触发模式下必须读空）
            uint64_t exp = 0;
            ::read(fd_, &exp, sizeof(exp));

            std::unique_lock<std::mutex> lock(mutex_);
            auto now = steady_clock::now();
            while (!pending_.empty() && pending_.top().expire_time <= now)
            {
                auto cb = std::move(pending_.top().callback);
                pending_.pop();
                lock.unlock();
                cb(); // 注意：回调可能在当前线程执行，生产级应提交到线程池
                lock.lock();
            }
            update_timer(); // 根据剩余最早任务重置定时器
        }

    private:
        /**
         * @struct TimerEntry
         * @brief 定时器队列条目，包含到期时间和回调。
         */
        struct TimerEntry
        {
            time_point expire_time; ///< 绝对到期时间点
            Callback callback;      ///< 到期时执行的回调

            /**
             * @brief 比较运算符，使优先队列按 earliest 优先排序。
             */
            bool operator<(const TimerEntry &other) const
            {
                return expire_time > other.expire_time; // 最小堆：最早到期在顶部
            }
        };

        /**
         * @brief 根据优先队列的顶部任务设置 timerfd 的到期时间。
         * @details 若队列为空，则取消定时器（设置为 0）。
         *          若最近任务已经过期，则设置在 1 纳秒后立即触发。
         */
        void update_timer()
        {
            if (pending_.empty())
            {
                // 取消定时器
                struct itimerspec its{};
                ::timerfd_settime(fd_, 0, &its, nullptr);
                return;
            }
            auto target = pending_.top().expire_time;
            auto now = steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                                              target > now ? target - now : std::chrono::nanoseconds(1)
                                                                             );
            struct itimerspec its{};
            its.it_value.tv_sec = delta.count() / 1'000'000'000;
            its.it_value.tv_nsec = delta.count() % 1'000'000'000;
            ::timerfd_settime(fd_, 0, &its, nullptr);
        }

        int fd_ = -1;                             ///< timerfd 文件描述符
        std::mutex mutex_;                        ///< 保护优先队列的互斥锁
        std::priority_queue<TimerEntry> pending_; ///< 待处理定时器队列（最早到期优先）
    };
#endif // __linux__

    // -------------------------------------------------------------------
    // Windows: 基于 CreateTimerQueueTimer
    // -------------------------------------------------------------------
#ifdef _WIN32
    /**
     * @class WindowsTimerService
     * @brief 基于 Windows TimerQueue 的定时器服务实现。
     */
    class WindowsTimerService : public TimerService
    {
    public:
        /**
         * @brief 构造 WindowsTimerService 实例。
         * @throw std::system_error 若创建定时器队列或自连接 socket 失败。
         */
        WindowsTimerService();

        /**
         * @brief 析构函数，删除定时器队列并关闭唤醒 socket。
         */
        ~WindowsTimerService() override;

        /**
         * @brief 调度延迟回调。
         * @param delay 延迟时长。
         * @param cb 到期回调。
         * @throw std::system_error 如果 CreateTimerQueueTimer 失败。
         * @details 创建一个 TimerContext，通过 CreateTimerQueueTimer 在系统线程池中触发。
         *          回调执行后会发送字节到内部 socket，以便 IOCP 循环可被唤醒处理。
         */
        void scheduleAfter( milliseconds delay, Callback cb) override;

        /**
         * @brief 返回用于唤醒 IOCP 的 socket 描述符。
         * @return 自连接 UDP socket。
         */
        socket_t getNotifierFd() const override;

        /**
         * @brief 处理到期事件（清空唤醒 socket 缓冲区）。
         * @details Windows 定时器的回调已在系统线程中执行，这里只需消费唤醒 socket 的数据，
         *          防止后续 poll() 被立即唤醒。
         */
        void processExpired() override;

    private:
        /**
         * @struct TimerContext
         * @brief 传递给定时器回调的上下文。
         */
        struct TimerContext
        {
            Callback callback;            ///< 用户回调
            WindowsTimerService *service; ///< 所属服务指针（用于发送唤醒消息）
            HANDLE timer;                 ///< 定时器句柄（用于清理）
        };

        /**
         * @brief 系统定时器回调例程（静态函数）。
         * @param lpParam 指向 TimerContext 的指针。
         * @details 执行用户回调后，删除定时器并释放上下文，发送字节到自连接 socket 以唤醒 IOCP 循环。
         */
        static void CALLBACK TimerRoutine(const PVOID lpParam, BOOLEAN /*TimerOrWaitFired*/);

        HANDLE m_timer_queue = nullptr;          ///< 定时器队列句柄
        socket_t m_wake_socket = INVALID_SOCKET; ///< 自连接 UDP socket 用于唤醒
    };
#endif // _WIN32
}

#endif
