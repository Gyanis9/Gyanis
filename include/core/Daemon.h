/**
 * @file Daemon.h
 * @brief 守护进程管理功能模块封装
 * @date 2025-03-31
 */

#ifndef DAEMON_H
#define DAEMON_H

#include "base/Singleton.h"
#include <functional>

namespace Gyanis::core
{
    /**
     * @struct ProcessInfo
     * @brief 用于保存进程的信息结构，包含父进程ID、主进程ID、启动时间和重启次数等信息。
     */
    struct ProcessInfo
    {
        pid_t parent_id = 0; ///< 父进程的PID
        pid_t main_id = 0; ///< 主进程的PID
        time_t parent_start_time = 0; ///< 父进程启动时间（Unix 时间戳）
        time_t main_start_time = 0; ///< 主进程启动时间（Unix 时间戳）
        uint32_t restart_count = 0; ///< 进程重启次数

        /**
         * @brief 将 ProcessInfo 对象转换为人类可读的字符串格式。
         */
        [[nodiscard]] std::string toString() const;
    };

    using ProcessInfoMgr = Singleton<ProcessInfo>;

    /**
     * @brief 启动守护进程
     * @param argc 命令行参数的数量
     * @param argv 命令行参数的数组
     * @param main_cb 守护进程启动后的回调函数，该函数会作为主进程执行
     * @param is_daemon 是否将进程作为守护进程运行的标志
     */
    int start_daemon(int argc, char** argv, const std::function<int(int argc, char** argv)>& main_cb, bool is_daemon);
}

#endif
