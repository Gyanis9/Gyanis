/**
* @file HttpSerializer.h
 * @brief HTTP 响应序列化器，将 HttpResponse 对象转换为字节流写入 Buffer。
 * @details 提供简单的序列化功能，生成符合 HTTP/1.1 标准的响应格式（状态行 + 头部 + 空行 + Body）。
 */

#ifndef HTTPSERIALIZER_H
#define HTTPSERIALIZER_H

#include "Buffer.h"
#include "HttpMessage.h"

#include <format>

namespace Net::Http
{
    /**
     * @brief HTTP 响应序列化器。
     * @details 将 HttpResponse 对象序列化为字节序列，便于通过网络发送。
     *          目前仅支持 HTTP/1.1。
     */
    class HttpResponseSerializer
    {
    public:
        /**
         * @brief 将 HttpResponse 对象序列化并追加到输出缓冲区。
         * @param res 要序列化的响应对象。
         * @param output 目标缓冲区，数据追加到末尾。
         * @details 序列化格式：
         *          - 状态行：HTTP/1.1 <statusCode> <statusMessage>\r\n
         *          - 头部：Key: Value\r\n（每个头部一行）
         *          - 空行：\r\n
         *          - 消息体（如果非空）
         */
        void serialize(const HttpResponse &res, Buffer &output) const
        {
            // 状态行
            output.append(std::format("HTTP/1.1 {} {}\r\n", res.statusCode, res.statusMessage));
            // 头部
            for (const auto &[key, val]: res.headers)
            {
                output.append(std::format("{}: {}\r\n", key, val));
            }
            output.append("\r\n");
            if (!res.body.empty())
            {
                output.append(res.body);
            }
        }
    };
}

#endif