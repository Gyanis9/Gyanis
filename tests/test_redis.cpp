#include "db/Redis.h"
#include "base/Log.h"
#include <thread>

#include "core/IOManager.h"
#include "core/Scheduler.h"
using namespace Gyanis::db;
using namespace std;

void test()
{
    try
    {
        RedisClient redis;

        if (const bool flag = redis.set("test_key", "hello world"); flag)
        {
            LOG_INFO(LOG_ROOT()) << "redis set test_key hello world";
        }
        else
        {
            LOG_ERROR(LOG_ROOT()) << "redis set test_key hello world error";
        }

        if (auto val = redis.get("test_key"))
        {
            LOG_INFO(LOG_ROOT()) << "Got value: " << *val;
        }

        const vector<pair<string, string>> items = {
            {"key1", "value1"},
            {"key2", "value2"}
        };
        redis.multiSet(items);

        thread([&redis]()
        {
            redis.subscribe("news", [](const string& channel, const string& msg)
            {
                LOG_INFO(LOG_ROOT()) << "Received message on " << channel << ": " << msg;
            });
        }).detach();

        redis.publish("news", "Breaking news!");

        // 删除key
        if (const bool flag2 = redis.del("test_key"); flag2)
        {
            LOG_INFO(LOG_ROOT()) << "redis delete test_key hello world";
        }
        else
        {
            LOG_ERROR(LOG_ROOT()) << "redis delete test_key hello world error";
        }
    }
    catch (const RedisException& e)
    {
        LOG_ERROR(LOG_ROOT()) << "Redis Error: " << e.what() << endl;
    }
    catch (const exception& e)
    {
        LOG_ERROR(LOG_ROOT()) << "Error: " << e.what() << endl;
    }
}

int main()
{
    Gyanis::core::IOManager iom;
    iom.schedule(test);
}
