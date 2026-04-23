/**
 * @file ConfigType.h
 * @brief 配置模块基础类型定义、异常类与通用工具
 * @copyright Copyright (c) 2026
 */
#ifndef CONFIGTYPE_H
#define CONFIGTYPE_H

#include <string>
#include <string_view>
#include <vector>

namespace Base
{
    /**
     * @brief 配置值的基础类型
     */
    enum class ConfigValueType : uint8_t
    {
        Null,   ///< 空值
        Bool,   ///< 布尔类型
        Int,    ///< 整数类型 (int64_t)
        Double, ///< 浮点类型 (double)
        String, ///< 字符串类型
        Array,  ///< 数组类型
        Object  ///< 对象类型（嵌套）
    };

    /**
     * @brief 将配置值类型枚举转换为可读字符串。
     * @param type 配置值类型枚举。
     * @return const char* 对应的类型名称，未知类型返回 "unknown"。
     */
    const char *typeName(ConfigValueType type) noexcept;

    /**
     * @brief 判断文件路径是否为 YAML 配置文件后缀。
     * @param file_path 待检查的文件路径。
     * @return bool 当后缀为 .yaml 或 .yml 时返回 true。
     */
    bool isYamlFile(std::string_view file_path) noexcept;

    /**
     * @brief 使用指定分隔符拆分配置键字符串。
     * @param key 待拆分的原始键字符串。
     * @param delimiter 分隔字符。
     * @return std::vector<std::string> 拆分后的键片段列表。
     */
    std::vector<std::string> splitKey(std::string_view key, char delimiter = '.');
}

#endif //CONFIGTYPE_H
