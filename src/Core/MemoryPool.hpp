/**
 * @file MemoryPool.hpp
 * @brief 协程内存池与分配器实现。
 */

#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <array>
#include <cstddef>
#include <mutex>
#include <new>
#include <vector>

namespace Core
{
    /**
     * @brief 线程安全的协程帧内存池（对象池）。
     * @details 按 8 字节对齐，最大池化块大小 1024 字节。
     *          使用分块向量（bucket）加互斥锁管理空闲列表。
     *          当请求大小超过最大块时，回退到全局 ::operator new/delete。
     */
    class CoroutineMemoryPool
    {
    public:
        /**
         * @brief 获取内存池单例。
         * @return 单例实例的引用。
         */
        static CoroutineMemoryPool &instance()
        {
            static CoroutineMemoryPool pool;
            return pool;
        }

        /**
         * @brief 分配指定大小的内存块。
         * @param size 请求的原始字节数，内部会向上对齐至 ALIGNMENT 的倍数。
         * @return 指向已分配内存的指针，对齐到 std::max_align_t。
         * @note 若 size > MAX_BLOCK_SIZE，则直接调用 ::operator new，不从池中分配。
         */
        void *allocate(std::size_t size)
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

        /**
         * @brief 释放之前由 allocate() 分配的内存块。
         * @param ptr 由 allocate() 返回的指针，若为空则直接返回。
         * @param size 分配时传入的原始大小，内部会重新对齐。
         * @note 必须保证 size 与分配时一致，否则行为未定义。
         */
        void deallocate(void *ptr, std::size_t size) noexcept
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

    private:
        static constexpr std::size_t ALIGNMENT = 8;                            ///<  对齐粒度
        static constexpr std::size_t MAX_BLOCK_SIZE = 1024;                    ///<  池中可缓存的最大单块大小。
        static constexpr std::size_t NUM_BUCKETS = MAX_BLOCK_SIZE / ALIGNMENT; ///<  空闲列表桶数量，按 ALIGNMENT 步长划分。

        CoroutineMemoryPool() = default;

        ~CoroutineMemoryPool() = default;

        /**
         * @brief 将 size 向上对齐为 ALIGNMENT 的倍数。
         * @param size 原始大小。
         * @return 对齐后的大小。
         */
        static std::size_t alignSize(const std::size_t size)
        {
            return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }

        /**
         * @brief 根据对齐后的大小计算对应的空闲列表桶索引。
         * @param size 对齐后的大小，必须为 ALIGNMENT 的正整数倍且 ≤ MAX_BLOCK_SIZE。
         * @return 桶索引（0-based）。
         */
        static std::size_t indexOf(const std::size_t size)
        {
            return (size / ALIGNMENT) - 1;
        }

        std::mutex m_mutex;                                        ///< 保护空闲列表的互斥锁。
        std::array<std::vector<void *>, NUM_BUCKETS> m_free_lists; ///< 分桶空闲列表，每个桶存放同大小块指针。
    };

    /**
     * @brief 适配器，供协程 Promise 的 operator new/delete 使用。
     * @details 通过重载 operator new/delete 使协程帧从内存池分配。
     * @tparam T 分配的元素类型，通常为 Promise 类型。
     */
    template<typename T>
    struct CoroutineAllocator
    {
        using value_type = T;

        CoroutineAllocator() = default;

        /**
         * @brief 允许从其他类型的分配器进行转换构造。
         * @tparam U 其他分配器的元素类型。
         */
        template<typename U>
        explicit CoroutineAllocator(const CoroutineAllocator<U> &) noexcept
        {
        }

        /**
         * @brief 分配能容纳 n 个 T 类型元素的内存。
         * @param n 元素个数。
         * @return 指向分配内存的 T 类型指针。
         */
        T *allocate(const std::size_t n)
        {
            void *ptr = CoroutineMemoryPool::instance().allocate(n * sizeof(T));
            return static_cast<T *>(ptr);
        }

        /**
         * @brief 释放由 allocate() 分配的内存。
         * @param ptr 要释放的指针。
         * @param n 分配时请求的元素个数。
         */
        void deallocate(T *ptr, const std::size_t n) noexcept
        {
            CoroutineMemoryPool::instance().deallocate(ptr, n * sizeof(T));
        }
    };

    /**
     * @brief 比较两个 CoroutineAllocator 是否相等。
     * @details 所有 CoroutineAllocator 实例均视为相等，因为它们共享同一个内存池。
     * @return 始终返回 true。
     */
    template<typename T, typename U>
    bool operator==(const CoroutineAllocator<T> &, const CoroutineAllocator<U> &) noexcept
    {
        return true;
    }

    /**
     * @brief 比较两个 CoroutineAllocator 是否不等。
     * @return 始终返回 false。
     */
    template<typename T, typename U>
    bool operator!=(const CoroutineAllocator<T> &, const CoroutineAllocator<U> &) noexcept
    {
        return false;
    }
}

#endif
