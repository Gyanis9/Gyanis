#include <stdexcept>

#include "base/Mutex.h"
#include "base/Log.h"

namespace Gyanis::base
{
    static auto g_logger = LOG_NAME("system");

    Semaphore::Semaphore(const uint32_t count)
    {
        if (sem_init(&m_semaphore, 0, count))
        {
            LOG_FATAL(g_logger) << "Semaphore::Semaphore - sem_init error";
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);
    }

    void Semaphore::wait()
    {
        if (sem_wait(&m_semaphore))
        {
            LOG_FATAL(g_logger) << "Semaphore::Semaphore - sem_wait error";
            throw std::logic_error("sem_wait error");
        }
    }

    void Semaphore::notify()
    {
        if (sem_post(&m_semaphore))
        {
            LOG_FATAL(g_logger) << "Semaphore::Semaphore - sem_post error";
            throw std::logic_error("sem_post error");
        }
    }

    Spinlock::Spinlock()
    {
        pthread_spin_init(&m_mutex, 0); // 初始化自旋锁，第二个参数为0表示锁为普通锁
    }

    Spinlock::~Spinlock()
    {
        pthread_spin_destroy(&m_mutex);
    }

    void Spinlock::lock()
    {
        pthread_spin_lock(&m_mutex);
    }

    void Spinlock::unlock()
    {
        pthread_spin_unlock(&m_mutex);
    }
}
