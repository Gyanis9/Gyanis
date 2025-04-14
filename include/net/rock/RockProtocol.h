/**
 * @file RockProtocol.h
 * @brief 该文件定义了 Rock 协议相关的类，包括请求、响应、通知消息类以及消息头和解码器
 * @date 2025-04-02
 */

#ifndef ROCKPROTOCOL_H
#define ROCKPROTOCOL_H

#include "net/protocol/Protocol.h"
#include "google/protobuf/message.h"

namespace Gyanis::net::rock
{
    /**
     * @brief RockBody 类用于表示 Rock 协议中的消息体部分，提供消息体的设置、序列化和反序列化功能
     */
    class RockBody
    {
    public:
        virtual ~RockBody() = default;

        /**
         * @brief 设置消息体的内容  
         * @param value 消息体的字符串数据  
         */
        void setBody(std::string value);

        /**
         * @brief 获取消息体的内容  
         * @return 返回消息体内容的字符串  
         */
        [[nodiscard]] std::string getBody() const;

        /**
         * @brief 将消息体序列化为字节数组  
         * @param byteArray 用于存储序列化结果的字节数组  
         * @return 如果序列化成功，则返回 true，否则返回 false  
         */
        virtual bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray);

        /**
         * @brief 从字节数组中解析消息体  
         * @param byteArray 包含消息体数据的字节数组  
         * @return 如果解析成功，则返回 true，否则返回 false  
         */
        virtual bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray);

        /**
         * @brief 将消息体转换为 Protobuf 对象  
         * @tparam T Protobuf 消息类型  
         * @return 返回一个 Protobuf 对象的共享指针，如果解析失败则返回 nullptr  
         */
        template <typename T>
        std::shared_ptr<T> getAsPB() const
        {
            try
            {
                if (std::shared_ptr<T> data(new T); data->ParseFromString(m_body))
                {
                    return data;
                }
            }
            catch (...)
            {
            }
            return nullptr;
        }

        /**
         * @brief 将 Protobuf 对象序列化为消息体  
         * @tparam T Protobuf 消息类型  
         * @param value 要序列化的 Protobuf 对象  
         * @return 如果序列化成功，则返回 true，否则返回 false  
         */
        template <typename T>
        bool setAsPB(const T& value)
        {
            try
            {
                return value.SerializeToString(&m_body);
            }
            catch (...)
            {
            }
            return false;
        }

    protected:
        std::string m_body; ///< 存储消息体的字符串
    };


    class RockResponse;
    /**
     * @brief RockRequest 类表示 Rock 协议中的请求消息，继承自 `Message` 和 `RockBody`
     */
    class RockRequest final : public protocol::Request, public RockBody
    {
    public:
        /**
         * @brief 创建并返回一个新的 `RockResponse` 对象
         */
        [[nodiscard]] std::shared_ptr<RockResponse> createResponse() const;

        /**
         * @brief 将请求消息转换为字符串，通常用于调试
         */
        [[nodiscard]] std::string toString() const override;

        /**
         * @brief 获取请求消息的名称  
         */
        [[nodiscard]] const std::string& getName() const override;

        /**
         * @brief 获取请求消息的类型  
         * @return 返回消息类型（`REQUEST`）  
         */
        [[nodiscard]] int32_t getType() const override;

        /**
         * @brief 将请求消息序列化为字节数组  
         * @param byteArray 用于存储序列化数据的字节数组  
         * @return 返回序列化是否成功  
         */
        bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;

        /**
         * @brief 从字节数组中解析请求消息  
         * @param byteArray 包含请求数据的字节数组  
         * @return 返回解析是否成功  
         */
        bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;
    };

    /**
     * @brief RockResponse 类表示 Rock 协议中的响应消息，继承自 `Message` 和 `RockBody`
     */
    class RockResponse final : public protocol::Response, public RockBody
    {
    public:
        /**
         * @brief 将响应消息转换为字符串，通常用于调试
         */
        [[nodiscard]] std::string toString() const override;

        /**
         * @brief 获取响应消息的名称
         */
        [[nodiscard]] const std::string& getName() const override;

        /**
         * @brief 获取响应消息的类型
         */
        [[nodiscard]] int32_t getType() const override;

        /**
         * @brief 将响应消息序列化为字节数组  
         * @param byteArray 用于存储序列化数据的字节数组  
         * @return 返回序列化是否成功  
         */
        bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;

        /**
         * @brief 从字节数组中解析响应消息  
         * @param byteArray 包含响应数据的字节数组  
         * @return 返回解析是否成功  
         */
        bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;
    };

    /**
     * @brief RockNotify 类表示 Rock 协议中的通知消息，继承自 `Message` 和 `RockBody`
     */
    class RockNotify final : public protocol::Notify, public RockBody
    {
    public:
        /**
         * @brief 将通知消息转换为字符串，通常用于调试
         */
        [[nodiscard]] std::string toString() const override;

        /**
         * @brief 获取通知消息的名称
         */
        [[nodiscard]] const std::string& getName() const override;

        /**
         * @brief 获取通知消息的类型
         */
        [[nodiscard]] int32_t getType() const override;

        /**
         * @brief 将通知消息序列化为字节数组  
         * @param byteArray 用于存储序列化数据的字节数组  
         * @return 返回序列化是否成功  
         */
        bool serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;

        /**
         * @brief 从字节数组中解析通知消息  
         * @param byteArray 包含通知数据的字节数组  
         * @return 返回解析是否成功  
         */
        bool parseFromByteArray(const std::shared_ptr<stream::ByteArray>& byteArray) override;
    };

    /**
     * @brief Rock 消息头结构，包含消息的基本信息，如魔数、版本、标志和消息长度  
     */
    struct RockMsgHeader
    {
        RockMsgHeader();

        uint8_t magic[2]; ///< 魔数，用于标识消息的类型
        uint8_t version; ///< 消息版本
        uint8_t flag; ///< 标志字段，用于附加标志
        size_t length; ///< 消息长度
    };

    /**
     * @class RockMessageDecoder
     * @brief RockMessageDecoder 类用于解析和序列化 Rock 协议的消息
     */
    class RockMessageDecoder final : public protocol::MessageDecoder
    {
    public:
        /**
         * @brief 将消息序列化到流中  
         * @param stream 目标流  
         * @param message 需要序列化的消息  
         * @return 返回序列化的字节数  
         */
        int32_t serializeTo(const std::shared_ptr<stream::Stream>& stream,
                            const std::shared_ptr<protocol::Message>& message) override;

        /**
         * @brief 从流中解析消息  
         * @param stream 输入流  
         * @return 返回解析得到的消息对象  
         */
        std::shared_ptr<protocol::Message> parseForm(const std::shared_ptr<stream::Stream>& stream) override;
    };
}

#endif
