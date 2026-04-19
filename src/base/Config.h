#ifndef CONFIG_H
#define CONFIG_H

#include <atomic>
#include <concepts>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>

#include "base/Log.h"
#include "base/Utils.h"

namespace Gyanis::base
{
    struct LogDefine;
    /**
     * @brief 配置变量的基类
     */
    class ConfigVarBase
    {
    public:
        /**
         * @brief 构造函数
         * @param[in] name 配置参数名称[0-9a-z_.]，用来标识该配置项 
         * @param[in] description 配置参数描述，用于说明该配置项的含义和作用 
         */
        ConfigVarBase(std::string name, std::string description);

        /**
         * @brief 析构函数
         */
        virtual ~ConfigVarBase() = default;

        /**
         * @brief 返回配置参数名称
         */
        [[nodiscard]] std::string getName() const;

        /**
         * @brief 返回配置参数的描述
         */
        [[nodiscard]] std::string getDescription() const;

        /**
         * @brief 转成字符串
         */
        [[nodiscard]] virtual std::string toString() const = 0;

        /**
         * @brief 从字符串初始化值
         * @param[in] value 配置项的字符串表示，用来初始化配置变量的值
         */
        virtual bool fromString(const std::string &value) = 0;

        /**
         * @brief 返回配置参数值的类型名称
         */
        [[nodiscard]] virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;        ///< 配置参数的名称
        std::string m_description; ///< 配置参数的描述
    };

    /**
     * @brief 类型转换模板类 (F 源类型, T 目标类型)
     *
     * @tparam F 源类型
     * @tparam T 目标类型
     */
    template<typename F, typename T>
    class LexicalCast
    {
    public:
        T operator()(const F &value) const
        {
            return boost::lexical_cast<T>(value);
        }
    };

    template<typename FromStr, typename ToStr, typename T>
    concept ConfigCaster = requires(FromStr from, ToStr to, const std::string &text, const T &value)
    {
        { from(text) } -> std::same_as<T>;
        { to(value) } -> std::convertible_to<std::string>;
    };

    /**
     * @brief 类型转换模板类片特化 (YAML String 转换成 std::vector<T>)
     */
    template<typename T>
    class LexicalCast<std::string, std::vector<T> >
    {
    public:
        std::vector<T> operator()(const std::string &value)
        {
            YAML::Node        node = YAML::Load(value);
            std::vector<T>    result;
            std::stringstream ss;
            for (auto &&i: node)
            {
                ss.str("");
                ss << i;
                result.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return result;
        }
    };

    /**
     * @brief 类型转换模板类片特化 (std::vector<T> 转换成 YAML String)
     */
    template<typename T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &value)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i: value)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化 (YAML String 转换成 std::list<T>)
     */
    template<typename T>
    class LexicalCast<std::string, std::list<T> >
    {
    public:
        std::list<T> operator()(const std::string &value)
        {
            YAML::Node        node = YAML::Load(value);
            std::list<T>      result;
            std::stringstream ss;
            for (auto &&i: node)
            {
                ss.str("");
                ss << i;
                result.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return result;
        }
    };

    /**
     * @brief 类型转换模板类片特化 (std::list<T> 转换成 YAML String)
     */
    template<typename T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &value)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i: value)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化 (YAML String 转换成 std::set<T>)
     */
    template<typename T>
    class LexicalCast<std::string, std::set<T> >
    {
    public:
        std::set<T> operator()(const std::string &value)
        {
            YAML::Node        node = YAML::Load(value);
            std::set<T>       result;
            std::stringstream ss;
            for (auto &&i: node)
            {
                ss.str("");
                ss << i;
                result.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return result;
        }
    };

    /**
     * @brief 类型转换模板类片特化 (std::set<T> 转换成 YAML String)
     */
    template<typename T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &value)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i: value)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化 (YAML String 转换成 std::unordered_set<T>)
     */
    template<typename T>
    class LexicalCast<std::string, std::unordered_set<T> >
    {
    public:
        std::unordered_set<T> operator()(const std::string &value)
        {
            YAML::Node            node = YAML::Load(value);
            std::unordered_set<T> result;
            std::stringstream     ss;
            for (auto &&i: node)
            {
                ss.str("");
                ss << i;
                result.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return result;
        }
    };

    /**
     * @brief 类型转换模板类片特化 (std::unordered_set<T> 转换成 YAML String)
     */
    template<typename T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &value)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i: value)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化 (YAML String 转换成 std::map<std::string, T>)
     */
    template<typename T>
    class LexicalCast<std::string, std::map<std::string, T> >
    {
    public:
        std::map<std::string, T> operator()(const std::string &value)
        {
            YAML::Node               node = YAML::Load(value);
            std::map<std::string, T> result;
            std::stringstream        ss;
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                result.insert(std::make_pair(it->first.Scalar(),
                                             LexicalCast<std::string, T>()(ss.str())));
            }
            return result;
        }
    };

    /**
     * @brief 类型转换模板类片特化 (std::map<std::string, T> 转换成 YAML String)
     */
    template<typename T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &value)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &i: value)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化 (YAML String 转换成 std::unordered_map<std::string, T>)
     */
    template<typename T>
    class LexicalCast<std::string, std::unordered_map<std::string, T> >
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &value)
        {
            YAML::Node                         node = YAML::Load(value);
            std::unordered_map<std::string, T> result;
            std::stringstream                  ss;
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                result.insert(std::make_pair(it->first.Scalar(),
                                             LexicalCast<std::string, T>()(ss.str())));
            }
            return result;
        }
    };

    /**
     * @brief 类型转换模板类片特化 (std::unordered_map<std::string, T> 转换成 YAML String)
     */
    template<typename T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &value)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &i: value)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 配置参数模板子类, 保存对应类型的参数值
     * @tparam T 配置项的值类型
     * @tparam FromStr 从 `std::string` 转换为 `T` 类型的仿函数，默认为 `LexicalCast<std::string, T>`
     * @tparam ToStr 从 `T` 类型转换为 `std::string` 的仿函数，默认为 `LexicalCast<T, std::string>`
     */
    template<typename T, typename FromStr = LexicalCast<std::string, T>, typename ToStr = LexicalCast<T, std::string> > requires ConfigCaster<FromStr, ToStr, T>
    class ConfigVar final : public ConfigVarBase
    {
    public:
        using CallBackMap = std::unordered_map<uint64_t, std::function<void(const T &old_value, const T &new_value)> >;
        /**
         * @brief 通过参数名, 参数值, 描述构造 ConfigVar
         * @param[in] name 配置项的名称，名称有效字符为 [0-9a-z_.]
         * @param[in] default_value 配置项的默认值
         * @param[in] description 配置项的描述
         */
        explicit ConfigVar(const std::string &name, T default_value, const std::string &description) : ConfigVarBase(name, description), m_value(std::move(default_value))
        {
        }

        /**
         * @brief 将参数值转换成 YAML 字符串
         */
        std::string toString() const override
        {
            try
            {
                std::shared_lock lock(m_mutex);
                return ToStr()(m_value);
            } catch (const std::exception &e)
            {
                LOG_ERROR(LOG_ROOT())
                    << "[配置] toString 转换异常"
                    << " | 变量名: " << m_name
                    << " | 类型: " << TypeToName<T>()
                    << " | 错误: " << e.what();
            }
            return "";
        }

        /**
         * @brief 从 YAML 字符串转换为参数的值
         */
        bool fromString(const std::string &value) override
        {
            try
            {
                setValue(FromStr()(value));
                return true;
            } catch (const std::exception &e)
            {
                LOG_ERROR(LOG_ROOT())
                    << "[配置] fromString 转换异常"
                    << " | 变量名: " << m_name
                    << " | 类型: " << TypeToName<T>()
                    << " | 输入: " << value
                    << " | 错误: " << e.what();
                return false;
            }
        }

        /**
         * @brief 获取当前配置参数的值
         */
        [[nodiscard]] const T &getValue() const
        {
            std::shared_lock lock(m_mutex);
            return m_value;
        }

        /**
         * @brief 设置当前配置参数的值
         */
        void setValue(const T &value)
        {
            {
                std::shared_lock lock(m_mutex);
                if (value == m_value)
                {
                    return;
                }
                for (auto &i: m_callbacks)
                {
                    i.second(m_value, value);
                }
            }
            std::unique_lock lock(m_mutex);
            m_value = value;
        }

        /**
         * @brief 返回配置项值的类型名称
         */
        [[nodiscard]] std::string getTypeName() const override
        {
            return TypeToName<T>();
        }

        /**
         * @brief 添加变化回调函数
         */
        uint64_t addListener(std::function<void(const T &old_value, const T &new_value)> callback)
        {
            static std::atomic<uint64_t> s_counter{0};
            std::unique_lock             lock(m_mutex);
            const auto                   id = ++s_counter;
            m_callbacks[id]                 = std::move(callback);
            return id;
        }

        /**
         * @brief 删除回调函数
         */
        void delListener(uint64_t id)
        {
            std::unique_lock lock(m_mutex);
            m_callbacks.erase(id);
        }

        /**
         * @brief 获取回调函数
         */
        std::function<void(const T &old_value, const T &new_value)> getListener(uint64_t id)
        {
            std::shared_lock lock(m_mutex);
            auto             it = m_callbacks.find(id);
            return it == m_callbacks.end() ? nullptr : it->second;
        }

        /**
         * @brief 清理所有的回调函数
         */
        void clearListeners()
        {
            std::unique_lock lock(m_mutex);
            m_callbacks.clear();
        }

    private:
        mutable std::shared_mutex m_mutex;     ///< 共享互斥锁，确保线程安全
        T                         m_value;     ///< 存储配置项的值
        CallBackMap               m_callbacks; ///< 回调函数列表
    };

    /**
     * @brief ConfigVar 的管理类
     */
    class Config
    {
    public:
        /**
         * @brief 获取/创建对应参数名的配置参数
         * @param[in] name 配置项的名称
         * @param[in] default_value 配置项的默认值
         * @param[in] description 配置项的描述
         */
        template<typename T>
        static std::shared_ptr<ConfigVar<T> > LookUp(const std::string &name, const T &default_value,
                                                     const std::string &description = "")
        {
            std::unique_lock lock(GetMutex());
            if (const auto it = GetDatas().find(name); it != GetDatas().end())
            {
                auto temp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
                if (temp != nullptr)
                {
                    LOG_INFO(LOG_ROOT()) << "[配置] 找到已存在配置项"
                                         << " | 名称: " << name;
                    return temp;
                }
                LOG_ERROR(LOG_ROOT())
                    << "[配置] 配置项类型不匹配"
                    << " | 名称: " << name
                    << " | 期望类型: " << TypeToName<T>()
                    << " | 实际类型: " << it->second->getTypeName()
                    << " | 当前值: " << it->second->toString();
                return nullptr;
            }
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                LOG_FATAL(LOG_ROOT()) << "[配置] 配置项名称非法"
                                     << " | 名称: " << name;
                throw std::invalid_argument(name);
            }
            std::shared_ptr<ConfigVar<T> > value = std::make_shared<ConfigVar<T> >(name, default_value, description);
            GetDatas()[name]                     = value;
            return value;
        }

        /**
         * @brief 查找配置参数
         */
        template<typename T>
        static std::shared_ptr<ConfigVar<T> > LookUp(const std::string &name)
        {
            std::shared_lock lock(GetMutex());
            if (const auto it = GetDatas().find(name); it != GetDatas().end())
            {
                return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
            }
            return nullptr;
        }

        /**
         * @brief 使用 YAML::Node 初始化配置模块
         */
        static void LoadFromYaml(const YAML::Node &root);

        /**
         * @brief 加载配置目录中的所有配置文件
         */
        static void LoadFromConfigDir(const std::string &path, bool force = false);

        /**
         * @brief 查找配置参数，返回配置参数的基类
         */
        static std::shared_ptr<ConfigVarBase> LookUpBase(const std::string &name);

        /**
         * @brief 遍历配置模块中的所有配置项
         */
        static void Visit(const std::function<void(std::shared_ptr<ConfigVarBase>)> &visitor);

    private:
        /**
         * @brief 返回所有的配置项
         */
        static std::unordered_map<std::string, std::shared_ptr<ConfigVarBase> > &GetDatas();

        /**
         * @brief 配置项的读写锁
         */
        static std::shared_mutex &GetMutex();
    };

    /**
     * @brief 列出所有成员
     */
    static void ListAllMember(const std::string &                                   prefix,
                              const YAML::Node &                                    node,
                              std::list<std::pair<std::string, const YAML::Node> > &output);

    /**
     * @brief 日志输出目标定义
     *
     * @param type 输出目标类型（1表示文件输出，2表示标准输出）
     * @param level 日志级别
     * @param formatter 输出格式化字符串
     * @param file 文件路径，仅在type为1时有效
     */
    struct LogAppenderDefine
    {
        int             type  = 0;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string     formatter;
        std::string     file;

        bool operator==(const LogAppenderDefine &oth) const
        {
            return type == oth.type
                   && level == oth.level
                   && formatter == oth.formatter
                   && file == oth.file;
        }
    };

    /**
     * @brief 日志定义，包含日志器名称、级别、格式器和多个日志输出目标
     *
     * @param name 日志器名称
     * @param level 日志级别
     * @param formatter 格式化字符串
     * @param appenders 日志输出目标列表
     */
    struct LogDefine
    {
        std::string                    name;
        LogLevel::Level                level = LogLevel::UNKNOW;
        std::string                    formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &oth) const
        {
            return name == oth.name
                   && level == oth.level
                   && formatter == oth.formatter
                   && appenders == oth.appenders;
        }

        bool operator<(const LogDefine &oth) const
        {
            return name < oth.name;
        }

        [[maybe_unused]] [[nodiscard]] bool isValid() const
        {
            return !name.empty();
        }
    };

    /**
     * @brief TCP 服务器配置结构体
     */
    struct TcpServerConf
    {
        std::vector<std::string>                     address;                   ///< 服务器监听的地址列表
        int                                          keepalive = 0;             ///< keepalive 设置，表示保持连接的超时时间
        int                                          timeout   = 1000 * 2 * 60; ///< 超时时间，单位：毫秒
        int                                          ssl       = 0;             ///< 是否启用 SSL 加密通信
        std::string                                  id;                        ///< 服务器的唯一标识符
        std::string                                  type = "http";             ///< 服务器类型（如 http, ws, rock）
        std::string                                  name;                      ///< 服务器名称
        std::string                                  cert_file;                 ///< SSL 证书文件路径
        std::string                                  key_file;                  ///< SSL 密钥文件路径
        std::string                                  accept_worker;             ///< 接受连接的工作线程
        std::string                                  io_worker;                 ///< IO 处理的工作线程
        std::string                                  process_worker;            ///< 处理请求的工作线程
        std::unordered_map<std::string, std::string> args;                      ///< 其他自定义参数

        [[nodiscard]] bool isValid() const
        {
            return !address.empty();
        }

        bool operator==(const TcpServerConf &oth) const
        {
            return address == oth.address &&
                   keepalive == oth.keepalive &&
                   timeout == oth.timeout &&
                   name == oth.name &&
                   ssl == oth.ssl &&
                   cert_file == oth.cert_file &&
                   key_file == oth.key_file &&
                   accept_worker == oth.accept_worker &&
                   io_worker == oth.io_worker &&
                   process_worker == oth.process_worker &&
                   args == oth.args &&
                   id == oth.id &&
                   type == oth.type;
        }
    };


    /**
     * @brief 将字符串转换为 LogDefine 对象
     * @param[in] value 配置字符串，通常为 YAML 格式
     * @return 转换后的 LogDefine 对象
     */
    template<>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &value) const
        {
            YAML::Node node = YAML::Load(value);
            LogDefine  log_define;
            if (!node["name"].IsDefined())
            {
                std::cout << "[配置] LogDefine 缺少 name 字段，原始内容: "
                        << node
                        << std::endl;
                throw std::logic_error("LogDefine 缺少 name 字段");
            }
            log_define.name  = node["name"].as<std::string>();
            log_define.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
            if (node["formatter"].IsDefined())
            {
                log_define.formatter = node["formatter"].as<std::string>();
            }

            if (node["appenders"].IsDefined())
            {
                for (size_t x = 0; x < node["appenders"].size(); ++x)
                {
                    auto appender = node["appenders"][x];
                    if (!appender["type"].IsDefined())
                    {
                        std::cout << "[配置] LogAppender 缺少 type 字段，原始内容: "
                                << appender
                                << std::endl;
                        continue;
                    }
                    auto              type = appender["type"].as<std::string>();
                    LogAppenderDefine log_appender_define;
                    if (type == "FileLogAppender")
                    {
                        log_appender_define.type = 1;
                        if (!appender["file"].IsDefined())
                        {
                            std::cout << "[配置] FileLogAppender 缺少 file 字段，原始内容: "
                                    << appender
                                    << std::endl;
                            continue;
                        }
                        log_appender_define.file = appender["file"].as<std::string>();
                        if (appender["formatter"].IsDefined())
                        {
                            log_appender_define.formatter = appender["formatter"].as<std::string>();
                        }
                    } else if (type == "StdoutLogAppender")
                    {
                        log_appender_define.type = 2;
                        if (appender["formatter"].IsDefined())
                        {
                            log_appender_define.formatter = appender["formatter"].as<std::string>();
                        }
                    } else
                    {
                        std::cout << "[配置] LogAppender 类型非法，原始内容: "
                                << appender
                                << std::endl;
                        continue;
                    }

                    log_define.appenders.push_back(log_appender_define);
                }
            }
            return log_define;
        }
    };

    /**
     * @brief 将 LogDefine 对象转换为字符串
     */
    template<>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine &value) const
        {
            YAML::Node node;
            node["name"] = value.name;
            if (value.level != LogLevel::UNKNOW)
            {
                node["level"] = LogLevel::ToString(value.level);
            }
            if (!value.formatter.empty())
            {
                node["formatter"] = value.formatter;
            }

            for (const auto &[type, level, formatter, file]: value.appenders)
            {
                YAML::Node node_appenders;
                if (type == 1)
                {
                    node_appenders["type"] = "FileLogAppender";
                    node_appenders["file"] = file;
                } else if (type == 2)
                {
                    node_appenders["type"] = "StdoutLogAppender";
                }
                if (level != LogLevel::UNKNOW)
                {
                    node_appenders["level"] = LogLevel::ToString(level);
                }
                if (!formatter.empty())
                {
                    node_appenders["formatter"] = formatter;
                }
                node["appenders"].push_back(node_appenders);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };


    /**
     * @brief `TcpServerConf` 配置的字符串转换
     */
    template<>
    class LexicalCast<std::string, TcpServerConf>
    {
    public:
        TcpServerConf operator()(const std::string &value) const
        {
            YAML::Node    node = YAML::Load(value);
            TcpServerConf server_conf;

            // 解析基础字段（带默认值）
            server_conf.id        = node["id"].as<std::string>(server_conf.id);
            server_conf.type      = node["type"].as<std::string>(server_conf.type);
            server_conf.keepalive = node["keepalive"].as<int>(server_conf.keepalive);
            server_conf.timeout   = node["timeout"].as<int>(server_conf.timeout);
            server_conf.name      = node["name"].as<std::string>(server_conf.name);
            server_conf.ssl       = node["ssl"].as<int>(server_conf.ssl);
            server_conf.cert_file = node["cert_file"].as<std::string>(server_conf.cert_file);
            server_conf.key_file  = node["key_file"].as<std::string>(server_conf.key_file);

            // 处理可能缺失的字段（空字符串兜底）
            server_conf.accept_worker  = node["accept_worker"].as<std::string>("");
            server_conf.io_worker      = node["io_worker"].as<std::string>("");
            server_conf.process_worker = node["process_worker"].as<std::string>("");

            // 解析 args 参数（兼容 YAML map 和字符串两种格式）
            server_conf.args.clear();
            if (node["args"].IsDefined())
            {
                if (node["args"].IsMap())
                {
                    for (YAML::const_iterator it = node["args"].begin(); it != node["args"].end(); ++it)
                    {
                        auto       key        = it->first.as<std::string>();
                        const auto string     = it->second.as<std::string>("");
                        server_conf.args[key] = string;
                    }
                } else
                {
                    // 若 args 不是 map，尝试按字符串解析
                    const auto args_str = node["args"].as<std::string>("");
                    server_conf.args    = LexicalCast<std::string, std::unordered_map<std::string, std::string> >()(
                        args_str);
                }
            }

            // 解析地址列表
            if (node["address"].IsDefined())
            {
                for (size_t i = 0; i < node["address"].size(); ++i)
                {
                    server_conf.address.push_back(node["address"][i].as<std::string>());
                }
            }

            return server_conf;
        }
    };

    template<>
    class LexicalCast<TcpServerConf, std::string>
    {
    public:
        std::string operator()(const TcpServerConf &value) const
        {
            YAML::Node node;
            node["id"]             = value.id;
            node["type"]           = value.type;
            node["keepalive"]      = value.keepalive;
            node["timeout"]        = value.timeout;
            node["name"]           = value.name;
            node["ssl"]            = value.ssl;
            node["cert_file"]      = value.cert_file;
            node["key_file"]       = value.key_file;
            node["accept_worker"]  = value.accept_worker;
            node["io_worker"]      = value.io_worker;
            node["process_worker"] = value.process_worker;
            node["args"]           = value.args; // YAML 自动处理 unordered_map 的序列化

            // 序列化地址列表
            node["address"] = YAML::Node(YAML::NodeType::Sequence);
            for (const auto &address: value.address)
            {
                node["address"].push_back(address);
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}

#endif
