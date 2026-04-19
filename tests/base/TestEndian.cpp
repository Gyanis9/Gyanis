#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "base/Endian.h"

TEST_CASE("endian byteswap supports 16 32 64 bit unsigned integers", "[base][endian][byteswap]")
{
    REQUIRE(Gyanis::base::byteswap<uint16_t>(0x1234u) == static_cast<uint16_t>(0x3412u));
    REQUIRE(Gyanis::base::byteswap<uint32_t>(0x12345678u) == static_cast<uint32_t>(0x78563412u));
    REQUIRE(Gyanis::base::byteswap<uint64_t>(0x1122334455667788ull) ==
            static_cast<uint64_t>(0x8877665544332211ull));
}

TEST_CASE("endian conversion helpers keep existing little big endian semantics", "[base][endian][helper]")
{
    constexpr uint16_t value = 0x1234u;

#if GYANIS_BYTE_ORDER == GYANIS_LITTLE_ENDIAN
    REQUIRE(Gyanis::base::byteswapOnLittleEndian(value) == static_cast<uint16_t>(0x3412u));
    REQUIRE(Gyanis::base::byteswapOnBigEndian(value) == value);
#else
    REQUIRE(Gyanis::base::byteswapOnLittleEndian(value) == value);
    REQUIRE(Gyanis::base::byteswapOnBigEndian(value) == static_cast<uint16_t>(0x3412u));
#endif
}
