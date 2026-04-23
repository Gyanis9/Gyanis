/**
 * @file LoggerConfig.h
 * @brief 从配置系统加载日志配置
 * @copyright Copyright (c) 2026
 */

#ifndef LOGCONFIG_H
#define LOGCONFIG_H

#include "ConfigManager.h"
#include "Logger.h"

namespace Base
{
    /**
     * @brief 日志配置加载器
     *
     * 从 ConfigManager 读取配置并初始化日志系统。
     * 支持配置多个 logger、sink 等。
     *
     * 配置示例 (YAML)：
     * @code
     * logging:
     *   global_level: INFO
     *   loggers:
     *     root:
     *       level: DEBUG
     *       sinks:
     *         - type: console
     *           color: true
     *         - type: file
     *           path: logs/app.log
     *     network:
     *       level: TRACE
     *       sinks:
     *         - type: rolling_file
     *           base_filename: network.log
     *           directory: logs
     *           policy: daily
     *           max_backup: 7
     *     database:
     *       level: INFO
     *       sinks:
     *         - type: async
     *           queue_size: 2048
     *           wrapped:
     *             type: file
     *             path: logs/db.log
     * @endcode
     */
    class LoggerConfigLoader
    {
    public:
        /**
         * @brief 从配置系统读取日志器配置并应用到注册表。
         * @param config_prefix 日志配置根键前缀。
         */
        static void loadFromConfig(const std::string &config_prefix = "logging");

    private:
        /**
         * @brief 将单个日志器配置应用到指定日志器实例。
         * @param logger 目标日志器。
         * @param logger_cfg 日志器配置对象，包含 level 与 sinks 等键。
         */
        static void applyLoggerConfig(Logger &logger, const ConfigValue &logger_cfg);

        /**
         * @brief 根据配置对象创建具体日志 Sink 实例。
         * @param sink_cfg Sink 配置对象。
         * @return std::unique_ptr<LogSink> 构造成功返回 Sink，失败返回 nullptr。
         */
        static std::unique_ptr<LogSink> createSinkFromConfig(const ConfigValue &sink_cfg);
    };
}


#endif //LOGCONFIG_H
