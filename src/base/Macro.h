/**
 * @file Macro.h
 * @brief 常用宏的封装
 * @date 2025-03-13
 */
#ifndef MACRO_H
#define MACRO_H

#include <iostream>
#include <cassert>
#include "base/Log.h"
#include "base/Utils.h"

/// ======================== 分支预测优化 ========================
#if defined(__GNUC__) || defined(__llvm__)
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
    if(UNLIKELY(!(expr))) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION FAILED: Expression '" #expr "' evaluated to false." \
                              << "\nBacktrace:\n" \
                              << Gyanis::base::backtraceToString(100, 2, "    "); \
                              assert(expr); \
    }

/**
 * @brief ASSERT_MSG宏：断言条件表达式是否为真，并输出自定义错误消息
 */
#define ASSERT_MSG(expr, msg) \
    if(UNLIKELY(!(expr))) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION FAILED: Expression '" #expr "' evaluated to false." \
                              << "\nMessage: " << (msg) \
                              << "\nBacktrace:\n" \
                              << Gyanis::base::backtraceToString(100, 2, "    "); \
                              assert(expr); \
    }


#endif
