#include <algorithm>
#include <cctype>
#include <stack>
#include <ranges>

#include "Config.h"


namespace Gyanis::base
{
    static auto g_logger = LOG_NAME("system");

    ConfigVarBase::ConfigVarBase(std::string name, std::string description) : m_name(std::move(name)),
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

    void Config::LoadFromYaml(const YAML::Node &root)
    {
        std::list<std::pair<std::string, const YAML::Node> > all_nodes;
        ListAllMember("", root, all_nodes);
        for (auto &[str, node]: all_nodes)
        {
            std::string key = str;
            if (key.empty())
            {
                continue;
            }
            std::ranges::transform(key, key.begin(), [](const unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
            if (const auto value = LookUpBase(key))
            {
                try
                {
                    if (node.IsScalar()) ///< 如果是标量
                    {
                        value->fromString(node.Scalar());
                    } else
                    {
                        std::stringstream ss;
                        ss << node; // 将整个节点序列转为字符串
                        value->fromString(ss.str());
                    }
                } catch (const std::exception &e)
                {
                    LOG_ERROR(g_logger) << "[配置] 从 YAML 加载配置失败"
                                        << " | 键: " << key
                                        << " | 错误: " << e.what();
                }
            }
        }
    }


    static std::unordered_map<std::string, uint64_t> s_file2ModifyTime;

    static std::shared_mutex s_rwmutex;

    struct FileMeta
    {
        std::string                path;
        uint64_t                   mtime;
        std::filesystem::file_type type;
    };

    void Config::LoadFromConfigDir(const std::string &path, const bool force)
    {
        try
        {
            const auto dir_path = canonical(std::filesystem::path(path));
            if (!is_directory(dir_path))
            {
                LOG_ERROR(g_logger)
                    << "[配置] 配置目录无效"
                    << " | 路径: " << dir_path.string();
                return;
            }

            std::vector<FileMeta> file_metas;
            for (const auto &entry: std::filesystem::recursive_directory_iterator(
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
                } catch (std::filesystem::filesystem_error &e)
                {
                    LOG_WARN(g_logger)
                        << "[配置] 跳过不可访问文件"
                        << " | 路径: " << e.path1().string();
                }
            }

            // 顺序处理文件，保证跨平台编译和行为一致性
            std::for_each(file_metas.begin(), file_metas.end(),
                          [&](const FileMeta &meta)
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
                                      << "[配置] 配置文件加载成功"
                                      << " | 路径: " << meta.path;
                              } catch (const YAML::Exception &e)
                              {
                                  LOG_ERROR(g_logger)
                                      << "[配置] YAML 解析失败"
                                      << " | 路径: " << meta.path
                                      << " | 错误: " << e.what();
                              } catch (const std::exception &e)
                              {
                                  LOG_ERROR(g_logger)
                                      << "[配置] 配置文件加载失败"
                                      << " | 路径: " << meta.path
                                      << " | 错误: " << e.what();
                              }
                          }
            );
        } catch (const std::filesystem::filesystem_error &e)
        {
            LOG_ERROR(g_logger)
                << "[配置] 目录访问失败"
                << " | 路径: " << path
                << " | 错误: " << e.what();
        }
    }

    std::shared_ptr<ConfigVarBase> Config::LookUpBase(const std::string &name)
    {
        std::shared_lock lock(GetMutex());
        const auto       it = GetDatas().find(name);
        return it != GetDatas().end() ? it->second : nullptr;
    }

    void Config::Visit(const std::function<void(std::shared_ptr<ConfigVarBase>)> &visitor)
    {
        std::shared_lock lock(GetMutex());
        for (const auto &datas = GetDatas(); const auto &sConVarBase: datas | std::views::values)
        {
            visitor(sConVarBase);
        }
    }

    std::unordered_map<std::string, std::shared_ptr<ConfigVarBase> > &Config::GetDatas()
    {
        static std::unordered_map<std::string, std::shared_ptr<ConfigVarBase> > s_datas;
        return s_datas;
    }

    std::shared_mutex &Config::GetMutex()
    {
        static std::shared_mutex s_mutex;
        return s_mutex;
    }

    void ListAllMember(const std::string &                                   prefix,
                       const YAML::Node &                                    node,
                       std::list<std::pair<std::string, const YAML::Node> > &output)
    {
        struct StackItem
        {
            std::string prefix;
            YAML::Node  node;

            StackItem(std::string p, const YAML::Node &n)
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
            if (current.prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                != std::string::npos)
            {
                LOG_ERROR(g_logger)
                    << "[配置] 发现非法配置名"
                    << " | 名称: " << current.prefix;
                continue;
            }

            output.emplace_back(current.prefix, current.node);
            if (current.node.IsMap())
            {
                /// 临时存储子节点以便逆序压栈
                std::vector<std::pair<YAML::Node, YAML::Node> > children;

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
            Config::LookUp<std::set<LogDefine> >("logs", std::set<LogDefine>(), "日志配置");

    struct LogIniter
    {
        LogIniter()
        {
            global_log_defines->addListener([](const std::set<LogDefine> &old_value,
                                               const std::set<LogDefine> &new_value)-> void
            {
                LOG_INFO(g_logger) << "[配置] 日志配置已更新。";
                for (auto &i: new_value)
                {
                    auto                    it     = old_value.find(i);
                    std::shared_ptr<Logger> logger = nullptr;
                    if (it == old_value.end())
                    {
                        logger = LOG_NAME(i.name);
                    } else
                    {
                        if (i != *it)
                        {
                            logger = LOG_NAME(i.name);
                        } else
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
                    for (const auto &[type, level, formatter, file]: i.appenders)
                    {
                        std::shared_ptr<LogAppender> ap = nullptr;
                        if (type == 1)
                        {
                            ap = std::make_shared<FileLogAppender>(file);
                        } else if (type == 2)
                        {
                            ap = std::make_shared<StdoutLogAppender>();
                        } else
                        {
                            continue;
                        }
                        ap->setLevel(level);
                        if (!formatter.empty())
                        {
                            if (auto fmt = std::make_shared<LogFormatter>(formatter); !fmt->isError())
                            {
                                ap->setFormatter(fmt);
                            } else
                            {
                                LOG_WARN(g_logger) << "[配置] Appender 格式非法"
                                                   << " | 日志名: " << i.name
                                                   << " | 类型: " << type
                                                   << " | 格式串: " << formatter;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }

                for (auto &i: old_value)
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
