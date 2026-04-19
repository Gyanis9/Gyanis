#ifndef FDMANAGER_H
#define FDMANAGER_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include "base/Singleton.h"

namespace Gyanis::core
{
    /**
     * @brief 文件描述符上下文类
     */
    class FdContext : public std::enable_shared_from_this<FdContext>
    {
    public:
        /**
         * @brief 构造函数
         * @param fd 文件描述符。
         */
        explicit FdContext(int fd);

        /**
         * @brief 析构函数
         */
        ~FdContext();

        /**
         * @brief 检查文件描述符是否已初始化
         */
        [[nodiscard]] bool isInit() const;

        /**
         * @brief 检查文件描述符是否为 Socket 类型
         */
        [[nodiscard]] bool isSocket() const;

        /**
         * @brief 检查文件描述符是否已关闭
         */
        [[nodiscard]] bool isClose() const;

        /**
         * @brief 设置用户层的非阻塞模式
         */
        void setUserNonblock(bool value);

        /**
         * @brief 获取用户层的非阻塞模式设置
         */
        [[nodiscard]] bool getUserNonblock() const;

        /**
         * @brief 设置系统层的非阻塞模式
         */
        void setSysNonblock(bool value);

        /**
         * @brief 获取系统层的非阻塞模式设置
         */
        [[nodiscard]] bool getSysNonblock() const;

        /**
         * @brief 设置文件描述符的超时值
         * @param type 超时类型（读取或写入）
         * @param value 超时值（毫秒）
         */
        void setTimeout(int type, uint64_t value);

        /**
         * @brief 获取文件描述符的超时值
         * @param type 超时类型（读取或写入）
         * @return std::chrono::milliseconds 超时值
         */
        [[nodiscard]] std::chrono::milliseconds getTimeout(int type) const;

        /**
         * @brief 获取文件描述符的信息
         */
        [[nodiscard]] std::string toString() const;

    private:
        /**
         * @brief 初始化文件描述符上下文
         */
        [[nodiscard]] bool init();

        bool                      m_isInit      : 1; ///< 是否初始化
        bool                      m_isSocket    : 1; ///< 是否为 Socket
        bool                      m_sysNonblock : 1; ///< 是否为系统非阻塞
        bool                      m_userNonblock: 1; ///< 是否为用户非阻塞
        bool                      m_isClosed    : 1; ///< 是否关闭
        int                       m_fd;              ///< 文件描述符
        std::chrono::milliseconds m_recvTimeout;     ///< 接收超时
        std::chrono::milliseconds m_sendTimeout;     ///< 发送超时
    };

    /**
     * @brief 文件描述符管理器类状态
     */
    class FdManager
    {
    public:
        /**
         * @brief 构造函数
         */
        FdManager();

        /**
         * @brief 析构函数
         */
        ~FdManager();

        /**
         * @brief 获取文件描述符的上下文
         * @param fd 文件描述符。
         * @param auto_create 是否自动创建文件描述符上下文（默认为 `false`）
         * @return std::shared_ptr<FdContext> 文件描述符的上下文
         */
        [[nodiscard]] std::shared_ptr<FdContext> get(int fd, bool auto_create = false);

        /**
         * @brief 删除文件描述符的上下文
         */
        void del(int fd);

    private:
        std::shared_mutex                        m_mutex; ///< 保护文件描述符上下文的互斥锁
        std::vector<std::shared_ptr<FdContext> > m_datas; ///< 存储文件描述符上下文
    };

    /**
     * @brief 文件描述符管理器单例
     */
    using FdMgr = Singleton<FdManager>;
}

#endif
