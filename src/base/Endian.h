/**
 * @file Endian.h
 * @brief 字节序操作模块封装 (大端/小端)
 * @date 2025-03-15
 */
#ifndef ENDIAN_H
#define ENDIAN_H
#define GYANIS_LITTLE_ENDIAN 1
#define GYANIS_BIG_ENDIAN 2
#include <byteswap.h>
#include <type_traits>

namespace Gyanis::base
{
    /**
     * @brief 8字节类型的字节序转化
     */
    template <typename T>
    std::enable_if_t<sizeof(T) == sizeof(uint64_t), T>
    byteswap(T value)
    {
        return static_cast<T>(bswap_64(static_cast<uint64_t>(value)));
    }

    /**
     * @brief 4字节类型的字节序转化
     */
    template <typename T>
    std::enable_if_t<sizeof(T) == sizeof(uint32_t), T>
    byteswap(T value)
    {
        return static_cast<T>(bswap_32(static_cast<uint32_t>(value)));
    }

    /**
     * @brief 2字节类型的字节序转化
     */
    template <typename T>
    std::enable_if_t<sizeof(T) == sizeof(uint16_t), T>
    byteswap(T value)
    {
        return static_cast<T>(bswap_16(static_cast<uint16_t>(value)));
    }

#if BYTE_ORDER == BIG_ENDIAN
#define GYANIS_BYTE_ORDER GYANIS_BIG_ENDIAN
#else
#define GYANIS_BYTE_ORDER GYANIS_LITTLE_ENDIAN
#endif

#if GYANIS_BYTE_ORDER == GYANIS_BIG_ENDIAN

    /**
     * @brief 只在小端机器上执行 byteswap, 在大端机器上什么都不做
     */
    template <typename T>
    T byteswapOnLittleEndian(T t)
    {
        return t;
    }

    /**
     * @brief 只在大端机器上执行 byteswap, 在小端机器上什么都不做
     */
    template <typename T>
    T byteswapOnBigEndian(T t)
    {
        return byteswap(t);
    }
#else
    /**
     * @brief 只在小端机器上执行 byteswap, 在大端机器上什么都不做
     */
    template <typename T>
    T byteswapOnLittleEndian(T t)
    {
        return byteswap(t);
    }

    /**
     * @brief 只在大端机器上执行 byteswap, 在小端机器上什么都不做
     */
    template <typename T>
    T byteswapOnBigEndian(T t)
    {
        return t;
    }
#endif
}

#endif
