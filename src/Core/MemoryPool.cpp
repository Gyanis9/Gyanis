#include "MemoryPool.hpp"

#include <new>

namespace Core
{
    CoroutineMemoryPool &CoroutineMemoryPool::instance()
    {
        static CoroutineMemoryPool pool;
        return pool;
    }

    void *CoroutineMemoryPool::allocate(std::size_t size)
    {
        size = alignSize(size);
        if (size > MAX_BLOCK_SIZE)
        {
            return ::operator new(size, std::align_val_t{alignof(std::max_align_t)});
        }

        std::lock_guard lock(m_mutex);
        auto &list = m_free_lists[indexOf(size)];

        if (list.empty())
        {
            return ::operator new(size, std::align_val_t{alignof(std::max_align_t)});
        }

        void *ptr = list.back();
        list.pop_back();
        return ptr;
    }

    void CoroutineMemoryPool::deallocate(void *ptr, std::size_t size) noexcept
    {
        if (!ptr)
        {
            return;
        }
        size = alignSize(size);
        if (size > MAX_BLOCK_SIZE)
        {
            ::operator delete(ptr, std::align_val_t{alignof(std::max_align_t)});
            return;
        }
        std::lock_guard lock(m_mutex);
        m_free_lists[indexOf(size)].push_back(ptr);
    }

    void CoroutineMemoryPool::clear() noexcept
    {
        std::lock_guard lock(m_mutex);
        for (auto &list: m_free_lists)
        {
            list.clear();
        }
    }

    std::size_t CoroutineMemoryPool::alignSize(const std::size_t size)
    {
        return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }

    std::size_t CoroutineMemoryPool::indexOf(const std::size_t size)
    {
        return (size / ALIGNMENT) - 1;
    }
}
