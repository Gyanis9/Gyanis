#include "net/stream/ZlibStream.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "base/Macro.h"

namespace Gyanis::net::stream
{
    std::shared_ptr<ZlibStream> ZlibStream::CreateGzip(const bool encode, const uint32_t buff_size)
    {
        return Create(encode, buff_size, GZIP);
    }

    std::shared_ptr<ZlibStream> ZlibStream::CreateZlib(const bool encode, const uint32_t buff_size)
    {
        return Create(encode, buff_size, ZLIB);
    }

    std::shared_ptr<ZlibStream> ZlibStream::CreateDeflate(const bool encode, const uint32_t buff_size)
    {
        return Create(encode, buff_size, DEFLATE);
    }

    std::shared_ptr<ZlibStream> ZlibStream::Create(bool encode, uint32_t buff_size, const Type type, const int level,
                                                   const int window_bits, const int memlevel, const Strategy strategy)
    {
        if (auto result = std::make_shared<ZlibStream>(encode, buff_size); result->init(
            type, level, window_bits, memlevel, strategy) == Z_OK)
        {
            return result;
        }
        return nullptr;
    }

    ZlibStream::ZlibStream(const bool encode, const uint32_t buff_size) : m_buffSize(buff_size)
                                                                          , m_encode(encode)
                                                                          , m_free(true)
    {
    }

    ZlibStream::~ZlibStream()
    {
        if (m_free)
        {
            for (auto& [iov_base, iov_len] : m_buffs)
            {
                free(iov_base);
            }
        }

        if (m_encode)
        {
            deflateEnd(&m_zstream);
        }
        else
        {
            inflateEnd(&m_zstream);
        }
    }

    long ZlibStream::read(void* buffer, size_t length)
    {
        throw std::logic_error("ZlibStream::read() failed. "
            "The read operation is invalid or misconfigured.");
    }

    long ZlibStream::read(const std::shared_ptr<ByteArray>& bytearray, size_t length)
    {
        throw std::logic_error("ZlibStream::read() failed. "
            "The read operation is invalid or misconfigured.");
    }

    long ZlibStream::write(const void* buffer, const size_t length)
    {
        iovec ivc = {};
        ivc.iov_base = const_cast<void*>(buffer);
        ivc.iov_len = length;
        if (m_encode)
        {
            return encode(&ivc, 1, false);
        }
        return decode(&ivc, 1, false);
    }

    long ZlibStream::write(const std::shared_ptr<ByteArray>& bytearray, const size_t length)
    {
        std::vector<iovec> buffers;
        bytearray->getReadBuffers(buffers, length);
        if (m_encode)
        {
            return encode(&buffers[0], buffers.size(), false);
        }
        return decode(&buffers[0], buffers.size(), false);
    }

    void ZlibStream::close()
    {
        flush();
    }

    int ZlibStream::flush()
    {
        iovec ivc = {};
        ivc.iov_base = nullptr;
        ivc.iov_len = 0;

        if (m_encode)
        {
            return encode(&ivc, 1, true);
        }
        return decode(&ivc, 1, true);
    }

    bool ZlibStream::isFree() const
    {
        return m_free;
    }

    void ZlibStream::setFree(const bool value)
    {
        m_free = value;
    }

    bool ZlibStream::isEncode() const
    {
        return m_encode;
    }

    void ZlibStream::setEndcode(const bool value)
    {
        m_encode = value;
    }

    std::vector<iovec>& ZlibStream::getBuffers()
    {
        return m_buffs;
    }

    std::string ZlibStream::getResult() const
    {
        std::string rt;
        for (const auto& [iov_base, iov_len] : m_buffs)
        {
            rt.append(static_cast<const char*>(iov_base), iov_len);
        }
        return rt;
    }

    std::shared_ptr<ByteArray> ZlibStream::getByteArray()
    {
        auto ba = std::make_shared<ByteArray>();
        for (const auto& [iov_base, iov_len] : m_buffs)
        {
            ba->write(iov_base, iov_len);
        }
        ba->setPosition(0);
        return ba;
    }

    int ZlibStream::init(const Type type, const int level, int window_bits, const int memlevel, const Strategy strategy)
    {
        ASSERT((level >= 0 && level <= 9) || level == DEFAULT_COMPRESSION);
        ASSERT((window_bits >= 8 && window_bits <= 15));
        ASSERT((memlevel >= 1 && memlevel <= 9));

        memset(&m_zstream, 0, sizeof(m_zstream));

        m_zstream.zalloc = nullptr;
        m_zstream.zfree = nullptr;
        m_zstream.opaque = nullptr;

        switch (type)
        {
        case DEFLATE:
            window_bits = -window_bits;
            break;
        case GZIP:
            window_bits += 16;
            break;
        case ZLIB:
        default:
            break;
        }

        if (m_encode)
        {
            return deflateInit2(&m_zstream, level, Z_DEFLATED
                                , window_bits, memlevel, strategy);
        }
        return inflateInit2(&m_zstream, window_bits);
    }

    int ZlibStream::encode(const iovec* value, const uint64_t& size, const bool finish)
    {
        int ret = 0;
        int flush = 0;
        for (uint64_t i = 0; i < size; ++i)
        {
            m_zstream.avail_in = value[i].iov_len;
            m_zstream.next_in = static_cast<Bytef*>(value[i].iov_base);

            flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

            iovec* ivc = nullptr;
            do
            {
                if (!m_buffs.empty() && m_buffs.back().iov_len != m_buffSize)
                {
                    ivc = &m_buffs.back();
                }
                else
                {
                    iovec vc = {};
                    vc.iov_base = malloc(m_buffSize);
                    vc.iov_len = 0;
                    m_buffs.push_back(vc);
                    ivc = &m_buffs.back();
                }

                m_zstream.avail_out = m_buffSize - ivc->iov_len;
                m_zstream.next_out = static_cast<Bytef*>(ivc->iov_base) + ivc->iov_len;

                ret = deflate(&m_zstream, flush);
                if (ret == Z_STREAM_ERROR)
                {
                    return ret;
                }
                ivc->iov_len = m_buffSize - m_zstream.avail_out;
            }
            while (m_zstream.avail_out == 0);
        }
        if (flush == Z_FINISH)
        {
            deflateEnd(&m_zstream);
        }
        return Z_OK;
    }

    int ZlibStream::decode(const iovec* value, const uint64_t& size, const bool finish)
    {
        int ret = 0;
        int flush = 0;
        for (uint64_t i = 0; i < size; ++i)
        {
            m_zstream.avail_in = value[i].iov_len;
            m_zstream.next_in = static_cast<Bytef*>(value[i].iov_base);

            flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

            iovec* ivc = nullptr;
            do
            {
                if (!m_buffs.empty() && m_buffs.back().iov_len != m_buffSize)
                {
                    ivc = &m_buffs.back();
                }
                else
                {
                    iovec vc = {};
                    vc.iov_base = malloc(m_buffSize);
                    vc.iov_len = 0;
                    m_buffs.push_back(vc);
                    ivc = &m_buffs.back();
                }

                m_zstream.avail_out = m_buffSize - ivc->iov_len;
                m_zstream.next_out = static_cast<Bytef*>(ivc->iov_base) + ivc->iov_len;

                ret = inflate(&m_zstream, flush);
                if (ret == Z_STREAM_ERROR)
                {
                    return ret;
                }
                ivc->iov_len = m_buffSize - m_zstream.avail_out;
            }
            while (m_zstream.avail_out == 0);
        }

        if (flush == Z_FINISH)
        {
            inflateEnd(&m_zstream);
        }
        return Z_OK;
    }
}
