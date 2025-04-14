/**
 * @file ZlibStream.h
 * @brief zlib库模块封装
 * @date 2025-03-18
 */
#ifndef ZLIBSTREAM_H
#define ZLIBSTREAM_H
#include "net/stream/Stream.h"
#include <zlib.h>
#include <sys/uio.h>
#include "ByteArray.h"

namespace Gyanis::net::stream
{
    /**
     * @brief ZlibStream 类实现了流式的压缩和解压操作，支持 Gzip、Zlib 和 Deflate 格式
     */
    class ZlibStream final : public Stream
    {
    public:
        using ptr = std::shared_ptr<ZlibStream>;

        /**
         * @brief 枚举类型，定义了支持的压缩格式
         */
        enum Type
        {
            ZLIB, ///< Zlib 压缩格式
            DEFLATE, ///< Deflate 压缩格式
            GZIP ///< Gzip 压缩格式
        };

        /**
         * @brief 枚举类型，定义了压缩策略
         */
        enum Strategy
        {
            DEFAULT = Z_DEFAULT_STRATEGY, ///< 默认压缩策略
            FILTERED = Z_FILTERED, ///< 过滤的压缩策略
            HUFFMAN = Z_HUFFMAN_ONLY, ///< 仅使用霍夫曼编码
            FIXED = Z_FIXED, ///< 固定的压缩策略
            RLE = Z_RLE ///< 行程长度编码
        };

        /**
         * @brief 枚举类型，定义了压缩级别
         */
        enum CompressLevel
        {
            NO_COMPRESSION = Z_NO_COMPRESSION, ///< 不压缩
            BEST_SPEED = Z_BEST_SPEED, ///< 最快压缩速度
            BEST_COMPRESSION = Z_BEST_COMPRESSION, ///< 最佳压缩效果
            DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION ///< 默认压缩级别
        };

        /**
         * @brief 创建一个 Gzip 压缩流对象
         * @param[in] encode 是否进行编码（压缩为 true，解压为 false）
         * @param[in] buff_size 缓冲区大小
         */
        static std::shared_ptr<ZlibStream> CreateGzip(bool encode, uint32_t buff_size = 4096);

        /**
         * @brief 创建一个 Zlib 压缩流对象
         * @param[in] encode 是否进行编码（压缩为 true，解压为 false）
         * @param[in] buff_size 缓冲区大小
         */
        static std::shared_ptr<ZlibStream> CreateZlib(bool encode, uint32_t buff_size = 4096);

        /**
         * @brief 创建一个 Deflate 压缩流对象
         * @param[in] encode 是否进行编码（压缩为 true，解压为 false）
         * @param[in] buff_size 缓冲区大小
         */
        static std::shared_ptr<ZlibStream> CreateDeflate(bool encode, uint32_t buff_size = 4096);

        /**
         * @brief 创建一个指定类型的压缩流对象
         * @param[in] encode 是否进行编码（压缩为 true，解压为 false）
         * @param[in] buff_size 缓冲区大小
         * @param[in] type 压缩类型（Gzip、Zlib、Deflate）
         * @param[in] level 压缩级别
         * @param[in] window_bits 窗口大小
         * @param[in] memlevel 内存级别
         * @param[in] strategy 压缩策略
         * @return 返回一个压缩流对象的共享指针
         */
        static std::shared_ptr<ZlibStream> Create(bool encode, uint32_t buff_size = 4096,
                                                  Type type = DEFLATE, int level = DEFAULT_COMPRESSION,
                                                  int window_bits = 15
                                                  , int memlevel = 8, Strategy strategy = DEFAULT);

        /**
         * @brief 构造函数，创建一个 ZlibStream 对象
         * @param[in] encode 是否进行编码（压缩为 true，解压为 false）
         * @param[in] buff_size 缓冲区大小
         */
        explicit ZlibStream(bool encode, uint32_t buff_size = 4096);

        /**
         * @brief 析构函数，关闭 ZlibStream 对象。
         */
        ~ZlibStream() override;

        /**
         * @brief 从流中读取指定长度的数据
         * @param[out] buffer 数据存储缓冲区
         * @param[in] length 要读取的字节数
         * @return 返回读取的字节数
         */
        long read(void* buffer, size_t length) override;

        /**
         * @brief 从 ByteArray 中读取指定长度的数据
         * @param[out] bytearray 数据存储的 ByteArray 对象
         * @param[in] length 要读取的字节数
         * @return 返回读取的字节数
         */
        long read(const std::shared_ptr<ByteArray>& bytearray, size_t length) override;

        /**
         * @brief 将数据写入流中。
         * @param[in] buffer 数据源缓冲区
         * @param[in] length 要写入的字节数
         * @return 返回写入的字节数
         */
        long write(const void* buffer, size_t length) override;

        /**
         * @brief 将数据写入 ByteArray 中
         * @param[in] bytearray 数据源 ByteArray 对象
         * @param[in] length 要写入的字节数
         * @return 返回写入的字节数
         */
        long write(const std::shared_ptr<ByteArray>& bytearray, size_t length) override;

        /**
         * @brief 关闭当前流并释放相关资源
         */
        void close() override;

        /**
         * @brief 刷新压缩流，确保所有数据被处理
         */
        int flush();

        /**
         * @brief 获取是否处于空闲状态
         */
        [[nodiscard]] bool isFree() const;

        /**
         * @brief 设置流的空闲状态
         */
        void setFree(bool value);
        /**
         * @brief 获取是否为压缩模式
         */
        [[nodiscard]] bool isEncode() const;

        /**
         * @brief 设置压缩模式
         */
        void setEndcode(bool value);

        /**
         * @brief 获取缓冲区中的数据块
         */
        std::vector<iovec>& getBuffers();

        /**
         * @brief 获取压缩或解压后的结果数据
         */
        [[nodiscard]] std::string getResult() const;

        /**
         * @brief 获取当前 ByteArray 对象
         */
        std::shared_ptr<ByteArray> getByteArray();

    private:
        /**
         * @brief 初始化压缩流对象
         * @param[in] type 压缩类型（Gzip、Zlib、Deflate）
         * @param[in] level 压缩级别
         * @param[in] window_bits 窗口大小
         * @param[in] memlevel 内存级别
         * @param[in] strategy 压缩策略
         * @return 返回初始化结果
         */
        int init(Type type = DEFLATE, int level = DEFAULT_COMPRESSION
                 , int window_bits = 15, int memlevel = 8, Strategy strategy = DEFAULT);

        /**
         * @brief 编码（压缩）数据
         * @param[in] value 数据块
         * @param[in] size 数据大小
         * @param[in] finish 是否是压缩结束
         * @return 返回编码结果
         */
        int encode(const iovec* value, const uint64_t& size, bool finish);

        /**
         * @brief 解码（解压）数据
         * @param[in] value 数据块
         * @param[in] size 数据大小
         * @param[in] finish 是否是解压结束
         * @return 返回解码结果
         */
        int decode(const iovec* value, const uint64_t& size, bool finish);

        z_stream m_zstream{}; ///< zlib 流对象
        uint32_t m_buffSize; ///< 缓冲区大小
        bool m_encode; ///< 是否为编码（压缩）模式
        bool m_free; ///< 是否为空闲状态
        std::vector<iovec> m_buffs; ///< 数据块缓冲区
    };
}

#endif
