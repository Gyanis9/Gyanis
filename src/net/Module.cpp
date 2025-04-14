#include "net/Module.h"
#include "base/Config.h"
#include "net/Application.h"
#include "core/Env.h"
#include "net/Library.h"

namespace Gyanis::net
{
    static auto g_module_path
        = base::Config::LookUp<std::string>("module.path", "module", "Module file path");
    static auto g_logger = LOG_NAME("system");

    Module::Module(const std::string& name, const std::string& version, std::string filename, const uint32_t type)
        : m_name(name), m_version(version), m_filename(std::move(filename)), m_id(name + "/" + version), m_type(type)
    {
    }

    void Module::onBeforeArgsParse(int argc, char** argv)
    {
    }

    void Module::onAfterArgsParse(int argc, char** argv)
    {
    }

    bool Module::onLoad()
    {
        return true;
    }

    bool Module::onUnload()
    {
        return true;
    }

    bool Module::onConnect(const std::shared_ptr<stream::Stream>& stream)
    {
        return true;
    }

    bool Module::onDisconnect(const std::shared_ptr<stream::Stream>& stream)
    {
        return true;
    }

    bool Module::onServerReady()
    {
        return true;
    }

    bool Module::onServerUp()
    {
        return true;
    }

    bool
    Module::handleRequest(const std::shared_ptr<protocol::Message>& request,
                          const std::shared_ptr<protocol::Message>& response,
                          const std::shared_ptr<stream::Stream>& stream)
    {
        LOG_DEBUG(g_logger)
            << "Module::handleRequest - Handling request. "
            << "Request details: " << request->toString()
            << " | Response details: " << response->toString()
            << " | Stream ID: " << stream;
        return true;
    }

    bool Module::handleNotify(const std::shared_ptr<protocol::Message>& notify,
                              const std::shared_ptr<stream::Stream>& stream)
    {
        LOG_DEBUG(g_logger)
            << "Module::handleNotify - Handling notification. "
            << "Notification details: " << notify->toString()
            << " | Stream ID: " << stream;
        return true;
    }

    std::string Module::statusString()
    {
        std::stringstream ss;
        ss << "Module Information: "
            << "Name: " << getName()
            << " | Version: " << getVersion()
            << " | Filename: " << getFilename()
            << std::endl;
        return ss.str();
    }

    const std::string& Module::getName() const
    {
        return m_name;
    }

    const std::string& Module::getVersion() const
    {
        return m_version;
    }

    const std::string& Module::getFilename() const
    {
        return m_filename;
    }

    const std::string& Module::getId() const
    {
        return m_id;
    }

    void Module::setFilename(const std::string& value)
    {
        m_filename = value;
    }

    uint32_t Module::getType() const
    {
        return m_type;
    }

    void Module::registerService(const std::string& server_type, const std::string& domain, const std::string& service)
    {
        //TODO
    }

    ModuleManager::ModuleManager() = default;

    void ModuleManager::add(const std::shared_ptr<Module>& value)
    {
        del(value->getId());
        std::unique_lock lock(m_mutex);
        m_modules[value->getId()] = value;
        m_type2Modules[value->getType()][value->getId()] = value;
    }

    void ModuleManager::del(const std::string& name)
    {
        std::unique_lock lock(m_mutex);
        const auto it = m_modules.find(name);
        if (it == m_modules.end())
        {
            return;
        }
        const std::shared_ptr<Module> module = it->second;
        m_modules.erase(it);
        m_type2Modules[module->getType()].erase(module->getId());
        if (m_type2Modules[module->getType()].empty())
        {
            m_type2Modules.erase(module->getType());
        }
        lock.unlock();
        module->onUnload();
    }

    void ModuleManager::delAll()
    {
        std::shared_lock lock(m_mutex);
        const auto tmp = m_modules;
        lock.unlock();

        for (const auto& [fst, snd] : tmp)
        {
            del(fst);
        }
    }

    void ModuleManager::init()
    {
        const auto path = core::EnvMgr::GetInstance()->getAbsolutePath(g_module_path->getValue());

        std::vector<std::string> files;
        base::FSUtil::ListAllFile(files, path, ".so");

        std::sort(files.begin(), files.end());
        for (auto& i : files)
        {
            initModule(i);
        }
    }

    std::shared_ptr<Module> ModuleManager::get(const std::string& name)
    {
        std::shared_lock lock(m_mutex);
        const auto it = m_modules.find(name);
        return it == m_modules.end() ? nullptr : it->second;
    }

    void ModuleManager::onConnect(const std::shared_ptr<stream::Stream>& stream)
    {
        std::vector<std::shared_ptr<Module>> ms;
        listAll(ms);

        for (const auto& module : ms)
        {
            module->onConnect(stream);
        }
    }

    void ModuleManager::onDisconnect(const std::shared_ptr<stream::Stream>& stream)
    {
        std::vector<std::shared_ptr<Module>> ms;
        listAll(ms);

        for (const auto& module : ms)
        {
            module->onDisconnect(stream);
        }
    }

    void ModuleManager::listAll(std::vector<std::shared_ptr<Module>>& ms)
    {
        std::shared_lock lock(m_mutex);
        for (auto& [fst, snd] : m_modules)
        {
            ms.push_back(snd);
        }
    }

    void ModuleManager::listByType(const uint32_t type, std::vector<std::shared_ptr<Module>>& ms)
    {
        std::shared_lock lock(m_mutex);
        const auto it = m_type2Modules.find(type);
        if (it == m_type2Modules.end())
        {
            return;
        }
        for (auto& [fst, snd] : it->second)
        {
            ms.push_back(snd);
        }
    }

    void ModuleManager::foreach(const uint32_t type, const std::function<void(std::shared_ptr<Module>)>& cb)
    {
        std::vector<std::shared_ptr<Module>> ms;
        listByType(type, ms);
        for (const auto& module : ms)
        {
            cb(module);
        }
    }

    void ModuleManager::initModule(const std::string& path)
    {
        if (const auto value = Library::GetModule(path))
        {
            add(value);
        }
    }
}
