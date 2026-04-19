#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ctime>
#include <filesystem>
#include <string>

#include "base/Log.h"

namespace
{
    void cleanupPath(const std::filesystem::path &path)
    {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
}

TEST_CASE("formatted debug macro is callable", "[base][log][stress]")
{
    auto logger = LOG_NAME("stress.macro");
    logger->setLevel(Gyanis::base::LogLevel::DEBUG);

    LOG_FMT_DEBUG(logger, "macro %s %d", "call", 1);

    SUCCEED();
}

TEST_CASE("file appender supports plain file name", "[base][log][stress]")
{
    const std::filesystem::path file = "plain_filename_stress.log";
    cleanupPath(file);

    auto appender = std::make_shared<Gyanis::base::FileLogAppender>(file.string());
    auto logger   = LOG_NAME("stress.file");

    logger->setLevel(Gyanis::base::LogLevel::DEBUG);
    logger->clearAppenders();
    logger->addAppender(appender);

    LOG_INFO(logger) << "hello plain filename";

    REQUIRE(std::filesystem::exists(file));
    REQUIRE(std::filesystem::is_regular_file(file));

    cleanupPath(file);
}

TEST_CASE("log event format keeps long payload", "[base][log][stress]")
{
    const std::string payload(4096, 'x');

    auto logger = LOG_NAME("stress.format");
    auto event  = std::make_shared<Gyanis::base::LogEvent>(
        logger,
        Gyanis::base::LogLevel::INFO,
        __FILE__,
        __LINE__,
        1u,
        static_cast<uint64_t>(std::time(nullptr)));

    event->format("prefix:%s:suffix", payload.c_str());

    REQUIRE(event->getContent() == ("prefix:" + payload + ":suffix"));
}

TEST_CASE("logger stream benchmark", "[base][log][stress][benchmark]")
{
    const std::filesystem::path benchDir = "log_bench";
    cleanupPath(benchDir);

    auto logger = LOG_NAME("stress.benchmark");
    logger->setLevel(Gyanis::base::LogLevel::DEBUG);
    logger->clearAppenders();
    logger->addAppender(std::make_shared<Gyanis::base::FileLogAppender>((benchDir / "stream.log").string()));

    BENCHMARK("stream 200 log lines")
    {
        for (int i = 0; i < 200; ++i)
        {
            LOG_DEBUG(logger) << "bench message " << i;
        }
        return 0;
    };

    cleanupPath(benchDir);
}
