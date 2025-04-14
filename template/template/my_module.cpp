#include "my_module.h"
#include "base/Config.h"
#include "base/Log.h"

namespace name_space
{
    static auto g_logger = LOG_NAME("system");

    MyModule::MyModule(): Module("project_name", "1.0", "")
    {
    }

    bool MyModule::onLoad()
    {
        LOG_INFO(g_logger) << "onLoad";
        return true;
    }

    bool MyModule::onUnload()
    {
        LOG_INFO(g_logger) << "onUnload";
        return true;
    }

    bool MyModule::onServerReady()
    {
        LOG_INFO(g_logger) << "onServerReady";
        return true;
    }

    bool MyModule::onServerUp()
    {
        LOG_INFO(g_logger) << "onServerUp";
        return true;
    }
}

extern "C" {
Gyanis::net::Module* CreateModule()
{
    Gyanis::net::Module* module = new name_space::MyModule;
    LOG_INFO(name_space::g_logger) << "CreateModule " << module;
    return module;
}

void DestoryModule(const Gyanis::net::Module* module)
{
    LOG_INFO(name_space::g_logger) << "CreateModule " << module;
    delete module;
}
}
