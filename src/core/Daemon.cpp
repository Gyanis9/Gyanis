#include <sys/wait.h>
#include "core/Daemon.h"
#include "base/Config.h"

namespace Gyanis::core
{
    static auto g_logger = LOG_NAME("system");
    auto g_daemon_restart_interval = base::Config::LookUp<uint32_t>("daemon.restart_interval", 5,
                                                                    "daemon restart interval");

    std::string ProcessInfo::toString() const
    {
        std::stringstream ss;
        ss << "\n[Process Information - "
            << "Parent ID: " << parent_id << "\n"
            << "Main ID: " << main_id << "\n"
            << "Parent Start Time: " << base::Time2Str(parent_start_time) << "\n"
            << "Main Start Time: " << base::Time2Str(main_start_time) << "\n"
            << "Restart Count: " << restart_count << "]";
        return ss.str();
    }


    static int real_start(const int argc, char** argv,
                          const std::function<int(int argc, char** argv)>& main_cb)
    {
        ProcessInfoMgr::GetInstance()->main_id = getpid();
        ProcessInfoMgr::GetInstance()->main_start_time = time(nullptr);
        return main_cb(argc, argv);
    }


    static int real_daemon(const int argc, char** argv, const std::function<int(int argc, char** argv)>& main_cb)
    {
        // 将当前进程转为守护进程，并忽略控制终端
        if (daemon(1, 0) == -1)
        {
            LOG_ERROR(g_logger)
                << "real_daemon - Daemon initialization failed. "
                << "Failed to start the daemon process.";
            return -1;
        }

        // 记录当前进程（父进程）的 PID 和启动时间
        ProcessInfoMgr::GetInstance()->parent_id = getpid();
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
                ProcessInfoMgr::GetInstance()->main_id = getpid();
                ProcessInfoMgr::GetInstance()->main_start_time = time(nullptr);

                // 输出日志，表示子进程已经启动
                LOG_INFO(g_logger) << "Process started. " << "Process ID (PID): " << getpid();

                // 调用 real_start 函数来执行传入的回调函数
                return real_start(argc, argv, main_cb);
            }

            // 如果 fork 失败，输出错误信息并退出
            if (pid < 0)
            {
                LOG_ERROR(g_logger)
                    << "Fork operation failed. "
                    << "Return value: " << pid
                    << " | Error number (errno): " << errno
                    << " | Error description: " << strerror(errno);

                return -1;
            }

            // 父进程等待子进程退出
            int status = 0;
            waitpid(pid, &status, 0);

            // 如果子进程异常退出，进行处理
            if (status)
            {
                if (status == 9)
                {
                    // 如果子进程是被杀死的（信号 9），则输出日志并退出
                    LOG_INFO(g_logger)
                        << "Child process terminated by signal 9 (kill signal).";
                    break;
                }

                // 如果子进程崩溃，输出错误日志
                LOG_ERROR(g_logger) << "Child process crashed. "
                                    << "Process ID (PID): " << pid
                                    << " | Exit status: " << status;
            }
            else
            {
                // 如果子进程正常退出，输出日志并退出循环
                LOG_INFO(g_logger)
                    << "Child process finished. "
                    << "Process ID (PID): " << pid;
                break;
            }

            // 如果子进程崩溃或被杀死，增加重启计数
            ProcessInfoMgr::GetInstance()->restart_count += 1;

            // 等待一段时间后重启子进程
            sleep(g_daemon_restart_interval->getValue());
        }
        return 0;
    }


    int start_daemon(const int argc, char** argv, const std::function<int(int, char**)>& main_cb, const bool is_daemon)
    {
        if (!is_daemon)
        {
            ProcessInfoMgr::GetInstance()->parent_id = getpid();
            ProcessInfoMgr::GetInstance()->parent_start_time = time(nullptr);
            return real_start(argc, argv, main_cb);
        }
        return real_daemon(argc, argv, main_cb);
    }
}
