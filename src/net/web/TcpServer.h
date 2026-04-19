/**
 * @file TcpServer.h
 * @brief TCP服务器的封装
 * @date 2025-03-17
 */
#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "../../base/NonCopyable.h"
#include "../../core/IOManager.h"
#include "../Socket.h"
#include "../Address.h"
#include "../../base/Config.h"

namespace Gyanis::net::web
{
    /**
     * @brief TCP 服务器类
     */
    class TcpServer : public std::enable_shared_from_this<TcpServer>, NonCopyable
    {
    public:
        /**
         * @brief 构造函数
         * @param worker IO 线程，用于处理客户端的读写
         * @param io_worker IO 工作线程
         * @param accept_worker 接受连接的工作线程
         */
        explicit TcpServer(core::IOManager* worker = core::IOManager::GetThis(),
                           core::IOManager* io_worker = core::IOManager::GetThis(),
                           core::IOManager* accept_worker = core::IOManager::GetThis());

        /**
         * @brief 析构函数
         */
        virtual ~TcpServer();

        /**
         * @brief 绑定地址并启动监听
         * @param address 要绑定的地址
         * @param ssl 是否启用 SSL 加密通信
         */
        virtual bool bind(const std::shared_ptr<Address>& address, bool ssl);

        /**
         * @brief 绑定多个地址并启动监听
         * @param addresses 要绑定的地址列表
         * @param fails 存储绑定失败的地址
         * @param ssl 是否启用 SSL 加密通信
         */
        virtual bool bind(const std::vector<std::shared_ptr<Address>>& addresses,
                          std::vector<std::shared_ptr<Address>>& fails, bool ssl);

        /**
         * @brief 加载 SSL 证书
         * @param cert_file SSL 证书文件路径
         * @param key_file SSL 密钥文件路径
         */
        bool loadCertificates(const std::string& cert_file, const std::string& key_file) const;

        /**
         * @brief 启动服务器
         */
        virtual bool start();

        /**
         * @brief 停止服务器
         */
        virtual void stop();

        /**
         * @brief 获取接收超时时间
         */
        std::chrono::milliseconds getRecvTimeout() const;

        /**
         * @brief 设置接收超时时间
         */
        void setRecvTimeout(const std::chrono::milliseconds timeout);

        /**
         * @brief 检查服务器是否已停止
         */
        bool isStop() const;

        /**
         * @brief 获取服务器配置
         */
        std::shared_ptr<base::TcpServerConf> getConf() const;

        /**
         * @brief 设置服务器配置
         */
        void setConf(const std::shared_ptr<base::TcpServerConf>& value);

        /**
         * @brief 设置服务器配置（通过传值）
         */
        void setConf(const base::TcpServerConf& value);
        /**
         * @brief 获取服务器的字符串表示
         * @param prefix 可选的前缀，用于格式化输出
         * @return 服务器的字符串表示
         */
        virtual std::string toString(const std::string& prefix);

        /**
         * @brief 获取服务器的套接字列表
         */
        std::vector<std::shared_ptr<Socket>> getSockets() const;

        /**
         * @brief 返回服务器名称
         */
        std::string getName() const;

        /**
         * @brief 设置服务器名称
         */
        virtual void setName(const std::string& value);

    protected:
        /**
         * @brief 处理客户端连接
         */
        virtual void handleClient(const std::shared_ptr<Socket>& client);

        /**
         * @brief 启动接收客户端连接
         */
        virtual void startAccept(const std::shared_ptr<Socket>& sock);

        std::vector<std::shared_ptr<Socket>> m_sockets; ///< 服务器监听的套接字列表
        core::IOManager* m_worker; ///< IO 工作线程
        core::IOManager* m_ioWorker; ///< IO 线程
        core::IOManager* m_acceptWorker; ///< 接受连接的工作线程
        std::chrono::milliseconds m_recvTimeout; ///< 接收超时时间
        std::string m_type = "tcp"; ///< 服务器类型（默认为 TCP）
        std::string m_name; ///< 服务器名称
        bool m_isStop; ///< 是否停止服务器
        bool m_ssl = false; ///< 是否启用 SSL
        std::shared_ptr<base::TcpServerConf> m_conf;
    };
}

#endif
