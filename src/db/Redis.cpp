#include "db/Redis.h"
#include "base/Config.h"
#include "base/Log.h"
#include "base/Utils.h"

namespace Gyanis::db
{
    static auto g_logger = LOG_NAME("system");
    static auto g_redis =
        base::Config::LookUp("redis.config",
                             std::unordered_map<std::string, std::string>(),
                             "redis config");

    RedisException::RedisException(std::string msg): m_msg(std::move(msg))
    {
    }

    const char* RedisException::what() const noexcept
    {
        return m_msg.c_str();
    }

    RedisClient::RedisClient()
    {
        try
        {
            sw::redis::ConnectionOptions opts;
            opts.host = base::GetParamValue<std::string>(g_redis->getValue(), "host", "127.0.0.1");
            opts.port = base::GetParamValue<int>(g_redis->getValue(), "port", 6379);
            opts.password = base::GetParamValue<std::string>(g_redis->getValue(), "password", "");
            opts.db = base::GetParamValue<int>(g_redis->getValue(), "db", 0);

            sw::redis::ConnectionPoolOptions pool_opts;
            pool_opts.size = base::GetParamValue<int>(g_redis->getValue(), "pool_size", 4);; // 连接池大小
            m_redis = std::make_unique<sw::redis::Redis>(opts, pool_opts);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Create Redis error: " << e.what();
            throw RedisException("Connection failed: " + std::string(e.what()));
        }
    }

    RedisClient::RedisClient(sw::redis::ConnectionOptions conn_opts,
                             const sw::redis::ConnectionPoolOptions& pool_opts): m_opts(std::move(conn_opts)),
        m_pool_opts(pool_opts)
    {
        try
        {
            m_redis = std::make_unique<sw::redis::Redis>(m_opts, m_pool_opts);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Create Redis error: " << e.what();
            throw RedisException("Connection failed: " + std::string(e.what()));
        }
    }

    bool RedisClient::set(const std::string& key, const std::string& value) const
    {
        try
        {
            return m_redis->set(key, value);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Set Redis error: " << e.what();
            throw RedisException("SET error: " + std::string(e.what()));
        }
    }

    bool RedisClient::hset(const std::string& key, const std::string& field, const std::string& value) const
    {
        try
        {
            return m_redis->hset(key, field, value);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Set Redis error: " << e.what();
            throw RedisException("HSET error: " + std::string(e.what()));
        }
    }

    sw::redis::Optional<std::string> RedisClient::get(const std::string& key) const
    {
        try
        {
            return m_redis->get(key);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Get Redis error: " << e.what();
            throw RedisException("GET error: " + std::string(e.what()));
        }
    }

    sw::redis::Optional<std::string> RedisClient::hget(const std::string& key, const std::string& field) const
    {
        try
        {
            return m_redis->hget(key, field);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Get Redis error: " << e.what();
            throw RedisException("HGET error: " + std::string(e.what()));
        }
    }

    bool RedisClient::exists(const std::string& key) const
    {
        try
        {
            return m_redis->exists(key);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Exists Redis error: " << e.what();
            throw RedisException("EXISTS error: " + std::string(e.what()));
        }
    }

    long long RedisClient::del(const std::string& key) const
    {
        try
        {
            return m_redis->del(key);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Delete Redis error: " << e.what();
            throw RedisException("DEL error: " + std::string(e.what()));
        }
    }

    long long RedisClient::lpush(const std::string& key, const std::vector<std::string>& values) const
    {
        try
        {
            return m_redis->lpush(key, values.begin(), values.end());
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Push Redis error: " << e.what();
            throw RedisException("LPUSH error: " + std::string(e.what()));
        }
    }

    bool RedisClient::sadd(const std::string& key, const std::string& member) const
    {
        try
        {
            return m_redis->sadd(key, member);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "SADD error: " << e.what();
            throw RedisException("SADD error: " + std::string(e.what()));
        }
    }

    void RedisClient::multiSet(const std::vector<std::pair<std::string, std::string>>& kv_pairs) const
    {
        try
        {
            auto tx = m_redis->transaction();
            for (const auto& [fst, snd] : kv_pairs)
            {
                tx.set(fst, snd);
            }
            tx.exec();
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "MultiSet Redis error: " << e.what();
            throw RedisException("Transaction error: " + std::string(e.what()));
        }
    }

    void RedisClient::publish(const std::string& channel, const std::string& message) const
    {
        try
        {
            m_redis->publish(channel, message);
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Publish Redis error: " << e.what();
            throw RedisException("Publish error: " + std::string(e.what()));
        }
    }

    void RedisClient::subscribe(const std::string& channel,
                                const std::function<void(std::string, std::string)>& callback) const
    {
        try
        {
            auto sub = m_redis->subscriber();
            sub.on_message([&](std::string temp_channel, std::string msg)
            {
                callback(move(temp_channel), move(msg));
            });
            sub.subscribe(channel);
            while (true)
            {
                sub.consume();
            }
        }
        catch (const sw::redis::Error& e)
        {
            LOG_FATAL(g_logger) << "Subscribe Redis error: " << e.what();
            throw RedisException("Subscribe error: " + std::string(e.what()));
        }
    }
}
