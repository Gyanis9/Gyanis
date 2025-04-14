#include "base/Config.h"
#include "base/Log.h"
using namespace Gyanis::base;


void testConfigVar()
{
    const auto intConfig = Config::LookUp<int>("my_int", 42, "一个整数配置项");

    LOG_INFO(LOG_ROOT()) << "初始值: " << intConfig->getValue();

    intConfig->addListener([](const int& old_value, const int& new_value)
    {
        LOG_INFO(LOG_ROOT()) << "配置变化------> 旧值:" << old_value << " 新值:" << new_value;
    });
    intConfig->setValue(100);
    LOG_INFO(LOG_ROOT()) << "更新后的值:" << intConfig->getValue();
    intConfig->setValue(42);
}

void testConfigVarWithTypes()
{
    const auto vectorConfig = Config::LookUp<std::vector<int>>("my_vector", {1, 2, 3}, "一个 vector 配置项");
    LOG_INFO(LOG_ROOT()) << "初始 vector 值: " << vectorConfig->toString();

    vectorConfig->setValue({4, 5, 6});
    LOG_INFO(LOG_ROOT()) << "更新后的 vector 值: " << vectorConfig->toString();

    const auto mapConfig = Config::LookUp<std::map<std::string, int>>("my_map", {{"key1", 10}, {"key2", 20}},
                                                                      "一个 map 配置项");
    LOG_INFO(LOG_ROOT()) << "初始 map 值: " << mapConfig->toString();

    mapConfig->setValue({{"key1", 100}, {"key2", 200}});
    LOG_INFO(LOG_ROOT()) << "更新后的 map 值: " << mapConfig->toString();
}

void testLexicalCast()
{
    const std::string yamlString = "[1, 2, 3, 4]";
    LexicalCast<std::string, std::vector<int>> cast;
    const std::vector<int> result = cast(yamlString);
    LOG_INFO(LOG_ROOT()) << "从 YAML 字符串解析的 vector: ";
    for (const int num : result)
    {
        LOG_INFO(LOG_ROOT()) << num << " ";
    }
}

void testLoadFromConfigDir()
{
    const std::string configDir = "./config";

    Config::LoadFromConfigDir(configDir);
}


int main()
{
    // testConfigVar();

    // testConfigVarWithTypes();

    // testLexicalCast();

    testLoadFromConfigDir();

    return 0;
}
