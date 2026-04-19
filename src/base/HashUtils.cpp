#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <optional>
#include <random>
#include <stdexcept>
#include <string_view>

#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>

#include "base/HashUtils.h"
#include "base/Log.h"

namespace Gyanis::base
{
    namespace
    {
        static auto g_logger = LOG_NAME("system");

        constexpr std::string_view kBase64Alphabet =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        constexpr uint32_t kMurmur3C1 = 0xcc9e2d51U;
        constexpr uint32_t kMurmur3C2 = 0x1b873593U;

        [[nodiscard]] constexpr uint32_t ByteSwap32(const uint32_t value) noexcept
        {
            return ((value & 0x000000ffU) << 24) |
                   ((value & 0x0000ff00U) << 8) |
                   ((value & 0x00ff0000U) >> 8) |
                   ((value & 0xff000000U) >> 24);
        }

        [[nodiscard]] uint32_t Fmix32(uint32_t h) noexcept
        {
            h ^= h >> 16;
            h *= 0x85ebca6bU;
            h ^= h >> 13;
            h *= 0xc2b2ae35U;
            h ^= h >> 16;
            return h;
        }

        [[nodiscard]] uint32_t LoadUint32LE(const uint8_t *ptr) noexcept
        {
            uint32_t value = 0;
            std::memcpy(&value, ptr, sizeof(value));
            if constexpr (std::endian::native == std::endian::big)
            {
                value = ByteSwap32(value);
            }
            return value;
        }

        [[nodiscard]] int DecodeBase64Char(const char ch) noexcept
        {
            if (ch >= 'A' && ch <= 'Z')
            {
                return ch - 'A';
            }
            if (ch >= 'a' && ch <= 'z')
            {
                return ch - 'a' + 26;
            }
            if (ch >= '0' && ch <= '9')
            {
                return ch - '0' + 52;
            }
            if (ch == '+')
            {
                return 62;
            }
            if (ch == '/')
            {
                return 63;
            }
            return -1;
        }

        [[nodiscard]] std::optional<uint8_t> HexNibble(const char ch) noexcept
        {
            if (ch >= '0' && ch <= '9')
            {
                return static_cast<uint8_t>(ch - '0');
            }
            if (ch >= 'a' && ch <= 'f')
            {
                return static_cast<uint8_t>(ch - 'a' + 10);
            }
            if (ch >= 'A' && ch <= 'F')
            {
                return static_cast<uint8_t>(ch - 'A' + 10);
            }
            return std::nullopt;
        }

        [[nodiscard]] std::string DigestByName(const std::string_view digest_name, const void *data, const size_t len)
        {
            if (data == nullptr && len > 0)
            {
                LOG_ERROR(g_logger) << "[哈希] 计算摘要失败：输入数据为空。";
                return {};
            }

            std::array<unsigned char, EVP_MAX_MD_SIZE> buffer{};
            size_t                                     out_len = 0;
            const std::string                          digest_name_str(digest_name);

#if OPENSSL_VERSION_MAJOR >= 3
            if (EVP_Q_digest(nullptr,
                             digest_name_str.c_str(),
                             nullptr,
                             data,
                             len,
                             buffer.data(),
                             &out_len) != 1)
            {
                LOG_ERROR(g_logger) << "[哈希] OpenSSL EVP_Q_digest 计算失败，算法=" << digest_name;
                return {};
            }
#else
            const EVP_MD *md = EVP_get_digestbyname(digest_name_str.c_str());
            if (md == nullptr)
            {
                LOG_ERROR(g_logger) << "[哈希] 获取摘要算法失败，算法=" << digest_name;
                return {};
            }

            EVP_MD_CTX *ctx = EVP_MD_CTX_new();
            if (ctx == nullptr)
            {
                LOG_ERROR(g_logger) << "[哈希] 创建摘要上下文失败，算法=" << digest_name;
                return {};
            }

            unsigned int out_len_u32 = 0;
            const bool   ok          = EVP_DigestInit_ex(ctx, md, nullptr) == 1 &&
                                       (len == 0 || EVP_DigestUpdate(ctx, data, len) == 1) &&
                                       EVP_DigestFinal_ex(ctx, buffer.data(), &out_len_u32) == 1;

            EVP_MD_CTX_free(ctx);

            if (!ok)
            {
                LOG_ERROR(g_logger) << "[哈希] 摘要计算失败，算法=" << digest_name;
                return {};
            }
            out_len = out_len_u32;
#endif

            return {reinterpret_cast<const char *>(buffer.data()), out_len};
        }

        [[nodiscard]] std::string HmacByName(const std::string_view digest_name,
                                             const std::string_view text,
                                             const std::string_view key)
        {
            std::array<unsigned char, EVP_MAX_MD_SIZE> buffer{};
            size_t                                     out_len = 0;
            const std::string                          digest_name_str(digest_name);

#if OPENSSL_VERSION_MAJOR >= 3
            EVP_MAC *mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
            if (mac == nullptr)
            {
                LOG_ERROR(g_logger) << "[哈希] 获取 OpenSSL HMAC 提供者失败。";
                return {};
            }

            EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
            if (ctx == nullptr)
            {
                LOG_ERROR(g_logger) << "[哈希] 创建 OpenSSL HMAC 上下文失败。";
                EVP_MAC_free(mac);
                return {};
            }

            auto *     digest_name_ptr = const_cast<char *>(digest_name_str.c_str());
            OSSL_PARAM params[]        = {
                OSSL_PARAM_construct_utf8_string(const_cast<char *>(OSSL_MAC_PARAM_DIGEST), digest_name_ptr, 0),
                OSSL_PARAM_construct_end()
            };

            const bool ok = EVP_MAC_init(ctx,
                                         reinterpret_cast<const unsigned char *>(key.data()),
                                         key.size(),
                                         params) == 1 &&
                            (text.empty() ||
                             EVP_MAC_update(ctx,
                                            reinterpret_cast<const unsigned char *>(text.data()),
                                            text.size()) == 1) &&
                            EVP_MAC_final(ctx, buffer.data(), &out_len, buffer.size()) == 1;

            EVP_MAC_CTX_free(ctx);
            EVP_MAC_free(mac);

            if (!ok)
            {
                LOG_ERROR(g_logger) << "[哈希] HMAC 计算失败，算法=" << digest_name;
                return {};
            }
#else
            const EVP_MD *md = EVP_get_digestbyname(digest_name_str.c_str());
            if (md == nullptr)
            {
                LOG_ERROR(g_logger) << "[哈希] 获取 HMAC 摘要算法失败，算法=" << digest_name;
                return {};
            }

            unsigned int out_len_tmp = 0;
            const auto * result      = HMAC(md,
                                            reinterpret_cast<const unsigned char *>(key.data()),
                                            static_cast<int>(key.size()),
                                            reinterpret_cast<const unsigned char *>(text.data()),
                                            text.size(),
                                            buffer.data(),
                                            &out_len_tmp);
            if (result == nullptr)
            {
                LOG_ERROR(g_logger) << "[哈希] HMAC 计算失败，算法=" << digest_name;
                return {};
            }
            out_len = out_len_tmp;
#endif

            return {reinterpret_cast<const char *>(buffer.data()), out_len};
        }
    }

    uint32_t murmur3_hash(const void *data, const uint32_t size, const uint32_t seed)
    {
        if (data == nullptr)
        {
            if (size > 0)
            {
                LOG_WARN(g_logger) << "[哈希] murmur3_hash 输入数据为空，返回 0。";
            }
            return 0;
        }

        const auto *bytes = static_cast<const uint8_t *>(data);
        uint32_t    h1    = seed;

        const uint32_t blocks = size / 4;
        for (uint32_t i = 0; i < blocks; ++i)
        {
            uint32_t k1 = LoadUint32LE(bytes + i * 4);
            k1          *= kMurmur3C1;
            k1          = std::rotl(k1, 15);
            k1          *= kMurmur3C2;

            h1 ^= k1;
            h1 = std::rotl(h1, 13);
            h1 = h1 * 5U + 0xe6546b64U;
        }

        const auto *tail = bytes + blocks * 4;
        uint32_t    k1   = 0;
        switch (size & 3U)
        {
            case 3:
                k1 ^= static_cast<uint32_t>(tail[2]) << 16U;
            case 2:
                k1 ^= static_cast<uint32_t>(tail[1]) << 8U;
            case 1:
                k1 ^= static_cast<uint32_t>(tail[0]);
                k1 *= kMurmur3C1;
                k1 = std::rotl(k1, 15);
                k1 *= kMurmur3C2;
                h1 ^= k1;
            default:
                break;
        }

        h1 ^= size;
        return Fmix32(h1);
    }

    uint32_t murmur3_hash(const char *str, const uint32_t seed)
    {
        if (str == nullptr)
        {
            return 0;
        }
        return murmur3_hash(str, static_cast<uint32_t>(std::strlen(str)), seed);
    }

    uint64_t murmur3_hash64(const void *str, const uint32_t size, const uint32_t seed, const uint32_t seed2)
    {
        return (static_cast<uint64_t>(murmur3_hash(str, size, seed)) << 32U) |
               murmur3_hash(str, size, seed2);
    }

    uint64_t murmur3_hash64(const char *str, const uint32_t seed, const uint32_t seed2)
    {
        return (static_cast<uint64_t>(murmur3_hash(str, seed)) << 32U) |
               murmur3_hash(str, seed2);
    }

    uint32_t quick_hash(const char *str)
    {
        if (str == nullptr)
        {
            return 0;
        }

        uint32_t h = 0;
        for (const unsigned char ch: std::string_view(str))
        {
            h = 31U * h + ch;
        }
        return h;
    }

    uint32_t quick_hash(const void *tmp, const uint32_t size)
    {
        if (tmp == nullptr)
        {
            if (size > 0)
            {
                LOG_WARN(g_logger) << "[哈希] quick_hash 输入数据为空，返回 0。";
            }
            return 0;
        }

        const auto *bytes = static_cast<const unsigned char *>(tmp);
        uint32_t    h     = 0;
        for (uint32_t i = 0; i < size; ++i)
        {
            h = 31U * h + bytes[i];
        }
        return h;
    }

    std::string base64decode(const std::string &src)
    {
        if (src.empty())
        {
            return {};
        }
        if (src.size() % 4 != 0)
        {
            LOG_WARN(g_logger) << "[哈希] Base64 解码失败：输入长度不是 4 的倍数。";
            return {};
        }

        std::string result;
        result.reserve(src.size() / 4 * 3);

        for (size_t i = 0; i < src.size(); i += 4)
        {
            std::array<int, 4> sextets{};
            int                padding = 0;

            for (size_t j = 0; j < 4; ++j)
            {
                const char ch = src[i + j];
                if (ch == '=')
                {
                    ++padding;
                    sextets[j] = 0;
                    continue;
                }
                if (padding > 0)
                {
                    LOG_WARN(g_logger) << "[哈希] Base64 解码失败：填充字符位置非法。";
                    return {};
                }

                const int value = DecodeBase64Char(ch);
                if (value < 0)
                {
                    LOG_WARN(g_logger) << "[哈希] Base64 解码失败：包含非法字符。";
                    return {};
                }
                sextets[j] = value;
            }

            if (padding > 2)
            {
                LOG_WARN(g_logger) << "[哈希] Base64 解码失败：填充字符数量非法。";
                return {};
            }
            if (padding > 0 && i + 4 != src.size())
            {
                LOG_WARN(g_logger) << "[哈希] Base64 解码失败：填充字符只能出现在末尾。";
                return {};
            }

            const uint32_t packed =
                    (static_cast<uint32_t>(sextets[0]) << 18U) |
                    (static_cast<uint32_t>(sextets[1]) << 12U) |
                    (static_cast<uint32_t>(sextets[2]) << 6U) |
                    static_cast<uint32_t>(sextets[3]);

            result.push_back(static_cast<char>((packed >> 16U) & 0xffU));
            if (padding != 2)
            {
                result.push_back(static_cast<char>((packed >> 8U) & 0xffU));
            }
            if (padding == 0)
            {
                result.push_back(static_cast<char>(packed & 0xffU));
            }
        }

        return result;
    }

    std::string base64encode(const std::string &data)
    {
        return base64encode(data.data(), data.size());
    }

    std::string base64encode(const void *data, const size_t len)
    {
        if (len == 0)
        {
            return {};
        }
        if (data == nullptr)
        {
            LOG_WARN(g_logger) << "[哈希] Base64 编码失败：输入数据为空。";
            return {};
        }

        const auto *bytes = static_cast<const unsigned char *>(data);
        std::string out;
        out.reserve(((len + 2) / 3) * 4);

        for (size_t i = 0; i < len; i += 3)
        {
            const size_t remain = len - i;
            uint32_t     packed = static_cast<uint32_t>(bytes[i]) << 16U;
            if (remain > 1)
            {
                packed |= static_cast<uint32_t>(bytes[i + 1]) << 8U;
            }
            if (remain > 2)
            {
                packed |= static_cast<uint32_t>(bytes[i + 2]);
            }

            out.push_back(kBase64Alphabet[(packed >> 18U) & 0x3fU]);
            out.push_back(kBase64Alphabet[(packed >> 12U) & 0x3fU]);
            out.push_back(remain > 1 ? kBase64Alphabet[(packed >> 6U) & 0x3fU] : '=');
            out.push_back(remain > 2 ? kBase64Alphabet[packed & 0x3fU] : '=');
        }

        return out;
    }

    std::string md5(const std::string &data)
    {
        const auto digest = md5sum(data);
        return hexstring_from_data(digest.data(), digest.size());
    }

    std::string sha1(const std::string &data)
    {
        const auto digest = sha1sum(data);
        return hexstring_from_data(digest.data(), digest.size());
    }

    std::string md5sum(const void *data, const size_t len)
    {
        return DigestByName("MD5", data, len);
    }

    std::string md5sum(const std::string &data)
    {
        return md5sum(data.data(), data.size());
    }

    std::string sha1sum(const void *data, const size_t len)
    {
        return DigestByName("SHA1", data, len);
    }

    std::string sha1sum(const std::string &data)
    {
        return sha1sum(data.data(), data.size());
    }

    std::string hmac_md5(const std::string &text, const std::string &key)
    {
        return HmacByName("MD5", text, key);
    }

    std::string hmac_sha1(const std::string &text, const std::string &key)
    {
        return HmacByName("SHA1", text, key);
    }

    std::string hmac_sha256(const std::string &text, const std::string &key)
    {
        return HmacByName("SHA256", text, key);
    }

    void hexstring_from_data(const void *data, const size_t len, char *output)
    {
        if (len == 0)
        {
            return;
        }
        if (data == nullptr || output == nullptr)
        {
            LOG_ERROR(g_logger) << "[哈希] 十六进制编码失败：输入或输出缓冲区为空。";
            throw std::invalid_argument("hexstring_from_data 输入或输出缓冲区为空");
        }

        constexpr std::array<char, 16> hex = {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
        };

        const auto *buf = static_cast<const unsigned char *>(data);
        for (size_t i = 0, j = 0; i < len; ++i)
        {
            output[j++] = hex[(buf[i] >> 4U) & 0x0fU];
            output[j++] = hex[buf[i] & 0x0fU];
        }
    }

    std::string hexstring_from_data(const void *data, const size_t len)
    {
        if (len == 0)
        {
            return {};
        }

        std::string result(len * 2, '\0');
        hexstring_from_data(data, len, result.data());
        return result;
    }

    std::string hexstring_from_data(const std::string &data)
    {
        return hexstring_from_data(data.data(), data.size());
    }

    void data_from_hexstring(const char *hexstring, const size_t length, void *output)
    {
        if (length == 0)
        {
            return;
        }
        if (length % 2 != 0)
        {
            LOG_ERROR(g_logger) << "[哈希] 十六进制解码失败：输入长度必须为偶数。";
            throw std::invalid_argument("data_from_hexstring 输入长度必须为偶数");
        }
        if (hexstring == nullptr || output == nullptr)
        {
            LOG_ERROR(g_logger) << "[哈希] 十六进制解码失败：输入或输出缓冲区为空。";
            throw std::invalid_argument("data_from_hexstring 输入或输出缓冲区为空");
        }

        auto *buf = static_cast<unsigned char *>(output);
        for (size_t i = 0, j = 0; i < length; i += 2, ++j)
        {
            const auto high = HexNibble(hexstring[i]);
            const auto low  = HexNibble(hexstring[i + 1]);
            if (!high.has_value() || !low.has_value())
            {
                LOG_ERROR(g_logger) << "[哈希] 十六进制解码失败：检测到非法字符。";
                throw std::invalid_argument("data_from_hexstring 包含非法十六进制字符");
            }
            buf[j] = static_cast<unsigned char>((*high << 4U) | *low);
        }
    }

    std::string data_from_hexstring(const char *hexstring, const size_t length)
    {
        if (length == 0)
        {
            return {};
        }
        if (length % 2 != 0)
        {
            LOG_ERROR(g_logger) << "[哈希] 十六进制解码失败：输入长度必须为偶数。";
            throw std::invalid_argument("data_from_hexstring 输入长度必须为偶数");
        }

        std::string result(length / 2, '\0');
        data_from_hexstring(hexstring, length, result.data());
        return result;
    }

    std::string data_from_hexstring(const std::string &hexstring)
    {
        return data_from_hexstring(hexstring.data(), hexstring.size());
    }

    std::string replace(const std::string &str1, const char find, const char replaceWith)
    {
        auto result = str1;
        std::ranges::replace(result, find, replaceWith);
        return result;
    }

    std::string replace(const std::string &str1, const char find, const std::string &replaceWith)
    {
        std::string result;
        result.reserve(str1.size());

        for (const char ch: str1)
        {
            if (ch == find)
            {
                result.append(replaceWith);
            } else
            {
                result.push_back(ch);
            }
        }
        return result;
    }

    std::string replace(const std::string &str1, const std::string &find, const std::string &replaceWith)
    {
        if (find.empty())
        {
            return str1;
        }

        std::string result;
        result.reserve(str1.size());

        size_t start = 0;
        while (start < str1.size())
        {
            const size_t pos = str1.find(find, start);
            if (pos == std::string::npos)
            {
                result.append(str1, start, std::string::npos);
                break;
            }
            result.append(str1, start, pos - start);
            result.append(replaceWith);
            start = pos + find.size();
        }

        if (start == str1.size())
        {
            return result;
        }

        return result;
    }

    std::vector<std::string> split(const std::string &str, const char delim, size_t max)
    {
        std::vector<std::string> result;
        if (str.empty())
        {
            return result;
        }

        size_t last = 0;
        size_t pos  = str.find(delim, last);
        while (pos != std::string::npos)
        {
            result.push_back(str.substr(last, pos - last));
            last = pos + 1;
            if (--max == 1)
            {
                break;
            }
            pos = str.find(delim, last);
        }
        result.push_back(str.substr(last));
        return result;
    }

    std::vector<std::string> split(const std::string &str, const char *delims, size_t max)
    {
        std::vector<std::string> result;
        if (str.empty())
        {
            return result;
        }
        if (delims == nullptr || delims[0] == '\0')
        {
            result.push_back(str);
            return result;
        }

        size_t last = 0;
        size_t pos  = str.find_first_of(delims, last);
        while (pos != std::string::npos)
        {
            result.push_back(str.substr(last, pos - last));
            last = pos + 1;
            if (--max == 1)
            {
                break;
            }
            pos = str.find_first_of(delims, last);
        }
        result.push_back(str.substr(last));
        return result;
    }

    std::string random_string(const size_t len, const std::string &chars)
    {
        if (len == 0 || chars.empty())
        {
            return {};
        }

        thread_local std::mt19937_64          engine{std::random_device{}()};
        std::uniform_int_distribution<size_t> distribution(0, chars.size() - 1);

        std::string out(len, '\0');
        std::ranges::generate(out, [&]()
        {
            return chars[distribution(engine)];
        });
        return out;
    }
}
