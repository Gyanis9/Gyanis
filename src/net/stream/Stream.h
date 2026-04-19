/**
 * @file Stream.h
 * @brief 流接口模块封装
 * @date 2025-03-17
 */
#ifndef STREAM_H
#define STREAM_H
#include "ByteArray.h"

namespace Gyanis::net::stream
{
    /**
     * @brief 流接口类
     */
    class Stream
    {
    public:
        /**
         * @brief 析构函数
         */
        virtual ~Stream() = default;

        /**
         * @brief 从流中读取数据
         * @param[in] buffer 数据缓冲区，用于存储读取的数据
         * @param[in] length 要读取的字节数
         * @return 返回读取的字节数，失败时返回负值
         */

        virtual long read(void* buffer, size_t length) =0;

        /**
         * @brief 从流中读取数据到 ByteArray
         * @param[in] buffer `ByteArray` 对象，用于存储读取的数据
         * @param[in] length 要读取的字节数
         * @return 返回读取的字节数，失败时返回负值
         */
        virtual long read(const std::shared_ptr<ByteArray>& buffer, size_t length) =0;

        /**
         * @brief 从流中读取固定大小的数据
         * @param[in] buffer 数据缓冲区，用于存储读取的数据
         * @param[in] length 要读取的字节数（固定大小）
         * @return 返回读取的字节数，失败时返回负值
         */
        virtual long readFixSize(void* buffer, size_t length);

        /**
         * @brief 从流中读取固定大小的数据到 ByteArray
         * @param[in] buffer `ByteArray` 对象，用于存储读取的数据
         * @param[in] length 要读取的字节数（固定大小）
         * @return 返回读取的字节数，失败时返回负值
         */
        virtual long readFixSize(const std::shared_ptr<ByteArray>& buffer, size_t length);

        /**
         * @brief 向流中写入数据
         * @param[in] buffer 数据缓冲区，包含要写入的数据
         * @param[in] length 要写入的字节数
         * @return 返回写入的字节数，失败时返回负值
         */
        virtual long write(const void* buffer, size_t length) =0;

        /**
         * @brief 向流中写入数据来自 ByteArray
         * @param[in] buffer `ByteArray` 对象，包含要写入的数据
         * @param[in] length 要写入的字节数
         * @return 返回写入的字节数，失败时返回负值
         */
        virtual long write(const std::shared_ptr<ByteArray>& buffer, size_t length) =0;

        /**
         * @brief 向流中写入固定大小的数据
         *
         * @param[in] buffer 数据缓冲区，包含要写入的数据
         * @param[in] length 要写入的字节数（固定大小）
         * @return 返回写入的字节数，失败时返回负值
         */
        virtual long writeFixSize(const void* buffer, size_t length);

        /**
         * @brief 向流中写入固定大小的数据来自 ByteArray
         * @param[in] buffer `ByteArray` 对象，包含要写入的数据
         * @param[in] length 要写入的字节数（固定大小）
         * @return 返回写入的字节数，失败时返回负值
         */
        virtual long writeFixSize(const std::shared_ptr<ByteArray>& buffer, size_t length);

        /**
         * @brief 关闭流
         */
        virtual void close() =0;
    };
}

#endif
