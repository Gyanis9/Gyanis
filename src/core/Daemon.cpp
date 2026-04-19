#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "core/Daemon.h"
#include "base/Config.h"
#include "base/Log.h"
#include "base/Utils.h"

namespace Gyanis::core
{
    namespace
    {
        [[nodiscard]] pid_t GetCurrentProcessId()
        {
#if defined(_WIN32)
            return static_cast<pid_t>(_getpid());
#else
            return getpid();
#endif
        }

        void SleepSeconds(const uint32_t seconds)
        {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
        }
    }

    static auto g_logger                  = LOG_NAME("system");
    auto        g_daemon_restart_interval = base::Config::LookUp<uint32_t>("daemon.restart_interval", 5,
                                                                    "daemon restart interval");

    std::string ProcessInfo::toString() const
    {
        std::stringstream ss;
        ss << "\n[进程信息"
                << "\n父进程ID: " << parent_id
                << "\n主进程ID: " << main_id
                << "\n父进程启动时间: " << base::Time2Str(parent_start_time)
                << "\n主进程启动时间: " << base::Time2Str(main_start_time)
                << "\n重启次数: " << restart_count
                << "\n]";
        return ss.str();
    }


    static int real_start(const int             argc, char **argv,
                          const DaemonCallback &main_cb)
    {
        ProcessInfoMgr::GetInstance()->main_id         = GetCurrentProcessId();
        ProcessInfoMgr::GetInstance()->main_start_time = time(nullptr);
        return main_cb(argc, argv);
    }


    static int real_daemon(const int argc, char **argv, const DaemonCallback &main_cb)
    {
#if defined(_WIN32)
        LOG_WARN(g_logger)
            << "[守护进程] Windows 平台当前使用回退模式，将以前台方式运行主回调。";
        ProcessInfoMgr::GetInstance()->parent_id         = GetCurrentProcessId();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(nullptr);
        return real_start(argc, argv, main_cb);
#else
        // 将当前进程转为守护进程，并忽略控制终端
        if (daemon(1, 0) == -1)
        {
            LOG_ERROR(g_logger)
                << "[守护进程] 初始化失败，无法启动 daemon 模式。"
                << " errno=" << errno << "，错误=" << std::strerror(errno);
            return -1;
        }

        // 记录当前进程（父进程）的 PID 和启动时间
        ProcessInfoMgr::GetInstance()->parent_id         = GetCurrentProcessId();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(nullptr);

        // 进入一个持续运行的循环，不断检查子进程的状态
        while (true)
        {
            // 创建一个子进程
            const pid_t pid = fork();

            // 如果是子进程，执行以下操作
            if (pid == 0)
            {
                // 记录子进程的 PID 和启动时间
                ProcessInfoMgr::GetInstance()->main_id         = GetCurrentProcessId();
                ProcessInfoMgr::GetInstance()->main_start_time = time(nullptr);

                // 输出日志，表示子进程已经启动
                LOG_INFO(g_logger)
                    << "[守护进程] 子进程启动成功，PID=" << GetCurrentProcessId();

                // 调用 real_start 函数来执行传入的回调函数
                return real_start(argc, argv, main_cb);
            }

            // 如果 fork 失败，输出错误信息并退出
            if (pid < 0)
            {
                LOG_ERROR(g_logger)
                    << "[守护进程] fork 失败，返回值=" << pid
                    << "，errno=" << errno
                    << "，错误=" << std::strerror(errno);

                return -1;
            }

            // 父进程等待子进程退出
            int         status   = 0;
            const pid_t wait_ret = waitpid(pid, &status, 0);
            if (wait_ret < 0)
            {
                LOG_ERROR(g_logger)
                    << "[守护进程] waitpid 失败，pid=" << pid
                    << "，errno=" << errno
                    << "，错误=" << std::strerror(errno);
                return -1;
            }

            // 如果子进程异常退出，进行处理
            if (WIFSIGNALED(status))
            {
                const int signal_no = WTERMSIG(status);
                if (signal_no == SIGKILL)
                {
                    LOG_INFO(g_logger)
                        << "[守护进程] 子进程收到 SIGKILL，停止重启循环。";
                    break;
                }

                LOG_ERROR(g_logger)
                    << "[守护进程] 子进程被信号终止，pid=" << pid
                    << "，signal=" << signal_no;
            } else if (WIFEXITED(status))
            {
                const int exit_code = WEXITSTATUS(status);
                if (exit_code == 0)
                {
                    LOG_INFO(g_logger)
                        << "[守护进程] 子进程正常退出，pid=" << pid;
                    break;
                }

                LOG_ERROR(g_logger)
                    << "[守护进程] 子进程异常退出，pid=" << pid
                    << "，退出码=" << exit_code;
            } else
            {
                LOG_INFO(g_logger)
                    << "[守护进程] 子进程状态未知，pid=" << pid
                    << "，status=" << status;
            }

            // 如果子进程崩溃或被杀死，增加重启计数
            ProcessInfoMgr::GetInstance()->restart_count += 1;

            // 等待一段时间后重启子进程
            SleepSeconds(g_daemon_restart_interval->getValue());
        }
        return 0;
#endif
    }


    int start_daemon(const int argc, char **argv, const DaemonCallback &main_cb, const bool is_daemon)
    {
        if (!is_daemon)
        {
            ProcessInfoMgr::GetInstance()->parent_id         = GetCurrentProcessId();
            ProcessInfoMgr::GetInstance()->parent_start_time = time(nullptr);
            return real_start(argc, argv, main_cb);
        }

        return real_daemon(argc, argv, main_cb);
    }
}
