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
     * 配置管理器，提供以下核心功能：
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
         * @brief 获取配置管理器单例。
         * @return ConfigManager& 单例引用。
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
         * @brief 从目录加载 YAML 配置文件。
         * @param config_dir 配置目录。
         * @param recursive 是否递归扫描子目录。
         * @return ConfigLoadResult 加载结果。
         */
        ConfigLoadResult loadFromDirectory(const std::filesystem::path &config_dir, bool recursive = true);

        /**
         * @brief 加载指定文件列表中的配置。
         * @param file_paths 配置文件路径列表。
         * @return ConfigLoadResult 加载结果。
         */
        ConfigLoadResult loadFiles(const std::vector<std::filesystem::path> &file_paths);

        /**
         * @brief 使用当前目录配置执行一次重载。
         * @return ConfigLoadResult 重载结果。
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
         * @brief 关闭热重载监听并释放监听资源。
         */
        void disableHotReload();

        /**
         * @brief 查询热重载当前是否启用。
         * @return bool 启用返回 true。
         */
        bool isHotReloadEnabled() const noexcept;

        // ========================================================================
        // 配置访问（类型安全）
        // ========================================================================

        /**
         * @brief 按键获取配置值，键不存在时抛出异常。
         * @param key 配置键。
         * @return ConfigValue 配置值副本。
         */
        ConfigValue get(std::string_view key) const;

        /**
         * @brief 安全获取配置值。
         * @param key 配置键。
         * @return std::optional<ConfigValue> 键存在时返回值，否则为空。
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

        /**
         * @brief 读取布尔配置值，缺失或类型不匹配时返回默认值。
         * @param key 配置键。
         * @param default_value 默认值。
         * @return bool 配置值或默认值。
         */
        bool getBool(std::string_view key, bool default_value = false) const noexcept;

        /**
         * @brief 读取整型配置值，缺失或类型不匹配时返回默认值。
         * @param key 配置键。
         * @param default_value 默认值。
         * @return int64_t 配置值或默认值。
         */
        int64_t getInt(std::string_view key, int64_t default_value = 0) const noexcept;

        /**
         * @brief 读取浮点配置值，缺失或类型不匹配时返回默认值。
         * @param key 配置键。
         * @param default_value 默认值。
         * @return double 配置值或默认值。
         */
        double getDouble(std::string_view key, double default_value = 0.0) const noexcept;

        /**
         * @brief 读取字符串配置值，缺失或类型不匹配时返回默认值。
         * @param key 配置键。
         * @param default_value 默认值。
         * @return std::string 配置值或默认值。
         */
        std::string getString(std::string_view key, const std::string &default_value = "") const;

        // ========================================================================
        // 配置查询
        // ========================================================================

        /**
         * @brief 检查配置键是否存在。
         * @param key 配置键。
         * @return bool 存在返回 true。
         */
        bool has(std::string_view key) const noexcept;

        /**
         * @brief 返回所有配置键并按字典序排序。
         * @return std::vector<std::string> 配置键列表。
         */
        std::vector<std::string> keys() const;

        /**
         * @brief 导出当前配置快照。
         * @return std::unordered_map<std::string, ConfigValue> 配置字典副本。
         */
        std::unordered_map<std::string, ConfigValue> dump() const;

        /**
         * @brief 获取当前已加载文件列表。
         * @return std::vector<std::string> 文件路径列表。
         */
        std::vector<std::string> loadedFiles() const;

        /**
         * @brief 获取当前配置目录。
         * @return std::filesystem::path 配置目录路径。
         */
        std::filesystem::path configDirectory() const;

        /**
         * @brief 清空所有配置数据。
         */
        void clear();

        // ========================================================================
        // 验证
        // ========================================================================

        /**
         * @brief 校验必需配置键是否都已存在。
         * @param required_keys 必需键列表。
         * @return std::vector<std::string> 缺失键列表。
         */
        std::vector<std::string> validateRequired(const std::vector<std::string> &required_keys) const;

    private:
        ConfigManager() = default;

        /**
         * @brief 析构时自动关闭热重载监听。
         */
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

        /**
         * @brief 目录加载的内部实现，负责扫描、解析并原子替换配置快照。
         * @param config_dir 配置目录。
         * @param recursive 是否递归扫描。
         * @return ConfigLoadResult 加载结果。
         */
        ConfigLoadResult loadFromDirectoryImpl(const std::filesystem::path &config_dir, bool recursive);

        /**
         * @brief 读取并扁平化单个 YAML 文件到配置字典。
         * @param file_path YAML 文件路径。
         * @param values 目标配置字典。
         * @param errors 错误信息收集容器。
         */
        void loadYamlFile(const std::filesystem::path &file_path, std::unordered_map<std::string, ConfigValue> &values, std::vector<std::string> &errors);

        /**
         * @brief 递归扁平化 YAML 节点，将嵌套键转换为点号路径。
         * @param node 当前 YAML 节点。
         * @param prefix 键前缀。
         * @param values 扁平化结果容器。
         */
        void flattenYamlNode(const YAML::Node &node, const std::string &prefix, std::unordered_map<std::string, ConfigValue> &values);

        /**
         * @brief 将 YAML 节点转换为 ConfigValue。
         * @param node YAML 节点。
         * @return ConfigValue 转换后的配置值。
         */
        ConfigValue convertYamlNode(const YAML::Node &node);

        /**
         * @brief 处理文件监听回调并触发后台重载。
         * @param file_path 发生变化的文件路径。
         * @param event 文件变更事件。
         */
        void handleFileChange(std::string_view file_path, FileChangeEvent event);

        /**
         * @brief 在互斥保护下执行一次目录重载。
         * @return ConfigLoadResult 重载结果。
         */
        ConfigLoadResult doReload();

        /**
         * @brief 扫描目录中的 YAML 文件并按路径排序。
         * @param dir 待扫描目录。
         * @param recursive 是否递归。
         * @return std::vector<std::filesystem::path> YAML 文件路径列表。
         */
        std::vector<std::filesystem::path> scanYamlFiles(const std::filesystem::path &dir, bool recursive) const;
    };
}


#endif //CONFIGMANAGER_H
