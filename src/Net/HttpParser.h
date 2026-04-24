/**
 * @file HttpParser.h
 * @brief HTTP 请求解析器，支持流式解析请求行、头部、普通 body 和 chunked body。
 */
#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include "Buffer.h"
#include "HttpMessage.h"

#include <system_error>

namespace Net::Http
{
    /**
     * @brief HTTP 请求解析器类。
     * @details 维护内部状态机，支持普通 Content-Length 和 Transfer-Encoding: chunked 两种 body 传输方式。
     *          调用 parse() 不断喂入 Buffer，直到解析出一个完整的 HttpRequest 对象，然后自动重置等待下一个请求。
     */
    class HttpRequestParser
    {
    public:
        /**
         * @brief 解析器状态枚举。
         */
        enum class State
        {
            RequestLine, ///< 正在解析请求行
            Headers,     ///< 正在解析头部
            Body,        ///< 正在解析普通消息体（基于 Content-Length）
            BodyChunked, ///< 正在解析分块传输的 body
            Complete,    ///< 一个完整请求已解析完成（实际不会停留，会直接返回）
            Error        ///< 解析错误
        };

        /**
         * @brief 默认构造函数，初始化解析器状态为 RequestLine。
         */
        HttpRequestParser() = default;

        /**
         * @brief 向解析器输入新数据缓冲区，尝试解析出一个完整的 HTTP 请求。
         * @param buffer 输入缓冲区，其中包含尚未解析的网络数据。
         * @return 若成功解析一个完整请求，返回包含 HttpRequest 的 optional；否则返回 nullopt。
         */
        std::optional<HttpRequest> parse(Buffer &buffer);

    private:
        /**
         * @brief 分块解析的内部子状态。
         */
        enum class ChunkState
        {
            Size,    ///< 等待读取分块大小行
            Data,    ///< 读取分块数据块
            Trailer, ///< 读取尾部（trailer headers）部分
            End      ///< 分块完成，无更多数据
        };

        /**
         * @brief 解析请求行。
         * @param buf 输入缓冲区。
         * @return 成功返回 true，失败返回 false（并置状态为 Error）。
         */
        bool parseRequestLine(Buffer &buf);

        /**
         * @brief 解析 HTTP 头部。
         * @param buf 输入缓冲区。
         * @return 成功返回 true，失败返回 false（状态置 Error）。
         * @details 持续读取行，直到遇到空行（表示头部结束）。
         *          每行格式：键: 值，键名转换为小写存储，值去除首尾空白。
         */
        bool parseHeaders(Buffer &buf);

        /**
         * @brief 解析基于 Content-Length 的普通消息体。
         * @param buf 输入缓冲区。
         * @return 成功返回 true，失败（数据不足）返回 false。
         */
        bool parseBody(Buffer &buf);

        /**
         * @brief 解析 chunked 分块传输的 body。
         * @param buf 输入缓冲区。
         * @return 成功返回 true，失败（数据不足或格式错误）返回 false。
         */
        bool parseChunkedBody(Buffer &buf);

        /**
         * @brief 从缓冲区中提取一行（以 \r\n 结尾），返回不包含 \r\n 的字符串视图。
         * @param buf 输入缓冲区。
         * @return 若找到完整的行则返回 string_view；若数据不足则返回 nullopt。
         */
        std::optional<std::string_view> extractLine(Buffer &buf);

        /**
         * @brief 完成一个请求的解析，返回请求对象并重置解析器内部状态。
         * @return 完整解析的 HttpRequest。
         */
        std::optional<HttpRequest> finalize();

        /**
         * @brief 重置解析器状态，准备解析下一个请求。
         */
        void reset();

        State m_state = State::RequestLine;         ///< 当前主解析状态
        HttpRequest m_request;                      ///< 正在构建的请求对象
        size_t m_contentLength = 0;                 ///< Content-Length 值（非分块时使用）
        size_t m_chunkSize = 0;                     ///< 当前分块大小
        ChunkState m_chunkState = ChunkState::Size; ///< 分块解析子状态
        std::string m_lineBuffer;                   ///< 暂存当前提取的行内容，用于返回 string_view
    };
}

#endif // HTTPPARSER_H
