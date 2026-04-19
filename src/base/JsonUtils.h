#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace Gyanis::base
{
    /**
     * @brief JSON 工具类
     */
    class JsonUtils
    {
    public:
        /**
         * @brief 检查字符串是否需要转义
         */
        [[nodiscard]] static bool NeedEscape(const std::string &value);

        /**
         * @brief 转义字符串中的特殊字符
         */
        [[nodiscard]] static std::string Escape(const std::string &value);

        /**
         * @brief 从 JSON 中获取字符串值
         */
        [[nodiscard]] static std::string GetString(const nlohmann::json &json
                                                   , const std::string & name
                                                   , const std::string & default_value = "");

        /**
         * @brief 从 JSON 中获取浮动类型值
         */
        [[nodiscard]] static double GetDouble(const nlohmann::json &json
                                              , const std::string & name
                                              , double              default_value = 0);

        /**
         * @brief 从 JSON 中获取 32 位整数值
         */
        [[nodiscard]] static int32_t GetInt32(const nlohmann::json &json
                                              , const std::string & name
                                              , int32_t             default_value = 0);

        /**
         * @brief 从 JSON 中获取 32 位无符号整数值
         */
        [[nodiscard]] static uint32_t GetUint32(const nlohmann::json &json
                                                , const std::string & name
                                                , uint32_t            default_value = 0);

        /**
         * @brief 从 JSON 中获取 64 位整数值
         */
        [[nodiscard]] static int64_t GetInt64(const nlohmann::json &json
                                              , const std::string & name
                                              , int64_t             default_value = 0);

        /**
         * @brief 从 JSON 中获取 64 位无符号整数
         */
        [[nodiscard]] static uint64_t GetUint64(const nlohmann::json &json
                                                , const std::string & name
                                                , uint64_t            default_value = 0);

        /**
         * @brief 将字符串转换为 JSON 对象
         */
        [[nodiscard]] static bool FromString(nlohmann::json &json, const std::string &value);

        /**
         * @brief 将 JSON 对象转换为字符串
         */
        [[nodiscard]] static std::string ToString(const nlohmann::json &json);
    };
}

#endif
