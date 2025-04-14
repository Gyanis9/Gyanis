#include "net/stream/Stream.h"

namespace Gyanis::net::stream
{
    long Stream::readFixSize(void* buffer, const size_t length)
    {
        size_t offset = 0;
        size_t left = length;
        while (left > 0)
        {
            const size_t len = read(static_cast<char*>(buffer) + offset, left);
            if (len == 0)
            {
                return -1;
            }
            offset += len;
            left -= len;
        }
        return static_cast<long>(length);;
    }

    long Stream::readFixSize(const std::shared_ptr<ByteArray>& buffer, const size_t length)
    {
        size_t left = length;
        while (left > 0)
        {
            const size_t len = read(buffer, left);
            if (len == 0)
            {
                return -1;
            }
            left -= len;
        }
        return static_cast<long>(length);
    }

    long Stream::writeFixSize(const void* buffer, const size_t length)
    {
        size_t offset = 0;
        size_t left = length;
        while (left > 0)
        {
            const int64_t len = write(static_cast<const char*>(buffer) + offset, left);
            if (len == 0)
            {
                return -1;
            }
            offset += len;
            left -= len;
        }
        return static_cast<long>(length);
    }

    long Stream::writeFixSize(const std::shared_ptr<ByteArray>& buffer, const size_t length)
    {
        size_t left = length;
        while (left > 0)
        {
            const int64_t len = write(buffer, left);
            if (len == 0)
            {
                return -1;
            }
            left -= len;
        }
        return static_cast<long>(length);
    }
}
