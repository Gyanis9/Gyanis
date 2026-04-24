/**
 * @file Buffer.h
 * @brief 非连续内存缓冲区实现（固定块链式缓冲区）
 * @details 采用 std::deque<Block> 管理多个 4KB 内存块，支持高效读写、零拷贝查看、模式查找等操作。
 *          适用网络数据包处理、流式数据缓存等场景。
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace Net
{
    /**
     * @brief 动态缓冲区类，以块链表形式存储数据，支持顺序读写和零拷贝查看。
     * @details 内部每个块大小为 BLOCK_SIZE (4096 字节)，新数据追加到末尾，读取时从头部释放空块。
     *          不支持随机访问，适用于网络流式数据收发。
     */
    class Buffer
    {
    public:
        static constexpr size_t BLOCK_SIZE = 4096; ///< 每个内存块的大小（字节）

        /**
         * @brief 默认构造函数，构造空缓冲区。
         */
        Buffer() = default;

        /**
         * @brief 默认析构函数。
         */
        ~Buffer() = default;

        // 禁止拷贝构造与拷贝赋值
        Buffer(const Buffer &) = delete;

        Buffer &operator=(const Buffer &) = delete;

        // 支持移动构造与移动赋值
        Buffer(Buffer &&) noexcept = default;

        Buffer &operator=(Buffer &&) noexcept = default;

        /**
         * @brief 获取缓冲区中可读数据的字节数。
         * @return 可读数据总长度。
         */
        [[nodiscard]] size_t readableSize() const noexcept;

        /**
         * @brief 从缓冲区读取数据到用户内存。
         * @param dest 目标内存指针，可以为 nullptr（此时仅丢弃数据，相当于 skip）。
         * @param len 希望读取的最大字节数。
         * @return 实际读取的字节数（等于 len，除非缓冲区数据不足，但函数会尽可能读取，返回值为已读取字节数）。
         * @note 若 dest 为 nullptr，则行为与 skip(len) 相同。
         * @warning 如果 len 大于可读数据量，则仅读取全部可读数据，返回值可能小于 len。
         */
        size_t read(void *dest, size_t len);

        /**
         * @brief 跳过（丢弃）指定字节的数据，不进行拷贝。
         * @param len 要跳过的字节数。
         */
        void skip(size_t len);

        /**
         * @brief 查看第一个块的可读数据（零拷贝）。
         * @return std::string_view 指向第一个块的可读数据区域，若缓冲区为空则返回空视图。
         * @note 该视图仅在缓冲区未被修改或重新分配前有效。
         */
        [[nodiscard]] std::string_view peek() const noexcept;

        /**
         * @brief 预留可写空间，返回一个可填充的 span（预提交）。
         * @param size 希望预留的字节数。
         * @return std::span<uint8_t> 指向最后一个块中可写区域的起始位置，长度不超过 size（受块剩余空间限制）。
         * @note 与 commit() 配合使用，典型用法：
         *       auto span = reservePrepare(n);
         *       向 span 中写入数据；
         *       commit(written);
         */
        [[nodiscard]] std::span<uint8_t> reservePrepare(size_t size);

        /**
         * @brief 提交已写入的数据量（与 reservePrepare 配对使用）。
         * @param len 实际已写入的字节数，必须不超过上次 reservePrepare 返回的 span 长度。
         * @note 如果 len 超过当前块剩余空间，会自动创建新块并继续提交。
         */
        void commit(size_t len);

        /**
         * @brief 追加数据到缓冲区末尾（拷贝方式）。
         * @param data 源数据指针
         * @param len  源数据长度
         */
        void append(const void *data, const size_t len);

        /**
         * @brief 追加 std::string 到缓冲区（便捷重载）。
         * @param s 要追加的字符串
         */
        void append(const std::string &s);

        /**
         * @brief 在缓冲区可读数据中查找指定子串。
         * @param pattern 要查找的子串模式
         * @return 若找到则返回子串起始位置的偏移（相对于整个可读数据的起始字节），否则返回 std::nullopt。
         * @note 复杂度 O(N)，其中 N 为可读数据总长度。
         */
        [[nodiscard]] std::optional<size_t> find(const std::string_view pattern) const noexcept;

        /**
         * @brief 清空缓冲区，释放所有内存块。
         */
        void clear();

    private:
        /**
         * @brief 单个内存块结构。
         */
        struct Block
        {
            std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(BLOCK_SIZE); ///< 数据存储区
            size_t readPos = 0;                                                        ///< 当前读位置（相对于块起始）
            size_t writePos = 0;                                                       ///< 当前写位置（相对于块起始），writePos - readPos 即为该块可读数据长度
        };

        /**
         * @brief 在末尾添加一个新块。
         */
        void addBlock();

        /**
         * @brief 确保最后一个块至少有 size 字节的可写空间。
         * @param size 需要保证的连续可写空间大小。
         * @details 若当前最后一个块剩余空间不足，则连续添加新块，直到总剩余空间 ≥ size。
         */
        void ensureWritable(size_t size);

        std::deque<Block> m_blocks; ///< 双端队列管理的所有内存块，首块用于读取，末块用于写入
    };
}

#endif
