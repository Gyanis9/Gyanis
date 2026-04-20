/**
 * @file Protocol.h
 * @brief 协议模块封装
 * @date 2025-04-02
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "net/stream/Stream.h"
#include "net/stream/ByteArray.h"

#include <string>

namespace Gyanis::net::protocol
{
    /**
     * @brief 消息基类，定义了消息的基本接口
     */
    class Message
    {
    public:
        enum MessageType
        {
            REQUEST = 1, ///< 请求类型
            RESPONSE = 2, ///< 响应类型
            NOTIFY = 3 ///< 通知类型
        };

        virtual ~Message() = default;

        /**
         * @brief 将消息转换为字节数组
         */
        virtual std::shared_ptr<stream::ByteArray> toByteArray();

        /**
         * @brief 将消息序列化到字节数组中
         * @param byteArray 用于保存序列化数据的字节数组
         * @return 返回序列化是否成功
         */
        virtual bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) = 0;

        /**
         * @brief 从字节数组中解析消息
         * @param bytearray 包含消息数据的字节数组
         * @return 返回解析是否成功
         */
        virtual bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& bytearray) = 0;

        /**
         * @brief 将消息转换为字符串
         */
        [[nodiscard]] virtual std::string toString() const = 0;

        /**
         * @brief 获取消息名称
         */
        [[nodiscard]] virtual const std::string& getName() const = 0;

        /**
         * @brief 获取消息类型（REQUEST、RESPONSE、NOTIFY）
         */
        [[nodiscard]] virtual int32_t getType() const = 0;
    };

    /**
     * @brief 消息解码器，用于解析消息并将其序列化或反序列化。
     */
    class MessageDecoder
    {
    public:
        virtual ~MessageDecoder() = default;

        /**
         * @brief 解析消息流并返回一个 Message 对象
         */
        virtual std::shared_ptr<Message> parseForm(const std::shared_ptr<stream::Stream>& stream) = 0;

        /**
         * @brief 将消息序列化到流中
         * @param stream 目标流
         * @param message 需要序列化的消息对象
         * @return 返回序列化的字节数
         */
        virtual int32_t serializeTo(const std::shared_ptr<stream::Stream>& stream,
                                    const std::shared_ptr<Message>& message) = 0;
    };

    /**
     * @brief 请求消息类，继承自 Message，表示请求消息
     */
    class Request : public Message
    {
    public:
        Request();

        /**
         * @brief 获取序列号（SN）
         */
        [[nodiscard]] uint32_t getSn() const;

        /**
         * @brief 获取命令码（CMD）
         */
        [[nodiscard]] uint32_t getCmd() const;

        /**
         * @brief 设置序列号（SN）
         */
        void setSn(uint32_t value);

        /**
         * @brief 设置命令码（CMD）
         * @param value 命令码值
         */
        void serCmd(uint32_t value);

        /**
         * @brief 将请求消息序列化到字节数组中
         * @param byteArray 用于存储序列化数据的字节数组
         * @return 返回序列化是否成功
         */
        bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;

        /**
         * @brief 从字节数组中解析请求消息
         * @param bytearray 包含请求数据的字节数组
         * @return 返回解析是否成功
         */
        bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& bytearray) override;

    protected:
        uint32_t m_sn; ///< 序列号
        uint32_t m_cmd; ///< 命令码
    };

    /**
     * @brief 响应消息类，继承自 Message，表示响应消息
     */
    class Response : public Message
    {
    public:
        Response();

        /**
         * @brief 设置序列号（SN）
         */
        void setSn(uint32_t Sn);

        /**
         * @brief 设置命令码（CMD）
         */
        void setCmd(uint32_t Cmd);

        /**
         * @brief 设置结果码
         */
        void setResult(uint32_t Result);

        /**
         * @brief 设置结果描述
         */
        void setResultStr(const std::string& ResultStr);

        /**
         * @brief 获取序列号（SN）
         */
        [[nodiscard]] uint32_t getSn() const;

        /**
         * @brief 获取命令码（CMD）
         */
        [[nodiscard]] uint32_t getCmd() const;

        /**
         * @brief 获取结果码
         */
        [[nodiscard]] uint32_t getResult() const;

        /**
         * @brief 获取结果描述
         */
        [[nodiscard]] const std::string& getResultStr() const;

        /**
         * @brief 将响应消息序列化到字节数组中
         * @param byteArray 用于存储序列化数据的字节数组
         * @return 返回序列化是否成功
         */
        bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;

        /**
         * @brief 从字节数组中解析响应消息
         * @param bytearray 包含响应数据的字节数组
         * @return 返回解析是否成功
         */
        bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& bytearray) override;

    protected:
        uint32_t m_sn; ///< 序列号
        uint32_t m_cmd; ///< 命令码
        uint32_t m_result; ///< 结果码
        std::string m_resultStr; ///< 结果描述
    };

    /**
     * @brief 通知消息类，继承自 Message，表示通知消息
     */
    class Notify : public Message
    {
    public:
        Notify();

        /**
         * @brief 设置通知标识
         * @param notify 通知标识值
         */
        void setNotify(uint32_t notify);

        /**
         * @brief 获取通知标识
         * @return 返回通知标识值
         */
        [[nodiscard]] uint32_t getNotify() const;

        /**
         * @brief 将通知消息序列化到字节数组中。
         * @param byteArray 用于存储序列化数据的字节数组。
         * @return 返回序列化是否成功。
         */
        bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;

        /**
         * @brief 从字节数组中解析通知消息。
         * @param bytearray 包含通知数据的字节数组。
         * @return 返回解析是否成功。
         */
        bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& bytearray) override;

    protected:
        uint32_t m_notify; ///< 通知标识
    };
}

#endif
