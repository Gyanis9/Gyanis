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
         * @brief 从配置管理器加载日志配置
         * @param config_prefix 配置键前缀，默认为 "logging"
         */
        static void loadFromConfig(const std::string &config_prefix = "logging");

    private:
        static std::unique_ptr<LogSink> createSinkFromConfig(const ConfigValue &sink_cfg);

        static void applyLoggerConfig(Logger &logger, const ConfigValue &logger_cfg);
    };
}


#endif //LOGCONFIG_H
