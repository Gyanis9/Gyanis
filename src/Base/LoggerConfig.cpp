#include "LoggerConfig.h"

#include <iostream>


namespace Base
{
    void LoggerConfigLoader::loadFromConfig(const std::string &config_prefix)
    {
        const auto &cfg = ConfigManager::instance();

        // 全局日志等级（可选）
        const std::string global_level_key = config_prefix + ".global_level";
        if (cfg.has(global_level_key))
        {
            auto level_str = cfg.get<std::string>(global_level_key, "INFO");
            // 可设置全局默认等级，但各 logger 可覆盖
        }

        // 读取 loggers 配置节
        const std::string loggers_key = config_prefix + ".loggers";
        auto loggers_opt = cfg.getOptional(loggers_key);
        if (!loggers_opt || !loggers_opt->is<ConfigObject>())
        {
            // 无 loggers 配置，创建默认 root logger
            auto &root = LoggerRegistry::instance().getRootLogger();
            root.addSink(std::make_unique<ConsoleSink>(true));
            return;
        }

        for (const auto &loggers_obj = loggers_opt->as<ConfigObject>(); const auto &[name, logger_cfg]: loggers_obj)
        {
            auto &logger = LoggerRegistry::instance().getLogger(name);
            applyLoggerConfig(logger, logger_cfg);
        }
    }

    void LoggerConfigLoader::applyLoggerConfig(Logger &logger, const ConfigValue &logger_cfg)
    {
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
