# JSON工具接口

## 范围

本文档描述以下接口：

- src/base/JsonUtils.h
- src/base/JsonUtils.cpp

适用平台：

- Ubuntu（Linux）
- Windows

---

## 模块目标

JsonUtils 模块用于 JSON 字符串处理与常见字段提取，提供：

- 字符串转义检查与转义。
- 字符串和数值字段读取（带默认值回退）。
- 字符串与 JSON 对象互转。

---

## 接口说明

### NeedEscape(value)

用途：

- 判断字符串是否包含需要 JSON 转义的字符。

当前支持的转义字符：

- `\f`、`\t`、`\r`、`\n`、`\b`、`"`、`\\`。

返回：

- true：至少包含一个需转义字符。
- false：无需转义。

### Escape(value)

用途：

- 将字符串中的控制字符和引号、反斜杠转为 JSON 可安全表示的转义形式。

返回：

- 转义后的字符串。

### GetString(json, name, default_value)

用途：

- 读取字符串字段。

行为：

- 字段不存在或类型不是字符串时，返回 default_value。

### GetDouble(json, name, default_value)

用途：

- 读取数值字段并转为 double。

行为：

- 字段不存在或不是数值类型时，返回 default_value。

### GetInt32 / GetInt64

用途：

- 读取整数字段并转为有符号整数。

行为：

- 支持读取 signed 和 unsigned JSON 整数。
- 若值超出目标类型范围，返回 default_value。

### GetUint32 / GetUint64

用途：

- 读取整数字段并转为无符号整数。

行为：

- 支持读取 signed 和 unsigned JSON 整数。
- 若值为负数或超出目标类型范围，返回 default_value。

### FromString(json, value)

用途：

- 解析 JSON 字符串到 json 对象。

返回：

- true：解析成功。
- false：解析失败。

日志：

- 解析失败时输出中文错误日志（含异常信息）。

### ToString(json)

用途：

- 将 json 对象序列化为字符串。

返回：

- 序列化结果。

---

## C++20 重构点

当前实现使用了以下 C++20 能力：

- std::ranges::any_of
- unordered_set::contains
- std::integral / std::signed_integral
- std::in_range

重构收益：

- 整型读取逻辑统一且类型安全。
- signed 与 unsigned 互读行为更一致。
- 溢出与负值场景由范围检查统一处理。

---

## 跨平台说明（Ubuntu + Windows）

- 模块仅依赖 C++ 标准库与 nlohmann::json。
- 不依赖平台专有系统调用或头文件。
- 在 Ubuntu（GCC/Clang）与 Windows（MSVC）下行为一致。

---

## 中文日志规范

本模块错误日志以中文为主，建议格式：

- [JSON] 行为描述 | 错误: 详情

示例：

- [JSON] 字符串解析失败 | 错误: parse error ...

---

## 最小示例

```cpp
#include "base/JsonUtils.h"

nlohmann::json doc;
if (Gyanis::base::JsonUtils::FromString(doc, R"({"name":"svc","port":8080})"))
{
    const auto name = Gyanis::base::JsonUtils::GetString(doc, "name", "unknown");
    const auto port = Gyanis::base::JsonUtils::GetUint32(doc, "port", 80);
}
```

---

## 对应测试

- tests/base/TestJsonUtils.cpp

覆盖点：

- 转义字符识别与转义输出。
- signed/unsigned 混合整型读取语义。
- 字符串解析与序列化基本行为。
