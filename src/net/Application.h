/**
 * @file Application.h
 * @brief 管理应用程序模块封装
 * @date 2025-04-03
 */

#ifndef APPLICATION_H
#define APPLICATION_H
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

#include "net/web/TcpServer.h"

namespace Gyanis::net
{
    /**
     * @brief 应用程序类，负责应用程序的初始化、运行、服务器管理等操作
     */
    class Application
    {
    public:
        /**
         * @brief 构造函数，初始化 `Application` 对象
         */
        Application();

        /**
         * @brief 获取 `Application` 类的单例实例
         */
        static Application* GetInstance();

        /**
         * @brief 初始化应用程序，解析命令行参数并进行必要的初始化操作
         * @param argc 参数个数
         * @param argv 参数值数组

         */
        bool init(int argc, char** argv);

        /**
         * @brief 运行应用程序，启动应用的主循环
         */
        bool run();

        /**
         * @brief 根据服务器类型获取对应的服务器列表
         * @param type 服务器类型
         * @param servers 存储返回服务器对象的向量
         */
        bool getServer(const std::string& type, std::vector<std::shared_ptr<web::TcpServer>>& servers);

        /**
         * @brief 获取所有的服务器并以类型为键组织成一个字典
         * @param servers 存储所有服务器信息的映射
         */
        void listAllServer(
            std::unordered_map<std::string, std::vector<std::shared_ptr<web::TcpServer>>>& servers) const;

    private:
        /**
         * @brief 应用程序的主函数，解析命令行参数并执行初始化任务
         * @param argc 参数个数
         * @param argv 参数值数组
         */
        int main(int argc, char** argv);

        /**
         * @brief 启动协程来运行应用程序
         */
        int run_fiber();

        int m_argc = 0; ///< 命令行参数的个数
        char** m_argv = nullptr; ///< 命令行参数数组
        std::unordered_map<std::string, std::vector<std::shared_ptr<web::TcpServer>>> m_servers; ///< 服务器类型与服务器对象的映射
        std::shared_ptr<core::IOManager> m_mainIOManager; ///< 应用程序的主 IO 管理器
        static Application* s_instance; ///< `Application` 的单例实例
    };
}

#endif
