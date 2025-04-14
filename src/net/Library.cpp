#include <dlfcn.h>
#include "net/Library.h"
#include "base/Log.h"
#include "base/Config.h"
#include "core/Env.h"

namespace Gyanis::net
{
    static auto g_logger = LOG_NAME("system");

    typedef Module*(*create_module)();

    typedef void (*destory_module)(Module*);

    class ModuleCloser
    {
    public:
        ModuleCloser(void* handle, const destory_module destory)
            : m_handle(handle), m_destory(destory)
        {
        }

        void operator()(Module* module) const
        {
            const std::string name = module->getName();
            const std::string version = module->getVersion();
            const std::string path = module->getFilename();
            m_destory(module);
            if (const int result = dlclose(m_handle); result)
            {
                LOG_ERROR(g_logger)
                    << "ModuleCloser - dlclose failed. "
                    << "Handle: " << m_handle
                    << " | Name: " << name
                    << " | Version: " << version
                    << " | Path: " << path
                    << " | Error: " << dlerror();
            }
            else
            {
                LOG_INFO(g_logger)
                    << "ModuleCloser - Module destroyed successfully. "
                    << "Name: " << name
                    << " | Version: " << version
                    << " | Path: " << path
                    << " | Handle: " << m_handle;
            }
        }

    private:
        void* m_handle;
        destory_module m_destory;
    };

    std::shared_ptr<Module> Library::GetModule(const std::string& path)
    {
        void* handle = dlopen(path.c_str(), RTLD_NOW);
        if (!handle)
        {
            LOG_ERROR(g_logger)
                << "Library::GetModule - Failed to load library. "
                << "Library path: " << path
                << " | Error: " << dlerror();
            return nullptr;
        }

        const auto create = reinterpret_cast<create_module>(dlsym(handle, "CreateModule"));
        if (!create)
        {
            LOG_ERROR(g_logger)
                << "Library::GetModule - Failed to load symbol 'CreateModule' in library. "
                << "Library path: " << path
                << " | Error: " << dlerror();
            dlclose(handle);
            return nullptr;
        }

        const auto destory = reinterpret_cast<destory_module>(dlsym(handle, "DestoryModule"));
        if (!destory)
        {
            LOG_ERROR(g_logger)
                << "Library::GetModule - Failed to load symbol 'DestroyModule' in library. "
                << "Library path: " << path
                << " | Error: " << dlerror();
            dlclose(handle);
            return nullptr;
        }

        std::shared_ptr<Module> module(create(), ModuleCloser(handle, destory));
        module->setFilename(path);
        LOG_INFO(g_logger)
            << "Library::GetModule - Module loaded successfully. "
            << "Name: " << module->getName()
            << " | Version: " << module->getVersion()
            << " | Path: " << module->getFilename();
        base::Config::LoadFromConfigDir(core::EnvMgr::GetInstance()->getConfigPath(), true);
        return module;
    }
}
