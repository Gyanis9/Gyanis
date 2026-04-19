/**
 * @file ByteArray.h
 * @brief 二进制数组(序列化/反序列化)
 * @date 2025-03-15
 */

#ifndef BYTEARRAY_H
#define BYTEARRAY_H
#include <limits>
#include <memory>
#include <vector>
#include <bits/types/struct_iovec.h>

namespace Gyanis::net::stream
{
    /**
     * @brief 二进制数据数组类
     */
    class ByteArray
    {
    public:
        /**
         * @brief 内存节点结构，用于存储数据块
         */
        struct Node
        {
            /**
             * @brief 构造函数
             * @param[in] s 节点大小
             */
            explicit Node(size_t s);

            /**
             * @brief 默认构造函数
             */
            Node();

            /**
             * @brief 析构函数，释放节点内存
             */
            ~Node();

            char* base; ///< 指向数据块的指针

            Node* next; ///< 指向下一个节点的指针

            size_t size; ///< 数据块的大小
        };

        /**
         * @brief 构造函数
         * @param[in] base_size 内存块的初始大小
         */
        explicit ByteArray(size_t base_size = 4096);

        /**
         * @brief 析构函数
         */
        ~ByteArray();

        /// 写入方法：序列化数据到字节数组
        void writeFint8(int8_t value); ///< 写入 8 位有符号整数

        void writeFuint8(uint8_t value); ///< 写入 8 位无符号整数

        void writeFint16(int16_t value); ///< 写入 16 位有符号整数

        void writeFuint16(uint16_t value); ///< 写入 16 位无符号整数

        void writeFint32(int32_t value); ///< 写入 32 位有符号整数

        void writeFuint32(uint32_t value); ///< 写入 32 位无符号整数

        void writeFint64(int64_t value); ///< 写入 64 位有符号整数

        void writeFuint64(uint64_t value); ///< 写入 64 位无符号整数

        void writeInt32(int32_t value); ///< 写入 32 位有符号整数（不使用符号）

        void writeUint32(uint32_t value); ///< 写入 32 位无符号整数（不使用符号）

        void writeInt64(int64_t value); ///< 写入 64 位有符号整数（不使用符号）

        void writeUint64(uint64_t value); ///< 写入 64 位无符号整数（不使用符号）

        void writeFloat(float value); ///< 写入单精度浮点数

        void writeDouble(double value); ///< 写入双精度浮点数

        void writeStringF16(const std::string& value); ///< 写入 16 位长度字符串

        void writeStringF32(const std::string& value); ///< 写入 32 位长度字符串

        void writeStringF64(const std::string& value); ///< 写入 64 位长度字符串

        void writeStringVint(const std::string& value); ///< 写入变长编码字符串

        void writeStringWithoutLength(const std::string& value); ///< 写入不包含长度的字符串

        /// 读取方法：反序列化数据从字节数组
        int8_t readFint8(); ///< 读取 8 位有符号整数

        uint8_t readFuint8(); ///< 读取 8 位无符号整数

        int16_t readFint16(); ///< 读取 16 位有符号整数

        uint16_t readFuint16(); ///< 读取 16 位无符号整数

        int32_t readFint32(); ///< 读取 32 位有符号整数

        uint32_t readFuint32(); ///< 读取 32 位无符号整数

        int64_t readFint64(); ///< 读取 64 位有符号整数

        uint64_t readFuint64(); ///< 读取 64 位无符号整数

        int32_t readInt32(); ///< 读取 32 位有符号整数（不使用符号）

        uint32_t readUint32(); ///< 读取 32 位无符号整数（不使用符号）

        int64_t readInt64(); ///< 读取 64 位有符号整数（不使用符号）

        uint64_t readUint64(); ///< 读取 64 位无符号整数（不使用符号）

        float readFloat(); ///< 读取单精度浮点数

        double readDouble(); ///< 读取双精度浮点数

        std::string readStringF16(); ///< 读取 16 位长度字符串

        std::string readStringF32(); ///< 读取 32 位长度字符串

        std::string readStringF64(); ///< 读取 64 位长度字符串

        std::string readStringVint(); ///< 读取变长编码字符串

        /**
         * @brief 清空字节数组内容
         */
        void clear();

        /**
         * @brief 写入原始字节数据
         * @param[in] buf 数据缓冲区
         * @param[in] size 数据大小
         */
        void write(const void* buf, size_t size);

        /**
         * @brief 读取原始字节数据
         * @param[out] buf 数据缓冲区
         * @param[in] size 数据大小
         */
        void read(void* buf, size_t size);

        /**
         * @brief 从指定位置读取原始字节数据
         *
         * @param[out] buf 数据缓冲区
         * @param[in] size 数据大小
         * @param[in] position 读取位置
         */
        void read(void* buf, size_t size, size_t position) const;

        /**
         * @brief 获取当前操作位置
         */
        [[nodiscard]] size_t getPosition() const;

        /**
         * @brief 设置操作位置
         */
        void setPosition(size_t position);

        /**
         * @brief 写入数据到文件
         * @param[in] filename 文件名
         * @return 是否成功写入
         */
        [[nodiscard]] bool writeToFile(const std::string& filename) const;

        /**
         * @brief 从文件读取数据
         * @param[in] filename 文件名
         * @return 是否成功读取
         */
        bool readFromFile(const std::string& filename);

        /**
         * @brief 获取字节数组的基础大小
         */
        [[nodiscard]] size_t getBaseSize() const;

        /**
         * @brief 获取剩余可读取的大小
         */
        [[nodiscard]] size_t getReadSize() const;

        /**
         * @brief 判断字节序是否为小端
         */
        [[nodiscard]] bool isLittleEndian() const;

        /**
         * @brief 设置字节序
         */
        void setLittleEndian(bool little_endian);

        /**
         * @brief 获取字节数组的字符串表示
         */
        [[nodiscard]] std::string toString() const;

        /**
         * @brief 获取字节数组的十六进制字符串表示
         */
        [[nodiscard]] std::string toHexString() const;

        /**
         * @brief 获取读取缓冲区
         * @param[out] buffer 缓冲区
         * @param[in] len 需要的缓冲区大小
         * @return 返回实际获取的缓冲区大小
         */
        uint64_t getReadBuffers(std::vector<iovec>& buffer, uint64_t len = std::numeric_limits<uint64_t>::max()) const;

        /**
         * @brief 获取从指定位置的读取缓冲区
         * @param[out] buffer 缓冲区
         * @param[in] len 需要的缓冲区大小
         * @param[in] position 起始位置
         * @return 返回实际获取的缓冲区大小
         */
        uint64_t getReadBuffers(std::vector<iovec>& buffer, uint64_t len, uint64_t position) const;

        /**
         * @brief 获取写入缓冲区
         * @param[out] buffer 缓冲区
         * @param[in] len 需要的缓冲区大小
         * @return 返回实际获取的缓冲区大小
         */
        uint64_t getWriteBuffers(std::vector<iovec>& buffer, uint64_t len);

        /**
         * @brief 获取字节数组的大小
         */
        [[nodiscard]] size_t getSize() const;

    private:
        /**
         * @brief 增加字节数组容量
         */
        void addCapacity(size_t size);

        /**
         * @brief 获取剩余的容量
         */
        [[nodiscard]] size_t getCapacity() const;

        size_t m_baseSize; ///< 内存块的基础大小
        size_t m_position; ///< 当前读写位置
        size_t m_capacity; ///< 当前字节数组的总容量
        size_t m_size; ///< 当前字节数组的数据大小
        int8_t m_endian; ///< 字节序，默认大端
        Node* m_root; ///< 内存块链表的根节点
        Node* m_current; ///< 当前操作的内存块指针
    };
}

#endif
