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
    // ============================================================================
    // 配置值类型枚举
    // ============================================================================

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
     * @brief 获取类型名称字符串
     */
    const char *typeName(ConfigValueType type) noexcept;

    // ============================================================================
    // 文件扩展名工具
    // ============================================================================

    /**
     * @brief 检查文件是否为 YAML/YML 格式
     */
    bool isYamlFile(std::string_view file_path) noexcept;

    /**
     * @brief 字符串分割工具（用于解析点号分隔的键）
     */
    std::vector<std::string> splitKey(std::string_view key, char delimiter = '.');
}

#endif //CONFIGTYPE_H
