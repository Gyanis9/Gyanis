#include "net/rock/RockProtocol.h"

#include <memory>
#include "base/Config.h"
#include "net/stream/ZlibStream.h"
#include "base/Endian.h"

namespace Gyanis::net::rock
{
    static auto g_logger = LOG_NAME("system");
    static auto g_rock_protocol_max_length
        = base::Config::LookUp<uint32_t>("rock.protocol.max_length",
                                         1024 * 1024 * 64, "rock protocol max length");
    static auto g_rock_protocol_gzip_min_length
        = base::Config::LookUp<uint32_t>("rock.protocol.gzip_min_length",
                                         1024 * 4, "rock protocol gizp min length");

    void RockBody::setBody(std::string value)
    {
        m_body = std::move(value);
    }

    std::string RockBody::getBody() const
    {
        return m_body;
    }

    bool RockBody::serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        byteArray->writeStringVint(m_body);
        return true;
    }

    bool RockBody::parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        m_body = byteArray->readStringVint();
        return true;
    }

    std::shared_ptr<RockResponse> RockRequest::createResponse() const
    {
        auto result = std::make_shared<RockResponse>();
        result->setSn(m_sn);
        result->setCmd(m_cmd);
        return result;
    }

    std::string RockRequest::toString() const
    {
        std::stringstream ss;
        ss << "[RockRequest Information: "
            << "Serial Number (sn): " << m_sn
            << " | Command (cmd): " << m_cmd
            << " | Body length: " << m_body.size()
            << "]";
        return ss.str();
    }

    const std::string& RockRequest::getName() const
    {
        static const std::string& s_name = "RockRequest";
        return s_name;
    }

    int32_t RockRequest::getType() const
    {
        return Message::REQUEST;
    }

    bool RockRequest::serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        try
        {
            bool result = true;
            result &= Request::serializeToByteArray(byteArray);
            result &= RockBody::serializeToByteArray(byteArray);
            return result;
        }
        catch (...)
        {
            LOG_ERROR(g_logger)
                << "RockRequest serialization failed. "
                << "An error occurred while serializing the request to a byte array.";
        }
        return false;
    }

    bool RockRequest::parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        try
        {
            bool result = true;
            result &= Request::parseFromByteArray(byteArray);
            result &= RockBody::parseFromByteArray(byteArray);
            return result;
        }
        catch (...)
        {
            LOG_ERROR(g_logger)
                << "RockRequest deserialization failed. "
                << "An error occurred while parsing the byte array into a RockRequest.";
        }
        return false;
    }

    std::string RockResponse::toString() const
    {
        std::stringstream ss;
        ss << "[RockResponse Information: "
            << "Serial Number (sn): " << m_sn
            << " | Command (cmd): " << m_cmd
            << " | Result: " << m_result
            << " | Result Message: " << m_resultStr
            << " | Body length: " << m_body.size()
            << "]";
        return ss.str();
    }

    const std::string& RockResponse::getName() const
    {
        static const std::string& s_name = "RockResponse";
        return s_name;
    }

    int32_t RockResponse::getType() const
    {
        return Message::RESPONSE;
    }

    bool RockResponse::serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        try
        {
            bool result = true;
            result &= Response::serializeToByteArray(byteArray);
            result &= RockBody::serializeToByteArray(byteArray);
            return result;
        }
        catch (...)
        {
            LOG_ERROR(g_logger)
                << "RockResponse serialization failed. "
                << "An error occurred while serializing the response to a byte array.";
        }
        return false;
    }

    bool RockResponse::parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        try
        {
            bool result = true;
            result &= Response::parseFromByteArray(byteArray);
            result &= RockBody::parseFromByteArray(byteArray);
            return result;
        }
        catch (...)
        {
            LOG_ERROR(g_logger)
                << "RockResponse deserialization failed. "
                << "An error occurred while parsing the byte array into a RockResponse.";
        }
        return false;
    }

    std::string RockNotify::toString() const
    {
        std::stringstream ss;
        ss << "[RockNotify Information: "
            << "Notification: " << m_notify
            << " | Body length: " << m_body.size()
            << "]";
        return ss.str();
    }

    const std::string& RockNotify::getName() const
    {
        static const std::string& s_name = "RockNotify";
        return s_name;
    }

    int32_t RockNotify::getType() const
    {
        return Message::NOTIFY;
    }

    bool RockNotify::serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        try
        {
            bool result = true;
            result &= Notify::serializeToByteArray(byteArray);
            result &= RockBody::serializeToByteArray(byteArray);
            return result;
        }
        catch (...)
        {
            LOG_ERROR(g_logger)
              << "RockNotify serialization failed. "
              << "An error occurred while serializing the notification to a byte array.";
        }
        return false;
    }

    bool RockNotify::parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        try
        {
            bool result = true;
            result &= Notify::parseFromByteArray(byteArray);
            result &= RockBody::parseFromByteArray(byteArray);
            return result;
        }
        catch (...)
        {
            LOG_ERROR(g_logger)
                << "RockNotify deserialization failed. "
                << "An error occurred while parsing the byte array into a RockNotify object.";
        }
        return false;
    }

    static constexpr uint8_t s_rock_magic[2] = {0xab, 0xcd};

    RockMsgHeader::RockMsgHeader()
        : magic{0xab, 0xcd}, version(1), flag(0), length(0)
    {
    }

    int32_t
    RockMessageDecoder::serializeTo(const std::shared_ptr<stream::Stream>& stream,
                                    const std::shared_ptr<protocol::Message>& message)
    {
        RockMsgHeader header;
        auto byteArray = message->toByteArray();
        byteArray->setPosition(0);
        header.length = byteArray->getSize();
        if (static_cast<uint32_t>(header.length) >= g_rock_protocol_gzip_min_length->getValue())
        {
            const auto zstream = stream::ZlibStream::CreateGzip(true);
            if (zstream->write(byteArray, -1) != Z_OK)
            {
                LOG_ERROR(g_logger)
                    << "RockMessageDecoder serialization to GZIP failed. "
                    << "An error occurred while serializing the message to GZIP format.";

                return -1;
            }
            if (zstream->flush() != Z_OK)
            {
                LOG_ERROR(g_logger)
                    << "RockMessageDecoder GZIP flush failed. "
                    << "An error occurred while flushing the GZIP buffer.";
                return -2;
            }

            byteArray = zstream->getByteArray();
            header.flag |= 0x1;
            header.length = byteArray->getSize();
        }
        header.length = base::byteswapOnLittleEndian(header.length);
        if (stream->writeFixSize(&header, sizeof(header)) <= 0)
        {
            LOG_ERROR(g_logger)
                << "RockMessageDecoder header write failed. "
                << "An error occurred while writing the header during serialization.";
            return -3;
        }
        if (stream->writeFixSize(byteArray, byteArray->getReadSize()) <= 0)
        {
            LOG_ERROR(g_logger)
                << "RockMessageDecoder body write failed. "
                << "An error occurred while writing the body during serialization.";

            return -4;
        }
        return sizeof(header) + byteArray->getSize();
    }

    std::shared_ptr<protocol::Message> RockMessageDecoder::parseForm(const std::shared_ptr<stream::Stream>& stream)
    {
        try
        {
            RockMsgHeader header;
            if (stream->readFixSize(&header, sizeof(header)) <= 0)
            {
                LOG_ERROR(g_logger)
                    << "RockMessageDecoder header decoding failed. "
                    << "An error occurred while decoding the message header.";
                return nullptr;
            }

            if (memcmp(header.magic, s_rock_magic, sizeof(s_rock_magic)) != 0)
            {
                LOG_ERROR(g_logger)
                    << "RockMessageDecoder header magic validation failed. "
                    << "The magic value in the header is incorrect or invalid.";
                return nullptr;
            }

            if (header.version != 0x1)
            {
                LOG_ERROR(g_logger)
                << "RockMessageDecoder header version mismatch. "
                << "Expected version: 0x1, but found: " << header.version;
                return nullptr;
            }

            header.length = base::byteswapOnLittleEndian(header.length);
            if (static_cast<uint32_t>(header.length) >= g_rock_protocol_max_length->getValue())
            {
                LOG_ERROR(g_logger)
                    << "RockMessageDecoder header length exceeds maximum allowed. "
                    << "Header length: " << header.length
                    << " | Maximum allowed length: " << g_rock_protocol_max_length->getValue();
                return nullptr;
            }
            auto ba = std::make_shared<stream::ByteArray>();
            if (stream->readFixSize(ba, header.length) <= 0)
            {
                LOG_ERROR(g_logger)
                   << "RockMessageDecoder failed to read body. "
                   << "Body length: " << header.length
                   << " | An error occurred while reading the message body.";
                return nullptr;
            }

            ba->setPosition(0);
            if (header.flag & 0x1)
            {
                const auto zstream = stream::ZlibStream::CreateGzip(false);
                if (zstream->write(ba, -1) != Z_OK)
                {
                    LOG_ERROR(g_logger)
                      << "RockMessageDecoder GZIP decompression failed. "
                      << "An error occurred while decompressing the data using GZIP.";
                    return nullptr;
                }
                if (zstream->flush() != Z_OK)
                {
                    LOG_ERROR(g_logger)
                       << "RockMessageDecoder GZIP flush failed. "
                       << "An error occurred while flushing the decompressed data in GZIP format.";
                    return nullptr;
                }
                ba = zstream->getByteArray();
            }
            const uint8_t type = ba->readFuint8();
            std::shared_ptr<protocol::Message> msg;
            switch (type)
            {
            case protocol::Message::REQUEST:
                msg = std::make_shared<RockRequest>();
                break;
            case protocol::Message::RESPONSE:
                msg = std::make_shared<RockResponse>();
                break;
            case protocol::Message::NOTIFY:
                msg = std::make_shared<RockNotify>();
                break;
            default:
                LOG_ERROR(g_logger)
                    << "RockMessageDecoder received invalid type. "
                    << "Type value: " << static_cast<int>(type)
                    << ". Please verify the message type.";
                return nullptr;
            }

            if (!msg->parseFromByteArray(ba))
            {
                LOG_ERROR(g_logger)
                    << "RockMessageDecoder deserialization failed. "
                    << "Invalid type: " << static_cast<int>(type)
                    << ". An error occurred while parsing the byte array.";
                return nullptr;
            }
            return msg;
        }
        catch (std::exception& e)
        {
            LOG_ERROR(g_logger)
                << "RockMessageDecoder exception occurred. "
                << "Error details: " << e.what();
        } catch (...)
        {
            LOG_ERROR(g_logger)
                << "RockMessageDecoder exception occurred. "
                << "An unexpected error has occurred during the decoding process.";
        }
        return nullptr;
    }
}
