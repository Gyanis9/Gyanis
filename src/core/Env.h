/**
 * @file Env.h
 * @brief 环境变量和命令行参数模块封装
 * @date 2025-04-02
 */

#ifndef ENV_H
#define ENV_H

#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include "base/Singleton.h"

namespace Gyanis::core
{
    /**
     * @brief Env 类用于管理命令行参数、环境变量及相关操作
     */
    class Env
    {
    public:
        /**
         * @brief 初始化环境，处理命令行参数
         * @param argc 命令行参数数量
         * @param argv 命令行参数数组
         */
        bool init(int argc, char** argv);

        /**
         * @brief 向环境中添加一个键值对
         */
        void add(const std::string& key, const std::string& value);

        /**
         * @brief 检查环境中是否包含指定的键
         */
        bool has(const std::string& key);

        /**
         * @brief 从环境中删除指定的键值对
         */
        void del(const std::string& key);

        /**
         * @brief 获取指定键的值，如果键不存在则返回默认值
         */
        std::string get(const std::string& key, const std::string& default_value = "");

        /**
         * @brief 为指定的键添加帮助描述信息
         */
        void addHelp(const std::string& key, const std::string& desc);

        /**
         * @brief 移除指定键的帮助描述信息
         */
        void removeHelp(const std::string& key);

        /**
         * @brief 打印所有键的帮助描述信息
         */
        void printHelp();

        /**
         * @brief 获取当前程序的可执行文件名
         */
        std::string_view getExe() const;

        /**
         * @brief 获取当前程序的工作目录
         */
        std::string getCwd() const;

        /**
         * @brief 设置环境变量
         */
        static bool setEnv(const std::string& key, const std::string& value);

        /**
         * @brief 获取环境变量的值
         */
        static std::string getEnv(const std::string& key, const std::string& default_value = "");

        /**
         * @brief 获取指定相对路径的绝对路径
         */
        std::string getAbsolutePath(const std::string& path) const;

        /**
         * @brief 获取指定相对路径在当前工作目录下的绝对路径
         */
        static std::string getAbsoluteWorkPath(const std::string& path);

        /**
         * @brief 获取配置文件的路径
         */
        std::string getConfigPath();

    private:
        std::shared_mutex m_mutex; ///< 用于线程安全操作环境变量的互斥锁
        std::unordered_map<std::string, std::string> m_args; ///< 存储命令行参数的键值对
        std::vector<std::pair<std::string, std::string>> m_helps; ///< 存储帮助描述信息的列表（键值对）
        std::string m_program; ///< 程序名
        std::string m_exe; ///< 可执行文件名（例如运行程序的名称）
        std::string m_cwd; ///< 当前工作目录
    };

    using EnvMgr = Singleton<Env>;
}

#endif
