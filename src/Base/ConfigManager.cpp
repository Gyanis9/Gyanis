#include "ConfigManager.h"

#include <algorithm>
#include <ranges>

namespace Base
{
    ConfigManager &ConfigManager::instance() noexcept
    {
        static ConfigManager instance;
        return instance;
    }

    ConfigManager::~ConfigManager()
    {
        disableHotReload();
    }

    // ============================================================================
    // 配置加载公共接口
    // ============================================================================

    ConfigLoadResult ConfigManager::loadFromDirectory(const std::filesystem::path &config_dir, const bool recursive)
    {
        return loadFromDirectoryImpl(config_dir, recursive);
    }

    ConfigLoadResult ConfigManager::loadFiles(const std::vector<std::filesystem::path> &file_paths)
    {
        ConfigLoadResult result;
        result.timestamp = std::chrono::steady_clock::now();

        if (file_paths.empty())
        {
            result.success = true;
            return result;
        }

        std::unordered_map<std::string, ConfigValue> values;

        for (const auto &file_path: file_paths)
        {
            if (!std::filesystem::exists(file_path))
            {
                result.failed_files.push_back(file_path.string());
                result.errors.push_back("File does not exist: " + file_path.string());
                continue;
            }

            if (!isYamlFile(file_path.string()))
            {
                result.failed_files.push_back(file_path.string());
                result.errors.push_back("Not a YAML file: " + file_path.string());
                continue;
            }

            loadYamlFile(file_path, values, result.errors);

            if (result.errors.empty() || result.errors.back().find(file_path.string()) == std::string::npos)
            {
                result.loaded_files.push_back(file_path.string());
            } else
            {
                result.failed_files.push_back(file_path.string());
            }
        }

        // 原子替换配置数据
        const auto new_data = std::make_shared<ConfigData>();
        new_data->values = std::move(values);
        new_data->loaded_files = result.loaded_files;
        new_data->load_time = result.timestamp;

        m_data.store(new_data, std::memory_order_release);

        result.success = result.failed_files.empty();
        return result;
    }

    ConfigLoadResult ConfigManager::reload()
    {
        if (const auto current_data = m_data.load(std::memory_order_acquire); current_data->config_dir.empty())
        {
            ConfigLoadResult result;
            result.success = false;
            result.errors.emplace_back("No configuration directory set. Call loadFromDirectory first.");
            return result;
        }
        return doReload();
    }

    // ============================================================================
    // 内部加载实现
    // ============================================================================

    ConfigLoadResult ConfigManager::loadFromDirectoryImpl(const std::filesystem::path &config_dir, const bool recursive)
    {
        ConfigLoadResult result;
        result.timestamp = std::chrono::steady_clock::now();

        // 检查目录是否存在
        std::error_code ec;
        if (!std::filesystem::exists(config_dir, ec))
        {
            result.success = false;
            result.errors.push_back("Configuration directory does not exist: " + config_dir.string());
            if (ec)
            {
                result.errors.push_back("Error: " + ec.message());
            }
            return result;
        }

        if (!std::filesystem::is_directory(config_dir, ec))
        {
            result.success = false;
            result.errors.push_back("Path is not a directory: " + config_dir.string());
            return result;
        }

        // 扫描所有 YAML 文件
        const auto yaml_files = scanYamlFiles(config_dir, recursive);

        if (yaml_files.empty())
        {
            const auto new_data = std::make_shared<ConfigData>();
            new_data->config_dir = config_dir;
            new_data->load_time = result.timestamp;
            m_data.store(new_data, std::memory_order_release);

            result.success = true;
            return result;
        }

        std::unordered_map<std::string, ConfigValue> values;

        for (const auto &file_path: yaml_files)
        {
            loadYamlFile(file_path, values, result.errors);

            if (result.errors.empty() || result.errors.back().find(file_path.string()) == std::string::npos)
            {
                result.loaded_files.push_back(file_path.string());
            } else
            {
                result.failed_files.push_back(file_path.string());
            }
        }

        // 原子替换配置数据
        const auto new_data = std::make_shared<ConfigData>();
        new_data->values = std::move(values);
        new_data->loaded_files = result.loaded_files;
        new_data->config_dir = config_dir;
        new_data->load_time = result.timestamp;

        m_data.store(new_data, std::memory_order_release);

        result.success = result.failed_files.empty();
        return result;
    }

    void ConfigManager::loadYamlFile(const std::filesystem::path &file_path, std::unordered_map<std::string, ConfigValue> &values, std::vector<std::string> &errors)
    {
        try
        {
            const YAML::Node root = YAML::LoadFile(file_path.string());

            if (root.IsNull())
            {
                // 空文件，不报错
                return;
            }

            if (!root.IsMap())
            {
                errors.push_back("File '" + file_path.string() +
                                 "': Root node must be a map, got " +
                                 std::string(root.Type() == YAML::NodeType::Sequence ? "sequence" : root.Type() == YAML::NodeType::Scalar ? "scalar" : "null"));
                return;
            }

            flattenYamlNode(root, "", values);
        } catch (const YAML::ParserException &e)
        {
            errors.push_back("YAML parse error in '" + file_path.string() +
                             "': " + e.what() + " at line " + std::to_string(e.mark.line + 1) +
                             ", column " + std::to_string(e.mark.column + 1));
        } catch (const YAML::BadFile &e)
        {
            errors.push_back("Cannot open file '" + file_path.string() + "': " + e.what());
        } catch (const std::exception &e)
        {
            errors.push_back("Unexpected error loading '" + file_path.string() + "': " + e.what());
        }
    }

    void ConfigManager::flattenYamlNode(const YAML::Node &node, const std::string &prefix, std::unordered_map<std::string, ConfigValue> &values)
    {
        if (!node.IsMap())
        {
            return;
        }

        for (const auto &kv: node)
        {
            auto key = kv.first.as<std::string>();
            std::string full_key = prefix.empty() ? key : prefix + "." + key;

            if (const YAML::Node &value_node = kv.second; value_node.IsMap())
            {
                // 嵌套对象：递归展开
                flattenYamlNode(value_node, full_key, values);
            } else
            {
                // 叶子节点：转换为 ConfigValue 并存储
                values[full_key] = convertYamlNode(value_node);
            }
        }
    }

    ConfigValue ConfigManager::convertYamlNode(const YAML::Node &node)
    {
        if (node.IsNull())
        {
            return ConfigValue(nullptr);
        } else if (node.IsScalar())
        {
            // 尝试解析为布尔值
            try
            {
                return ConfigValue(node.as<bool>());
            } catch (...)
            {
            }

            // 尝试解析为整数
            try
            {
                return ConfigValue(node.as<int64_t>());
            } catch (...)
            {
            }

            // 尝试解析为浮点数
            try
            {
                return ConfigValue(node.as<double>());
            } catch (...)
            {
            }

            // 默认作为字符串
            return ConfigValue(node.as<std::string>());
        } else if (node.IsSequence())
        {
            ConfigArray arr;
            for (const auto &item: node)
            {
                arr.push_back(convertYamlNode(item));
            }
            return ConfigValue(std::move(arr));
        } else if (node.IsMap())
        {
            ConfigObject obj;
            for (const auto &kv: node)
            {
                auto key = kv.first.as<std::string>();
                obj[key] = convertYamlNode(kv.second);
            }
            return ConfigValue(std::move(obj));
        }

        return ConfigValue(nullptr);
    }

    std::vector<std::filesystem::path> ConfigManager::scanYamlFiles(const std::filesystem::path &dir, const bool recursive) const
    {
        std::vector<std::filesystem::path> yaml_files;

        std::error_code ec;
        if (recursive)
        {
            for (const auto &entry: std::filesystem::recursive_directory_iterator(dir, ec))
            {
                if (ec)
                {
                    break;
                }
                if (entry.is_regular_file() && isYamlFile(entry.path().string()))
                {
                    yaml_files.push_back(entry.path());
                }
            }
        } else
        {
            for (const auto &entry: std::filesystem::directory_iterator(dir, ec))
            {
                if (ec)
                {
                    break;
                }
                if (entry.is_regular_file() && isYamlFile(entry.path().string()))
                {
                    yaml_files.push_back(entry.path());
                }
            }
        }

        // 按文件名排序，保证加载顺序一致
        std::ranges::sort(yaml_files);

        return yaml_files;
    }

    // ============================================================================
    // 配置访问
    // ============================================================================

    ConfigValue ConfigManager::get(const std::string_view key) const
    {
        const auto current_data = m_data.load(std::memory_order_acquire);
        const auto it = current_data->values.find(key.data());
        if (it == current_data->values.end())
        {
            throw ConfigKeyNotFoundException(std::string(key));
        }
        return it->second;
    }

    std::optional<ConfigValue> ConfigManager::getOptional(const std::string_view key) const noexcept
    {
        const auto current_data = m_data.load(std::memory_order_acquire);
        const auto it = current_data->values.find(key.data());
        if (it == current_data->values.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    bool ConfigManager::has(const std::string_view key) const noexcept
    {
        const auto current_data = m_data.load(std::memory_order_acquire);
        return current_data->values.contains(key.data());
    }

    std::vector<std::string> ConfigManager::keys() const
    {
        const auto current_data = m_data.load(std::memory_order_acquire);
        std::vector<std::string> result;
        result.reserve(current_data->values.size());
        for (const auto &key: current_data->values | std::views::keys)
        {
            result.push_back(key);
        }
        std::ranges::sort(result);
        return result;
    }

    std::unordered_map<std::string, ConfigValue> ConfigManager::dump() const
    {
        const auto current_data = m_data.load(std::memory_order_acquire);
        return current_data->values;
    }

    std::vector<std::string> ConfigManager::loadedFiles() const
    {
        const auto current_data = m_data.load(std::memory_order_acquire);
        return current_data->loaded_files;
    }

    std::filesystem::path ConfigManager::configDirectory() const
    {
        const auto current_data = m_data.load(std::memory_order_acquire);
        return current_data->config_dir;
    }

    void ConfigManager::clear()
    {
        const auto new_data = std::make_shared<ConfigData>();
        m_data.store(new_data, std::memory_order_release);
    }

    // ============================================================================
    // 便捷访问方法
    // ============================================================================

    bool ConfigManager::getBool(const std::string_view key, bool default_value) const noexcept
    {
        return get<bool>(key, std::move(default_value));
    }

    int64_t ConfigManager::getInt(const std::string_view key, int64_t default_value) const noexcept
    {
        return get<int64_t>(key, std::move(default_value));
    }

    double ConfigManager::getDouble(const std::string_view key, double default_value) const noexcept
    {
        return get<double>(key, std::move(default_value));
    }

    std::string ConfigManager::getString(const std::string_view key, const std::string &default_value) const
    {
        return get<std::string>(key, default_value.data());
    }

    // ============================================================================
    // 热加载
    // ============================================================================

    bool ConfigManager::enableHotReload(HotReloadCallback callback, std::chrono::milliseconds debounce_ms)
    {
        if (m_hot_reload_enabled.load(std::memory_order_acquire))
        {
            return true; // 已经启用
        }

        const auto current_data = m_data.load(std::memory_order_acquire);
        if (current_data->config_dir.empty())
        {
            return false;
        }

        try
        {
            m_file_watcher = FileWatcherFactory::create();
            if (!m_file_watcher)
            {
                return false;
            }

            m_hot_reload_callback = std::move(callback);

#ifdef __linux__
            auto *watcher = dynamic_cast<InotifyFileWatcher *>(m_file_watcher.get());
            if (watcher)
            {
                watcher->setDebounceInterval(debounce_ms);
            }
#elif defined(_WIN32)
            if (auto *watcher = dynamic_cast<Win32FileWatcher *>(m_file_watcher.get()))
            {
                watcher->setDebounceInterval(debounce_ms);
            }
#endif

            m_file_watcher->setCallback([this](const std::string_view file_path, const FileChangeEvent event)
            {
                handleFileChange(file_path, event);
            });

            if (!m_file_watcher->addWatch(current_data->config_dir.string(), true))
            {
                return false;
            }

            if (!m_file_watcher->start())
            {
                return false;
            }

            m_hot_reload_enabled.store(true, std::memory_order_release);
            return true;
        } catch (...)
        {
            return false;
        }
    }

    void ConfigManager::disableHotReload()
    {
        if (!m_hot_reload_enabled.load(std::memory_order_acquire))
        {
            return;
        }

        m_hot_reload_enabled.store(false, std::memory_order_release);

        if (m_file_watcher)
        {
            m_file_watcher->stop();
            m_file_watcher.reset();
        }
    }

    bool ConfigManager::isHotReloadEnabled() const noexcept
    {
        return m_hot_reload_enabled.load(std::memory_order_acquire);
    }

    void ConfigManager::handleFileChange(const std::string_view file_path, const FileChangeEvent event)
    {
        // 只处理 YAML 文件的修改事件
        if (!isYamlFile(file_path))
        {
            return;
        }

        if (event != FileChangeEvent::Modified && event != FileChangeEvent::Created)
        {
            return;
        }

        // 在独立线程中执行重载，避免阻塞监听线程
        std::thread([this]()
        {
            const auto result = doReload();

            if (m_hot_reload_callback)
            {
                m_hot_reload_callback(result);
            }
        }).detach();
    }

    ConfigLoadResult ConfigManager::doReload()
    {
        std::unique_lock lock(m_reload_mutex);

        const auto current_data = m_data.load(std::memory_order_acquire);
        ConfigLoadResult result = loadFromDirectoryImpl(current_data->config_dir, true);

        // 如果重载成功但文件列表为空，保留原有配置
        if (result.success && result.loaded_files.empty())
        {
            result.success = true;
        }

        return result;
    }

    // ============================================================================
    // 验证
    // ============================================================================

    std::vector<std::string> ConfigManager::validateRequired(
        const std::vector<std::string> &required_keys) const
    {
        std::vector<std::string> missing;
        const auto current_data = m_data.load(std::memory_order_acquire);

        for (const auto &key: required_keys)
        {
            if (!current_data->values.contains(key))
            {
                missing.push_back(key);
            }
        }

        return missing;
    }
}
