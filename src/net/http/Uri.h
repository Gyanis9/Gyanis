/**
 * @file Uri.h
 * @brief URI 解析与处理
 * @date 2025-03-18
 */
#ifndef URI_H
#define URI_H
#include <string>
#include "../Address.h"

/**
 * 示例 URI：foo://user@baidu.com:8042/over/there?name=ferret#nose
 * 解析后的组成部分：
 *       \_/   \______________/\_________/ \_________/ \__/
 *        |           |            |            |        |
 *     scheme     authority       path        query   fragment
 */

namespace Gyanis::net::http
{
    /**
     * @brief URI 解析类
     */
    class Uri
    {
    public:
        /**
         * @brief 默认构造 URI 对象
         */
        Uri();

        /**
         * @brief 获取 URI 协议（scheme）
         */
        [[nodiscard]] const std::string& getScheme() const { return m_scheme; }

        /**
         * @brief 获取 URI 主机（host）
         */
        [[nodiscard]] const std::string& getHost() const { return m_host; }

        /**
         * @brief 获取 URI 路径（path）
         */
        [[nodiscard]] const std::string& getPath() const;

        /**
         * @brief 获取 URI 查询（query）
         */
        [[nodiscard]] const std::string& getQuery() const { return m_query; }

        /**
         * @brief 获取 URI 片段（fragment）（例如 `#nose`）
         */
        [[nodiscard]] const std::string& getFragment() const { return m_fragment; }

        /**
         * @brief 获取 URI 用户信息（userinfo）（例如 `user@baidu.com`）
         */
        [[nodiscard]] const std::string& getUserinfo() const { return m_userinfo; }

        /**
         * @brief 获取 URI 端口（port）
         */
        [[nodiscard]] int32_t getPort() const;

        /**
         * @brief 设置 URI 协议（scheme）
         */
        void setScheme(const std::string& scheme) { m_scheme = scheme; }

        /**
         * @brief 设置 URI 主机（host）
         */
        void setHost(const std::string& host) { m_host = host; }

        /**
         * @brief 设置 URI 路径（path）
         */
        void setPath(const std::string& path) { m_path = path; }

        /**
         * @brief 设置 URI 查询（query）
         */
        void setQuery(const std::string& query) { m_query = query; }

        /**
         * @brief 设置 URI 片段（fragment）
         */
        void setFragment(const std::string& fragment) { m_fragment = fragment; }

        /**
         * @brief 设置 URI 用户信息（userinfo）
         */
        void setUserinfo(const std::string& info) { m_userinfo = info; }

        /**
         * @brief 设置 URI 端口（port）
         */
        void setPort(const int32_t port) { m_port = port; }

        /**
         * @brief 打印 URI 对象
         * @param os 输出流
         * @return std::ostream& 返回输出流
         */
        std::ostream& dump(std::ostream& os) const;

        /**
         * @brief 将 URI 转换为字符串
         */
        [[nodiscard]] std::string toString() const;

        /**
         * @brief 创建 URI 地址对象
         */
        [[nodiscard]] std::shared_ptr<Address> createAddress() const;
        /**
         * @brief 创建 URI 对象
         */
        static std::shared_ptr<Uri> Create(const std::string& uri);

    private:
        /**
         * @brief 检查 URI 是否使用默认端口
         */
        [[nodiscard]] bool isDefaultPort() const;

        std::string m_scheme; ///< URI 协议部分（如 `http`、`https`）。
        std::string m_userinfo; ///< URI 用户信息部分（如 `user@baidu.com`）。
        std::string m_host; ///< URI 主机部分（如 `www.example.com`）。
        std::string m_path; ///< URI 路径部分（如 `/over/there`）。
        std::string m_query; ///< URI 查询部分（如 `name=ferret`）。
        std::string m_fragment; ///< URI 片段部分（如 `#nose`）。
        int32_t m_port; ///< URI 端口号（如 `8042`）。
    };
}

#endif
