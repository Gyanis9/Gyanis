#ifndef HASHUTILS_H
#define HASHUTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace Gyanis::base
{
    /**
     * @brief Murmur3 哈希函数 (32位版本)
     * @param[in] str 待计算哈希的字符串
     * @param[in] seed 哈希种子，默认值为 1060627423
     * @return 返回计算出的 32 位哈希值
     */
    [[nodiscard]] uint32_t murmur3_hash(const char *str, uint32_t seed = 1060627423);

    /**
     * @brief Murmur3 哈希函数 (32位版本) - 从内存数据计算
     * @param[in] data 指向内存数据的指针
     * @param[in] size 数据大小
     * @param[in] seed 哈希种子，默认值为 1060627423
     * @return 返回计算出的 32 位哈希值
     */
    [[nodiscard]] uint32_t murmur3_hash(const void *data, uint32_t size, uint32_t seed = 1060627423);

    /**
     * @brief Murmur3 哈希函数 (64位版本)
     * @param[in] str 待计算哈希的字符串
     * @param[in] seed 哈希种子，默认值为 1060627423
     * @param[in] seed2 第二个哈希种子，默认值为 1050126127
     * @return 返回计算出的 64 位哈希值
     */
    [[nodiscard]] uint64_t murmur3_hash64(const char *str, uint32_t seed = 1060627423, uint32_t seed2 = 1050126127);


    /**
     * @brief Murmur3 哈希函数 (64位版本) - 从内存数据计算
     * @param[in] str 指向内存数据的指针
     * @param[in] size 数据大小
     * @param[in] seed 哈希种子，默认值为 1060627423
     * @param[in] seed2 第二个哈希种子，默认值为 1050126127
     * @return 返回计算出的 64 位哈希值
     */
    [[nodiscard]] uint64_t murmur3_hash64(const void *str, uint32_t size, uint32_t seed = 1060627423,
                                          uint32_t seed2 = 1050126127);

    /**
     * @brief 快速哈希函数 (32位版本)
     * @param[in] str 待计算哈希的字符串
     * @return 返回计算出的 32 位哈希值
     */
    [[nodiscard]] uint32_t quick_hash(const char *str);

    /**
     * @brief 快速哈希函数 (32位版本) - 从内存数据计算
     * @param[in] tmp 指向内存数据的指针
     * @param[in] size 数据大小
     * @return 返回计算出的 32 位哈希值
     */
    [[nodiscard]] uint32_t quick_hash(const void *tmp, uint32_t size);

    /**
     * @brief Base64 解码
     * @param[in] src Base64 编码的字符串
     * @return 返回解码后的字符串
     */
    [[nodiscard]] std::string base64decode(const std::string &src);

    /**
     * @brief Base64 编码
     * @param[in] data 待编码的原始数据
     * @return 返回 Base64 编码后的字符串
     */
    [[nodiscard]] std::string base64encode(const std::string &data);

    /**
     * @brief Base64 编码 - 从内存数据计算
     * @param[in] data 指向原始数据的指针
     * @param[in] len 数据长度
     * @return 返回 Base64 编码后的字符串
     */
    [[nodiscard]] std::string base64encode(const void *data, size_t len);

    /**
     * @brief MD5 哈希计算
     * @param[in] data 待计算哈希的字符串
     * @return 返回计算出的 MD5 哈希值
     */
    [[nodiscard]] std::string md5(const std::string &data);

    /**
     * @brief SHA1 哈希计算
     * @param[in] data 待计算哈希的字符串
     * @return 返回计算出的 SHA1 哈希值
     */
    [[nodiscard]] std::string sha1(const std::string &data);

    /**
     * @brief MD5 校验和计算
     * @param[in] data 待计算校验和的字符串
     * @return 返回计算出的 MD5 校验和
     */
    [[nodiscard]] std::string md5sum(const std::string &data);

    /**
     * @brief MD5 校验和计算 - 从内存数据计算
     * @param[in] data 指向内存数据的指针
     * @param[in] len 数据长度
     * @return 返回计算出的 MD5 校验和
     */
    [[nodiscard]] std::string md5sum(const void *data, size_t len);

    /**
     * @brief SHA1 校验和计算
     * @param[in] data 待计算校验和的字符串
     * @return 返回计算出的 SHA1 校验和
     */
    [[nodiscard]] std::string sha1sum(const std::string &data);

    /**
     * @brief SHA1 校验和计算 - 从内存数据计算
     * @param[in] data 指向内存数据的指针
     * @param[in] len 数据长度
     * @return 返回计算出的 SHA1 校验和
     */
    [[nodiscard]] std::string sha1sum(const void *data, size_t len);

    /**
     * @brief HMAC-MD5 计算
     * @param[in] text 待计算 HMAC 的文本
     * @param[in] key HMAC 使用的密钥
     * @return 返回计算出的 HMAC-MD5 值
     */
    [[nodiscard]] std::string hmac_md5(const std::string &text, const std::string &key);

    /**
     * @brief HMAC-SHA1 计算
     * @param[in] text 待计算 HMAC 的文本
     * @param[in] key HMAC 使用的密钥
     * @return 返回计算出的 HMAC-SHA1 值
     */
    [[nodiscard]] std::string hmac_sha1(const std::string &text, const std::string &key);

    /**
     * @brief HMAC-SHA256 计算
     * @param[in] text 待计算 HMAC 的文本
     * @param[in] key HMAC 使用的密钥
     * @return 返回计算出的 HMAC-SHA256 值
     */
    [[nodiscard]] std::string hmac_sha256(const std::string &text, const std::string &key);

    /**
     * @brief 将数据转换为十六进制字符串
     * @param[in] data 指向内存数据的指针
     * @param[in] len 数据长度
     * @param[out] output 输出缓冲区，保存十六进制字符串
     */
    void hexstring_from_data(const void *data, size_t len, char *output);

    /**
     * @brief 将数据转换为十六进制字符串
     * @param[in] data 指向内存数据的指针
     * @param[in] len 数据长度
     * @return 返回转换后的十六进制字符串
     */
    [[nodiscard]] std::string hexstring_from_data(const void *data, size_t len);

    /**
     * @brief 将字符串转换为十六进制字符串
     * @param[in] data 待转换的字符串
     * @return 返回转换后的十六进制字符串
     */
    [[nodiscard]] std::string hexstring_from_data(const std::string &data);

    /**
     * @brief 将十六进制字符串转换为数据
     * @param[in] hexstring 十六进制字符串
     * @param[in] length 字符串长度
     * @param[out] output 输出缓冲区，保存转换后的数据
     */
    void data_from_hexstring(const char *hexstring, size_t length, void *output);

    /**
     * @brief 将十六进制字符串转换为数据
     * @param[in] hexstring 十六进制字符串
     * @param[in] length 字符串长度
     * @return 返回转换后的原始数据
     */
    [[nodiscard]] std::string data_from_hexstring(const char *hexstring, size_t length);

    /**
     * @brief 将十六进制字符串转换为数据
     * @param[in] hexstring 十六进制字符串
     * @return 返回转换后的原始数据
     */
    [[nodiscard]] std::string data_from_hexstring(const std::string &hexstring);

    /**
     * @brief 替换字符串中的指定字符
     * @param[in] str 待处理的字符串
     * @param[in] find 要查找的字符
     * @param[in] replaceWith 替换为的字符
     * @return 返回替换后的字符串
     */
    [[nodiscard]] std::string replace(const std::string &str, char find, char replaceWith);

    /**
     * @brief 替换字符串中的指定字符
     * @param[in] str 待处理的字符串
     * @param[in] find 要查找的字符
     * @param[in] replaceWith 替换为的字符串
     * @return 返回替换后的字符串
     */
    [[nodiscard]] std::string replace(const std::string &str, char find, const std::string &replaceWith);

    /**
     * @brief 替换字符串中的指定子字符串
     * @param[in] str 待处理的字符串
     * @param[in] find 要查找的子字符串
     * @param[in] replaceWith 替换为的子字符串
     * @return 返回替换后的字符串
     */
    [[nodiscard]] std::string replace(const std::string &str, const std::string &find, const std::string &replaceWith);

    /**
     * @brief 按指定分隔符分割字符串
     * @param[in] str 待拆分的字符串
     * @param[in] delim 分隔符
     * @param[in] max 最大拆分次数
     * @return 返回拆分后的字符串向量
     */
    [[nodiscard]] std::vector<std::string> split(const std::string &str, char delim, size_t max = ~0U);

    /**
     * @brief 按多个指定分隔符分割字符串
     * @param[in] str 待拆分的字符串
     * @param[in] delims 分隔符字符串
     * @param[in] max 最大拆分次数
     * @return 返回拆分后的字符串向量
     */
    [[nodiscard]] std::vector<std::string> split(const std::string &str, const char *delims, size_t max = ~0U);

    /**
     * @brief 生成随机字符串
     * @param[in] len 字符串的长度
     * @param[in] chars 可选的字符集，默认包含数字和字母
     * @return 返回生成的随机字符串
     */
        [[nodiscard]] std::string random_string(size_t             len,
                            const std::string &chars =
                                "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

#endif
