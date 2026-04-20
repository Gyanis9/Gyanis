#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "base/Log.h"

namespace
{
    size_t CountSubString(std::string_view haystack, std::string_view needle)
    {
        if (needle.empty())
        {
            return 0;
        }

        size_t count = 0;
        size_t pos = 0;
        while ((pos = haystack.find(needle, pos)) != std::string_view::npos)
        {
            ++count;
            pos += needle.size();
        }
        return count;
    }

    bool Check(const bool condition, const std::string_view message)
    {
        if (!condition)
        {
            std::cerr << "[FAIL] " << message << std::endl;
            return false;
        }
        return true;
    }
}

int main()
{
    using namespace Gyanis::base;

    bool ok = true;
    ok &= Check(LogLevel::FromString("debug") == LogLevel::DEBUG, "LogLevel::FromString(debug)");
    ok &= Check(LogLevel::FromString("INFO") == LogLevel::INFO, "LogLevel::FromString(INFO)");
    ok &= Check(LogLevel::FromString("unknown") == LogLevel::UNKNOW, "LogLevel::FromString(unknown)");

    const std::filesystem::path log_file =
            std::filesystem::temp_directory_path() / "gyanis_test_async_log.log";
    std::error_code ec;
    std::filesystem::remove(log_file, ec);

    auto logger = std::make_shared<Logger>("test_log");
    logger->clearAppenders();
    logger->setLevel(LogLevel::DEBUG);

    auto appender = std::make_shared<FileLogAppender>(log_file.string());
    appender->setLevel(LogLevel::DEBUG);
    appender->setAsyncConfig(4096, 512, std::chrono::milliseconds(20));
    logger->addAppender(appender);

    LOG_INFO(logger) << "stream message";
    LOG_FMT_INFO(logger, "format message %d", 42);
    LOG_FMT_WARN(logger, "format message without args");

    constexpr int kBulkLines = 2000;
    for (int i = 0; i < kBulkLines; ++i)
    {
        LOG_FMT_DEBUG(logger, "bulk line %d", i);
    }

    logger->delAppender(appender);
    appender.reset();

    std::ifstream ifs(log_file);
    ok &= Check(ifs.is_open(), "open log file");

    const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ok &= Check(content.find("stream message") != std::string::npos, "stream log exists");
    ok &= Check(content.find("format message 42") != std::string::npos, "format log exists");
    ok &= Check(content.find("format message without args") != std::string::npos, "format no args log exists");
    ok &= Check(content.find("bulk line 1999") != std::string::npos, "bulk tail log exists");

    const size_t bulk_count = CountSubString(content, "bulk line ");
    ok &= Check(bulk_count == kBulkLines, "bulk line count equals emitted count");

    std::filesystem::remove(log_file, ec);

    if (!ok)
    {
        return 1;
    }

    std::cout << "[PASS] test_log" << std::endl;
    return 0;
}
