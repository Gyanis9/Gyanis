#include <stack>
#include <algorithm>
#include <sys/stat.h>
#include <execution>

#include "base/Config.h"

namespace Gyanis::base
{
    static auto g_logger = LOG_NAME("system");

    ConfigVarBase::ConfigVarBase(std::string name, std::string description): m_name(std::move(name)),
                                                                             m_description(std::move(description))
    {
    }

    std::string ConfigVarBase::getName() const
    {
        return m_name;
    }

    std::string ConfigVarBase::getDescription() const
    {
        return m_description;
    }

    void Config::LoadFromYaml(const YAML::Node& root)
    {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);
        for (auto& [str, node] : all_nodes)
        {
            std::string key = str;
            if (key.empty())
            {
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            if (const auto value = LookUpBase(key))
            {
                try
                {
                    if (node.IsScalar()) ///< 如果是标量
                    {
                        value->fromString(node.Scalar());
                    }
                    else
                    {
                        std::stringstream ss;
                        ss << node; // 将整个节点序列转为字符串
                        value->fromString(ss.str());
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR(g_logger) << "Config::LoadFromYaml - Failed to load value for key: "
                                           << key << " due to error: " << e.what();
                }
            }
        }
    }


    static std::unordered_map<std::string, uint64_t> s_file2ModifyTime;

    static std::shared_mutex s_rwmutex;

    struct FileMeta
    {
        std::string path;
        uint64_t mtime;
        std::filesystem::file_type type;
    };

    void Config::LoadFromConfigDir(const std::string& path, const bool force)
    {
        try
        {
            const auto dir_path = canonical(std::filesystem::path(path));
            if (!is_directory(dir_path))
            {
                LOG_ERROR(g_logger)
                    << "Config::LoadFromConfigDir - Invalid configuration directory provided. "
                    << "Directory path: " << dir_path.string();
                return;
            }

            std::vector<FileMeta> file_metas;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                     dir_path,
                     std::filesystem::directory_options::skip_permission_denied
                 ))
            {
                try
                {
                    if (entry.is_regular_file() &&
                        entry.path().extension() == ".yml")
                    {
                        const auto ftime = entry.last_write_time().time_since_epoch().count();
                        file_metas.push_back({
                            entry.path().string(),
                            static_cast<uint64_t>(ftime),
                            entry.status().type()
                        });
                    }
                }
                catch (std::filesystem::filesystem_error& e)
                {
                    LOG_WARN(g_logger)
                        << "Config::LoadFromConfigDir - Skipping inaccessible file. "
                        << "File path: " << e.path1().string();
                }
            }

            // 并行处理文件
            std::for_each(std::execution::par, file_metas.begin(), file_metas.end(),
                          [&](const FileMeta& meta)
                          {
                              {
                                  std::shared_lock read_lock(s_rwmutex);
                                  if (!force && s_file2ModifyTime.count(meta.path) &&
                                      s_file2ModifyTime.at(meta.path) == meta.mtime)
                                  {
                                      return;
                                  }
                              }
                              try
                              {
                                  const YAML::Node root = YAML::LoadFile(meta.path);
                                  std::unique_lock write_lock(s_rwmutex);
                                  LoadFromYaml(root);
                                  s_file2ModifyTime[meta.path] = meta.mtime;
                                  LOG_INFO(g_logger)
                                      << "Config::LoadFromConfigDir - Successfully loaded configuration file. "
                                      << "File path: " << meta.path;
                              }
                              catch (const YAML::Exception& e)
                              {
                                  LOG_ERROR(g_logger)
                                      << "Config::LoadFromConfigDir - YAML Error encountered while processing configuration file. "
                                      << "File path: " << meta.path
                                      << " | Error details: " << e.what();
                              }
                              catch (const std::exception& e)
                              {
                                  LOG_ERROR(g_logger)
                                      << "Config::LoadFromConfigDir - Failed to load configuration file. "
                                      << "File path: " << meta.path
                                      << " | Error details: " << e.what();
                              }
                          }
            );
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR(g_logger)
                << "Path Error - " << "Error details: " << e.what()
                << " | Path: " << path;
        }
    }

    std::shared_ptr<ConfigVarBase> Config::LookUpBase(const std::string& name)
    {
        std::shared_lock lock(GetMutex());
        const auto it = GetDatas().find(name);
        return it != GetDatas().end() ? it->second : nullptr;
    }

    void Config::Visit(const std::function<void(std::shared_ptr<ConfigVarBase>)>& visitor)
    {
        std::shared_lock lock(GetMutex());
        const auto& datas = GetDatas();
        for (const auto& [str, sConVarBase] : datas)
        {
            visitor(sConVarBase);
        }
    }

    std::unordered_map<std::string, std::shared_ptr<ConfigVarBase>>& Config::GetDatas()
    {
        static std::unordered_map<std::string, std::shared_ptr<ConfigVarBase>> s_datas;
        return s_datas;
    }

    std::shared_mutex& Config::GetMutex()
    {
        static std::shared_mutex s_mutex;
        return s_mutex;
    }

    void ListAllMember(const std::string& prefix, const YAML::Node& node,
                       std::list<std::pair<std::string, const YAML::Node>>& output)
    {
        struct StackItem
        {
            std::string prefix;
            YAML::Node node;

            StackItem(std::string p, const YAML::Node& n)
                : prefix(std::move(p)), node(n)
            {
            }
        };
        std::stack<StackItem> stack;
        stack.emplace(prefix, node);

        while (!stack.empty())
        {
            auto current = std::move(stack.top());
            stack.pop();
            if (current.prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")
                != std::string::npos)
            {
                LOG_ERROR(g_logger)
                    << "ListAllMember - Invalid configuration name encountered. "
                    << "Invalid name: " << prefix;
                continue;
            }

            output.emplace_back(current.prefix, current.node);
            if (current.node.IsMap())
            {
                /// 临时存储子节点以便逆序压栈
                std::vector<std::pair<YAML::Node, YAML::Node>> children;

                /// 收集所有子节点
                for (auto it = current.node.begin(); it != current.node.end(); ++it)
                {
                    children.emplace_back(it->first, it->second);
                }

                /// 逆序压栈保持处理顺序一致
                for (auto rit = children.rbegin(); rit != children.rend(); ++rit)
                {
                    std::string new_prefix = current.prefix.empty()
                                                 ? rit->first.Scalar()
                                                 : current.prefix + "." + rit->first.Scalar();

                    stack.emplace(std::move(new_prefix), rit->second);
                }
            }
        }
    }

    inline auto global_log_defines =
        Config::LookUp<std::set<LogDefine>>("logs", std::set<LogDefine>(), "logs Configuration");

    struct LogIniter
    {
        LogIniter()
        {
            global_log_defines->addListener([](const std::set<LogDefine>& old_value,
                                               const std::set<LogDefine>& new_value)-> void
            {
                LOG_INFO(g_logger) << "LogIniter - The log configuration has been updated.";
                for (auto& i : new_value)
                {
                    auto it = old_value.find(i);
                    std::shared_ptr<Logger> logger = nullptr;
                    if (it == old_value.end())
                    {
                        logger = LOG_NAME(i.name);
                    }
                    else
                    {
                        if (!(i == *it))
                        {
                            logger = LOG_NAME(i.name);
                        }
                        else
                        {
                            continue;
                        }
                    }
                    logger->setLevel(i.level);
                    if (!i.formatter.empty())
                    {
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for (const auto& appender_define : i.appenders)
                    {
                        std::shared_ptr<LogAppender> ap = nullptr;
                        if (appender_define.type == 1)
                        {
                            ap = std::make_shared<FileLogAppender>(appender_define.file);
                        }
                        else if (appender_define.type == 2)
                        {
                            auto stdout_ap = std::make_shared<StdoutLogAppender>();
                            stdout_ap->setColorEnabled(appender_define.color);
                            ap = stdout_ap;
                        }
                        else
                        {
                            continue;
                        }
                        ap->setLevel(appender_define.level);
                        if (!appender_define.formatter.empty())
                        {
                            if (auto fmt = std::make_shared<LogFormatter>(appender_define.formatter); !fmt->isError())
                            {
                                ap->setFormatter(fmt);
                            }
                            else
                            {
                                LOG_WARN(g_logger) << "LogIniter - Invalid appender configuration. "
                                    << "Log name: " << i.name
                                    << " | Appender type: " << appender_define.type
                                    << " | Formatter: " << appender_define.formatter
                                    << " is invalid." << std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }

                for (auto& i : old_value)
                {
                    if (auto it = new_value.find(i); it == new_value.end())
                    {
                        const auto logger = LOG_NAME(i.name);
                        logger->setLevel(static_cast<LogLevel::Level>(0));
                        logger->clearAppenders();
                    }
                }
            });
        }
    };

    static LogIniter log_init;
}
