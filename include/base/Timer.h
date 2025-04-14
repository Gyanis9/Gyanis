/**
 * @file Timer.h
 * @brief 定时器模块封装
 * @date 2025-03-12
 */
#ifndef TIMER_H
#define TIMER_H
#include <chrono>
#include <functional>
#include <memory>
#include <set>
#include <unordered_map>
#include <atomic>
#include <shared_mutex>

namespace Gyanis::base
{
    /**
     * @brief 定时器结构体，表示一个定时器实例
     */
    struct Timer
    {
        uint64_t id; /// 定时器的唯一标识符
        std::chrono::time_point<std::chrono::high_resolution_clock> expired; /// 定时器的过期时间点
        std::chrono::milliseconds interval; /// 定时器的间隔时间
        std::function<void()> callback; /// 定时器触发时调用的回调函数
        bool recurring; /// 标志是否为周期性定时器

        /**
         * @brief 构造函数
         * @param id 定时器的 ID
         * @param interval 定时器的间隔
         * @param callback 定时器到期时执行的回调函数
         * @param recurring 是否是周期性定时器
         */
        explicit Timer(uint64_t id, std::chrono::milliseconds interval, std::function<void()> callback,
                       bool recurring);

        /**
         * @brief 比较器，用于排序定时器
         */
        struct Comparator
        {
            bool operator()(const std::shared_ptr<Timer>& lhs, const std::shared_ptr<Timer>& rhs) const;
        };
    };

    /**
     * @brief 定时器管理器，用于管理多个定时器的添加、删除和执行
     */
    class TimerManager
    {
    public:
        /**
         * @brief 构造函数
         */
        TimerManager();

        /**
         * @brief 析构函数
         */
        virtual ~TimerManager();

        /**
         * @brief 添加一个定时器
         * @param interval 定时器的间隔时间
         * @param callback 定时器触发时执行的回调函数
         * @param recurring 是否是周期性定时器
         * @return 返回定时器的唯一 ID
         */
        uint64_t addTimer(uint64_t interval, const std::function<void()>& callback, bool recurring = false);

        /**
         * @brief 添加一个依赖条件的定时器
         * @param interval 定时器的间隔时间
         * @param callback 定时器触发时执行的回调函数
         * @param weak_cond 依赖的条件对象
         * @param recurring 是否是周期性定时器
         * @return 返回定时器的唯一 ID
         */
        uint64_t addConditionTimer(uint64_t interval, std::function<void()> callback, std::weak_ptr<void> weak_cond,
                                   bool recurring = false);

        /**
         * @brief 取消一个定时器
         */
        bool cancel(uint64_t id);

        /**
         * @brief 刷新定时器，使其重置过期时间
         */
        bool refresh(uint64_t id);

        /**
         * @brief 重置定时器的过期时间
         */
        bool reset(uint64_t id, uint64_t interval, bool from_now);

        /**
         * @brief 获取下一个定时器的剩余时间
         */
        std::chrono::milliseconds getNextTimer();

        /**
         * @brief 列出所有已经过期的定时器的回调函数
         */
        void ListExpiredCb(std::vector<std::function<void()>>& callbacks);

        /**
         * @brief 判断是否有定时器
         */
        bool hasTimer();

    protected:
        /**
         * @brief 子类需要实现的虚函数，用于在定时器插入时执行特殊操作
         */
        virtual void onTimerInsertedAtFront() = 0;

    private:
        /**
         * @brief 处理时间回滚的情况
         */
        void handleTimeRollback();

        /**
         * @brief 检查系统时间是否发生回滚
         */
        void checkTimeRollback();

        /**
         * @brief 添加定时器的内部实现
         *
         * @param timer 定时器对象
         * @return 定时器的唯一 ID
         */
        uint64_t addTimerInternal(const std::shared_ptr<Timer>& timer);

        std::shared_mutex m_mutex; ///< 多线程环境中对定时器进行操作时的锁
        std::atomic<uint64_t> cur_id{0}; ///< 当前定时器的唯一 ID
        std::atomic<bool> m_tickled{false}; ///< 标志是否需要触发定时器操作
        std::multiset<std::shared_ptr<Timer>, Timer::Comparator> m_timers; ///< 使用 multiset 保存定时器按过期时间排序
        std::unordered_map<uint64_t, std::multiset<std::shared_ptr<Timer>>::iterator> m_timer_map; ///< 定时器 ID 映射
        std::chrono::high_resolution_clock::time_point m_last_check_time; ///< 上次检查的时间，用于时间回滚检测
    };
}


#endif
