#include "net/Application.h"
#include <ctime>
#include <iostream>
#include <random>

void setupTimezone()
{
    setenv("TZ", ":/etc/localtime", 1);
    tzset();
}

int main(const int argc, char** argv)
{
    setupTimezone();

    // 使用 std::random_device 生成一个更不可预测的种子
    std::random_device rd;
    std::srand(rd()); // 使用 random_device 来设置种子

    // 初始化并运行应用程序
    if (const auto app = std::make_shared<Gyanis::net::Application>(); app->init(argc, argv))
    {
        return app->run(); // 返回应用程序的运行状态
    }
    std::cerr << "Application initialization failed." << std::endl;
    return 0; // 返回非零值表示错误
}
