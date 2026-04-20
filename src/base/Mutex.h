/**
 * @file Mutex.h
 * @brief 线程间通信模块的封装
 * @date 2025-03-29
 */
#ifndef MUTEX_H
#define MUTEX_H

#include "base/NonCopyable.h"

#include <pthread.h>
#include <semaphore.h>
#include <cstdint>

namespace Gyanis::base
{
    /**
 * @brief 信号量
 */
    class Semaphore : NonCopyable
    {
    public:
        /**
         * @brief 构造函数
         * @param[in] count 信号量值的大小
         */
        explicit Semaphore(uint32_t count = 0);

        /**
         * @brief 析构函数
         */
        ~Semaphore();

        /**
         * @brief 获取信号量
         */
        void wait();

        /**
         * @brief 释放信号量
         */
        void notify();

    private:
        sem_t m_semaphore{};
    };

    /**
     * @brief 自旋锁类
     */
    class Spinlock : NonCopyable
    {
    public:
        /**
         * @brief 构造函数
         */
        Spinlock();

        /**
         * @brief 析构函数
         */
        ~Spinlock();

        /**
         * @brief 上锁
         */
        void lock();

        /**
         * @brief 解锁
         */
        void unlock();

    private:
        pthread_spinlock_t m_mutex{}; /// 自旋锁对象，使用 POSIX 线程库的自旋锁类型
    };
}
#endif
