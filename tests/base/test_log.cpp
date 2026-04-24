#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "base/Log.h"

namespace
{
    size_t CountSubString(const std::string_view haystack, const std::string_view needle)
    {
        if (needle.empty())
        {
            return 0;
        }

        size_t count = 0;
        size_t pos   = 0;
        while ((pos = haystack.find(needle, pos)) != std::string_view::npos)
        {
            ++count;
            pos += needle.size();
        }
        return count;
    }

    std::string ReadTextFile(const std::filesystem::path &path)
    {
        std::ifstream ifs(path);
        if (!ifs.is_open())
        {
            return "";
        }
        return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }

    std::filesystem::path UniqueTempPath(const std::string_view filename)
    {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path() / (std::string(filename) + "_" + std::to_string(ticks) + ".log");
    }
}

TEST_CASE("LogLevel conversion is case-insensitive", "[base][log][level]")
{
    using Gyanis::base::LogLevel;

    REQUIRE(LogLevel::FromString("debug") == LogLevel::DEBUG);
    REQUIRE(LogLevel::FromString("INFO") == LogLevel::INFO);
    REQUIRE(LogLevel::FromString("Warn") == LogLevel::WARN);
    REQUIRE(LogLevel::FromString("error") == LogLevel::ERROR);
    REQUIRE(LogLevel::FromString("FATAL") == LogLevel::FATAL);
    REQUIRE(LogLevel::FromString("unexpected") == LogLevel::UNKNOW);

    REQUIRE(std::string(LogLevel::ToString(LogLevel::DEBUG)) == "DEBUG");
    REQUIRE(std::string(LogLevel::ToString(LogLevel::FATAL)) == "FATAL");
    REQUIRE(std::string(LogLevel::ToString(static_cast<LogLevel::Level>(99))) == "UNKNOW");
}

TEST_CASE("LogEvent format handles long payload", "[base][log][event]")
{
    using namespace Gyanis::base;

    auto logger = std::make_shared<Logger>("event_long");
    auto event  = std::make_shared<LogEvent>(logger, LogLevel::INFO, "test.cpp", 1, 123, LogEvent::GetCurrentTime());

    std::string payload(5000, 'x');
    event->format("payload:%s", payload.c_str());

    const auto content = event->getContent();
    REQUIRE(content.size() == payload.size() + 8);
    REQUIRE(content.find(payload) != std::string::npos);
}

TEST_CASE("LogFormatter reports invalid pattern", "[base][log][formatter]")
{
    using namespace Gyanis::base;

    LogFormatter invalid_formatter("%d{%Y-%m-%d %H:%M:%S");
    REQUIRE(invalid_formatter.isError());

    auto       logger = std::make_shared<Logger>("invalid_pattern");
    auto       event  = std::make_shared<LogEvent>(logger, LogLevel::INFO, "bad.cpp", 2, 1, LogEvent::GetCurrentTime());
    const auto text   = invalid_formatter.format(logger, LogLevel::INFO, event);
    REQUIRE(text.find("pattern_error") != std::string::npos);
}

TEST_CASE("Source location format supports width alignment", "[base][log][formatter]")
{
    using namespace Gyanis::base;

    auto         logger = std::make_shared<Logger>("line_align");
    LogFormatter formatter("%f{32}:%l{4}%T%m");

    auto make_line = [&](const char *file, const int line, const std::string &msg)
    {
        auto event = std::make_shared<LogEvent>(logger, LogLevel::INFO, file, line, 1, LogEvent::GetCurrentTime());
        event->getSS() << msg;
        return formatter.format(logger, LogLevel::INFO, event);
    };

    const auto s1 = make_line("src/a.cpp", 1, "msgA");
    const auto s2 = make_line("src/base/Utils.cpp", 10, "msgB");
    const auto s3 = make_line("src/net/http/HttpConnection.cpp", 100, "msgC");

    REQUIRE(s1.find("msgA") == s2.find("msgB"));
    REQUIRE(s2.find("msgB") == s3.find("msgC"));
}

TEST_CASE("Default logger format keeps message columns aligned", "[base][log][formatter]")
{
    using namespace Gyanis::base;

    auto       logger    = std::make_shared<Logger>("default_align");
    const auto formatter = logger->getFormatter();
    REQUIRE(formatter != nullptr);

    auto make_line = [&](const char *file, const int line, const std::string &msg)
    {
        auto event = std::make_shared<LogEvent>(logger, LogLevel::INFO, file, line, 1, LogEvent::GetCurrentTime());
        event->getSS() << msg;
        return formatter->format(logger, LogLevel::INFO, event);
    };

    const auto s1 = make_line("src/base/Utils.cpp", 1, "msg1");
    const auto s2 = make_line("src/base/Config.cpp", 10, "msg10");
    const auto s3 = make_line("src/net/Application.cpp", 100, "msg100");

    REQUIRE(s1.find("msg1") == s2.find("msg10"));
    REQUIRE(s2.find("msg10") == s3.find("msg100"));
}

TEST_CASE("Logger level filtering works", "[base][log][logger]")
{
    using namespace Gyanis::base;

    const auto      log_file = UniqueTempPath("level_filter");
    std::error_code ec;
    std::filesystem::remove(log_file, ec);

    auto logger = std::make_shared<Logger>("level_filter");
    logger->clearAppenders();

    auto appender = std::make_shared<FileLogAppender>(log_file.string());
    appender->setLevel(LogLevel::DEBUG);
    appender->setAsyncConfig(2048, 256, std::chrono::milliseconds(10));
    logger->addAppender(appender);
    logger->setLevel(LogLevel::WARN);

    LOG_INFO(logger) << "should_not_appear";
    LOG_WARN(logger) << "warn_appear";
    LOG_ERROR(logger) << "error_appear";

    logger->delAppender(appender);
    appender.reset();

    const auto content = ReadTextFile(log_file);
    REQUIRE(content.find("should_not_appear") == std::string::npos);
    REQUIRE(content.find("warn_appear") != std::string::npos);
    REQUIRE(content.find("error_appear") != std::string::npos);

    std::filesystem::remove(log_file, ec);
}

TEST_CASE("Formatting macros support empty and variadic arguments", "[base][log][macro]")
{
    using namespace Gyanis::base;

    const auto      log_file = UniqueTempPath("fmt_macro");
    std::error_code ec;
    std::filesystem::remove(log_file, ec);

    auto logger = std::make_shared<Logger>("fmt_macro");
    logger->clearAppenders();
    auto appender = std::make_shared<FileLogAppender>(log_file.string());
    appender->setAsyncConfig(2048, 256, std::chrono::milliseconds(10));
    logger->addAppender(appender);

    LOG_FMT_INFO(logger, "format %d", 42);
    LOG_FMT_WARN(logger, "format without args");

    logger->delAppender(appender);
    appender.reset();

    const auto content = ReadTextFile(log_file);
    REQUIRE(content.find("format 42") != std::string::npos);
    REQUIRE(content.find("format without args") != std::string::npos);

    std::filesystem::remove(log_file, ec);
}

TEST_CASE("Async FileLogAppender keeps all logs under burst", "[base][log][file][async]")
{
    using namespace Gyanis::base;

    const auto      log_file = UniqueTempPath("async_burst");
    std::error_code ec;
    std::filesystem::remove(log_file, ec);

    auto logger = std::make_shared<Logger>("async_burst");
    logger->clearAppenders();

    auto appender = std::make_shared<FileLogAppender>(log_file.string());
    appender->setLevel(LogLevel::DEBUG);
    appender->setAsyncConfig(4096, 512, std::chrono::milliseconds(10));
    logger->addAppender(appender);

    constexpr int kCount = 3000;
    for (int i = 0; i < kCount; ++i)
    {
        LOG_FMT_DEBUG(logger, "burst line %d", i);
    }

    logger->delAppender(appender);
    appender.reset();

    const auto content = ReadTextFile(log_file);
    REQUIRE(content.find("burst line 0") != std::string::npos);
    REQUIRE(content.find("burst line 2999") != std::string::npos);
    REQUIRE(CountSubString(content, "burst line ") == static_cast<size_t>(kCount));

    std::filesystem::remove(log_file, ec);
}

TEST_CASE("FileLogAppender creates nested directories", "[base][log][file]")
{
    using namespace Gyanis::base;

    const auto      base_dir = std::filesystem::temp_directory_path() / "gyanis_test_nested_logs";
    const auto      log_file = base_dir / "a" / "b" / "nested.log";
    std::error_code ec;
    std::filesystem::remove_all(base_dir, ec);

    {
        auto logger = std::make_shared<Logger>("nested_dir");
        logger->clearAppenders();
        auto appender = std::make_shared<FileLogAppender>(log_file.string());
        appender->setAsyncConfig(1024, 128, std::chrono::milliseconds(10));
        logger->addAppender(appender);
        LOG_INFO(logger) << "nested directory output";
        logger->delAppender(appender);
    }

    REQUIRE(std::filesystem::exists(log_file));
    const auto content = ReadTextFile(log_file);
    REQUIRE(content.find("nested directory output") != std::string::npos);

    std::filesystem::remove_all(base_dir, ec);
}

TEST_CASE("Logger and Appender YAML serialization contains key fields", "[base][log][yaml]")
{
    using namespace Gyanis::base;

    const auto      log_file = UniqueTempPath("yaml_check");
    std::error_code ec;
    std::filesystem::remove(log_file, ec);

    auto logger = std::make_shared<Logger>("yaml_logger");
    logger->clearAppenders();

    auto appender = std::make_shared<FileLogAppender>(log_file.string());
    appender->setAsyncConfig(1024, 64, std::chrono::milliseconds(10));
    logger->addAppender(appender);

    const auto appender_yaml = appender->toYamlString();
    REQUIRE(appender_yaml.find("FileLogAppender") != std::string::npos);
    REQUIRE(appender_yaml.find("flush_batch_size") != std::string::npos);

    const auto logger_yaml = logger->toYamlString();
    REQUIRE(logger_yaml.find("yaml_logger") != std::string::npos);
    REQUIRE(logger_yaml.find("appenders") != std::string::npos);

    logger->delAppender(appender);
    appender.reset();
    std::filesystem::remove(log_file, ec);
}

TEST_CASE("StdoutLogAppender color switch is configurable", "[base][log][stdout][color]")
{
    using namespace Gyanis::base;

    auto appender = std::make_shared<StdoutLogAppender>();
    REQUIRE(appender->isColorEnabled());

    appender->setColorEnabled(false);
    REQUIRE_FALSE(appender->isColorEnabled());

    const auto yaml = appender->toYamlString();
    REQUIRE(yaml.find("StdoutLogAppender") != std::string::npos);
    REQUIRE(yaml.find("color") != std::string::npos);
    REQUIRE(yaml.find("false") != std::string::npos);
}

TEST_CASE("LoggerManager returns stable logger instances", "[base][log][manager]")
{
    using namespace Gyanis::base;

    auto *manager = LoggerMgr::GetInstance();
    auto  root    = manager->getRoot();
    REQUIRE(root != nullptr);
    REQUIRE(root->getName() == "root");

    auto logger1 = manager->getLogger("manager_case");
    auto logger2 = manager->getLogger("manager_case");
    REQUIRE(logger1 == logger2);
}
