#include "net/http/servlets/StatusServlet.h"
#include <iomanip>
#include "net/Module.h"
#include "base/Singleton.h"
#include "base/Log.h"
#include "core/Worker.h"
#include "base/Utils.h"
#include "core/Daemon.h"
#include "net/web/TcpServer.h"
#include "net/Application.h"
#include "net/http/HttpServer.h"

namespace Gyanis::net::http
{
    // 辅助函数：生成带颜色的标题
    std::string make_title(const std::string& title)
    {
        std::stringstream ss;
        ss << "╔═══════════════════════════════════════════╗\n"
            << "║ " << std::setw(84) << std::left << title << " ║\n"
            << "╚═══════════════════════════════════════════╝";
        return ss.str();
    }

    std::string format_server_status(bool is_running)
    {
        return is_running ? "✅ 运行中" : "❌ 已停止";
    }

    struct ServerInfo
    {
        std::string type;
        std::string name;
        bool ssl;
        std::string address;
        std::string protocol_version;
    };

    ServerInfo parse_server_info(const std::shared_ptr<web::TcpServer>& server)
    {
        ServerInfo info = {};
        std::string raw = server->toString("");

        // 解析类型
        size_t type_pos = raw.find("Type: ");
        if (type_pos != std::string::npos)
        {
            size_t end = raw.find(" | ", type_pos);
            info.type = raw.substr(type_pos + 6, end - (type_pos + 6));
        }

        // 解析SSL状态
        size_t ssl_pos = raw.find("SSL enabled: ");
        if (ssl_pos != std::string::npos)
        {
            info.ssl = (raw.substr(ssl_pos + 13, 4) == "true");
        }

        // 解析地址（取第一个socket地址）
        size_t addr_start = raw.find("[socket]: ");
        if (addr_start != std::string::npos)
        {
            size_t addr_end = raw.find("address", addr_start + 10);
            info.address = raw.substr(addr_start + 10, addr_end - (addr_start + 10));
        }

        // 映射协议版本
        info.protocol_version =
            (info.type == "http")
                ? "HTTP/1.1"
                : (info.type == "https")
                ? "HTTP/2"
                : (info.type == "ws")
                ? "WebSocket"
                : info.type;

        return info;
    }


    std::string format_used_time(int64_t ts)
    {
        std::stringstream ss;
        bool v = false;
        if (ts >= 3600 * 24)
        {
            ss << (ts / 3600 / 24) << "d ";
            ts = ts % (3600 * 24);
            v = true;
        }
        if (ts >= 3600)
        {
            ss << (ts / 3600) << "h ";
            ts = ts % 3600;
            v = true;
        }
        else if (v)
        {
            ss << "0h ";
        }

        if (ts >= 60)
        {
            ss << (ts / 60) << "m ";
            ts = ts % 60;
        }
        else if (v)
        {
            ss << "0m ";
        }
        ss << ts << "s";
        return ss.str();
    }

    StatusServlet::StatusServlet() : Servlet("StatusServlet")
    {
    }

    int32_t
    StatusServlet::handle(const std::shared_ptr<HttpRequest>& request, const std::shared_ptr<HttpResponse>& response,
                          const std::shared_ptr<HttpSession>& session)
    {
        response->setHeader("Content-Type", "text/text; charset=utf-8");
        std::stringstream ss;
        // 在原有代码中修改输出部分：
        ss << make_title("Gyanis Server Status") << "\n\n";

        // 系统信息
        ss << "【系统信息】" "\n";
        ss << std::setw(20) << std::left << "• 服务器版本" ": Gyanis/1.0.0\n";

        std::vector<std::shared_ptr<Module>> ms;
        ModuleMgr::GetInstance()->listAll(ms);
        ss << std::setw(20) << std::left << "• 加载模块" ": ";
        for (size_t i = 0; i < ms.size(); ++i)
        {
            if (i > 0) ss << ", ";
            ss << "[" << (i + 1) << "] " << ms[i]->getId();
        }
        ss << "\n\n";

        // 主机信息
        ss << "【主机信息】" "\n";
        ss << "├─ 主机名" "      : " << base::GetHostName() << "\n";
        ss << "├─ IPv4地址" "    : " << base::GetIPv4() << "\n";
        ss << "├─ 守护进程ID" "  : " << core::ProcessInfoMgr::GetInstance()->parent_id << "\n";
        ss << "└─ 主进程ID" "    : " << core::ProcessInfoMgr::GetInstance()->main_id << "\n\n";

        // 时间信息
        ss << "【运行时间】" "\n";
        auto time_fmt = [](time_t t) { return base::Time2Str(t); };
        ss << "├─ 守护进程启动" " : " << time_fmt(
            core::ProcessInfoMgr::GetInstance()->parent_start_time) << "\n";
        ss << "├─ 主进程启动" "   : " << time_fmt(
            core::ProcessInfoMgr::GetInstance()->main_start_time) << "\n";
        ss << "├─ 运行时长（守护）" " : "
            << format_used_time(time(nullptr) - core::ProcessInfoMgr::GetInstance()->parent_start_time) << "\n";
        ss << "└─ 运行时长（主）" "   : "
            << format_used_time(time(nullptr) - core::ProcessInfoMgr::GetInstance()->main_start_time) << "\n\n";

        // 服务状态
        ss << "【服务状态】" "\n";
        std::unordered_map<std::string, std::vector<std::shared_ptr<web::TcpServer>>> servers;
        Application::GetInstance()->listAllServer(servers);

        for (const auto& [server_type, server_list] : servers)
        {
            ss << "╠═══════════════════════════════════════════╣\n"
                << "║ " << std::setw(88) << std::left << ("服务类型: " + server_type) << " ║\n"
                << "╚═══════════════════════════════════════════╝" "\n";

            std::shared_ptr<HttpServer> hs;
            for (size_t i = 0; i < server_list.size(); ++i)
            {
                const auto& server = server_list[i];
                ServerInfo si = parse_server_info(server);

                ss << "  ■ 实例 #" << (i + 1) << "\n";
                ss << "  ├─ 监听地址" "   : " << (si.address.empty() ? "N/A" : si.address);
                if (si.ssl) ss << " 🔒";
                ss << "\n";
                ss << "  ├─ 协议版本" "   : " << si.protocol_version << "\n";
                ss << "  ├─ 服务名称" "   : " << (server_list[i]->getName().empty() ? "未命名服务" : server_list[i]->getName())
                    << "\n";
                ss << "  ├─ 运行状态" "   : "
                    << (!server->isStop() ? "✅ 运行中" : "❌ 已停止") << "\n";
                ss << "  └─ 工作线程" "   : " << 10 << "\n";

                if (!hs)
                {
                    hs = std::dynamic_pointer_cast<HttpServer>(server);
                }
            }

            if (hs && hs->getServletDispatch())
            {
                auto sd = hs->getServletDispatch();
                std::unordered_map<std::string, std::shared_ptr<IServletCreator>> infos;

                // 精确路由
                sd->listAllServletCreator(infos);
                if (!infos.empty())
                {
                    ss << "\n  [精确路由]" "\n";
                    for (const auto& [pattern, creator] : infos)
                    {
                        ss << "  ├─ " << std::setw(35) << std::left << pattern
                            << " ⇒ " << creator->getName() << "\n";
                    }
                    infos.clear();
                }

                // 通配路由
                sd->listGlobalServletCreator(infos);
                if (!infos.empty())
                {
                    ss << "\n  [通配路由]" "\n";
                    for (const auto& [pattern, creator] : infos)
                    {
                        ss << "  ├─ " << std::setw(35) << std::left << pattern
                            << " ⇒ " << creator->getName() << "\n";
                    }
                }
            }
        }

        // 模块状态
        if (!ms.empty())
        {
            ss <<
                "\n╔═══════════════════════════════════════════╗\n"
                << "║ 模块运行状态 (" << ms.size() << ")                 ║\n"
                << "╚═══════════════════════════════════════════╝" "\n";

            for (size_t i = 0; i < ms.size(); ++i)
            {
                ss << "  ■ 模块 #" << (i + 1) << ": " << ms[i]->getId() << "\n";
                ss << ms[i]->statusString() << "\n\n";
            }
        }

        response->setBody(ss.str());
        return 0;
    }
}
