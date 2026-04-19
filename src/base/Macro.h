#ifndef MACRO_H
#define MACRO_H

#include <cassert>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>

#include "Log.h"
#include "base/Utils.h"

namespace Gyanis::base::detail
{
    inline std::string BuildAssertFailureMessage(
        const std::string_view      expression,
        const std::string_view      message,
        const std::source_location &location = std::source_location::current())
    {
        std::ostringstream stream;
        stream << "断言失败"
                << " | 表达式: " << expression
                << " | 文件: " << location.file_name()
                << " | 行号: " << location.line()
                << " | 函数: " << location.function_name();

        if (!message.empty())
        {
            stream << " | 说明: " << message;
        }

        return stream.str();
    }
}

/// ======================== 分支预测优化 ========================
#if defined(__GNUC__) || defined(__clang__)
/**
 * @brief LIKELY宏：提示编译器条件表达式大概率为真
 */
#define LIKELY(x)   __builtin_expect(!!(x), 1)  // 条件大概率为真

/**
 * @brief UNLIKELY宏：提示编译器条件表达式大概率为假
 */
#define UNLIKELY(x) __builtin_expect(!!(x), 0)  // 条件大概率为假
#else
/// 如果不是GCC或LLVM编译器，直接返回表达式本身
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

/// ======================== 断言封装 ========================
/**
 * @brief ASSERT宏：断言条件表达式是否为真
 */
#define ASSERT(expr) \
    do { \
        if (UNLIKELY(!(expr))) { \
            LOG_ERROR(LOG_ROOT()) << Gyanis::base::detail::BuildAssertFailureMessage( \
                                      #expr, \
                                      "", \
                                      std::source_location::current()) \
                                  << "\n调用栈:\n" \
                                  << Gyanis::base::backtraceToString(100, 2, "    "); \
            assert(expr); \
        } \
    } while (0)

/**
 * @brief ASSERT_MSG宏：断言条件表达式是否为真，并输出自定义错误消息
 */
#define ASSERT_MSG(expr, msg) \
    do { \
        if (UNLIKELY(!(expr))) { \
            LOG_ERROR(LOG_ROOT()) << Gyanis::base::detail::BuildAssertFailureMessage( \
                                      #expr, \
                                      (msg), \
                                      std::source_location::current()) \
                                  << "\n调用栈:\n" \
                                  << Gyanis::base::backtraceToString(100, 2, "    "); \
            assert(expr); \
        } \
    } while (0)


#endif
