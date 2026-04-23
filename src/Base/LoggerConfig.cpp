#include "LoggerConfig.h"

#include <iostream>
#include <set>

namespace Base
{
    void LoggerConfigLoader::loadFromConfig(const std::string &config_prefix)
    {
        const auto &cfg = ConfigManager::instance();

        const std::string global_level_key = config_prefix + ".global_level";
        const auto global_level = cfg.get<std::string>(global_level_key, "INFO");
        const auto default_level = logLevelFromString(global_level);

        // ConfigManager 使用扁平化键存储，此处从 key 前缀提取 logger 名。
        const std::string logger_prefix = config_prefix + ".loggers.";
        std::set<std::string> logger_names;
        for (const auto &key: cfg.keys())
        {
            if (key.rfind(logger_prefix, 0) != 0)
            {
                continue;
            }

            const auto name_start = logger_prefix.size();
            const auto dot_pos = key.find('.', name_start);
            const auto name = key.substr(name_start, dot_pos == std::string::npos ? std::string::npos : dot_pos - name_start);
            if (!name.empty())
            {
                logger_names.insert(name);
            }
        }

        if (logger_names.empty())
        {
            // 无 loggers 配置，创建默认 root logger
            auto &root = LoggerRegistry::instance().getRootLogger();
            root.clearSinks();
            root.setLevel(default_level);
            root.addSink(std::make_unique<ConsoleSink>(true));
            return;
        }

        for (const auto &name: logger_names)
        {
            auto &logger = LoggerRegistry::instance().getLogger(name);

            ConfigObject logger_cfg_obj;
            const auto base = logger_prefix + name;
            if (const auto level_opt = cfg.getOptional(base + ".level"); level_opt.has_value())
            {
                logger_cfg_obj.emplace("level", *level_opt);
            }
            if (const auto sinks_opt = cfg.getOptional(base + ".sinks"); sinks_opt.has_value())
            {
                logger_cfg_obj.emplace("sinks", *sinks_opt);
            }

            logger.setLevel(default_level);
            applyLoggerConfig(logger, ConfigValue(std::move(logger_cfg_obj)));
        }
    }

    void LoggerConfigLoader::applyLoggerConfig(Logger &logger, const ConfigValue &logger_cfg)
    {
        logger.clearSinks();

        // 设置等级
        if (logger_cfg.contains("level"))
        {
            const auto level_str = logger_cfg["level"].as<std::string>();
            logger.setLevel(logLevelFromString(level_str));
        }

        // 设置 Sinks
        if (logger_cfg.contains("sinks"))
        {
            for (const auto &sinks_arr = logger_cfg["sinks"].as<ConfigArray>(); const auto &sink_cfg: sinks_arr)
            {
                if (auto sink = createSinkFromConfig(sink_cfg))
                {
                    logger.addSink(std::move(sink));
                }
            }
        }
    }

    std::unique_ptr<LogSink> LoggerConfigLoader::createSinkFromConfig(const ConfigValue &sink_cfg)
    {
        if (!sink_cfg.is<ConfigObject>())
        {
            return nullptr;
        }

        const auto type = sink_cfg["type"].as<std::string>();
        std::unique_ptr<LogSink> sink;

        if (type == "console")
        {
            bool color = sink_cfg.get<bool>("color").value_or(true);
            sink = std::make_unique<ConsoleSink>(color);
        } else if (type == "file")
        {
            auto path = sink_cfg["path"].as<std::string>();
            bool truncate = sink_cfg.get<bool>("truncate").value_or(false);
            sink = std::make_unique<FileSink>(path, truncate);
        } else if (type == "rolling_file")
        {
            auto base_filename = sink_cfg["base_filename"].as<std::string>();
            std::string dir = sink_cfg.get<std::string>("directory").value_or("logs");
            const std::string policy_str = sink_cfg.get<std::string>("policy").value_or("size");

            RollingPolicy policy;
            if (policy_str == "size")
            {
                policy = RollingPolicy::Size;
            } else if (policy_str == "daily")
            {
                policy = RollingPolicy::Daily;
            } else if (policy_str == "hourly")
            {
                policy = RollingPolicy::Hourly;
            } else
            {
                policy = RollingPolicy::Size;
            }

            size_t max_size = sink_cfg.get<int64_t>("max_size_mb").value_or(10) * 1024 * 1024;
            size_t max_backup = sink_cfg.get<int64_t>("max_backup").value_or(10);

            sink = std::make_unique<RollingFileSink>(base_filename, dir, policy, max_size, max_backup);
        } else if (type == "async")
        {
            auto wrapped = createSinkFromConfig(sink_cfg["wrapped"]);
            if (!wrapped)
                return nullptr;

            size_t queue_size = sink_cfg.get<int64_t>("queue_size").value_or(1024);
            const std::string overflow = sink_cfg.get<std::string>("overflow_policy").value_or("block");
            AsyncSink::OverflowPolicy policy = (overflow == "drop")
                                                   ? AsyncSink::OverflowPolicy::Drop
                                                   : AsyncSink::OverflowPolicy::Block;

            sink = std::make_unique<AsyncSink>(std::move(wrapped), queue_size, policy);
        } else
        {
            std::cerr << "Unknown sink type: " << type << std::endl;
            return nullptr;
        }

        // 设置 sink 等级过滤（可选）
        if (sink && sink_cfg.contains("level"))
        {
            const auto level_str = sink_cfg["level"].as<std::string>();
            sink->setLevel(logLevelFromString(level_str));
        }

        return sink;
    }
}
