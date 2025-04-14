#include "net/http/HttpParser.h"
#include "base/Log.h"
#include "base/Config.h"

namespace Gyanis::net::http
{
    static auto g_logger = LOG_NAME("system");
    static auto g_http_request_buffer_size =
        base::Config::LookUp<uint64_t>("http.request.buffer_size"
                                       , 4 * 1024, "http request buffer size");

    static auto g_http_request_max_body_size =
        base::Config::LookUp<uint64_t>("http.request.max_body_size"
                                       , 64 * 1024 * 1024, "http request max body size");

    static auto g_http_response_buffer_size =
        base::Config::LookUp<uint64_t>("http.response.buffer_size"
                                       , 4 * 1024, "http response buffer size");

    static auto g_http_response_max_body_size =
        base::Config::LookUp<uint64_t>("http.response.max_body_size"
                                       , 64 * 1024 * 1024, "http response max body size");

    static uint64_t s_http_request_buffer_size = 0;
    static uint64_t s_http_request_max_body_size = 0;
    static uint64_t s_http_response_buffer_size = 0;
    static uint64_t s_http_response_max_body_size = 0;

    namespace
    {
        struct RequestSizeIniter
        {
            RequestSizeIniter()
            {
                s_http_request_buffer_size = g_http_request_buffer_size->getValue();
                s_http_request_max_body_size = g_http_request_max_body_size->getValue();
                s_http_response_buffer_size = g_http_response_buffer_size->getValue();
                s_http_response_max_body_size = g_http_response_max_body_size->getValue();

                g_http_request_buffer_size->addListener(
                    [](const uint64_t&, const uint64_t& nv)
                    {
                        s_http_request_buffer_size = nv;
                    });

                g_http_request_max_body_size->addListener(
                    [](const uint64_t&, const uint64_t& nv)
                    {
                        s_http_request_max_body_size = nv;
                    });

                g_http_response_buffer_size->addListener(
                    [](const uint64_t&, const uint64_t& nv)
                    {
                        s_http_response_buffer_size = nv;
                    });

                g_http_response_max_body_size->addListener(
                    [](const uint64_t&, const uint64_t& nv)
                    {
                        s_http_response_max_body_size = nv;
                    });
            }
        };

        [[maybe_unused]] RequestSizeIniter init;
    }

    uint64_t HttpResponseParser::GetHttpResponseBufferSize()
    {
        return s_http_response_buffer_size;
    }

    uint64_t HttpResponseParser::GetHttpResponseMaxBodySize()
    {
        return s_http_response_max_body_size;
    }

    void on_request_method(void* data, const char* at, const size_t length)
    {
        auto* parser = static_cast<HttpRequestParser*>(data);
        const HttpMethod method = CharsToHttpMethod(at);

        if (method == HttpMethod::INVALID_METHOD)
        {
            LOG_WARN(g_logger)
                << "Invalid HTTP request method. "
                << "Method: " << std::string(at, length)
                << ". Please verify the request format.";
            parser->setError(1000);
            return;
        }
        parser->getData()->setMethod(method);
    }

    void on_request_uri(void*, const char*, size_t)
    {
    }

    void on_request_fragment(void* data, const char* at, const size_t length)
    {
        const auto* parser = static_cast<HttpRequestParser*>(data);
        parser->getData()->setFragment(std::string(at, length));
    }

    void on_request_path(void* data, const char* at, const size_t length)
    {
        const auto* parser = static_cast<HttpRequestParser*>(data);
        parser->getData()->setPath(std::string(at, length));
    }

    void on_request_query(void* data, const char* at, const size_t length)
    {
        const auto* parser = static_cast<HttpRequestParser*>(data);
        parser->getData()->setQuery(std::string(at, length));
    }

    void on_request_version(void* data, const char* at, const size_t length)
    {
        auto* parser = static_cast<HttpRequestParser*>(data);
        uint8_t version = 0;
        if (strncmp(at, "HTTP/1.1", length) == 0)
        {
            version = 0x11;
        }
        else if (strncmp(at, "HTTP/1.0", length) == 0)
        {
            version = 0x10;
        }
        else
        {
            LOG_WARN(g_logger)
                << "Invalid HTTP request version. "
                << "Version: " << std::string(at, length)
                << ". Please verify the request format.";
            parser->setError(1001);
            return;
        }
        parser->getData()->setVersion(version);
    }

    void on_request_header_done(void*, const char*, size_t)
    {
    }

    void on_request_http_field(void* data, const char* field, const size_t flen
                               , const char* value, const size_t vlen)
    {
        const auto* parser = static_cast<HttpRequestParser*>(data);
        if (flen == 0)
        {
            LOG_WARN(g_logger)
                << "Invalid HTTP request. "
                << "Field length is zero. Please check the request format.";
            //parser->setError(1002);
            return;
        }
        parser->getData()->setHeader(std::string(field, flen)
                                     , std::string(value, vlen));
    }

    HttpRequestParser::HttpRequestParser(): m_data(std::make_shared<HttpRequest>()), m_error(0)
    {
        http_parser_init(&m_parser);
        m_parser.request_method = on_request_method;
        m_parser.request_uri = on_request_uri;
        m_parser.fragment = on_request_fragment;
        m_parser.request_path = on_request_path;
        m_parser.query_string = on_request_query;
        m_parser.http_version = on_request_version;
        m_parser.header_done = on_request_header_done;
        m_parser.http_field = on_request_http_field;
        m_parser.data = this;
    }

    HttpRequestParser::~HttpRequestParser() = default;

    size_t HttpRequestParser::execute(char* data, const size_t size)
    {
        const size_t offset = http_parser_execute(&m_parser, data, size, 0);
        memmove(data, data + offset, (size - offset));
        return offset;
    }

    int HttpRequestParser::isFinished()
    {
        return http_parser_finish(&m_parser);
    }

    int HttpRequestParser::hasError()
    {
        return m_error || http_parser_has_error(&m_parser);
    }

    std::shared_ptr<HttpRequest> HttpRequestParser::getData() const
    {
        return m_data;
    }

    void HttpRequestParser::setError(const int errorCode)
    {
        m_error = errorCode;
    }

    uint64_t HttpRequestParser::getContentLength() const
    {
        return m_data->getHeaderAs<uint64_t>("content-length", 0);
    }

    const http_parser& HttpRequestParser::getParser() const
    {
        return m_parser;
    }


    void on_response_reason(void* data, const char* at, const size_t length)
    {
        const auto* parser = static_cast<HttpResponseParser*>(data);
        parser->getData()->setReason(std::string(at, length));
    }

    void on_response_status(void* data, const char* at, size_t)
    {
        const auto* parser = static_cast<HttpResponseParser*>(data);
        const auto status = static_cast<HttpStatus>(std::stoi(at));
        parser->getData()->setStatus(status);
    }

    void on_response_chunk(void*, const char*, size_t)
    {
    }

    void on_response_version(void* data, const char* at, const size_t length)
    {
        auto* parser = static_cast<HttpResponseParser*>(data);
        uint8_t value = 0;
        if (strncmp(at, "HTTP/1.1", length) == 0)
        {
            value = 0x11;
        }
        else if (strncmp(at, "HTTP/1.0", length) == 0)
        {
            value = 0x10;
        }
        else
        {
            LOG_WARN(g_logger)
                << "Invalid HTTP response version. "
                << "Version: " << std::string(at, length)
                << ". Please verify the response format.";
            parser->setError(1001);
            return;
        }

        parser->getData()->setVersion(value);
    }

    void on_response_header_done(void*, const char*, size_t)
    {
    }

    void on_response_last_chunk(void*, const char*, size_t)
    {
    }

    void on_response_http_field(void* data, const char* field, const size_t flen
                                , const char* value, const size_t vlen)
    {
        const auto* parser = static_cast<HttpResponseParser*>(data);
        if (flen == 0)
        {
            LOG_WARN(g_logger)
                << "Invalid HTTP response. "
                << "Field length is zero. Please check the response format.";
            //parser->setError(1002);
            return;
        }
        parser->getData()->setHeader(std::string(field, flen)
                                     , std::string(value, vlen));
    }

    HttpResponseParser::HttpResponseParser(): m_data(std::make_shared<HttpResponse>()), m_error(0)
    {
        httpclient_parser_init(&m_parser);
        m_parser.reason_phrase = on_response_reason;
        m_parser.status_code = on_response_status;
        m_parser.chunk_size = on_response_chunk;
        m_parser.http_version = on_response_version;
        m_parser.header_done = on_response_header_done;
        m_parser.last_chunk = on_response_last_chunk;
        m_parser.http_field = on_response_http_field;
        m_parser.data = this;
    }

    HttpResponseParser::~HttpResponseParser() = default;

    size_t HttpResponseParser::execute(char* data, const size_t size, const bool chunk)
    {
        if (chunk)
        {
            httpclient_parser_init(&m_parser);
        }
        const size_t offset = httpclient_parser_execute(&m_parser, data, size, 0);

        memmove(data, data + offset, size - offset);
        return offset;
    }

    int HttpResponseParser::isFinished()
    {
        return httpclient_parser_finish(&m_parser);
    }

    int HttpResponseParser::hasError()
    {
        return m_error || httpclient_parser_has_error(&m_parser);
    }

    std::shared_ptr<HttpResponse> HttpResponseParser::getData() const
    {
        return m_data;
    }

    void HttpResponseParser::setError(const int errorCode)
    {
        m_error = errorCode;
    }

    uint64_t HttpResponseParser::getContentLength() const
    {
        return m_data->getHeaderAs<uint64_t>("content-length", 0);
    }

    const httpclient_parser& HttpResponseParser::getParser() const
    {
        return m_parser;
    }

    uint64_t HttpRequestParser::GetHttpRequestBufferSize()
    {
        return s_http_request_buffer_size;
    }

    uint64_t HttpRequestParser::GetHttpRequestMaxBodySize()
    {
        return s_http_request_max_body_size;
    }
}
