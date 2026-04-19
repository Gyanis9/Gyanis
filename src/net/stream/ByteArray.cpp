#include "ByteArray.h"
#include "base/Log.h"
#include "base/Endian.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cmath>


namespace Gyanis::net::stream
{
    static auto g_logger = LOG_NAME("system");

    ByteArray::Node::Node(const size_t s): base(new char[s]), next(nullptr), size(s)
    {
    }

    ByteArray::Node::Node(): base(nullptr), next(nullptr), size(0)
    {
    }

    ByteArray::Node::~Node()
    {
        delete[] base;
    }

    ByteArray::ByteArray(const size_t base_size): m_baseSize(base_size), m_position(0), m_capacity(base_size),
                                                  m_size(0),
                                                  m_endian(GYANIS_BIG_ENDIAN), m_root(new Node(base_size)),
                                                  m_current(m_root)
    {
    }

    ByteArray::~ByteArray()
    {
        Node* temp = m_root;
        while (temp)
        {
            m_current = temp;
            temp = temp->next;
            delete m_current;
        }
    }

    void ByteArray::writeFint8(const int8_t value)
    {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint8(const uint8_t value)
    {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint16(int16_t value)
    {
        if (m_endian != GYANIS_BYTE_ORDER)
        {
            value = base::byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint16(uint16_t value)
    {
        if (m_endian != GYANIS_BYTE_ORDER)
        {
            value = base::byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint32(int32_t value)
    {
        if (m_endian != GYANIS_BYTE_ORDER)
        {
            value = base::byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint32(uint32_t value)
    {
        if (m_endian != GYANIS_BYTE_ORDER)
        {
            value = base::byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint64(int64_t value)
    {
        if (m_endian != GYANIS_BYTE_ORDER)
        {
            value = base::byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint64(uint64_t value)
    {
        if (m_endian != GYANIS_BYTE_ORDER)
        {
            value = base::byteswap(value);
        }
        write(&value, sizeof(value));
    }

    /**
     * @brief Zigzag 编码
     *
     * Zigzag 编码是一种整数编码方式，常用于将有符号整数转换为无符号整数。它通过将负数映射为奇数，正数映射为偶数，确保负数和正数在编码后具有相同的数字范围
     *
     * 例如：
     * - -1 编码为 1
     * - 1 编码为 2
     * - -2 编码为 3
     * - 2 编码为 4
     *
     * @tparam T 输入类型，支持有符号整数类型
     * @param[in] v 输入值，需要编码的有符号整数
     * @return 返回 Zigzag 编码后的无符号整数
     */
    template <typename T>
    static auto EncodeZigzag(const T& v) -> std::make_unsigned_t<T>
    {
        return (v < 0)
                   ? (static_cast<std::make_unsigned_t<T>>(-v) * 2 - 1)
                   : (v * 2);
    }

    /**
     * @brief Zigzag 解码
     *
     * Zigzag 解码用于将无符号整数解码为原始的有符号整数。它通过反转 Zigzag 编码的过程将偶数解码为正数，奇数解码为负数
     *
     * 例如：
     * - 1 解码为 -1
     * - 2 解码为 1
     * - 3 解码为 -2
     * - 4 解码为 2
     *
     * @tparam T 输入类型，支持无符号整数类型
     * @param[in] v 输入值，需要解码的无符号整数
     * @return 返回解码后的有符号整数
     */
    template <typename T>
    static T DecodeZigzag(const std::make_unsigned_t<T>& v)
    {
        return (v >> 1) ^ -(v & 1);
    }

    void ByteArray::writeInt32(const int32_t value)
    {
        writeUint32(EncodeZigzag(value));
    }

    void ByteArray::writeUint32(uint32_t value)
    {
        uint8_t temp[5];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            temp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        temp[i++] = value;
        write(temp, i);
    }

    void ByteArray::writeInt64(const int64_t value)
    {
        writeUint64(EncodeZigzag(value));
    }

    void ByteArray::writeUint64(uint64_t value)
    {
        uint8_t tmp[10];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeFloat(const float value)
    {
        uint32_t result;
        memcpy(&result, &value, sizeof(result));
        writeFuint32(result);
    }

    void ByteArray::writeDouble(const double value)
    {
        uint64_t result;
        memcpy(&result, &value, sizeof(result));
        writeFuint64(result);
    }

    void ByteArray::writeStringF16(const std::string& value)
    {
        writeFuint16(value.size());
        write(value.data(), value.size());
    }

    void ByteArray::writeStringF32(const std::string& value)
    {
        writeFuint32(value.size());
        write(value.data(), value.size());
    }

    void ByteArray::writeStringF64(const std::string& value)
    {
        writeFuint64(value.size());
        write(value.data(), value.size());
    }

    void ByteArray::writeStringVint(const std::string& value)
    {
        writeUint64(value.size());
        write(value.data(), value.size());
    }

    void ByteArray::writeStringWithoutLength(const std::string& value)
    {
        write(value.data(), value.size());
    }

    int8_t ByteArray::readFint8()
    {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint8_t ByteArray::readFuint8()
    {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }

    int16_t ByteArray::readFint16()
    {
        int16_t value;
        read(&value, sizeof(value));
        if (m_endian == GYANIS_BYTE_ORDER) { return value; }
        return base::byteswap(value);;
    }

    uint16_t ByteArray::readFuint16()
    {
        uint16_t value;
        read(&value, sizeof(value));
        if (m_endian == GYANIS_BYTE_ORDER) { return value; }
        {
            return base::byteswap(value);
        };
    }

    int32_t ByteArray::readFint32()
    {
        int32_t value;
        read(&value, sizeof(value));
        if (m_endian == 1) { return value; }
        {
            return base::byteswap(value);
        };
    }

    uint32_t ByteArray::readFuint32()
    {
        uint32_t value;
        read(&value, sizeof(value));
        if (m_endian == GYANIS_BYTE_ORDER) { return value; }
        {
            return base::byteswap(value);
        };
    }

    int64_t ByteArray::readFint64()
    {
        int64_t value;
        read(&value, sizeof(value));
        if (m_endian == GYANIS_BYTE_ORDER) { return value; }
        {
            return base::byteswap(value);
        };
    }

    uint64_t ByteArray::readFuint64()
    {
        uint64_t value;
        read(&value, sizeof(value));
        if (m_endian == GYANIS_BYTE_ORDER) { return value; }
        {
            return base::byteswap(value);
        };
    }

    int32_t ByteArray::readInt32()
    {
        return DecodeZigzag<int32_t>(readUint32());
    }

    uint32_t ByteArray::readUint32()
    {
        uint32_t result = 0;
        for (int i = 0; i < 32; i += 7)
        {
            const uint8_t b = readFuint8();
            if (b < 0x80)
            {
                result |= static_cast<uint32_t>(b) << i;
                break;
            }
            result |= static_cast<uint32_t>(b & 0x7f) << i;
        }
        return result;
    }

    int64_t ByteArray::readInt64()
    {
        return DecodeZigzag<int64_t>(readUint64());
    }

    uint64_t ByteArray::readUint64()
    {
        uint64_t result = 0;
        for (int i = 0; i < 64; i += 7)
        {
            const uint8_t b = readFuint8();
            if (b < 0x80)
            {
                result |= static_cast<uint64_t>(b) << i;
                break;
            }
            result |= static_cast<uint64_t>(b & 0x7f) << i;
        }
        return result;
    }

    float ByteArray::readFloat()
    {
        const uint32_t v = readFuint32();
        float value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    double ByteArray::readDouble()
    {
        const uint64_t v = readFuint64();
        double value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    std::string ByteArray::readStringF16()
    {
        const uint16_t len = readFuint16();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringF32()
    {
        const uint32_t len = readFuint32();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringF64()
    {
        const uint64_t len = readFuint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringVint()
    {
        const uint64_t len = readUint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    void ByteArray::clear()
    {
        m_position = m_size = 0;
        m_capacity = m_baseSize;
        Node* tmp = m_root->next;
        while (tmp)
        {
            m_current = tmp;
            tmp = tmp->next;
            delete m_current;
        }
        m_current = m_root;
        m_root->next = nullptr;
    }

    void ByteArray::write(const void* buf, size_t size)
    {
        if (size == 0)
        {
            return;
        }
        addCapacity(size);

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_current->size - npos;
        size_t bpos = 0;

        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy(m_current->base + npos, static_cast<const char*>(buf) + bpos, size);
                if (m_current->size == npos + size)
                {
                    m_current = m_current->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy(m_current->base + npos, static_cast<const char*>(buf) + bpos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_current = m_current->next;
                ncap = m_current->size;
                npos = 0;
            }
        }

        if (m_position > m_size)
        {
            m_size = m_position;
        }
    }

    void ByteArray::read(void* buf, size_t size)
    {
        if (size > getReadSize())
        {
            throw std::out_of_range("ByteArray::read - failed. "
                "Insufficient length to read the requested data.");
        }

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_current->size - npos;
        size_t bpos = 0;
        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy(static_cast<char*>(buf) + bpos, m_current->base + npos, size);
                if (m_current->size == npos + size)
                {
                    m_current = m_current->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy(static_cast<char*>(buf) + bpos, m_current->base + npos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_current = m_current->next;
                ncap = m_current->size;
                npos = 0;
            }
        }
    }

    void ByteArray::read(void* buf, size_t size, size_t position) const
    {
        if (size > m_size - position)
        {
            throw std::out_of_range("ByteArray::read - failed. "
                "Insufficient length to read the requested data.");
        }

        size_t npos = position % m_baseSize;
        size_t ncap = m_current->size - npos;
        size_t bpos = 0;
        const Node* cur = m_current;
        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy(static_cast<char*>(buf) + bpos, cur->base + npos, size);
                if (cur->size == npos + size)
                {
                    cur = cur->next;
                }
                position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy(static_cast<char*>(buf) + bpos, cur->base + npos, ncap);
                position += ncap;
                bpos += ncap;
                size -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
        }
    }

    size_t ByteArray::getPosition() const
    {
        return m_position;
    }

    void ByteArray::setPosition(size_t position)
    {
        if (position > m_capacity)
        {
            throw std::out_of_range("ByteArray::setPosition - failed. "
                "Position is out of range.");
        }
        m_position = position;
        if (m_position > m_size)
        {
            m_size = m_position;
        }
        m_current = m_root;
        while (position > m_current->size)
        {
            position -= m_current->size;
            m_current = m_current->next;
        }
        if (position == m_current->size)
        {
            m_current = m_current->next;
        }
    }

    bool ByteArray::writeToFile(const std::string& filename) const
    {
        std::ofstream ofs;
        ofs.open(filename, std::ios::trunc | std::ios::binary);
        if (!ofs)
        {
            LOG_ERROR(g_logger)
                << "ByteArray::writeToFile - failed to open file: "
                << filename;
            return false;
        }

        size_t read_size = getReadSize();
        size_t pos = m_position;
        const Node* cur = m_current;

        while (read_size > 0)
        {
            const auto diff = pos % m_baseSize;
            const auto len = (read_size > m_baseSize ? m_baseSize : read_size) - diff;
            ofs.write(cur->base + diff, static_cast<long>(len));
            cur = cur->next;
            pos += len;
            read_size -= len;
        }
        return true;
    }

    bool ByteArray::readFromFile(const std::string& filename)
    {
        std::ifstream ifs;
        ifs.open(filename, std::ios::binary);
        if (!ifs)
        {
            LOG_ERROR(g_logger)
                << "ByteArray::readFromFile - failed to open file: " << filename;

            return false;
        }

        const auto buff = std::make_unique<char[]>(m_baseSize);
        while (!ifs.eof())
        {
            ifs.read(buff.get(), static_cast<long>(m_baseSize));
            write(buff.get(), ifs.gcount());
        }
        return true;
    }

    size_t ByteArray::getBaseSize() const
    {
        return m_baseSize;
    }

    size_t ByteArray::getReadSize() const
    {
        return m_size - m_position;
    }

    bool ByteArray::isLittleEndian() const
    {
        return m_endian == GYANIS_LITTLE_ENDIAN;
    }

    void ByteArray::setLittleEndian(const bool little_endian)
    {
        if (little_endian)
        {
            m_endian = GYANIS_LITTLE_ENDIAN;
        }
        else
        {
            m_endian = GYANIS_BIG_ENDIAN;
        }
    }

    std::string ByteArray::toString() const
    {
        std::string str;
        str.resize(getReadSize());
        if (str.empty())
        {
            return str;
        }
        read(&str[0], str.size(), m_position);
        return str;
    }

    std::string ByteArray::toHexString() const
    {
        const std::string str = toString();
        std::stringstream ss;

        for (size_t i = 0; i < str.size(); ++i)
        {
            if (i > 0 && i % 32 == 0)
            {
                ss << '\n';
            }
            ss << std::setw(2) << std::setfill('0') << std::hex
                << static_cast<int>(static_cast<uint8_t>(str[i])) << " ";
        }

        return ss.str();
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffer, uint64_t len) const
    {
        len = len > getReadSize() ? getReadSize() : len;
        if (len == 0)
        {
            return 0;
        }

        const uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_current->size - npos;
        const Node* cur = m_current;

        while (len > 0)
        {
            iovec iov{};
            if (ncap >= len)
            {
                iov.iov_base = cur->base + npos;
                iov.iov_len = len;
                len = 0;
            }
            else
            {
                iov.iov_base = cur->base + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffer.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffer, uint64_t len, const uint64_t position) const
    {
        len = len > getReadSize() ? getReadSize() : len;
        if (len == 0)
        {
            return 0;
        }

        const uint64_t size = len;

        size_t npos = position % m_baseSize;
        size_t count = position / m_baseSize;
        const Node* cur = m_root;
        while (count > 0)
        {
            cur = cur->next;
            --count;
        }

        size_t ncap = cur->size - npos;
        while (len > 0)
        {
            iovec iov{};
            if (ncap >= len)
            {
                iov.iov_base = cur->base + npos;
                iov.iov_len = len;
                len = 0;
            }
            else
            {
                iov.iov_base = cur->base + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffer.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffer, uint64_t len)
    {
        if (len == 0)
        {
            return 0;
        }
        addCapacity(len);
        const uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_current->size - npos;
        const Node* cur = m_current;
        while (len > 0)
        {
            iovec iov = {};
            if (ncap >= len)
            {
                iov.iov_base = cur->base + npos;
                iov.iov_len = len;
                len = 0;
            }
            else
            {
                iov.iov_base = cur->base + npos;
                iov.iov_len = ncap;

                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffer.push_back(iov);
        }
        return size;
    }

    size_t ByteArray::getSize() const
    {
        return m_size;
    }

    void ByteArray::addCapacity(size_t size)
    {
        if (size == 0)
        {
            return;
        }
        const size_t old_cap = getCapacity();
        if (old_cap >= size)
        {
            return;
        }

        size = size - old_cap;
        const size_t count = ceil(1.0 * static_cast<double>(size) / static_cast<double>(m_baseSize));
        Node* tmp = m_root;
        while (tmp->next)
        {
            tmp = tmp->next;
        }

        Node* first = nullptr;
        for (size_t i = 0; i < count; ++i)
        {
            tmp->next = new Node(m_baseSize);
            if (first == nullptr)
            {
                first = tmp->next;
            }
            tmp = tmp->next;
            m_capacity += m_baseSize;
        }

        if (old_cap == 0)
        {
            m_current = first;
        }
    }

    size_t ByteArray::getCapacity() const
    {
        return m_capacity - m_position;
    }
}
