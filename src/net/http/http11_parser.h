/**
 * @file http11_parser.h
 * @brief HTTP/1.1 协议解析器
 *
 * 本文件定义了 HTTP/1.1 协议解析器的结构体和相关函数，用于解析 HTTP 请求和响应的各个部分
 *
 * @date 2025-03-16
 */
#ifndef HTTP11_PARSER_H
#define HTTP11_PARSER_H
#include "http11_common.h"

/**
 * @brief HTTP 解析器结构体
 */
typedef struct http_parser
{
    int cs; ///< 当前解析状态机的状态
    size_t body_start; ///< 请求体的起始位置
    int content_len; ///< 请求体的内容长度
    size_t nread; ///< 已读取的数据长度
    size_t mark; ///< 标记当前解析的位置
    size_t field_start; ///< 当前字段的起始位置
    size_t field_len; ///< 当前字段的长度
    size_t query_start; ///< 查询字符串的起始位置
    int xml_sent; ///< 是否已发送 XML 数据标志
    int json_sent; ///< 是否已发送 JSON 数据标志
    void* data; ///< 用户定义的数据，可以在回调中使用
    int uri_relaxed; ///< URI 是否允许宽松解析
    field_cb http_field; ///< HTTP 字段的回调函数
    element_cb request_method; ///< 请求方法的回调函数
    element_cb request_uri; ///< 请求 URI 的回调函数
    element_cb fragment; ///< URI 片段的回调函数
    element_cb request_path; ///< 请求路径的回调函数
    element_cb query_string; ///< 查询字符串的回调函数
    element_cb http_version; ///< HTTP 版本的回调函数
    element_cb header_done; ///< HTTP 头部解析完成的回调函数
} http_parser;

/**
 * @brief 初始化 HTTP 解析器
 *
 * @param parser HTTP 解析器对象
 * @return int 返回 0 表示初始化成功，非 0 值表示初始化失败
 */
int http_parser_init(http_parser* parser);

/**
 * @brief 完成 HTTP 解析
 *
 * @param parser HTTP 解析器对象
 * @return int 返回 0 表示解析完成，非 0 值表示失败
 */
int http_parser_finish(http_parser* parser);

/**
 * @brief 执行 HTTP 解析
 *
 * @param parser HTTP 解析器对象
 * @param data 输入数据
 * @param len 数据长度
 * @param off 偏移量，指示从数据的哪个位置开始解析
 * @return size_t 返回已成功解析的字节数
 */
size_t http_parser_execute(http_parser* parser, const char* data, size_t len, size_t off);

/**
 * @brief 检查 HTTP 解析器是否出现错误
 *
 * @param parser HTTP 解析器对象
 * @return int 如果发生错误返回非零值，否则返回 0
 */
int http_parser_has_error(http_parser* parser);

/**
 * @brief 检查 HTTP 解析是否完成
 *
 * @param parser HTTP 解析器对象
 * @return int 如果解析完成返回非零值，否则返回 0
 */
int http_parser_is_finished(http_parser* parser);

/**
 * @brief 获取解析器已读取的数据长度
 * 
 * @param parser HTTP 解析器对象
 * @return size_t 返回已读取的数据长度
 */
#define http_parser_nread(parser) (parser)->nread
#endif
