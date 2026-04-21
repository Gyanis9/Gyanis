/**
 * @file ConfigManager.h
 * @brief 配置管理器核心接口
 * @copyright Copyright (c) 2026
 */
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "ConfigFileWatcher.h"
#include "ConfigValue.h"

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Base
{
    // 前向声明
    class ConfigManager;

    /**
     * @brief 配置加载结果
     */
    struct ConfigLoadResult
    {
        bool success{false};
        std::vector<std::string> loaded_files;
        std::vector<std::string> failed_files;
        std::vector<std::string> errors;
        std::chrono::steady_clock::time_point timestamp;

        explicit operator bool() const noexcept
        {
            return success;
        }
    };

    /**
     * @brief 热加载事件回调类型
     */
    using HotReloadCallback = std::function<void(const ConfigLoadResult &result)>;

    /**
     * @brief 配置管理器类
     *
     * 企业级配置管理器，提供以下核心功能：
     *   - 从指定目录递归加载所有 .yml/.yaml 文件
     *   - 扁平化存储配置键值对（支持点号访问：server.port）
     *   - 线程安全的读写访问（std::shared_mutex）
     *   - 热加载支持（监听配置文件变更，自动重载）
     *   - 类型安全的配置值访问
     *
     * 使用单例模式（Meyers' Singleton）确保全局唯一实例。
     * 配置数据使用 std::atomic<std::shared_ptr<ConfigData>> 实现无锁热替换。
     *
     * 使用示例：
     * @code
     *   // 初始化
     *   auto& cfg = ConfigManager::instance();
     *   auto result = cfg.loadFromDirectory("./config");
     *   if (!result) {
     *       for (const auto& err : result.errors) {
     *           std::cerr << err << std::endl;
     *       }
     *       return -1;
     *   }
     *
     *   // 启用热加载
     *   cfg.enableHotReload([](const ConfigLoadResult& r) {
     *       std::cout << "Config reloaded, " << r.loaded_files.size() << " files" << std::endl;
     *   });
     *
     *   // 读取配置
     *   auto port = cfg.get<int64_t>("server.port", 8080);
     *   auto host = cfg.get<std::string>("server.host", "0.0.0.0");
     *   auto debug = cfg.get<bool>("debug.enabled", false);
     *
     *   // 检查键是否存在
     *   if (cfg.has("database.url")) {
     *       auto url = cfg.get<std::string>("database.url");
     *   }
     * @endcode
     */
    class ConfigManager
    {
    public:
        /**
         * @brief 获取单例实例
         */
        static ConfigManager &instance() noexcept;

        // 禁止拷贝和移动
        ConfigManager(const ConfigManager &) = delete;

        ConfigManager &operator=(const ConfigManager &) = delete;

        ConfigManager(ConfigManager &&) = delete;

        ConfigManager &operator=(ConfigManager &&) = delete;

        // ========================================================================
        // 初始化和加载
        // ========================================================================

        /**
         * @brief 从指定目录加载所有 YAML/YML 配置文件
         * @param config_dir 配置文件目录路径
         * @param recursive 是否递归扫描子目录
         * @return 加载结果，包含成功/失败信息
         */
        ConfigLoadResult loadFromDirectory(const std::filesystem::path &config_dir, bool recursive = true);

        /**
         * @brief 加载指定的配置文件列表
         * @param file_paths 配置文件路径列表
         * @return 加载结果，包含成功/失败信息
         */
        ConfigLoadResult loadFiles(const std::vector<std::filesystem::path> &file_paths);

        /**
         * @brief 重新加载配置（使用之前设置的配置目录）
         * @return 加载结果
         */
        ConfigLoadResult reload();

        // ========================================================================
        // 热加载
        // ========================================================================

        /**
         * @brief 启用热加载（监听配置文件变更，自动重载）
         * @param callback 热加载完成后的回调函数
         * @param debounce_ms 防抖间隔（毫秒），默认 500ms
         * @return true 成功，false 失败（可能是不支持热加载的平台）
         */
        bool enableHotReload(HotReloadCallback callback = nullptr, std::chrono::milliseconds debounce_ms = std::chrono::milliseconds(500));

        /**
         * @brief 禁用热加载
         */
        void disableHotReload();

        /**
         * @brief 检查热加载是否已启用
         */
        bool isHotReloadEnabled() const noexcept;

        // ========================================================================
        // 配置访问（类型安全）
        // ========================================================================

        /**
         * @brief 获取配置值（强类型，返回 ConfigValue 对象）
         * @param key 配置键，支持点号访问嵌套结构（如 "server.port"）
         * @return ConfigValue 对象
         * @throws ConfigKeyNotFoundException 键不存在时抛出
         */
        ConfigValue get(std::string_view key) const;

        /**
         * @brief 安全获取配置值（返回 std::optional<ConfigValue>）
         */
        std::optional<ConfigValue> getOptional(std::string_view key) const noexcept;

        /**
         * @brief 获取指定类型的配置值
         * @param key 配置键
         * @param default_value 默认值（键不存在或类型不匹配时返回）
         * @return 配置值或默认值
         */
        template<typename T>
        T get(const std::string_view key, T &&default_value) const noexcept
        {
            const auto opt = getOptional(key);
            if (!opt)
            {
                return std::forward<T>(default_value);
            }

            auto typed = opt->get<T>();
            return typed.value_or(std::forward<T>(default_value));
        }

        /**
         * @brief 获取配置值，键不存在时抛出异常
         * @throws ConfigKeyNotFoundException 键不存在
         * @throws ConfigTypeException 类型不匹配
         */
        template<typename T>
        T getRequired(const std::string_view key) const
        {
            return get(key).template as<T>();
        }

        // 便捷方法
        bool getBool(std::string_view key, bool default_value = false) const noexcept;

        int64_t getInt(std::string_view key, int64_t default_value = 0) const noexcept;

        double getDouble(std::string_view key, double default_value = 0.0) const noexcept;

        std::string getString(std::string_view key, const std::string &default_value = "") const;

        // ========================================================================
        // 配置查询
        // ========================================================================

        /**
         * @brief 检查配置键是否存在
         */
        bool has(std::string_view key) const noexcept;

        /**
         * @brief 获取所有配置键
         */
        std::vector<std::string> keys() const;

        /**
         * @brief 获取所有配置项（用于调试）
         */
        std::unordered_map<std::string, ConfigValue> dump() const;

        /**
         * @brief 获取已加载的配置文件列表
         */
        std::vector<std::string> loadedFiles() const;

        /**
         * @brief 获取配置目录
         */
        std::filesystem::path configDirectory() const;

        /**
         * @brief 清空所有配置
         */
        void clear();

        // ========================================================================
        // 验证
        // ========================================================================

        /**
         * @brief 验证必需的配置键是否存在
         * @param required_keys 必需的配置键列表
         * @return 缺失的配置键列表
         */
        std::vector<std::string> validateRequired(const std::vector<std::string> &required_keys) const;

    private:
        ConfigManager() = default;

        ~ConfigManager();

        // 内部数据结构：使用原子 shared_ptr 实现无锁热替换
        struct ConfigData
        {
            std::unordered_map<std::string, ConfigValue> values;
            std::vector<std::string> loaded_files;
            std::filesystem::path config_dir;
            std::chrono::steady_clock::time_point load_time;
        };

        std::atomic<std::shared_ptr<ConfigData> > m_data{std::make_shared<ConfigData>()};

        // 读写锁（仅用于配置数据的构建过程，读取无需加锁）
        mutable std::shared_mutex m_reload_mutex;

        // 热加载相关
        std::unique_ptr<IFileWatcher> m_file_watcher;
        HotReloadCallback m_hot_reload_callback;
        std::atomic<bool> m_hot_reload_enabled{false};

        // 内部方法
        ConfigLoadResult loadFromDirectoryImpl(const std::filesystem::path &config_dir, bool recursive);

        void loadYamlFile(const std::filesystem::path &file_path, std::unordered_map<std::string, ConfigValue> &values, std::vector<std::string> &errors);

        void flattenYamlNode(const YAML::Node &node, const std::string &prefix, std::unordered_map<std::string, ConfigValue> &values);

        ConfigValue convertYamlNode(const YAML::Node &node);

        void handleFileChange(std::string_view file_path, FileChangeEvent event);

        ConfigLoadResult doReload();

        // 递归扫描 YAML 文件
        std::vector<std::filesystem::path> scanYamlFiles(const std::filesystem::path &dir, bool recursive) const;
    };
}


#endif //CONFIGMANAGER_H
