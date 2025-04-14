#include "core/Env.h"
#include "base/Log.h"
#include "base/Config.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <optional>
#include <sys/ioctl.h>
#include <locale>
#include <codecvt>

namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");

    bool Env::init(const int argc, char** argv)
    {
        try
        {
            m_exe = std::filesystem::read_symlink("/proc/self/exe").string();
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR(g_logger)
                << "Env::init - Failed to read symbolic link for executable path. "
                << "Error details: " << e.what();
            return false;
        }

        // 获取当前程序的工作目录
        const auto pos = m_exe.find_last_of('/');
        m_cwd = pos != std::string::npos ? m_exe.substr(0, pos + 1) : "";

        // 获取程序的名称
        m_program = argv[0];

        std::optional<std::string> now_key;

        // 处理命令行参数
        for (int i = 1; i < argc; ++i)
        {
            if (const char* arg = argv[i]; arg[0] == '-')
            {
                // 参数为单独的选项（如 "-key"）
                if (strlen(arg) > 1)
                {
                    if (now_key)
                    {
                        add(*now_key, "");
                    }
                    now_key = arg + 1; // 去掉前导的 '-'
                }
                else
                {
                    LOG_ERROR(g_logger)
                        << "Env::init - Invalid argument encountered at index " << i
                        << ". Argument value: " << arg;
                    return false;
                }
            }
            else
            {
                // 参数为值（如 "-key value"）
                if (now_key)
                {
                    add(*now_key, arg);
                    now_key.reset();
                }
                else
                {
                    LOG_ERROR(g_logger)
                        << "Env::init - Invalid argument encountered at index " << i
                        << ". Argument value: " << arg;
                    return false;
                }
            }
        }

        // 处理最后一个未完成的参数（如果有）
        if (now_key)
        {
            add(*now_key, "");
        }

        return true;
    }

    void Env::add(const std::string& key, const std::string& value)
    {
        std::unique_lock lock(m_mutex);
        m_args[key] = value;
    }

    bool Env::has(const std::string& key)
    {
        std::shared_lock lock(m_mutex);
        const auto it = m_args.find(key);
        return it != m_args.end();
    }

    void Env::del(const std::string& key)
    {
        std::unique_lock lock(m_mutex);
        m_args.erase(key);
    }

    std::string Env::get(const std::string& key, const std::string& default_value)
    {
        std::shared_lock lock(m_mutex);
        const auto it = m_args.find(key);
        return it != m_args.end() ? it->second : default_value;
    }

    void Env::addHelp(const std::string& key, const std::string& desc)
    {
        removeHelp(key);
        std::unique_lock lock(m_mutex);
        m_helps.emplace_back(key, desc);
    }

    void Env::removeHelp(const std::string& key)
    {
        std::unique_lock lock(m_mutex);
        for (auto it = m_helps.begin(); it != m_helps.end();)
        {
            if (it->first == key)
            {
                m_helps.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Env::printHelp()
    {
        std::shared_lock lock(m_mutex);

        // 定义ANSI颜色代码
        constexpr auto RESET = "\033[0m";

        try
        {
            constexpr auto YELLOW = "\033[1;33m";
            constexpr auto GREEN = "\033[1;32m";
            constexpr auto CYAN = "\033[1;36m";
            // 获取终端宽度
            winsize w{};
            size_t console_width = 80;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
            {
                console_width = w.ws_col;
            }

            std::cout << "\n" << CYAN << m_program << " - Usage" << RESET << "\n\n";

            std::cout << "Usage: " << YELLOW << m_program << RESET << " [options]\n\n";

            // 选项表头
            std::cout << GREEN << std::left << std::setw(20) << "OPTION"
                << "DESCRIPTION" << RESET << "\n";
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::cout << converter.to_bytes(std::wstring(console_width, L'\u2500')) << "\n";

            // 打印选项详情
            for (const auto& [option, desc] : m_helps)
            {
                constexpr auto MAGENTA = "\033[1;35m";
                constexpr size_t OPTION_COL_WIDTH = 20;
                // 选项列（带颜色）
                std::cout << MAGENTA << std::left << std::setw(OPTION_COL_WIDTH)
                    << ("-" + option) << RESET;

                // 智能换行描述
                size_t current_pos = OPTION_COL_WIDTH;
                std::istringstream iss(desc);
                std::string word;

                while (iss >> word)
                {
                    if (const size_t space_left = console_width - current_pos; word.length() > space_left)
                    {
                        std::cout << "\n" << std::string(OPTION_COL_WIDTH, ' ');
                        current_pos = OPTION_COL_WIDTH;
                    }
                    std::cout << word << " ";
                    current_pos += word.length() + 1;
                }
                std::cout << "\n";
            }
            std::cout << std::endl;
        }
        catch (...)
        {
            std::cout << RESET; // 确保异常时重置颜色
            throw;
        }
    }

    std::string_view Env::getExe() const
    {
        return m_exe;
    }

    std::string Env::getCwd() const
    {
        return m_cwd;
    }

    bool Env::setEnv(const std::string& key, const std::string& value)
    {
        return !setenv(key.c_str(), value.c_str(), 1);
    }

    std::string Env::getEnv(const std::string& key, const std::string& default_value)
    {
        const char* value = getenv(key.c_str());
        if (value == nullptr)
        {
            return default_value;
        }
        return value;
    }

    std::string Env::getAbsolutePath(const std::string& path) const
    {
        if (path.empty())
        {
            return "/";
        }
        if (path[0] == '/')
        {
            return path;
        }
        return m_cwd + path;
    }

    std::string Env::getAbsoluteWorkPath(const std::string& path)
    {
        if (path.empty())
        {
            return "/";
        }
        if (path[0] == '/')
        {
            return path;
        }
        const auto g_server_work_path = base::Config::LookUp<std::string>("server.work_path");
        return g_server_work_path->getValue() + '/' + path;
    }

    std::string Env::getConfigPath()
    {
        const auto& config = get("c", "config");
        return getAbsolutePath(config);
    }
}
