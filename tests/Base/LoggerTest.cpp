#include "../../src/Base/ConfigFileWatcher.h"
#include "../../src/Base/ConfigManager.h"
#include "../../src/Base/ConfigType.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
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
            dir_ = makeUniquePath();
            std::filesystem::create_directories(dir_);
        }

        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(dir_, ec);
        }

        [[nodiscard]] const std::filesystem::path &path() const noexcept
        {
            return dir_;
        }

        std::filesystem::path write(const std::filesystem::path &relative, const std::string &content) const
        {
            const auto file = dir_ / relative;
            std::filesystem::create_directories(file.parent_path());

            std::ofstream out(file, std::ios::binary | std::ios::trunc);
            if (!out.is_open())
            {
                throw std::runtime_error("failed to create test file: " + file.string());
            }
            out << content;
            out.close();
            return file;
        }

    private:
        static std::filesystem::path makeUniquePath()
        {
            static std::atomic<unsigned long long> counter{0};
            const auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto id = counter.fetch_add(1, std::memory_order_relaxed);
            return std::filesystem::temp_directory_path() /
                   ("gyanis_logger_test_" + std::to_string(ts) + "_" + std::to_string(id));
        }

        std::filesystem::path dir_;
    };
}

TEST_CASE("Empty directory load should retain directory context for lifecycle APIs", "[config][manager][regression]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    const auto result = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(result.success);
    CHECK(result.loaded_files.empty());
    CHECK(result.failed_files.empty());

    CHECK(cfg.configDirectory() == tmp.path());

    const auto reload_result = cfg.reload();
    CHECK(reload_result.success);
}

TEST_CASE("Non-map YAML root should fail with explicit diagnostic", "[config][manager][parsing]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    const auto bad_file = tmp.write("bad.yml", "- first\n- second\n");
    const auto result = cfg.loadFromDirectory(tmp.path(), true);

    REQUIRE_FALSE(result.success);
    REQUIRE(result.failed_files.size() == 1);
    CHECK(result.failed_files.front() == bad_file.string());
    REQUIRE_FALSE(result.errors.empty());
    CHECK_THAT(result.errors.front(), ContainsSubstring("Root node must be a map"));
}

TEST_CASE("Scalar conversion should parse bool/int/double and preserve text", "[config][manager][typing]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    tmp.write("typed.yml",
              "typed:\n"
              "  bool_true: true\n"
              "  int_value: 42\n"
              "  double_value: 3.5\n"
              "  text_value: 42ms\n"
              "  null_value: ~\n"
              "  endpoints:\n"
              "    - host: 127.0.0.1\n"
              "      port: 8080\n");

    const auto result = cfg.loadFromDirectory(tmp.path(), true);
    REQUIRE(result.success);

    CHECK(cfg.getRequired<bool>("typed.bool_true"));
    CHECK(cfg.getRequired<int64_t>("typed.int_value") == 42);
    CHECK(cfg.getRequired<double>("typed.double_value") == 3.5);
    CHECK(cfg.getRequired<std::string>("typed.text_value") == "42ms");

    const auto null_opt = cfg.getOptional("typed.null_value");
    REQUIRE(null_opt.has_value());
    CHECK(null_opt->isNull());

    const auto endpoints = cfg.getRequired<Base::ConfigArray>("typed.endpoints");
    REQUIRE(endpoints.size() == 1);
    REQUIRE(endpoints[0].is<Base::ConfigObject>());
    CHECK(endpoints[0]["host"].asString() == "127.0.0.1");
    CHECK(endpoints[0]["port"].asInt() == 8080);
}

TEST_CASE("LoadFiles should keep successful values when parser errors exist", "[config][manager][io]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    const auto good = tmp.write("ok.yaml", "service:\n  retries: 5\n");
    const auto broken = tmp.write("broken.yaml", "service:\n  retries: [1,2\n");

    const auto result = cfg.loadFiles({good, broken});
    REQUIRE_FALSE(result.success);
    CHECK(result.loaded_files.size() == 1);
    CHECK(result.failed_files.size() == 1);
    CHECK(cfg.getInt("service.retries", -1) == 5);
}

TEST_CASE("Concurrent readers should observe consistent values during reload", "[config][manager][concurrency][slow]")
{
    ScopedConfigReset guard;
    auto &cfg = Base::ConfigManager::instance();
    TempDir tmp;

    tmp.write("runtime.yml", "runtime:\n  version: 0\n");
    REQUIRE(cfg.loadFromDirectory(tmp.path(), true).success);

    std::atomic<bool> running{true};
    std::atomic<int> read_errors{0};

    std::vector<std::thread> readers;
    readers.reserve(4);
    for (int i = 0; i < 4; ++i)
    {
        readers.emplace_back([&cfg, &running, &read_errors]()
        {
            while (running.load(std::memory_order_acquire))
            {
                try
                {
                    const auto value = cfg.getInt("runtime.version", -1);
                    if (value < 0)
                    {
                        read_errors.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (...)
                {
                    read_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (int version = 1; version <= 30; ++version)
    {
        tmp.write("runtime.yml", "runtime:\n  version: " + std::to_string(version) + "\n");
        REQUIRE(cfg.reload().success);
    }

    running.store(false, std::memory_order_release);
    for (auto &reader: readers)
    {
        reader.join();
    }

    CHECK(read_errors.load(std::memory_order_relaxed) == 0);
    CHECK(cfg.getInt("runtime.version", -1) == 30);
}

TEST_CASE("Watcher contract should gracefully handle missing paths", "[config][watcher]")
{
    const auto watcher = Base::FileWatcherFactory::create();
    REQUIRE(watcher != nullptr);

    watcher->setCallback([](const std::string_view, const Base::FileChangeEvent)
    {
    });

    CHECK_FALSE(watcher->addWatch("this/path/does/not/exist", false));
    CHECK_FALSE(watcher->removeWatch("this/path/does/not/exist"));

    const bool started = watcher->start();
    if (started)
    {
        CHECK(watcher->isRunning());
    }
    watcher->stop();
    CHECK_FALSE(watcher->isRunning());
}
