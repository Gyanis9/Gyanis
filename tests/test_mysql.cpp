#include "core/IOManager.h"
#include "base/Log.h"
#include "db/Mysql.h"


void run()
{
    do
    {
        std::unordered_map<std::string, std::string> params;
        params["host"] = "127.0.0.1";
        params["user"] = "root";
        params["password"] = "781680696";
        params["dbname"] = "blog";
        const auto mysql = std::make_shared<Gyanis::db::MySQL>(params);
        if (!mysql->connect())
        {
            LOG_ERROR(LOG_ROOT()) << "Failed to connect to MySQL database";
            return;
        }
        const auto stmt = Gyanis::db::MySQLStmt::Create(mysql, "update user set update_time = ? where id = 1");
        stmt->bindString(1, "2025-04-12 12:10:10");
        const int rt = stmt->execute();
        LOG_INFO(LOG_ROOT()) << "result:" << rt;
    }
    while (false);
    LOG_INFO(LOG_ROOT()) << "Done";
}

int main()
{
    const auto iom = std::make_shared<Gyanis::core::IOManager>(2);
    iom->addTimer(1000, run, true);
    return 0;
}
