#include "../../src/Base/ConfigFileWatcher.h"
#include "../../src/Base/ConfigManager.h"
#include "../../src/Base/ConfigType.h"
#include "../../src/Base/ConfigValue.h"
#include "../../src/Base/Expection.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
    using Catch::Matchers::ContainsSubstring;
    using namespace std::chrono_literals;

    class ScopedConfigReset
    {
    public:
        ScopedConfigReset()
        {
            reset();
        }

        ~ScopedConfigReset()
        {
            reset();
        }

    private:
        static void reset()
        {
            auto &cfg = Base::ConfigManager::instance();
            cfg.disableHotReload();
            cfg.clear();
        }
    };

    class TempDir
    {
    public:
        TempDir()
        {
            path_ = makeUniquePath();
            std::filesystem::create_directories(path_);
        }

        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(path_, ec);
        }

        [[nodiscard]] const std::filesystem::path &path() const noexcept
        {
            return path_;
        }

        std::filesystem::path writeFile(const std::filesystem::path &relative_path, const std::string &content) const
        {
            const auto file_path = path_ / relative_path;
            std::filesystem::create_directories(file_path.parent_path());

            std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
            if (!out.is_open())
            {
                throw std::runtime_error("Failed to open test file: " + file_path.string());
            }

            out << content;
            out.close();
            return file_path;
        }

    private:
        static std::filesystem::path makeUniquePath()
        {
            static std::atomic<unsigned long long> counter{0};
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto id = counter.fetch_add(1, std::memory_order_relaxed);
            return std::filesystem::temp_directory_path() /
                   ("gyanis_config_test_" + std::to_string(stamp) + "_" + std::to_string(id));
        }

        std::filesystem::path path_;
    };

    bool waitUntil(const std::function<bool()> &predicate,
                   const std::chrono::milliseconds timeout,
                   const std::chrono::milliseconds interval = 20ms)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (predicate())
            {
                return true;
            }
            std::this_thread::sleep_for(interval);
        }
        return predicate();
    }
}

TEST_CASE("ConfigType::typeName should map enum values", "[config][type]")
{
    CHECK(std::string(Base::typeName(Base::ConfigValueType::Null)) == "null");
    CHECK(std::string(Base::typeName(Base::ConfigValueType::Bool)) == "bool");
    CHECK(std::string(Base::typeName(Base::ConfigValueType::Int)) == "int");
    CHECK(std::string(Base::typeName(Base::ConfigValueType::Double)) == "double");
    CHECK(std::string(Base::typeName(Base::ConfigValueType::String)) == "string");
    CHECK(std::string(Base::typeName(Base::ConfigValueType::Array)) == "array");
    CHECK(std::string(Base::typeName(Base::ConfigValueType::Object)) == "object");
    CHECK(std::string(Base::typeName(static_cast<Base::ConfigValueType>(99))) == "unknown");
}

TEST_CASE("ConfigType::isYamlFile should detect supported suffixes", "[config][type]")
{
    CHECK(Base::isYamlFile("app.yml"));
    CHECK(Base::isYamlFile("app.yaml"));
    CHECK(Base::isYamlFile("/tmp/config/server.yml"));
    CHECK_FALSE(Base::isYamlFile("app.json"));
    CHECK_FALSE(Base::isYamlFile("app.yml.bak"));
    CHECK_FALSE(Base::isYamlFile("a"));
    CHECK_FALSE(Base::isYamlFile(""));
}

TEST_CASE("ConfigType::splitKey should split and ignore empty segments", "[config][type]")
{
    CHECK(Base::splitKey("").empty());
    CHECK(Base::splitKey("server.port") == std::vector<std::string>{"server", "port"});
    CHECK(Base::splitKey("server..port") == std::vector<std::string>{"server", "port"});
    CHECK(Base::splitKey(".server.port.") == std::vector<std::string>{"server", "port"});
}

TEST_CASE("ConfigValue should represent all supported value categories", "[config][value]")
{
    const Base::ConfigValue null_value;
    const Base::ConfigValue bool_value(true);
    const Base::ConfigValue int_value(42);
    const Base::ConfigValue double_value(3.14);
    const Base::ConfigValue string_value("hello");
    const Base::ConfigValue array_value(Base::ConfigArray{Base::ConfigValue(1), Base::ConfigValue("x")});
    const Base::ConfigValue object_value(Base::ConfigObject{{"name", Base::ConfigValue("svc")}});

    CHECK(null_value.type() == Base::ConfigValueType::Null);
    CHECK(bool_value.type() == Base::ConfigValueType::Bool);
    CHECK(int_value.type() == Base::ConfigValueType::Int);
    CHECK(double_value.type() == Base::ConfigValueType::Double);
    CHECK(string_value.type() == Base::ConfigValueType::String);
    CHECK(array_value.type() == Base::ConfigValueType::Array);
    CHECK(object_value.type() == Base::ConfigValueType::Object);

    CHECK(null_value.empty());
    CHECK_FALSE(bool_value.empty());
    CHECK_FALSE(int_value.empty());
    CHECK_FALSE(double_value.empty());
    CHECK_FALSE(string_value.empty());
    CHECK_FALSE(array_value.empty());
    CHECK_FALSE(object_value.empty());

    const Base::ConfigValue empty_string(std::string{});
    const Base::ConfigValue empty_array(Base::ConfigArray{});
    const Base::ConfigValue empty_object(Base::ConfigObject{});
    CHECK(empty_string.empty());
    CHECK(empty_array.empty());
    CHECK(empty_object.empty());
}

TEST_CASE("ConfigValue access APIs should be type-safe", "[config][value]")
{
    const Base::ConfigValue value(123);

    REQUIRE(value.asInt() == 123);
    REQUIRE(value.getInt().has_value());
    CHECK(value.getInt().value() == 123);
    CHECK_FALSE(value.getString().has_value());
    CHECK(value.intOr(0) == 123);
    CHECK(value.stringOr("fallback") == "fallback");

    try
    {
        static_cast<void>(value.as<std::string>());
        FAIL("Expected ConfigTypeException");
    } catch (const Base::ConfigTypeException &ex)
    {
        CHECK(ex.key() == "<unknown>");
        CHECK(ex.actualType() == "int");
        CHECK_THAT(std::string(ex.what()), ContainsSubstring("Type mismatch"));
    }
}

TEST_CASE("ConfigValue object and array operations should validate bounds", "[config][value]")
{
    const Base::ConfigValue object(Base::ConfigObject{
        {"enabled", Base::ConfigValue(true)},
        {"port", Base::ConfigValue(8080)}
    });
    const Base::ConfigValue array(Base::ConfigArray{
        Base::ConfigValue("a"),
        Base::ConfigValue("b")
    });

    CHECK(object.contains("enabled"));
    CHECK_FALSE(object.contains("missing"));
    CHECK(object["enabled"].asBool());
    CHECK(array[1].asString() == "b");
    CHECK(array.size() == 2);

    try
    {
        static_cast<void>(object["missing"]);
        FAIL("Expected ConfigKeyNotFoundException");
    } catch (const Base::ConfigKeyNotFoundException &ex)
    {
        CHECK(ex.key() == "missing");
    }

    try
    {
        static_cast<void>(array[99]);
        FAIL("Expected ConfigKeyNotFoundException");
    } catch (const Base::ConfigKeyNotFoundException &ex)
    {
        CHECK(ex.key() == "[99]");
    }
}

TEST_CASE("Config exceptions should expose structured metadata", "[config][exception]")
{
    const Base::ConfigFileException file_ex("server.yml", "read failed");
    const Base::ConfigParseException parse_ex("server.yml", "bad syntax");
    const Base::ConfigKeyNotFoundException key_ex("db.url");
    const Base::ConfigTypeException type_ex("server.port", "int", "string");
    const Base::ConfigValidationException validation_ex("server.port", "must be positive");

    CHECK(file_ex.filePath() == "server.yml");
    CHECK(parse_ex.filePath() == "server.yml");
    CHECK(key_ex.key() == "db.url");
    CHECK(type_ex.key() == "server.port");
    CHECK(type_ex.expectedType() == "int");
    CHECK(type_ex.actualType() == "string");
    CHECK(validation_ex.key() == "server.port");

    CHECK_THAT(std::string(file_ex.what()), ContainsSubstring("[ConfigException]"));
    CHECK_THAT(std::string(type_ex.what()), ContainsSubstring("Type mismatch"));
    CHECK(file_ex.location().line() > 0);
}

TEST_CASE("ConfigManager::loadFromDirectory should reject invalid paths", "[config][manager]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    const auto missing = cfg.loadFromDirectory(tmp.path() / "not-exist", true);
    REQUIRE_FALSE(missing.success);
    REQUIRE_FALSE(missing.errors.empty());
    CHECK_THAT(missing.errors.front(), ContainsSubstring("does not exist"));

    const auto plain_file = tmp.writeFile("readme.txt", "not a directory");
    const auto not_dir = cfg.loadFromDirectory(plain_file, true);
    REQUIRE_FALSE(not_dir.success);
    REQUIRE_FALSE(not_dir.errors.empty());
    CHECK_THAT(not_dir.errors.front(), ContainsSubstring("not a directory"));
}

TEST_CASE("ConfigManager should parse YAML and respect recursive option", "[config][manager]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    tmp.writeFile("server.yml",
                  "server:\n"
                  "  host: 127.0.0.1\n"
                  "  port: 8080\n"
                  "metrics:\n"
                  "  enabled: true\n");

    tmp.writeFile("nested/feature.yaml",
                  "nested:\n"
                  "  enabled: true\n"
                  "  weights: [1, 2, 3]\n");

    const auto non_recursive = cfg.loadFromDirectory(tmp.path(), false);
    REQUIRE(non_recursive.success);
    CHECK(cfg.has("server.host"));
    CHECK_FALSE(cfg.has("nested.enabled"));

    const auto recursive = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(recursive.success);
    CHECK(cfg.getRequired<std::string>("server.host") == "127.0.0.1");
    CHECK(cfg.getRequired<int64_t>("server.port") == 8080);
    CHECK(cfg.getRequired<bool>("metrics.enabled"));
    CHECK(cfg.getRequired<bool>("nested.enabled"));
    CHECK(cfg.getRequired<Base::ConfigArray>("nested.weights").size() == 3);
}

TEST_CASE("ConfigManager should apply deterministic file override order", "[config][manager]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    tmp.writeFile("b.yml", "service:\n  mode: second\n");
    tmp.writeFile("a.yml", "service:\n  mode: first\n");

    const auto result = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(result.success);
    CHECK(cfg.getString("service.mode", "") == "second");
}

TEST_CASE("ConfigManager should keep partial results when some files fail", "[config][manager]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    const auto good = tmp.writeFile("ok.yml", "ok:\n  value: 7\n");
    const auto bad = tmp.writeFile("bad.yml", "broken: [1, 2\n");

    const auto result = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE_FALSE(result.success);
    CHECK(result.loaded_files.size() == 1);
    CHECK(result.failed_files.size() == 1);
    CHECK(result.loaded_files.front() == good.string());
    CHECK(result.failed_files.front() == bad.string());
    CHECK(cfg.getInt("ok.value", -1) == 7);
}

TEST_CASE("ConfigManager::loadFiles should report mixed outcomes", "[config][manager]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    const auto good = tmp.writeFile("good.yaml", "api:\n  retries: 3\n");
    const auto not_yaml = tmp.writeFile("note.txt", "plain text");
    const auto missing = tmp.path() / "missing.yml";

    const auto result = cfg.loadFiles({good, not_yaml, missing});
    REQUIRE_FALSE(result.success);
    CHECK(result.loaded_files.size() == 1);
    CHECK(result.failed_files.size() == 2);
    CHECK(cfg.getInt("api.retries", -1) == 3);
    CHECK(cfg.configDirectory().empty());
}

TEST_CASE("ConfigManager access APIs should expose consistent state", "[config][manager]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    tmp.writeFile("state.yml",
                  "server:\n"
                  "  host: localhost\n"
                  "  port: 9000\n"
                  "  ratio: 0.5\n"
                  "  debug: false\n");

    const auto loaded = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(loaded.success);

    CHECK(cfg.has("server.host"));
    CHECK(cfg.getRequired<std::string>("server.host") == "localhost");
    CHECK(cfg.getInt("server.port", 0) == 9000);
    CHECK(cfg.getDouble("server.ratio", 0.0) == Catch::Approx(0.5));
    CHECK_FALSE(cfg.getBool("server.debug", true));
    CHECK(cfg.getString("server.host", "fallback") == "localhost");
    CHECK(cfg.getString("missing", "fallback") == "fallback");
    CHECK_FALSE(cfg.getOptional("missing").has_value());

    try
    {
        static_cast<void>(cfg.get("missing.key"));
        FAIL("Expected ConfigKeyNotFoundException");
    } catch (const Base::ConfigKeyNotFoundException &ex)
    {
        CHECK(ex.key() == "missing.key");
    }

    const auto keys = cfg.keys();
    REQUIRE(keys.size() == 4);
    CHECK(std::is_sorted(keys.begin(), keys.end()));
    CHECK(cfg.dump().size() == 4);
    CHECK(cfg.loadedFiles().size() == 1);
    CHECK(cfg.configDirectory() == tmp.path());
    CHECK(cfg.validateRequired({"server.port", "server.host", "missing.key"}) ==
          std::vector<std::string>{"missing.key"});

    cfg.clear();
    CHECK_FALSE(cfg.has("server.host"));
    CHECK(cfg.keys().empty());
}

TEST_CASE("ConfigManager::reload should honor lifecycle semantics", "[config][manager]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    const auto no_context = cfg.reload();
    REQUIRE_FALSE(no_context.success);
    REQUIRE_FALSE(no_context.errors.empty());
    CHECK_THAT(no_context.errors.front(), ContainsSubstring("loadFromDirectory"));

    tmp.writeFile("runtime.yml", "runtime:\n  timeout_ms: 100\n");
    const auto initial = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(initial.success);
    CHECK(cfg.getInt("runtime.timeout_ms", 0) == 100);

    tmp.writeFile("runtime.yml", "runtime:\n  timeout_ms: 250\n");
    const auto reloaded = cfg.reload();
    REQUIRE(reloaded.success);
    CHECK(cfg.getInt("runtime.timeout_ms", 0) == 250);
}

TEST_CASE("FileWatcherFactory should provide a watcher instance", "[config][watcher]")
{
    const auto watcher = Base::FileWatcherFactory::create();
    REQUIRE(watcher != nullptr);
    CHECK_FALSE(watcher->isRunning());
}

TEST_CASE("Hot reload should enforce preconditions and idempotent toggle", "[config][hotreload][slow]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    CHECK_FALSE(cfg.enableHotReload(nullptr, 50ms));

    tmp.writeFile("server.yml", "server:\n  port: 8080\n");
    const auto loaded = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(loaded.success);

    REQUIRE(cfg.enableHotReload(nullptr, 50ms));
    CHECK(cfg.isHotReloadEnabled());
    CHECK(cfg.enableHotReload(nullptr, 50ms));

    cfg.disableHotReload();
    CHECK_FALSE(cfg.isHotReloadEnabled());
    cfg.disableHotReload();
}

TEST_CASE("Hot reload should update values when YAML changes", "[config][hotreload][slow]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    tmp.writeFile("runtime.yml", "runtime:\n  port: 8080\n");
    const auto loaded = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(loaded.success);
    REQUIRE(cfg.getInt("runtime.port", 0) == 8080);

    std::promise<Base::ConfigLoadResult> promise;
    auto future = promise.get_future();
    std::atomic<bool> captured{false};

    REQUIRE(cfg.enableHotReload([&](const Base::ConfigLoadResult &result)
    {
        if (!captured.exchange(true))
        {
            promise.set_value(result);
        }
    }, 50ms));

    tmp.writeFile("runtime.yml", "runtime:\n  port: 9090\n");

    REQUIRE(future.wait_for(6s) == std::future_status::ready);
    const auto callback_result = future.get();
    CHECK(callback_result.success);
    REQUIRE(waitUntil([&cfg]()
    {
        return cfg.getInt("runtime.port", -1) == 9090;
    }, 4s));

    cfg.disableHotReload();
}

TEST_CASE("Hot reload should ignore non-YAML changes", "[config][hotreload][slow]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    tmp.writeFile("app.yml", "app:\n  workers: 2\n");
    const auto loaded = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(loaded.success);

    std::atomic<int> callback_count{0};
    REQUIRE(cfg.enableHotReload([&callback_count](const Base::ConfigLoadResult &)
    {
        callback_count.fetch_add(1, std::memory_order_relaxed);
    }, 50ms));

    std::this_thread::sleep_for(200ms);
    callback_count.store(0, std::memory_order_relaxed);

    tmp.writeFile("notes.txt", "change");

    const bool triggered = waitUntil([&callback_count]()
    {
        return callback_count.load(std::memory_order_relaxed) > 0;
    }, 1500ms, 50ms);

    CHECK_FALSE(triggered);
    CHECK(cfg.getInt("app.workers", 0) == 2);

    cfg.disableHotReload();
}
