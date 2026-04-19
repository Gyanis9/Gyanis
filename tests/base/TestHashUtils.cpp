#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

#include "base/HashUtils.h"

TEST_CASE("quick hash overloads keep consistent result", "[base][hash][quick]")
{
    const std::string text = "abc";

    const auto h1 = Gyanis::base::quick_hash(text.c_str());
    const auto h2 = Gyanis::base::quick_hash(text.data(), static_cast<uint32_t>(text.size()));

    REQUIRE(h1 == h2);
}

TEST_CASE("hash utils base64 roundtrip", "[base][hash][base64]")
{
    const std::string raw = "hello#2026";

    const auto encoded = Gyanis::base::base64encode(raw);
    const auto decoded = Gyanis::base::base64decode(encoded);

    REQUIRE(decoded == raw);
}

TEST_CASE("hash utils md5 and sha1 match known vectors", "[base][hash][digest]")
{
    const std::string text = "abc";

    REQUIRE(Gyanis::base::md5(text) == "900150983cd24fb0d6963f7d28e17f72");
    REQUIRE(Gyanis::base::sha1(text) == "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST_CASE("hash utils hmac sha256 produces stable value", "[base][hash][hmac]")
{
    const std::string text = "The quick brown fox jumps over the lazy dog";
    const std::string key = "key";

    const auto raw = Gyanis::base::hmac_sha256(text, key);
    const auto hex = Gyanis::base::hexstring_from_data(raw);

    REQUIRE(hex == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");
}
