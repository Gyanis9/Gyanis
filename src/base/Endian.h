#ifndef ENDIAN_H
#define ENDIAN_H

#define GYANIS_LITTLE_ENDIAN 1
#define GYANIS_BIG_ENDIAN 2

#include <bit>
#include <concepts>
#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace Gyanis::base
{
    template<typename T>
    concept ByteSwapType = std::integral<T> &&
                           (sizeof(T) == sizeof(uint16_t) ||
                            sizeof(T) == sizeof(uint32_t) ||
                            sizeof(T) == sizeof(uint64_t));

    /**
     * @brief 对 2/4/8 字节整型执行字节序交换
     */
    template<typename T>
        requires ByteSwapType<T>
    constexpr T byteswap(const T value) noexcept
    {
        using UnsignedT = std::make_unsigned_t<T>;
        const auto unsigned_value = static_cast<UnsignedT>(value);

#if defined(_MSC_VER)
        if constexpr (sizeof(UnsignedT) == sizeof(uint16_t))
        {
            return static_cast<T>(_byteswap_ushort(static_cast<unsigned short>(unsigned_value)));
        }
        if constexpr (sizeof(UnsignedT) == sizeof(uint32_t))
        {
            return static_cast<T>(_byteswap_ulong(static_cast<unsigned long>(unsigned_value)));
        }
        return static_cast<T>(_byteswap_uint64(static_cast<unsigned __int64>(unsigned_value)));
#elif defined(__clang__) || defined(__GNUC__)
        if constexpr (sizeof(UnsignedT) == sizeof(uint16_t))
        {
            return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(unsigned_value)));
        }
        if constexpr (sizeof(UnsignedT) == sizeof(uint32_t))
        {
            return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(unsigned_value)));
        }
        return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(unsigned_value)));
#else
        if constexpr (sizeof(UnsignedT) == sizeof(uint16_t))
        {
            return static_cast<T>(
                (static_cast<UnsignedT>(unsigned_value >> 8)) |
                (static_cast<UnsignedT>(unsigned_value << 8))
            );
        }
        if constexpr (sizeof(UnsignedT) == sizeof(uint32_t))
        {
            return static_cast<T>(
                ((unsigned_value & static_cast<UnsignedT>(0x000000FFu)) << 24) |
                ((unsigned_value & static_cast<UnsignedT>(0x0000FF00u)) << 8) |
                ((unsigned_value & static_cast<UnsignedT>(0x00FF0000u)) >> 8) |
                ((unsigned_value & static_cast<UnsignedT>(0xFF000000u)) >> 24)
            );
        }
        return static_cast<T>(
            ((unsigned_value & static_cast<UnsignedT>(0x00000000000000FFull)) << 56) |
            ((unsigned_value & static_cast<UnsignedT>(0x000000000000FF00ull)) << 40) |
            ((unsigned_value & static_cast<UnsignedT>(0x0000000000FF0000ull)) << 24) |
            ((unsigned_value & static_cast<UnsignedT>(0x00000000FF000000ull)) << 8) |
            ((unsigned_value & static_cast<UnsignedT>(0x000000FF00000000ull)) >> 8) |
            ((unsigned_value & static_cast<UnsignedT>(0x0000FF0000000000ull)) >> 24) |
            ((unsigned_value & static_cast<UnsignedT>(0x00FF000000000000ull)) >> 40) |
            ((unsigned_value & static_cast<UnsignedT>(0xFF00000000000000ull)) >> 56)
        );
#endif
    }

#if defined(_WIN32)
#define GYANIS_BYTE_ORDER GYANIS_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define GYANIS_BYTE_ORDER GYANIS_BIG_ENDIAN
#else
#define GYANIS_BYTE_ORDER GYANIS_LITTLE_ENDIAN
#endif

    /**
     * @brief 只在小端机器上执行 byteswap, 在大端机器上什么都不做
     */
    template<typename T>
        requires ByteSwapType<T>
    constexpr T byteswapOnLittleEndian(const T t) noexcept
    {
#if defined(__cpp_lib_endian)
        if constexpr (std::endian::native == std::endian::little)
        {
            return byteswap(t);
        }
        return t;
#else
#if GYANIS_BYTE_ORDER == GYANIS_LITTLE_ENDIAN
        return byteswap(t);
#else
        return t;
#endif
#endif
    }

    /**
     * @brief 只在大端机器上执行 byteswap, 在小端机器上什么都不做
     */
    template<typename T>
        requires ByteSwapType<T>
    constexpr T byteswapOnBigEndian(const T t) noexcept
    {
#if defined(__cpp_lib_endian)
        if constexpr (std::endian::native == std::endian::big)
        {
            return byteswap(t);
        }
        return t;
#else
#if GYANIS_BYTE_ORDER == GYANIS_BIG_ENDIAN
        return byteswap(t);
#else
        return t;
#endif
#endif
    }
}

#endif
