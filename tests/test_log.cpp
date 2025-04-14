#include "base/Log.h"

using namespace Gyanis::base;


void test1()
{
    auto g_logger = LOG_NAME("test");
    LOG_INFO(LOG_ROOT()) << "this is a root test";
    LOG_INFO(g_logger) << "this is a temp test";
}

void test2()
{
    auto g_logger = LOG_NAME("test");
    const auto Appender = std::make_shared<StdoutLogAppender>();
    g_logger->addAppender(Appender);
    LOG_INFO(g_logger) << "this is a test";
    g_logger->setLevel(LogLevel::FATAL);
    LOG_INFO(g_logger) << "this is a test";
    g_logger->setFormatter("%d{%H:%M:%S} %T %m");
    LOG_FATAL(g_logger) << "this is a change test";
}

void test3()
{
    auto g_logger = LOG_NAME("test");
    const auto fileAppender = std::make_shared<FileLogAppender>("logs/test.log");
    g_logger->addAppender(fileAppender);
    LOG_INFO(g_logger) << "this is a test";
}

void test4()
{
    auto g_logger = LOG_NAME("test");
    LOG_INFO(g_logger) << g_logger->toYamlString();
}

int main()
{
    // test1();
    // test2();
    // test3();
    test4();
}
