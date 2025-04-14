#include <unistd.h>
#include <iostream>
#include <fstream>
#include "core/Env.h"

using namespace Gyanis::core;


struct A
{
    A()
    {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        for (size_t i = 0; i < content.size(); ++i)
        {
            std::cout << i << " - " << content[i] << " - " << static_cast<int>(content[i]) << std::endl;
        }
    }
};

A a;

int main(const int argc, char** argv)
{
    std::cout << "argc=" << argc << std::endl;
    EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    EnvMgr::GetInstance()->addHelp("p", "print help");
    if (!EnvMgr::GetInstance()->init(argc, argv))
    {
        EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << Env::getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << Env::getEnv("TEST", "") << std::endl;
    std::cout << "set env " << Env::setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << Env::getEnv("TEST", "") << std::endl;
    if (EnvMgr::GetInstance()->has("p"))
    {
        EnvMgr::GetInstance()->printHelp();
    }
}
