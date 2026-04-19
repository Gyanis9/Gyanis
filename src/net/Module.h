/**
 * @file Module.h
 * @brief 模块管理模块封装
 * @date 2025-04-03
 */

#ifndef MODULE_H
#define MODULE_H

#include <shared_mutex>
#include "../base/Singleton.h"
#include <unordered_map>
#include <functional>
#include "stream/Stream.h"
#include "protocol/Protocol.h"

namespace Gyanis::net
{
    /**
     * @brief 模块类，用于表示一个可加载和卸载的模块 
     */
    class Module
    {
    public:
        enum Type
        {
            MODULE = 0, ///< 普通模块
            ROCK = 1, ///< Rock 协议模块
        };

        /**
         * @brief 模块类构造函数，初始化模块的基本信息 
         * @param name 模块名称 
         * @param version 模块版本 
         * @param filename 模块的文件名 
         * @param type 模块类型，默认为普通模块（`MODULE`） 
         */
        explicit Module(const std::string& name, const std::string& version, std::string filename,
                        uint32_t type = MODULE);

        virtual ~Module() = default;

        /**
         * @brief 模块初始化前的参数解析操作，用户可以在此方法中解析命令行参数 
         * @param argc 参数个数 
         * @param argv 参数值数组 
         */
        virtual void onBeforeArgsParse(int argc, char** argv);

        /**
         * @brief 模块初始化后的参数解析操作，用户可以在此方法中处理解析后的参数 
         * @param argc 参数个数 
         * @param argv 参数值数组 
         */
        virtual void onAfterArgsParse(int argc, char** argv);

        /**
         * @brief 加载模块，执行模块加载的操作 通常用于模块初始化时的操作
         */
        virtual bool onLoad();

        /**
         * @brief 卸载模块，执行模块卸载的操作 通常用于模块卸载时的清理操作
         */
        virtual bool onUnload();

        /**
         * @brief 处理 WebSocket 连接的请求 
         * @param stream WebSocket 数据流
         */
        virtual bool onConnect(const std::shared_ptr<stream::Stream>& stream);

        /**
         * @brief 处理 WebSocket 断开连接的请求 
         * @param stream WebSocket 数据流
         */
        virtual bool onDisconnect(const std::shared_ptr<stream::Stream>& stream);

        /**
         * @brief 服务器准备好时的回调函数
         */
        virtual bool onServerReady();

        /**
         * @brief 服务器启动时的回调函数
         */
        virtual bool onServerUp();

        /**
         * @brief 处理请求消息 
         * @param request 请求消息对象 
         * @param response 响应消息对象 
         * @param stream 数据流
         */
        virtual bool
        handleRequest(const std::shared_ptr<protocol::Message>& request,
                      const std::shared_ptr<protocol::Message>& response,
                      const std::shared_ptr<stream::Stream>& stream);

        /**
         * @brief 处理通知消息 
         * @param notify 通知消息对象 
         * @param stream 数据流
         */
        virtual bool handleNotify(const std::shared_ptr<protocol::Message>& notify,
                                  const std::shared_ptr<stream::Stream>& stream);

        /**
         * @brief 获取模块的状态字符串
         */
        virtual std::string statusString();

        /**
         * @brief 获取模块的名称
         */
        [[nodiscard]] const std::string& getName() const;

        /**
         * @brief 获取模块的版本
         */
        [[nodiscard]] const std::string& getVersion() const;

        /**
         * @brief 获取模块的文件名
         */
        [[nodiscard]] const std::string& getFilename() const;

        /**
         * @brief 获取模块的 ID
         */
        [[nodiscard]] const std::string& getId() const;

        /**
         * @brief 设置模块的文件名
         */
        void setFilename(const std::string& value);

        /**
         * @brief 获取模块的类型
         */
        [[nodiscard]] uint32_t getType() const;

        /**
         * @brief 注册服务到指定的服务器类型、域名和服务名 
         * @param server_type 服务器类型 
         * @param domain 域名 
         * @param service 服务名 
         */
        static void registerService(const std::string& server_type,
                                    const std::string& domain, const std::string& service);

    protected:
        std::string m_name; ///< 模块名称
        std::string m_version; ///< 模块版本
        std::string m_filename; ///< 模块文件名
        std::string m_id; ///< 模块 ID
        uint32_t m_type; ///< 模块类型
    };

    /**
     * @brief 模块管理器类，负责管理系统中的所有模块，包括加载、卸载、连接、断开等操作 
     */
    class ModuleManager
    {
    public:
        /**
         * @brief 构造函数，初始化模块管理器 
         */
        ModuleManager();

        /**
         * @brief 向模块管理器中添加一个模块
         */
        void add(const std::shared_ptr<Module>& value);

        /**
         * @brief 根据名称删除模块
         */
        void del(const std::string& name);

        /**
         * @brief 删除所有模块 
         */
        void delAll();

        /**
         * @brief 初始化模块管理器，加载所有已注册的模块 
         */
        void init();

        /**
         * @brief 根据模块名称获取模块对象 
         * @param name 模块名称
         */
        std::shared_ptr<Module> get(const std::string& name);

        /**
         * @brief 处理模块的连接操作 
         * @param stream 数据流 
         */
        void onConnect(const std::shared_ptr<stream::Stream>& stream);

        /**
         * @brief 处理模块的断开连接操作 
         * @param stream 数据流 
         */
        void onDisconnect(const std::shared_ptr<stream::Stream>& stream);

        /**
         * @brief 获取所有模块的列表 
         * @param ms 存储模块对象的向量 
         */
        void listAll(std::vector<std::shared_ptr<Module>>& ms);

        /**
         * @brief 获取指定类型的模块列表 
         * @param type 模块类型 
         * @param ms 存储模块对象的向量 
         */
        void listByType(uint32_t type, std::vector<std::shared_ptr<Module>>& ms);

        /**
         * @brief 遍历指定类型的所有模块，并执行回调函数 
         * @param type 模块类型 
         * @param cb 回调函数 
         */
        void foreach(uint32_t type, const std::function<void(std::shared_ptr<Module>)>& cb);

    private:
        /**
         * @brief 初始化模块 
         * @param path 模块路径 
         */
        void initModule(const std::string& path);

        std::shared_mutex m_mutex; ///< 保护模块集合的互斥锁
        std::unordered_map<std::string, std::shared_ptr<Module>> m_modules; ///< 模块名称与模块对象的映射
        std::unordered_map<uint32_t, std::unordered_map<std::string, std::shared_ptr<Module>>> m_type2Modules;
        ///< 模块类型与模块对象的映射
    };

    using ModuleMgr = Singleton<ModuleManager>;
}

#endif
