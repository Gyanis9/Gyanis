/**
 * @file Redis.h
 * @brief 数据库Redis模块封装
 * @date 2025-04-12
 */
#ifndef REDIS_H
#define REDIS_H
#include <memory>
#include <functional>
#include <sw/redis++/redis++.h>
#include <string>

namespace Gyanis::db
{
    /**
     * @brief Redis 操作异常类
     */
    class RedisException final : public std::exception
    {
    public:
        /**
         * @brief 构造函数，初始化异常消息
         * @param msg 异常消息
         */
        explicit RedisException(std::string msg);

        /**
         * @brief 获取异常消息
         */
        [[nodiscard]] const char* what() const noexcept override;

    private:
        std::string m_msg; ///< 存储异常消息
    };

    /**
     * @brief Redis 客户端封装类，提供对 Redis 的基本操作
     */
    class RedisClient
    {
    public:
        /**
         * @brief 构造函数，使用主机、端口、密码和数据库号初始化 Redis 客户端
         */
         RedisClient();

        /**
         * @brief 构造函数，使用连接选项和连接池选项初始化 Redis 客户端
         * @param conn_opts Redis 连接选项
         * @param pool_opts Redis 连接池选项
         */
        explicit RedisClient(sw::redis::ConnectionOptions conn_opts,
                             const sw::redis::ConnectionPoolOptions& pool_opts);

        /**
         * @brief 设置键值对
         * @param key 键
         * @param value 值
         * @return 操作是否成功
         */
        [[nodiscard]] bool set(const std::string& key, const std::string& value) const;

        /**
         * @brief 设置哈希表字段值
         * @param key 哈希表键
         * @param field 字段名
         * @param value 字段值
         * @return 操作是否成功
         */
        [[nodiscard]] bool hset(const std::string& key,
                                const std::string& field,
                                const std::string& value) const;

        /**
         * @brief 获取指定键的值
         * @param key 键
         * @return 键对应的值（如果不存在则返回空）
         */
        [[nodiscard]] sw::redis::Optional<std::string> get(const std::string& key) const;

        /**
         * @brief 获取哈希表中指定字段的值
         * @param key 哈希表键
         * @param field 字段名
         * @return 字段对应的值（如果不存在则返回空）
         */
        [[nodiscard]] sw::redis::Optional<std::string> hget(const std::string& key,
                                                            const std::string& field) const;

        /**
         * @brief 检查键是否存在
         * @param key 键
         * @return 键是否存在
         */
        [[nodiscard]] bool exists(const std::string& key) const;

        /**
         * @brief 删除指定键
         * @param key 键
         * @return 删除的键数量
         */
        [[nodiscard]] long long del(const std::string& key) const;

        /**
         * @brief 将值列表推入列表左侧
         * @param key 列表键
         * @param values 要推入的值列表
         * @return 列表的新长度
         */
        [[nodiscard]] long long lpush(const std::string& key,
                                      const std::vector<std::string>& values) const;

        /**
         * @brief 将成员添加到集合
         * @param key 集合键
         * @param member 要添加的成员
         * @return 操作是否成功
         */
        [[nodiscard]] bool sadd(const std::string& key,
                                const std::string& member) const;

        /**
         * @brief 执行 Lua 脚本
         * @param script Lua 脚本内容
         * @param keys 键列表
         * @param args 额外参数
         * @return 脚本执行结果
         * @throws RedisException 如果脚本执行失败
         */
        template <typename... Args>
        auto eval(const std::string& script,
                  std::initializer_list<std::string> keys,
                  Args&&... args)
        {
            try
            {
                return m_redis->eval(script, keys, std::forward<Args>(args)...);
            }
            catch (const sw::redis::Error& e)
            {
                throw RedisException("EVAL error: " + std::string(e.what()));
            }
        }

        /**
         * @brief 批量设置多个键值对
         * @param kv_pairs 键值对列表
         */
        void multiSet(const std::vector<std::pair<std::string, std::string>>& kv_pairs) const;

        /**
         * @brief 发布消息到指定频道
         * @param channel 频道名
         * @param message 消息内容
         */
        void publish(const std::string& channel, const std::string& message) const;

        /**
         * @brief 订阅指定频道并处理消息
         * @param channel 频道名
         * @param callback 处理消息的回调函数，参数为频道名和消息内容
         */
        void subscribe(const std::string& channel,
                       const std::function<void(std::string, std::string)>& callback) const;

    private:
        std::unique_ptr<sw::redis::Redis> m_redis; ///< Redis 客户端实例
        sw::redis::ConnectionOptions m_opts; ///< 连接选项
        sw::redis::ConnectionPoolOptions m_pool_opts; ///< 连接池选项
    };
}

#endif
