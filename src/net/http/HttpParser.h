/**
 * @file HttpParser.h
 * @brief HTTP协议解析封装
 * @date 2025-03-16
 */
#ifndef HTTPPARSER_H
#define HTTPPARSER_H
#include "Http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace Gyanis::net::http
{
    /**
     * @brief HTTP 请求解析器
     */
    class HttpRequestParser
    {
    public:
        /**
         * @brief 构造 HTTP 请求解析器
         */
        HttpRequestParser();

        /**
         * @brief 析构 HTTP 请求解析器
         */
        ~HttpRequestParser();

        /**
         * @brief 执行 HTTP 请求解析
         * @param data 输入数据 
         * @param size 输入数据的长度 
         * @return size_t 返回已解析的字节数 
         */
        size_t execute(char* data, size_t size);

        /**
         * @brief 检查解析是否完成
         * @return int 如果解析完成返回非零值，否则返回 0 
         */
        int isFinished();

        /**
         * @brief 检查解析是否出现错误
         * @return int 如果出现错误返回非零值，否则返回 0 
         */
        int hasError();

        /**
         * @brief 获取 HTTP 请求数据
         */
        [[nodiscard]] std::shared_ptr<HttpRequest> getData() const;

        /**
         * @brief 设置解析错误码
         */
        void setError(int errorCode);

        /**
         * @brief 获取请求体内容长度
         */
        [[nodiscard]] uint64_t getContentLength() const;

        /**
         * @brief 获取 HTTP 请求解析器
         */
        [[nodiscard]] const http_parser& getParser() const;

        /**
         * @brief 获取 HTTP 请求缓冲区大小
         */
        static uint64_t GetHttpRequestBufferSize();

        /**
         * @brief 获取 HTTP 请求最大请求体大小
         */
        static uint64_t GetHttpRequestMaxBodySize();

    private:
        http_parser m_parser{}; ///< HTTP 请求解析器实例 
        std::shared_ptr<HttpRequest> m_data = nullptr; ///< 存储解析后的 HTTP 请求数据 
        /** 错误码
         1000: invalid method
         1001: invalid version
         1002: invalid field
         */
        int m_error; ///< 错误码，标识解析过程中发生的错误 
    };

    /**
     * @brief HTTP 响应解析器
     */
    class HttpResponseParser
    {
    public:
        /**
         * @brief 构造 HTTP 响应解析器
         */
        HttpResponseParser();

        /**
         * @brief 析构 HTTP 响应解析器
         */
        ~HttpResponseParser();

        /**
         * @brief 执行 HTTP 响应解析
         * @param data 输入数据 
         * @param size 输入数据的长度 
         * @param chunk 是否启用分块传输编码 
         * @return size_t 返回已解析的字节数 
         */
        size_t execute(char* data, size_t size, bool chunk);

        /**
         * @brief 检查解析是否完成
         * @return int 如果解析完成返回非零值，否则返回 0 
         */
        int isFinished();

        /**
         * @brief 检查解析是否出现错误
         * @return int 如果出现错误返回非零值，否则返回 0 
         */
        int hasError();

        /**
         * @brief 获取 HTTP 响应数据
         * @return std::shared_ptr<HttpResponse> 返回指向 HTTP 响应数据的共享指针 
         */
        [[nodiscard]] std::shared_ptr<HttpResponse> getData() const;

        /**
         * @brief 设置解析错误码
         */
        void setError(int errorCode);

        /**
         * @brief 获取响应体内容长度
         */
        [[nodiscard]] uint64_t getContentLength() const;

        /**
         * @brief 获取 HTTP 响应解析器
         */
        [[nodiscard]] const httpclient_parser& getParser() const;

        /**
         * @brief 获取 HTTP 响应缓冲区大小
         */
        static uint64_t GetHttpResponseBufferSize();

        /**
         * @brief 获取 HTTP 响应最大请求体大小
         */
        static uint64_t GetHttpResponseMaxBodySize();

    private:
        httpclient_parser m_parser; ///< HTTP 响应解析器实例 
        std::shared_ptr<HttpResponse> m_data = nullptr; ///< 存储解析后的 HTTP 响应数据 
        /** 错误码
         1001: invalid version
         1002: invalid field
         */
        int m_error; ///< 错误码，标识解析过程中发生的错误 
    };
}

#endif
