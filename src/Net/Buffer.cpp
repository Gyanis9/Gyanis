#include "Buffer.h"

namespace Net
{
    size_t Buffer::readableSize() const noexcept
    {
        size_t total = 0;
        for (const auto &blk: m_blocks)
        {
            total += (blk.writePos - blk.readPos);
        }
        return total;
    }

    size_t Buffer::read(void *dest, const size_t len)
    {
        auto ptr = static_cast<uint8_t *>(dest);
        size_t remaining = len;
        while (remaining > 0 && !m_blocks.empty())
        {
            auto &front = m_blocks.front();
            size_t available = front.writePos - front.readPos;
            const size_t toCopy = (std::min)(remaining, available);
            if (ptr)
            {
                std::memcpy(ptr, front.data.get() + front.readPos, toCopy);
            }
            front.readPos += toCopy;
            ptr += toCopy;
            remaining -= toCopy;
            if (front.readPos == front.writePos)
            {
                m_blocks.pop_front();
            }
        }
        return len - remaining;
    }

    void Buffer::skip(const size_t len)
    {
        read(nullptr, len);
    }

    std::string_view Buffer::peek() const noexcept
    {
        if (m_blocks.empty())
        {
            return {};
        }
        const auto &front = m_blocks.front();
        return {
            reinterpret_cast<const char *>(front.data.get() + front.readPos),
            front.writePos - front.readPos
        };
    }

    std::span<uint8_t> Buffer::reservePrepare(const size_t size)
    {
        ensureWritable(size);
        auto &back = m_blocks.back();
        const size_t available = BLOCK_SIZE - back.writePos;
        size_t use = (std::min)(size, available);
        return {back.data.get() + back.writePos, use};
    }

    void Buffer::commit(size_t len)
    {
        while (len > 0)
        {
            auto &back = m_blocks.back();
            size_t space = BLOCK_SIZE - back.writePos;
            const size_t add = (std::min)(len, space);
            back.writePos += add;
            len -= add;
            if (len > 0)
            {
                addBlock();
            }
        }
    }

    void Buffer::append(const void *data, const size_t len)
    {
        auto ptr = static_cast<const uint8_t *>(data);
        size_t remaining = len;
        while (remaining > 0)
        {
            auto span = reservePrepare(remaining);
            const size_t toCopy = (std::min)(span.size(), remaining);
            std::memcpy(span.data(), ptr, toCopy);
            commit(toCopy);
            ptr += toCopy;
            remaining -= toCopy;
        }
    }

    void Buffer::append(const std::string &s)
    {
        append(s.data(), s.size());
    }

    std::optional<size_t> Buffer::find(const std::string_view pattern) const noexcept
    {
        if (pattern.empty())
        {
            return 0;
        }
        size_t offset = 0;
        for (const auto &blk: m_blocks)
        {
            const char *start = reinterpret_cast<const char *>(blk.data.get()) + blk.readPos;
            const size_t len = blk.writePos - blk.readPos;
            std::string_view view(start, len);
            if (const auto pos = view.find(pattern); pos != std::string_view::npos)
            {
                return offset + pos;
            }
            offset += len;
        }
        return std::nullopt;
    }

    void Buffer::clear()
    {
        m_blocks.clear();
    }

    void Buffer::addBlock()
    {
        m_blocks.emplace_back();
    }

    void Buffer::ensureWritable(const size_t size)
    {
        if (m_blocks.empty())
        {
            addBlock();
        }
        size_t needed = size;
        const size_t lastSpace = BLOCK_SIZE - m_blocks.back().writePos;
        if (lastSpace >= needed)
        {
            return;
        }
        needed -= lastSpace;
        while (needed > 0)
        {
            addBlock();
            needed = (needed > BLOCK_SIZE) ? needed - BLOCK_SIZE : 0;
        }
    }
}
