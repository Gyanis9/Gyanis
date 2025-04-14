#include <csignal>
#include <memory>

#include "net/Application.h"
#include "core/Env.h"
#include "core/Daemon.h"
#include "core/Worker.h"
#include "net/http/HttpServer.h"
#include "net/http/WebSocketServer.h"
#include "net/Module.h"

namespace Gyanis::net
{
    static auto g_logger = LOG_NAME("system");

    static auto g_server_work_path =
        base::Config::LookUp("server.work_path", std::string("work"), "Server Working Directory Path");

    static auto g_server_pid_file =
        base::Config::LookUp("server.pid_file", std::string("base.pid"), "Server Process ID (PID) File");

    static auto g_servers_conf
        = base::Config::LookUp("servers", std::vector<base::TcpServerConf>(), "HTTP Server Configuration");

    Application* Application::s_instance = nullptr;

    Application::Application()
    {
        s_instance = this;
    }

    Application* Application::GetInstance()
    {
        return s_instance;
    }

    bool Application::init(const int argc, char** argv)
    {
        m_argc = argc;
        m_argv = argv;

        core::EnvMgr::GetInstance()->addHelp("s", "Start from the terminal");
        core::EnvMgr::GetInstance()->addHelp("d", "Run as a daemon process");
        core::EnvMgr::GetInstance()->addHelp("c", "Default configuration path: config");
        core::EnvMgr::GetInstance()->addHelp("p", "Display help information");

        bool is_print_help = false;
        if (!core::EnvMgr::GetInstance()->init(argc, argv))
        {
            is_print_help = true;
        }

        if (core::EnvMgr::GetInstance()->has("p"))
        {
            is_print_help = true;
        }

        const std::string config_path = core::EnvMgr::GetInstance()->getConfigPath();
        LOG_INFO(g_logger)
            << "Application::init - Loading configuration from path: " << config_path
            << " | Status: Normal";
        base::Config::LoadFromConfigDir(config_path);

        ModuleMgr::GetInstance()->init();
        std::vector<std::shared_ptr<Module>> modules;
        ModuleMgr::GetInstance()->listAll(modules);

        for (const auto& i : modules)
        {
            i->onBeforeArgsParse(argc, argv);
        }

        if (is_print_help)
        {
            core::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        for (const auto& i : modules)
        {
            i->onAfterArgsParse(argc, argv);
        }
        modules.clear();

        int run_type = 0;
        if (core::EnvMgr::GetInstance()->has("s"))
        {
            run_type = 1;
        }
        if (core::EnvMgr::GetInstance()->has("d"))
        {
            run_type = 2;
        }

        if (run_type == 0)
        {
            core::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        if (const std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
            base::FSUtil::IsRunningPidfile(pidfile))
        {
            LOG_FATAL(g_logger)
                << "Application::init - Server is currently running. "
                << "Process ID file: " << pidfile;
            return false;
        }

        if (!base::FSUtil::Mkdir(g_server_work_path->getValue()))
        {
            LOG_FATAL(g_logger)
                << "Application::init - Failed to create working directory at path: [" << g_server_work_path->getValue()
                << "] | Error code: " << errno
                << " | Error description: " << strerror(errno);
            return false;
        }
        return true;
    }

    bool Application::run()
    {
        const bool is_daemon = core::EnvMgr::GetInstance()->has("d");
        return core::start_daemon(m_argc, m_argv,
                                  [this](const int argc, char** argv) { return this->main(argc, argv); },
                                  is_daemon);
    }


    bool Application::getServer(const std::string& type, std::vector<std::shared_ptr<web::TcpServer>>& servers)
    {
        const auto it = m_servers.find(type);
        if (it == m_servers.end())
        {
            return false;
        }
        servers = it->second;
        return true;
    }

    void
    Application::listAllServer(
        std::unordered_map<std::string, std::vector<std::shared_ptr<web::TcpServer>>>& servers) const
    {
        servers = m_servers;
    }

    int Application::main(int, char**)
    {
        try
        {
            signal(SIGPIPE, SIG_IGN);
            LOG_INFO(g_logger) << "The main process is starting.";
            const std::string conf_path = core::EnvMgr::GetInstance()->getConfigPath();
            base::Config::LoadFromConfigDir(conf_path, true);
            {
                const std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
                std::ofstream ofs(pidfile);
                if (!ofs)
                {
                    LOG_ERROR(g_logger)
                        << "Application::main - Failed to open PID file: " << pidfile;
                    return false;
                }
                ofs << getpid();
            }

            m_mainIOManager = std::make_shared<core::IOManager>(1, "main");
            m_mainIOManager->schedule([&] { return run_fiber(); });
            m_mainIOManager->addTimer(2000, []()
            {
            }, true);
            m_mainIOManager->stop();
        }
        catch (std::exception& e)
        {
            LOG_ERROR(g_logger) << "Application::main - failed. "
                << "Error details: " << e.what();
            return -1;
        }
        return 0;
    }

    int Application::run_fiber()
    {
        std::vector<std::shared_ptr<Module>> modules;
        ModuleMgr::GetInstance()->listAll(modules);
        bool has_error = false;
        for (const auto& i : modules)
        {
            if (!i->onLoad())
            {
                LOG_ERROR(g_logger)
                    << "Application::run_fiber - Module information: "
                    << "Name: " << i->getName()
                    << " | Version: " << i->getVersion()
                    << " | Filename: " << i->getFilename();
                has_error = true;
            }
        }
        if (has_error)
        {
            _exit(0);
        }

        if (core::WorkerMgr::GetInstance()->init())
        {
            LOG_ERROR(g_logger)
                << "Application::run_fiber - Worker manager initialization failed. ";
        };

        auto http_confs = g_servers_conf->getValue();
        std::vector<std::shared_ptr<web::TcpServer>> servers;
        for (auto& value : http_confs)
        {
            LOG_DEBUG(g_logger) << std::endl << base::LexicalCast<base::TcpServerConf, std::string>()(value);

            std::vector<std::shared_ptr<Address>> address;
            for (auto& host : value.address)
            {
                const size_t pos = host.find(":");
                if (pos == std::string::npos)
                {
                    address.push_back(std::make_shared<UnixAddress>(host));
                    continue;
                }
                const auto port = std::strtol(host.substr(pos + 1).c_str(), nullptr, 10);

                if (port == 0 && host.substr(pos + 1) != "0")
                {
                    LOG_ERROR(g_logger) << "Application::run_fiber - Invalid port number. ";
                }

                if (auto addr = IPAddress::Create(host.substr(0, pos).c_str(), port))
                {
                    address.push_back(addr);
                    continue;
                }
                if (std::vector<std::pair<std::shared_ptr<Address>, uint32_t>> result;
                    Address::GetInterfaceAddresses(result, host.substr(0, pos)))
                {
                    for (auto& [fst, snd] : result)
                    {
                        auto ipaddr = std::dynamic_pointer_cast<IPAddress>(fst);
                        if (ipaddr)
                        {
                            if (const long port1 = std::strtol(host.substr(pos + 1).c_str(), nullptr, 10); port1 > 0 &&
                                port1 <= std::numeric_limits<int32_t>::max())
                            {
                                ipaddr->setPort(static_cast<int32_t>(port1)); // 设置端口号
                            }
                            else
                            {
                                LOG_ERROR(g_logger) << "Application::run_fiber - Invalid port number. ";
                            }
                        }
                        address.push_back(ipaddr);
                    }
                    continue;
                }

                if (auto aaddr = Address::LookupAny(host))
                {
                    address.push_back(aaddr);
                    continue;
                }
                LOG_ERROR(g_logger) << "Application::run_fiber - Invalid address provided. " << "Address: " << host;
                _exit(0);
            }
            auto accept_worker = core::IOManager::GetThis();
            auto io_worker = core::IOManager::GetThis();
            auto process_worker = core::IOManager::GetThis();
            if (!value.accept_worker.empty())
            {
                accept_worker = core::WorkerMgr::GetInstance()->getAsIOManager(value.accept_worker).get();
                if (!accept_worker)
                {
                    LOG_ERROR(g_logger) << "Application::run_fiber - accept_worker: " << value.accept_worker
                    << " does not exist.";
                    _exit(0);
                }
            }
            if (!value.io_worker.empty())
            {
                io_worker = core::WorkerMgr::GetInstance()->getAsIOManager(value.io_worker).get();
                if (!io_worker)
                {
                    LOG_ERROR(g_logger)
                        << "Application::run_fiber - io_worker: " << value.io_worker
                        << " does not exist.";
                    _exit(0);
                }
            }
            if (!value.process_worker.empty())
            {
                process_worker = core::WorkerMgr::GetInstance()->getAsIOManager(value.process_worker).get();
                if (!process_worker)
                {
                    LOG_ERROR(g_logger) << "Application::run_fiber - process_worker: " << value.process_worker <<
 " not exists";
                    _exit(0);
                }
            }
            std::shared_ptr<web::TcpServer> server;
            if (value.type == "http")
            {
                server = std::make_shared<http::HttpServer>(value.keepalive,
                                                            process_worker, io_worker, accept_worker);
            }
            else if (value.type == "ws")
            {
                server = std::make_shared<http::WSServer>(process_worker, io_worker, accept_worker);
            }
            else
            {
                LOG_ERROR(g_logger)
                    << "Application::run_fiber - Invalid server type: " << value.type
                    << " | Server configuration details: "
                    << base::LexicalCast<base::TcpServerConf, std::string>()(value);
                _exit(0);
            }
            if (!value.name.empty())
            {
                server->setName(value.name);
            }
            if (std::vector<std::shared_ptr<Address>> fails; !server->bind(address, fails, value.ssl))
            {
                for (const auto& x : fails)
                {
                    LOG_ERROR(g_logger)
                        << "Application::run_fiber - Failed to bind address: "
                        << x->toString();
                }
                _exit(0);
            }
            if (value.ssl)
            {
                if (!server->loadCertificates(value.cert_file, value.key_file))
                {
                    LOG_ERROR(g_logger)
                        << "Application::run_fiber - Failed to load certificates. "
                        << "Certificate file: " << value.cert_file
                        << " | Key file: " << value.key_file;
                }
            }
            server->setConf(value);
            m_servers[value.type].push_back(server);
            servers.push_back(server);
        }

        for (const auto& i : modules)
        {
            i->onServerReady();
        }

        for (const auto& i : servers)
        {
            i->start();
        }

        for (const auto& i : modules)
        {
            i->onServerUp();
        }
        return 0;
    }
}
