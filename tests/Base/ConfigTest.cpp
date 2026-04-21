#include "../../src/Base/ConfigManager.h"

#include <cstdint>
#include <iostream>

#ifndef CONFIG_TEST_CONFIG_DIR
#define CONFIG_TEST_CONFIG_DIR "./config"
#endif

int main()
{
    try
    {
        auto &cfg = Base::ConfigManager::instance();

        // 加载配置目录中的所有 YAML 文件
        const auto result = cfg.loadFromDirectory(CONFIG_TEST_CONFIG_DIR);
        if (!result)
        {
            for (const auto &err: result.errors)
            {
                std::cerr << "Config error: " << err << std::endl;
            }
            return 1;
        }

        std::cout << "Loaded " << result.loaded_files.size() << " config files" << std::endl;

        // 读取配置
        std::string server_host = cfg.get<std::string>("server.host", "127.0.0.1");
        int64_t server_port = cfg.get<int64_t>("server.port", 8080);
        bool debug_enabled = cfg.get<bool>("debug.enabled", false);

        if (server_host != "0.0.0.0" || server_port != 8080 || debug_enabled)
        {
            std::cerr << "Unexpected server config values" << std::endl;
            return 1;
        }

        // 检查配置是否存在
        if (cfg.has("database.connection_string"))
        {
            auto db_url = cfg.getRequired<std::string>("database.connection_string");
            std::cout << "Database URL: " << db_url << std::endl;
        }

        // 获取嵌套配置
        auto redis_host = cfg.get<std::string>("redis.default.host", "localhost");
        auto redis_port = cfg.get<int64_t>("redis.default.port", 6379);

        if (redis_host != "localhost" || redis_port != 6379)
        {
            std::cerr << "Unexpected redis config values" << std::endl;
            return 1;
        }

        // 验证必需配置
        const auto missing = cfg.validateRequired({"server.port", "database.host"});
        if (!missing.empty())
        {
            std::cerr << "Missing required config keys:" << std::endl;
            for (const auto &key: missing)
            {
                std::cerr << "  - " << key << std::endl;
            }
            return 1;
        }
    } catch (const Base::ConfigException &e)
    {
        std::cerr << "Configuration exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
