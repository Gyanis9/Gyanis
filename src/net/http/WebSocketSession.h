/**
 * @file WebSocketSession.h
 * @brief WebSocket会话模块封装
 * @date 2025-04-03
 */

#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

#include "HttpSession.h"
#include "base/Config.h"
#include "net/Socket.h"

namespace Gyanis::net::http
{
#pragma pack(1)  // 设置结构体按 1 字节对齐

    /**
     * @struct WSFrameHead
     * @brief WebSocket 数据帧头，定义了 WebSocket 帧的基本结构和操作码
     */
    struct WSFrameHead
    {
        enum OPCODE
        {
            CONTINUE = 0, ///< 数据分片帧
            TEXT_FRAME = 1, ///< 文本帧
            BIN_FRAME = 2, ///< 二进制帧
            CLOSE = 8, ///< 断开连接帧
            PING = 0x9, ///< PING 帧
            PONG = 0xA ///< PONG 帧
        };

        uint32_t opcode : 4; ///< 操作码（4 位）
        bool rsv3 : 1; ///< 保留位 3（用于扩展）
        bool rsv2 : 1; ///< 保留位 2（用于扩展）
        bool rsv1 : 1; ///< 保留位 1（用于扩展）
        bool fin : 1; ///< 是否为最后一帧（1 位）
        uint32_t payload : 7; ///< 负载数据长度（7 位）
        bool mask : 1; ///< 是否应用掩码（1 位）

        /**
         * @brief 将 WSFrameHead 转换为字符串，便于调试
         */
        [[nodiscard]] std::string toString() const;
    };

#pragma pack()  // 取消结构体按 1 字节对齐

    /**
     * @brief WebSocket 帧消息类，用于表示 WebSocket 消息帧，包含消息的操作码和数据
     */
    class WSFrameMessage
    {
    public:
        /**
         * @brief 构造函数，初始化 WebSocket 消息帧 
         * @param opcode 操作码，默认值为 0
         * @param data 消息数据，默认值为空字符串
         */
        explicit WSFrameMessage(int opcode = 0, std::string data = "");

        /**
         * @brief 获取操作码
         */
        [[nodiscard]] int getOpcode() const;

        /**
         * @brief 设置操作码
         */
        void setOpcode(int value);

        /**
         * @brief 获取消息数据
         */
        [[nodiscard]] const std::string& getData() const;

        /**
         * @brief 获取消息数据（可修改）
         */
        std::string& getData();

        /**
         * @brief 设置消息数据
         */
        void setData(const std::string& value);

    private:
        int m_opcode; ///< 操作码
        std::string m_data; ///< 消息数据
    };

    /**
     * @brief WebSocket 会话类，继承自 `HttpSession`，处理 WebSocket 协议的握手、消息接收与发送等操作 
     */
    class WSSession : public HttpSession
    {
    public:
        /**
         * @brief 构造函数，初始化 WebSocket 会话 
         * @param sock WebSocket 连接的套接字 
         * @param owner 是否拥有该套接字，默认为 true 
         */
        explicit WSSession(const std::shared_ptr<Socket>& sock, bool owner = true);

        /**
         * @brief 处理 WebSocket 握手请求
         */
        std::shared_ptr<HttpRequest> handleShake();

        /**
         * @brief 接收 WebSocket 消息
         */
        std::shared_ptr<WSFrameMessage> recvMessage();

        /**
         * @brief 发送 WebSocket 消息 
         * @param msg WebSocket 消息对象 
         * @param fin 是否为最后一帧，默认为 true 
         * @return 返回发送操作的状态码 
         */
        int32_t sendMessage(const std::shared_ptr<WSFrameMessage>& msg, bool fin = true);

        /**
         * @brief 发送字符串形式的 WebSocket 消息 
         * @param msg 消息内容字符串 
         * @param opcode 操作码，默认为文本帧（`TEXT_FRAME`） 
         * @param fin 是否为最后一帧，默认为 true 
         * @return 返回发送操作的状态码 
         */
        int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);

        /**
         * @brief 发送 PING 帧，用于检测连接
         */
        int32_t ping();

        /**
         * @brief 发送 PONG 帧，回应 PING 帧
         */
        int32_t pong();

    private:
        /**
         * @brief 处理服务器端的 WebSocket 握手
         */
        static bool handleServerShake();

        /**
         * @brief 处理客户端的 WebSocket 握手
         */
        static bool handleClientShake();
    };

    /**
     * @brief WebSocket 消息最大大小配置变量
     */
    // extern std::shared_ptr<base::ConfigVar<uint32_t>> g_websocket_message_max_size;

    /**
     * @brief 从流中接收 WebSocket 消息 
     * @param stream 输入流对象 
     * @param client 标识是否为客户端 
     * @return 返回接收到的 WebSocket 消息对象 
     */
    std::shared_ptr<WSFrameMessage> WSRecvMessage(stream::Stream* stream, bool client);

    /**
     * @brief 向流中发送 WebSocket 消息 
     * @param stream 输出流对象 
     * @param msg WebSocket 消息对象 
     * @param client 标识是否为客户端 
     * @param fin 是否为最后一帧 
     * @return 返回发送操作的状态码 
     */
    int32_t WSSendMessage(stream::Stream* stream, const std::shared_ptr<WSFrameMessage>& msg, bool client, bool fin);

    /**
     * @brief 向流中发送 PING 帧 
     * @param stream 输出流对象 
     * @return 返回发送操作的状态码 
     */
    int32_t WSPing(stream::Stream* stream);

    /**
     * @brief 向流中发送 PONG 帧 
     * @param stream 输出流对象 
     * @return 返回发送操作的状态码 
     */
    int32_t WSPong(stream::Stream* stream);
}

#endif
