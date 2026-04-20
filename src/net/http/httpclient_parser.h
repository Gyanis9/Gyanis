/**
 * @file httpclient_parser.h
 * @brief HTTP 客户端解析器模块封装
 * @date 2025-03-16
 */
#ifndef HTTPCLIENT_PARSER_H
#define HTTPCLIENT_PARSER_H
#include "http11_common.h"

/**
 * @brief HTTP 客户端解析器结构体
 */
typedef struct httpclient_parser
{
    int cs{}; ///< 当前解析状态机的状态
    size_t body_start{}; ///< 响应体的起始位置
    int content_len{}; ///< 响应体的内容长度
    int status{}; ///< HTTP 响应状态码
    int chunked{}; ///< 是否启用了分块传输编码
    int chunks_done{}; ///< 已完成的分块数量
    int close{}; ///< 是否关闭连接的标志
    size_t nread{}; ///< 已读取的数据长度
    size_t mark{}; ///< 标记当前解析的位置
    size_t field_start{}; ///< 当前字段的起始位置
    size_t field_len{}; ///< 当前字段的长度
    void* data{}; ///< 用户定义的数据，可以在回调中使用
    field_cb http_field{}; ///< 解析 HTTP 字段的回调函数
    element_cb reason_phrase{}; ///< 解析 HTTP 响应原因短语的回调函数
    element_cb status_code{}; ///< 解析 HTTP 响应状态码的回调函数
    element_cb chunk_size{}; ///< 解析 HTTP 分块大小的回调函数
    element_cb http_version{}; ///< 解析 HTTP 版本的回调函数
    element_cb header_done{}; ///< HTTP 头部解析完成的回调函数
    element_cb last_chunk{}; ///< 解析最后一个分块的回调函数
} httpclient_parser;

/**
 * @brief 初始化 HTTP 客户端解析器
 * @param parser HTTP 客户端解析器对象
 * @return int 返回 0 表示初始化成功，非 0 值表示初始化失败
 */
int httpclient_parser_init(httpclient_parser* parser);

/**
 * @brief 完成 HTTP 客户端解析
 * @param parser HTTP 客户端解析器对象
 * @return int 返回 0 表示解析完成，非 0 值表示失败
 */
int httpclient_parser_finish(httpclient_parser* parser);

/**
 * @brief 执行 HTTP 客户端解析
 * @param parser HTTP 客户端解析器对象
 * @param data 输入数据
 * @param len 数据长度
 * @param off 偏移量，指示从数据的哪个位置开始解析
 * @return size_t 返回已成功解析的字节数
 */
int httpclient_parser_execute(httpclient_parser* parser, const char* data, size_t len, size_t off);

/**
 * @brief 检查 HTTP 客户端解析器是否出现错误
 * @param parser HTTP 客户端解析器对象
 * @return int 如果发生错误返回非零值，否则返回 0
 */
int httpclient_parser_has_error(httpclient_parser* parser);

/**
 * @brief 检查 HTTP 客户端解析是否完成
 * @param parser HTTP 客户端解析器对象
 * @return int 如果解析完成返回非零值，否则返回 0
 */
int httpclient_parser_is_finished(httpclient_parser* parser);

/**
 * @brief 获取已读取的数据长度
 * @param parser HTTP 客户端解析器对象
 * @return size_t 返回已读取的数据长度
 */
#define httpclient_parser_nread(parser) (parser)->nread

#endif
