// ReSharper disable CppExpressionWithoutSideEffects
#include "Base/Logger.h"
#include "Base/LoggerConfig.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <iostream>


namespace
{
    using Catch::Matchers::ContainsSubstring;
    using Catch::Matchers::ContainsSubstring;
    using namespace std::chrono_literals;

    class ScopedLogAndConfigReset
    {
    public:
        ScopedLogAndConfigReset()
        {
            reset();
        }

        ~ScopedLogAndConfigReset()
        {
            reset();
        }

    private:
        static void reset()
        {
            auto &cfg = Base::ConfigManager::instance();
            cfg.disableHotReload();
            cfg.clear();
            Base::LoggerRegistry::instance().clear();
        }
    };

    class ScopedRegistryReset
    {
    public:
        ScopedRegistryReset()
        {
            reset();
        }

        ~ScopedRegistryReset()
        {
            reset();
        }

    private:
        static void reset()
        {
            Base::LoggerRegistry::instance().clear();
        }
    };

    class RecordingSink final : public Base::LogSink
    {
    public:
        void write(const Base::LogEvent &event) override
        {
            std::lock_guard lock(m_mutex);
            m_events.push_back(event);
            m_messages.push_back(event.message);
        }

        void flush() override
        {
            ++m_flush_count;
        }

        [[nodiscard]] std::vector<Base::LogEvent> snapshot() const
        {
            std::lock_guard lock(m_mutex);
            return m_events;
        }

        [[nodiscard]] size_t flushCount() const noexcept
        {
            return m_flush_count.load(std::memory_order_relaxed);
        }

        [[nodiscard]] size_t messageCount() const
        {
            std::lock_guard lock(m_mutex);
            return m_messages.size();
        }

    private:
        mutable std::mutex m_mutex;
        std::vector<Base::LogEvent> m_events;
        std::atomic<size_t> m_flush_count{0};
        std::vector<std::string> m_messages;
    };

    class BlockingSink final : public Base::LogSink
    {
    public:
        std::atomic<bool> hold{true};
        std::atomic<int> writes{0};

        void write(const Base::LogEvent &) override
        {
            while (hold.load(std::memory_order_acquire))
            {
                std::this_thread::sleep_for(1ms);
            }
            writes.fetch_add(1, std::memory_order_relaxed);
        }

        void flush() override
        {
        }
    };

    class TempDir
    {
    public:
        TempDir()
        {
            m_dir = makeUniquePath();
            std::filesystem::create_directories(m_dir);
        }

        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(m_dir, ec);
        }

        [[nodiscard]] const std::filesystem::path &path() const noexcept
        {
            return m_dir;
        }

        std::filesystem::path write(const std::filesystem::path &relative, const std::string &content) const
        {
            const auto file = m_dir / relative;
            std::filesystem::create_directories(file.parent_path());

            std::ofstream out(file, std::ios::binary | std::ios::trunc);
            if (!out.is_open())
            {
                throw std::runtime_error("Failed to write test file: " + file.string());
            }
            out << content;
            return file;
        }

    private:
        static std::filesystem::path makeUniquePath()
        {
            static std::atomic<unsigned long long> counter{0};
            const auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto id = counter.fetch_add(1, std::memory_order_relaxed);
            return std::filesystem::temp_directory_path() /
                   ("gyanis_logsink_test_" + std::to_string(ts) + "_" + std::to_string(id));
        }

        std::filesystem::path m_dir;
    };

    class ScopedStreamRedirect
    {
    public:
        ScopedStreamRedirect(std::ostream &stream, std::ostringstream &target)
            : stream_(stream), old_buf_(stream.rdbuf(target.rdbuf()))
        {
        }

        ~ScopedStreamRedirect()
        {
            stream_.rdbuf(old_buf_);
        }

    private:
        std::ostream &stream_;
        std::streambuf *old_buf_;
    };


    Base::SourceLocation testSourceLocation()
    {
        return Base::SourceLocation("tests/Base/LoggerTest.cpp", 42, "testSourceLocation");
    }

    Base::LogEvent makeEvent(const Base::LogLevel level, std::string message)
    {
        return Base::LogEvent{
            level,
            "2026-04-22 10:10:10.123",
            "thread-1",
            Base::SourceLocation("tests/Base/LogSinkTest.cpp", 88, "makeEvent"),
            "test-logger",
            std::move(message)
        };
    }


    size_t countLines(const std::filesystem::path &file)
    {
        std::ifstream in(file);
        if (!in.is_open())
        {
            return 0;
        }

        size_t lines = 0;
        std::string temp;
        while (std::getline(in, temp))
        {
            ++lines;
        }
        return lines;
    }

    std::string readAllText(const std::filesystem::path &file)
    {
        std::ifstream in(file, std::ios::binary);
        if (!in.is_open())
        {
            return {};
        }
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
}

TEST_CASE("LogCommon level conversions should remain stable", "[log][common]")
{
    CHECK(Base::logLevelToString(Base::LogLevel::TRACE) == std::string("TRACE"));
    CHECK(Base::logLevelToString(Base::LogLevel::INFO) == std::string("INFO "));
    CHECK(Base::logLevelToString(Base::LogLevel::FATAL) == std::string("FATAL"));

    CHECK(Base::logLevelFromString("TRACE") == Base::LogLevel::TRACE);
    CHECK(Base::logLevelFromString("DEBUG") == Base::LogLevel::DEBUG);
    CHECK(Base::logLevelFromString("INFO") == Base::LogLevel::INFO);
    CHECK(Base::logLevelFromString("WARN") == Base::LogLevel::WARN);
    CHECK(Base::logLevelFromString("ERROR") == Base::LogLevel::ERROR);
    CHECK(Base::logLevelFromString("FATAL") == Base::LogLevel::FATAL);
    CHECK(Base::logLevelFromString("OFF") == Base::LogLevel::OFF);
    CHECK(Base::logLevelFromString("unknown") == Base::LogLevel::INFO);
}

TEST_CASE("SourceLocation shortFileName should trim full path", "[log][common]")
{
    constexpr Base::SourceLocation location("c:/repo/src/Base/Logger.cpp", 11, "func");
    CHECK(std::string(location.shortFileName()) == "Logger.cpp");
}

TEST_CASE("Logger should filter by logger and sink levels", "[log][logger]")
{
    ScopedRegistryReset guard;

    Base::Logger logger("unit");
    auto sink = std::make_unique<RecordingSink>();
    auto *sink_ptr = sink.get();
    logger.addSink(std::move(sink));

    logger.setLevel(Base::LogLevel::INFO);
    logger.log(Base::LogLevel::DEBUG, testSourceLocation(), "hidden-debug");
    logger.log(Base::LogLevel::ERROR, testSourceLocation(), "visible-error");

    auto events = sink_ptr->snapshot();
    REQUIRE(events.size() == 1);
    CHECK(events[0].message == "visible-error");
    CHECK(events[0].logger_name == "unit");

    sink_ptr->setLevel(Base::LogLevel::FATAL);
    logger.log(Base::LogLevel::ERROR, testSourceLocation(), "hidden-by-sink");
    logger.log(Base::LogLevel::FATAL, testSourceLocation(), "visible-fatal");

    events = sink_ptr->snapshot();
    REQUIRE(events.size() == 2);
    CHECK(events[1].message == "visible-fatal");
}

TEST_CASE("Logger should capture call-site location with C++20 source_location", "[log][logger][location]")
{
    ScopedRegistryReset guard;

    Base::Logger logger("location");
    auto sink = std::make_unique<RecordingSink>();
    const auto *sink_ptr = sink.get();
    logger.addSink(std::move(sink));

    constexpr int expected_line = __LINE__ + 1;
    logger.log(Base::LogLevel::INFO, "auto-location");

    const auto events = sink_ptr->snapshot();
    REQUIRE(events.size() == 1);
    CHECK(events[0].location.line == expected_line);
    CHECK(std::string(events[0].location.shortFileName()) == "LoggerTest.cpp");
}

TEST_CASE("Logger flush should call each sink", "[log][logger]")
{
    Base::Logger logger("flush");

    auto sink_a = std::make_unique<RecordingSink>();
    auto sink_b = std::make_unique<RecordingSink>();
    const auto *sink_a_ptr = sink_a.get();
    const auto *sink_b_ptr = sink_b.get();

    logger.addSink(std::move(sink_a));
    logger.addSink(std::move(sink_b));

    logger.flush();

    CHECK(sink_a_ptr->flushCount() == 1);
    CHECK(sink_b_ptr->flushCount() == 1);
}

TEST_CASE("LoggerRegistry should manage lifecycle consistently", "[log][registry]")
{
    ScopedRegistryReset guard;
    auto &registry = Base::LoggerRegistry::instance();

    auto &alpha = registry.getLogger("alpha");
    auto &alpha_again = registry.getLogger("alpha");
    CHECK(std::addressof(alpha) == std::addressof(alpha_again));

    auto beta = std::make_unique<Base::Logger>("beta");
    registry.registerLogger(std::move(beta));

    const auto names = registry.getLoggerNames();
    CHECK(names.size() == 2);
    CHECK(std::find(names.begin(), names.end(), "alpha") != names.end());
    CHECK(std::find(names.begin(), names.end(), "beta") != names.end());

    size_t visited = 0;
    registry.forEachLogger([&visited](Base::Logger &)
    {
        ++visited;
    });
    CHECK(visited == 2);

    registry.unregisterLogger("alpha");
    const auto names_after_remove = registry.getLoggerNames();
    CHECK(names_after_remove.size() == 1);
    CHECK(names_after_remove.front() == "beta");
}

TEST_CASE("Root logger macros should write expected events", "[log][logger][macro]")
{
    ScopedRegistryReset guard;
    auto &registry = Base::LoggerRegistry::instance();
    auto &root = registry.getRootLogger();
    root.setLevel(Base::LogLevel::TRACE);
    root.clearSinks();

    auto sink = std::make_unique<RecordingSink>();
    auto *sink_ptr = sink.get();
    root.addSink(std::move(sink));

    LOG_INFO("macro-message");
    LOG_INFO_STREAM("stream-" << 123);

    const auto events = sink_ptr->snapshot();
    REQUIRE(events.size() == 2);
    CHECK(events[0].logger_name == "root");
    CHECK(events[0].message == "macro-message");
    CHECK_THAT(events[1].message, ContainsSubstring("stream-123"));
}

TEST_CASE("Formatters should include expected fields", "[log][sink][format]")
{
    Base::DefaultFormatter plain;
    Base::ColorFormatter color;
    const auto event = makeEvent(Base::LogLevel::WARN, "formatter-message");

    const auto plain_text = plain.format(event);
    CHECK_THAT(plain_text, ContainsSubstring("2026-04-22 10:10:10.123"));
    CHECK_THAT(plain_text, ContainsSubstring("WARN"));
    CHECK_THAT(plain_text, ContainsSubstring("test-logger"));
    CHECK_THAT(plain_text, ContainsSubstring("formatter-message"));

    const auto color_text = color.format(event);
    CHECK_THAT(color_text, ContainsSubstring("\033["));
    CHECK_THAT(color_text, ContainsSubstring("formatter-message"));
    CHECK_THAT(color_text, ContainsSubstring(Base::color::RESET));
}

TEST_CASE("ConsoleSink should route info to stdout and errors to stderr", "[log][sink][console]")
{
    std::ostringstream out_capture;
    std::ostringstream err_capture;
    ScopedStreamRedirect out_redirect(std::cout, out_capture);
    ScopedStreamRedirect err_redirect(std::cerr, err_capture);

    Base::ConsoleSink sink(false);
    sink.write(makeEvent(Base::LogLevel::INFO, "to-stdout"));
    sink.write(makeEvent(Base::LogLevel::ERROR, "to-stderr"));
    sink.flush();

    CHECK_THAT(out_capture.str(), ContainsSubstring("to-stdout"));
    CHECK_THAT(err_capture.str(), ContainsSubstring("to-stderr"));
    CHECK_THAT(out_capture.str(), !ContainsSubstring("\033["));
    CHECK_THAT(err_capture.str(), !ContainsSubstring("\033["));
}

TEST_CASE("FileSink should write, flush, and reopen correctly", "[log][sink][file]")
{
    TempDir tmp;
    const auto first_file = tmp.path() / "one.log";
    const auto second_file = tmp.path() / "two.log";

    Base::FileSink sink(first_file, true);
    sink.write(makeEvent(Base::LogLevel::INFO, "first-message"));
    sink.flush();

    sink.reopen(second_file);
    sink.write(makeEvent(Base::LogLevel::INFO, "second-message"));
    sink.flush();

    std::ifstream first_in(first_file);
    std::ifstream second_in(second_file);
    REQUIRE(first_in.is_open());
    REQUIRE(second_in.is_open());

    std::string first_content((std::istreambuf_iterator(first_in)), std::istreambuf_iterator<char>());
    std::string second_content((std::istreambuf_iterator(second_in)), std::istreambuf_iterator<char>());

    CHECK_THAT(first_content, ContainsSubstring("first-message"));
    CHECK_THAT(second_content, ContainsSubstring("second-message"));
}

TEST_CASE("RollingFileSink should rotate by size and cleanup backups", "[log][sink][rolling]")
{
    TempDir tmp;

    Base::RollingFileSink sink("app.log", tmp.path(), Base::RollingPolicy::Size, 1, 1);
    sink.write(makeEvent(Base::LogLevel::INFO, "entry-a"));
    sink.write(makeEvent(Base::LogLevel::INFO, "entry-b"));
    sink.write(makeEvent(Base::LogLevel::INFO, "entry-c"));
    sink.flush();

    size_t backup_count = 0;
    bool current_exists = false;
    for (const auto &entry: std::filesystem::directory_iterator(tmp.path()))
    {
        const auto filename = entry.path().filename().string();
        if (filename == "app.log")
        {
            current_exists = true;
        }
        if (filename.rfind("app.", 0) == 0 && filename != "app.log")
        {
            ++backup_count;
        }
    }

    CHECK(current_exists);
    CHECK(backup_count <= 1);
}

TEST_CASE("AsyncSink drop policy should drop under sustained pressure", "[log][sink][async][slow]")
{
    auto wrapped = std::make_unique<BlockingSink>();
    auto *wrapped_ptr = wrapped.get();

    Base::AsyncSink sink(std::move(wrapped), 4, Base::AsyncSink::OverflowPolicy::Drop);
    for (int i = 0; i < 200; ++i)
    {
        sink.write(makeEvent(Base::LogLevel::INFO, "drop-test-" + std::to_string(i)));
    }

    wrapped_ptr->hold.store(false, std::memory_order_release);
    sink.stop();

    const auto written = wrapped_ptr->writes.load(std::memory_order_relaxed);
    CHECK(written < 200);
}

TEST_CASE("AsyncSink flush and stop should drain queued entries", "[log][sink][async]")
{
    auto wrapped = std::make_unique<RecordingSink>();
    const auto *wrapped_ptr = wrapped.get();

    Base::AsyncSink sink(std::move(wrapped), 128, Base::AsyncSink::OverflowPolicy::Block);
    for (int i = 0; i < 50; ++i)
    {
        sink.write(makeEvent(Base::LogLevel::INFO, "drain-" + std::to_string(i)));
    }

    sink.flush();
    sink.stop();

    CHECK(wrapped_ptr->messageCount() == 50);
}

TEST_CASE("Concurrent logging through AsyncSink should remain complete and stable", "[log][concurrency][slow]")
{
    ScopedRegistryReset guard;
    const TempDir tmp;

    const auto output_file = tmp.path() / "concurrent.log";

    Base::Logger logger("concurrency");
    logger.setLevel(Base::LogLevel::TRACE);

    auto sink = std::make_unique<Base::AsyncSink>(
                                                  std::make_unique<Base::FileSink>(output_file, true),
                                                  8192,
                                                  Base::AsyncSink::OverflowPolicy::Block);
    logger.addSink(std::move(sink));

    constexpr int thread_count = 8;
    constexpr int per_thread_logs = 300;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int t = 0; t < thread_count; ++t)
    {
        workers.emplace_back([&logger, t]()
        {
            for (int i = 0; i < per_thread_logs; ++i)
            {
                logger.log(
                           Base::LogLevel::INFO,
                           Base::SourceLocation("LoggerTest.cpp", t, "worker"),
                           "thread-" + std::to_string(t) + " entry-" + std::to_string(i));
            }
        });
    }

    for (auto &worker: workers)
    {
        worker.join();
    }

    logger.flush();
    logger.clearSinks();

    constexpr auto expected_lines = static_cast<size_t>(thread_count * per_thread_logs);
    const auto actual_lines = countLines(output_file);
    CHECK(actual_lines == expected_lines);
}

TEST_CASE("LoggerConfigLoader should build file and async sinks from yaml", "[log][config]")
{
    ScopedLogAndConfigReset guard;
    TempDir tmp;

    const auto root_log = (tmp.path() / "logs" / "root.log").generic_string();
    const auto worker_log = (tmp.path() / "logs" / "worker.log").generic_string();

    tmp.write("logging.yml",
              "logging:\n"
              "  loggers:\n"
              "    root:\n"
              "      level: INFO\n"
              "      sinks:\n"
              "        - type: file\n"
              "          path: " + root_log + "\n"
              "          truncate: true\n"
              "    worker:\n"
              "      level: DEBUG\n"
              "      sinks:\n"
              "        - type: async\n"
              "          queue_size: 64\n"
              "          overflow_policy: block\n"
              "          wrapped:\n"
              "            type: file\n"
              "            path: " + worker_log + "\n"
              "            truncate: true\n");

    REQUIRE(Base::ConfigManager::instance().loadFromDirectory(tmp.path(), true).success);
    REQUIRE_NOTHROW(Base::LoggerConfigLoader::loadFromConfig("logging"));

    auto &root = Base::LoggerRegistry::instance().getRootLogger();
    auto &worker = Base::LoggerRegistry::instance().getLogger("worker");

    root.log(Base::LogLevel::INFO, Base::SourceLocation("cfg", 1, "root"), "root-message");
    worker.log(Base::LogLevel::DEBUG, Base::SourceLocation("cfg", 2, "worker"), "worker-message");
    root.flush();
    worker.flush();

    CHECK_THAT(readAllText(tmp.path() / "logs" / "root.log"), ContainsSubstring("root-message"));
    CHECK_THAT(readAllText(tmp.path() / "logs" / "worker.log"), ContainsSubstring("worker-message"));
}

TEST_CASE("LoggerConfigLoader should ignore unknown sink types without throwing", "[log][config]")
{
    ScopedLogAndConfigReset guard;
    const TempDir tmp;

    tmp.write("logging.yml",
              "logging:\n"
              "  loggers:\n"
              "    root:\n"
              "      level: INFO\n"
              "      sinks:\n"
              "        - type: unsupported_sink\n");

    REQUIRE(Base::ConfigManager::instance().loadFromDirectory(tmp.path(), true).success);
    REQUIRE_NOTHROW(Base::LoggerConfigLoader::loadFromConfig("logging"));

    const auto &root = Base::LoggerRegistry::instance().getRootLogger();
    CHECK(root.getLevel() == Base::LogLevel::INFO);
}

TEST_CASE("LoggerConfigLoader should throw on invalid sink type field", "[log][config]")
{
    ScopedLogAndConfigReset guard;
    const TempDir tmp;

    tmp.write("logging.yml",
              "logging:\n"
              "  loggers:\n"
              "    root:\n"
              "      sinks:\n"
              "        - type: 123\n");

    REQUIRE(Base::ConfigManager::instance().loadFromDirectory(tmp.path(), true).success);

    REQUIRE_THROWS_AS(Base::LoggerConfigLoader::loadFromConfig("logging"), Base::ConfigTypeException);
}
