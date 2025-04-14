/**
 * @file http11_common.h
 * @brief 该文件定义了 HTTP/1.1 协议中常见的回调函数类型和相关定义
 * @date 2025-03-16
 */
#ifndef HTTP11_COMMON_H
#define HTTP11_COMMON_H
#include <sys/types.h>

/**
 * @brief 元素回调函数类型
 * @param data 额外的用户数据，可用于传递上下文
 * @param at 元素的起始地址
 * @param length 元素的长度
 */
typedef void (*element_cb)(void* data, const char* at, size_t length);

/**
 * @brief 字段回调函数类型
 * @param data 额外的用户数据，可用于传递上下文
 * @param field 字段名（例如 `Content-Type`）
 * @param flen 字段名的长度
 * @param value 字段的值（例如 `text/html`）
 * @param vlen 字段值的长度
 */
typedef void (*field_cb)(void* data, const char* field, size_t flen, const char* value, size_t vlen);

#endif
