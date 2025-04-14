/**
 * @file Mysql.h
 * @brief Mysql数据库模块封装
 * @date 2025-04-12
 */
#ifndef MYSQL_H
#define MYSQL_H
#include <functional>
#include <list>
#include <mutex>
#include <mysql/mysql.h>
#include <unordered_map>
#include "db/Db.h"
#include "base/Singleton.h"

namespace Gyanis::db
{
    class MySQL;

    class MySQLStmt;

    /**
     * @brief 定义 MySQL 时间结构体
     */
    struct MySQLTime
    {
        /**
         * @brief 构造函数，接受一个时间戳并初始化 MySQLTime
         */
        explicit MySQLTime(time_t time);

        time_t time; ///< 存储时间戳
    };

    /**
     * @brief 将 MySQL 时间转换为 time_t 类型
     * @param mysql_time MySQL 时间结构体
     * @param time 返回的时间戳
     */
    bool mysql_time_to_time_t(const MYSQL_TIME& mysql_time, time_t& time);

    /**
     * @brief 将 time_t 类型转换为 MySQL 时间结构体
     * @param time 时间戳
     * @param mysql_time 返回的 MySQL 时间结构体
     */
    bool time_t_to_mysql_time(const time_t& time, MYSQL_TIME& mysql_time);

    /**
     * @brief MySQL 查询结果类，继承自 ISQLData
     */
    class MySQLRes final : public ISQLData
    {
    public:
        using data_cb = std::function<bool(MYSQL_ROW row, int field_count, int row_no)>;

        /**
         * @brief 构造函数，初始化 MySQL 查询结果
         * @param result MySQL 查询结果指针
         * @param err 错误码
         * @param errStr 错误信息
         */
        explicit MySQLRes(MYSQL_RES* result, int err, const char* errStr);

        /**
         * @brief 获取 MySQL 查询结果指针
         */
        [[nodiscard]] MYSQL_RES* get() const;

        /**
         * @brief 获取 SQL 操作的错误码
         */
        [[nodiscard]] int getError() const override;

        /**
         * @brief 获取 SQL 操作的错误信息
         */
        [[nodiscard]] const std::string& getErrStr() const override;

        /**
         * @brief 遍历查询结果并对每一行数据执行回调
         * @param cb 数据处理回调
         */
        bool foreach(const data_cb& cb);

        /**
         * @brief 获取查询结果的行数
         */
        int getDataCount() override;

        /**
         * @brief 获取查询结果的列数
         */
        int getColumnCount() override;

        /**
         * @brief 获取指定列的字节数
         * @param index 列索引
         */
        int getColumnBytes(int index) override;

        /**
         * @brief 获取指定列的数据类型
         * @param index 列索引
         */
        int getColumnType(int index) override;

        /**
         * @brief 获取指定列的名称
         * @param index 列索引
         */
        std::string getColumnName(int index) override;

        /**
         * @brief 判断指定列的数据是否为 NULL
         * @param index 列索引
         */
        bool isNull(int index) override;

        /**
         * @brief 获取指定列的 8 位整数值
         * @param index 列索引
         */
        int8_t getInt8(int index) override;

        /**
         * @brief 获取指定列的 8 位无符号整数值
         * @param index 列索引
         */
        uint8_t getUint8(int index) override;

        /**
         * @brief 获取指定列的 16 位整数值
         * @param index 列索引
         */
        int16_t getInt16(int index) override;

        /**
         * @brief 获取指定列的 16 位无符号整数值
         * @param index 列索引
         */
        uint16_t getUint16(int index) override;

        /**
         * @brief 获取指定列的 32 位整数值
         * @param index 列索引
         */
        int32_t getInt32(int index) override;

        /**
         * @brief 获取指定列的 32 位无符号整数值
         * @param index 列索引
         */
        uint32_t getUint32(int index) override;

        /**
         * @brief 获取指定列的 64 位整数值
         * @param index 列索引
         */
        int64_t getInt64(int index) override;

        /**
         * @brief 获取指定列的 64 位无符号整数值
         * @param index 列索引
         */
        uint64_t getUint64(int index) override;

        /**
         * @brief 获取指定列的浮点数值
         * @param index 列索引
         */
        float getFloat(int index) override;

        /**
         * @brief 获取指定列的双精度浮点数值
         * @param index 列索引
         */
        double getDouble(int index) override;

        /**
         * @brief 获取指定列的字符串值
         * @param index 列索引
         */
        std::string getString(int index) override;

        /**
         * @brief 获取指定列的 BLOB 数据
         * @param index 列索引
         */
        std::string getBlob(int index) override;

        /**
         * @brief 获取指定列的时间值
         * @param index 列索引
         */
        time_t getTime(int index) override;

        /**
         * @brief 获取下一行数据
         */
        bool next() override;

    private:
        int m_errno; ///< 错误码
        std::string m_errstr; ///< 错误信息
        MYSQL_ROW m_cur; ///< 当前行数据
        unsigned long* m_curLength; ///< 当前行数据的长度
        std::shared_ptr<MYSQL_RES> m_data; ///< 查询结果数据
    };


    /**
     * @brief MySQL 语句结果类，继承自 ISQLData
     */
    class MySQLStmtRes final : public ISQLData
    {
        friend class MySQLStmt; ///< 友元类 MySQLStmt，可以访问 MySQLStmtRes 的私有成员

    public:
        /**
         * @brief 创建 MySQL 语句结果对象
         * @param stmt 指向 MySQLStmt 对象的 shared_ptr
         */
        static std::shared_ptr<MySQLStmtRes> Create(const std::shared_ptr<MySQLStmt>& stmt);

        /**
         * @brief 析构函数
         */
        ~MySQLStmtRes() override;

        /**
         * @brief 获取 SQL 操作的错误码
         */
        [[nodiscard]] int getError() const override;

        /**
         * @brief 获取 SQL 操作的错误信息
         */
        [[nodiscard]] const std::string& getErrStr() const override;

        /**
         * @brief 获取查询结果的行数
         */
        int getDataCount() override;

        /**
         * @brief 获取查询结果的列数
         */
        int getColumnCount() override;

        /**
         * @brief 获取指定列的字节数
         * @param index 列索引
         */
        int getColumnBytes(int index) override;

        /**
         * @brief 获取指定列的数据类型
         * @param index 列索引
         */
        int getColumnType(int index) override;

        /**
         * @brief 获取指定列的名称
         * @param index 列索引
         */
        std::string getColumnName(int index) override;

        /**
         * @brief 判断指定列的数据是否为 NULL
         * @param index 列索引
         */
        bool isNull(int index) override;

        /**
         * @brief 获取指定列的 8 位整数值
         * @param index 列索引
         */
        int8_t getInt8(int index) override;

        /**
         * @brief 获取指定列的 8 位无符号整数值
         * @param index 列索引
         */
        uint8_t getUint8(int index) override;

        /**
         * @brief 获取指定列的 16 位整数值
         * @param index 列索引
         */
        int16_t getInt16(int index) override;

        /**
         * @brief 获取指定列的 16 位无符号整数值
         * @param index 列索引
         */
        uint16_t getUint16(int index) override;

        /**
         * @brief 获取指定列的 32 位整数值
         * @param index 列索引
         */
        int32_t getInt32(int index) override;

        /**
         * @brief 获取指定列的 32 位无符号整数值
         * @param index 列索引
         */
        uint32_t getUint32(int index) override;

        /**
         * @brief 获取指定列的 64 位整数值
         * @param index 列索引
         */
        int64_t getInt64(int index) override;

        /**
         * @brief 获取指定列的 64 位无符号整数值
         * @param index 列索引
         */
        uint64_t getUint64(int index) override;

        /**
         * @brief 获取指定列的浮点数值
         * @param index 列索引
         */
        float getFloat(int index) override;

        /**
         * @brief 获取指定列的双精度浮点数值
         * @param index 列索引
         */
        double getDouble(int index) override;

        /**
         * @brief 获取指定列的字符串值
         * @param index 列索引
         */
        std::string getString(int index) override;

        /**
         * @brief 获取指定列的 BLOB 数据
         * @param index 列索引
         */
        std::string getBlob(int index) override;

        /**
         * @brief 获取指定列的时间值
         * @param index 列索引
         */
        time_t getTime(int index) override;

        /**
         * @brief 获取下一行数据
         */
        bool next() override;

    private:
        /**
         * @brief 构造函数，初始化 MySQL 语句结果对象
         * @param stmt MySQLStmt 对象的 shared_ptr
         * @param eno 错误码
         * @param errStr 错误信息
         */
        MySQLStmtRes(const std::shared_ptr<MySQLStmt>& stmt, int eno, std::string errStr);

        /**
         * @brief MySQL 语句结果的数据结构
         */
        struct Data
        {
            /**
             * @brief 默认构造函数
             */
            Data();

            /**
             * @brief 析构函数
             */
            ~Data();

            /**
             * @brief 分配内存
             * @param size 内存大小
             */
            void alloc(size_t size);

            bool is_null; ///< 数据是否为 NULL
            bool error; ///< 数据是否出错
            enum_field_types type; ///< 数据类型
            unsigned long length; ///< 数据长度
            size_t data_length; ///< 数据的实际长度
            char* data; ///< 数据内容
        };

    private:
        int m_errno; ///< 错误码
        std::string m_errstr; ///< 错误信息
        std::shared_ptr<MySQLStmt> m_stmt; ///< MySQLStmt 对象的 shared_ptr
        std::vector<MYSQL_BIND> m_binds; ///< 用于绑定参数的结构体
        std::vector<Data> m_datas; ///< 存储查询结果的数据
    };


    class MySQLManager;

    /**
     * @brief MySQL 数据库操作类，继承自 IDB，并支持 shared_ptr
     */
    class MySQL final : public IDB, public std::enable_shared_from_this<MySQL>
    {
        friend class MySQLManager; ///< 友元类 MySQLManager，可以访问 MySQL 类的私有成员

    public:
        /**
         * @brief 构造函数，使用参数初始化 MySQL 对象
         * @param args 配置参数的哈希表（如用户名、密码、主机等）
         */
        explicit MySQL(const std::unordered_map<std::string, std::string>& args);

        /**
         * @brief 连接 MySQL 数据库
         */
        bool connect();

        /**
         * @brief 检查数据库连接是否仍然有效
         */
        bool ping();

        /**
         * @brief 执行 SQL 语句（格式化字符串版本）
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        int execute(const char* format, ...) override;

        /**
         * @brief 执行 SQL 语句（va_list 版本）
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        int execute(const char* format, va_list ap);

        /**
         * @brief 执行 SQL 语句（字符串版本）
         * @param sql SQL 语句
         */
        int execute(const std::string& sql) override;

        /**
         * @brief 获取上次插入记录的自增 ID
         */
        int64_t getLastInsertId() override;

        /**
         * @brief 获取当前 MySQL 对象的 shared_ptr
         */
        std::shared_ptr<MySQL> getMySQL();

        /**
         * @brief 获取原始的 MySQL 指针
         */
        std::shared_ptr<MYSQL> getRaw();

        /**
         * @brief 获取影响的行数
         */
        uint64_t getAffectedRows() const;

        /**
         * @brief 执行查询操作（格式化字符串版本）
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        std::shared_ptr<ISQLData> query(const char* format, ...) override;

        /**
         * @brief 执行查询操作（va_list 版本）
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        std::shared_ptr<ISQLData> query(const char* format, va_list ap);

        /**
         * @brief 执行查询操作（字符串版本）
         * @param sql SQL 语句
         */
        std::shared_ptr<ISQLData> query(const std::string& sql) override;

        /**
         * @brief 打开事务
         * @param auto_commit 是否自动提交
         */
        std::shared_ptr<ITransaction> openTransaction(bool auto_commit) override;

        /**
         * @brief 准备一个 SQL 语句
         * @param sql SQL 语句
         */
        std::shared_ptr<IStmt> prepare(const std::string& sql) override;

        /**
         * @brief 执行一个预处理语句（模板版本）
         * @param stmt SQL 语句
         * @param args 语句的参数
         */
        template <typename... Args>
        int execStmt(const char* stmt, Args&&... args);

        /**
         * @brief 执行一个预处理查询语句（模板版本）
         * @param stmt SQL 语句
         * @param args 语句的参数
         */
        template <class... Args>
        std::shared_ptr<ISQLData> queryStmt(const char* stmt, Args&&... args);

        /**
         * @brief 获取 MySQL 客户端命令
         * @return MySQL 客户端命令字符串
         */
        const char* cmd() const;

        /**
         * @brief 使用指定的数据库
         * @param dbname 数据库名称
         */
        bool use(const std::string& dbname);

        /**
         * @brief 获取 SQL 错误码
         */
        int getErrno() override;

        /**
         * @brief 获取 SQL 错误信息
         */
        std::string getErrStr() override;

        /**
         * @brief 获取插入操作的自增 ID
         */
        uint64_t getInsertId() const;

    private:
        /**
         * @brief 判断是否需要检查连接状态
         */
        bool isNeedCheck() const;

    private:
        std::unordered_map<std::string, std::string> m_params; ///< 存储连接参数
        std::shared_ptr<MYSQL> m_mysql; ///< MySQL 客户端对象的 shared_ptr
        std::string m_cmd; ///< MySQL 客户端命令
        std::string m_dbname; ///< 当前使用的数据库名称
        uint64_t m_lastUsedTime; ///< 上次使用时间
        bool m_hasError; ///< 是否发生了错误
        int32_t m_poolSize; ///< 数据库连接池大小
    };


    /**
 * @brief MySQL 事务类，继承自 ITransaction
 */
    class MySQLTransaction final : public ITransaction
    {
    public:
        /**
         * @brief 创建 MySQL 事务对象
         * @param mysql MySQL 对象的 shared_ptr
         * @param auto_commit 是否自动提交
         */
        static std::shared_ptr<MySQLTransaction> Create(const std::shared_ptr<MySQL>& mysql, bool auto_commit);

        /**
         * @brief 析构函数
         */
        ~MySQLTransaction() override;

        /**
         * @brief 开始一个新的事务
         */
        bool begin() override;

        /**
         * @brief 提交事务
         */
        bool commit() override;

        /**
         * @brief 回滚事务
         */
        bool rollback() override;

        /**
         * @brief 执行 SQL 语句（格式化字符串版本）
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        int execute(const char* format, ...) override;

        /**
         * @brief 执行 SQL 语句（va_list 版本）
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        int execute(const char* format, va_list ap);

        /**
         * @brief 执行 SQL 语句（字符串版本）
         * @param sql SQL 语句
         */
        int execute(const std::string& sql) override;

        /**
         * @brief 获取上次插入记录的自增 ID
         */
        int64_t getLastInsertId() override;

        /**
         * @brief 获取 MySQL 对象的 shared_ptr
         */
        std::shared_ptr<MySQL> getMySQL();

        /**
         * @brief 判断事务是否是自动提交模式
         */
        [[nodiscard]] bool isAutoCommit() const;

        /**
         * @brief 判断事务是否已经完成
         */
        [[nodiscard]] bool isFinished() const;

        /**
         * @brief 判断事务是否发生了错误
         */
        [[nodiscard]] bool isError() const;

    private:
        /**
         * @brief 构造函数，初始化 MySQL 事务对象
         * @param mysql MySQL 对象的 shared_ptr
         * @param auto_commit 是否自动提交
         */
        MySQLTransaction(const std::shared_ptr<MySQL>& mysql, bool auto_commit);

    private:
        std::shared_ptr<MySQL> m_mysql; ///< MySQL 对象的 shared_ptr
        bool m_autoCommit; ///< 是否自动提交事务
        bool m_isFinished; ///< 事务是否已完成
        bool m_hasError; ///< 事务是否发生错误
    };


    /**
     * @brief MySQL 语句类，继承自 IStmt，并支持 shared_ptr
     */
    class MySQLStmt final : public IStmt, public std::enable_shared_from_this<MySQLStmt>
    {
    public:
        /**
         * @brief 创建 MySQL 语句对象
         * @param db MySQL 数据库对象的 shared_ptr
         * @param stmt SQL 语句
         */
        static std::shared_ptr<MySQLStmt> Create(const std::shared_ptr<MySQL>& db, const std::string& stmt);

        /**
         * @brief 析构函数
         */
        ~MySQLStmt() override;

        /**
         * @brief 绑定整数值（8 位有符号整数）
         * @param index 参数索引
         * @param value 8 位有符号整数值
         */
        int bind(int index, const int8_t& value);

        /**
         * @brief 绑定无符号整数值（8 位无符号整数）
         * @param index 参数索引
         * @param value 8 位无符号整数值
         */
        int bind(int index, const uint8_t& value);

        /**
         * @brief 绑定整数值（16 位有符号整数）
         * @param index 参数索引
         * @param value 16 位有符号整数值
         */
        int bind(int index, const int16_t& value);

        /**
         * @brief 绑定无符号整数值（16 位无符号整数）
         * @param index 参数索引
         * @param value 16 位无符号整数值
         */
        int bind(int index, const uint16_t& value);

        /**
         * @brief 绑定整数值（32 位有符号整数）
         * @param index 参数索引
         * @param value 32 位有符号整数值
         */
        int bind(int index, const int32_t& value);

        /**
         * @brief 绑定无符号整数值（32 位无符号整数）
         * @param index 参数索引
         * @param value 32 位无符号整数值
         */
        int bind(int index, const uint32_t& value);

        /**
         * @brief 绑定整数值（64 位有符号整数）
         * @param index 参数索引
         * @param value 64 位有符号整数值
         */
        int bind(int index, const int64_t& value);

        /**
         * @brief 绑定无符号整数值（64 位无符号整数）
         * @param index 参数索引
         * @param value 64 位无符号整数值
         */
        int bind(int index, const uint64_t& value);

        /**
         * @brief 绑定浮点数值
         * @param index 参数索引
         * @param value 浮点数值
         */
        int bind(int index, const float& value);

        /**
         * @brief 绑定双精度浮点数值
         * @param index 参数索引
         * @param value 双精度浮点数值
         */
        int bind(int index, const double& value);

        /**
         * @brief 绑定字符串值
         * @param index 参数索引
         * @param value 字符串值
         */
        int bind(int index, const std::string& value);

        /**
         * @brief 绑定字符串值（C 字符串版本）
         * @param index 参数索引
         * @param value C 字符串值
         */
        int bind(int index, const char* value);

        /**
         * @brief 绑定二进制数据
         * @param index 参数索引
         * @param value 数据指针
         * @param len 数据长度
         */
        int bind(int index, const void* value, int len);

        /**
         * @brief 绑定空值（NULL）
         * @param index 参数索引
         */
        int bind(int index);

        /**
         * @brief 绑定整数值（8 位有符号整数，重写版本）
         * @param index 参数索引
         * @param value 8 位有符号整数值
         */
        int bindInt8(int index, const int8_t& value) override;

        /**
         * @brief 绑定无符号整数值（8 位无符号整数，重写版本）
         * @param index 参数索引
         * @param value 8 位无符号整数值
         */
        int bindUint8(int index, const uint8_t& value) override;

        /**
         * @brief 绑定整数值（16 位有符号整数，重写版本）
         * @param index 参数索引
         * @param value 16 位有符号整数值
         */
        int bindInt16(int index, const int16_t& value) override;

        /**
         * @brief 绑定无符号整数值（16 位无符号整数，重写版本）
         * @param index 参数索引
         * @param value 16 位无符号整数值
         */
        int bindUint16(int index, const uint16_t& value) override;

        /**
         * @brief 绑定整数值（32 位有符号整数，重写版本）
         * @param index 参数索引
         * @param value 32 位有符号整数值
         */
        int bindInt32(int index, const int32_t& value) override;

        /**
         * @brief 绑定无符号整数值（32 位无符号整数，重写版本）
         * @param index 参数索引
         * @param value 32 位无符号整数值
         */
        int bindUint32(int index, const uint32_t& value) override;

        /**
         * @brief 绑定整数值（64 位有符号整数，重写版本）
         * @param index 参数索引
         * @param value 64 位有符号整数值
         */
        int bindInt64(int index, const int64_t& value) override;

        /**
         * @brief 绑定无符号整数值（64 位无符号整数，重写版本）
         * @param index 参数索引
         * @param value 64 位无符号整数值
         */
        int bindUint64(int index, const uint64_t& value) override;

        /**
         * @brief 绑定浮点数值（重写版本）
         * @param index 参数索引
         * @param value 浮点数值
         */
        int bindFloat(int index, const float& value) override;

        /**
         * @brief 绑定双精度浮点数值（重写版本）
         * @param index 参数索引
         * @param value 双精度浮点数值
         */
        int bindDouble(int index, const double& value) override;

        /**
         * @brief 绑定字符串值（重写版本）
         * @param index 参数索引
         * @param value 字符串值
         */
        int bindString(int index, const char* value) override;

        /**
         * @brief 绑定字符串值（重写版本）
         * @param index 参数索引
         * @param value 字符串值
         */
        int bindString(int index, const std::string& value) override;

        /**
         * @brief 绑定二进制数据（重写版本）
         * @param index 参数索引
         * @param value 数据指针
         * @param size 数据大小
         */
        int bindBlob(int index, const void* value, int64_t size) override;

        /**
         * @brief 绑定二进制数据（重写版本）
         * @param index 参数索引
         * @param value 字符串数据
         */
        int bindBlob(int index, const std::string& value) override;

        /**
         * @brief 绑定时间值（重写版本）
         * @param index 参数索引
         * @param value 时间值
         */
        int bindTime(int index, const time_t& value) override;

        /**
         * @brief 绑定空值（NULL，重写版本）
         * @param index 参数索引
         */
        int bindNull(int index) override;

        /**
         * @brief 获取 SQL 错误码
         */
        int getErrno() override;

        /**
         * @brief 获取 SQL 错误信息
         */
        std::string getErrStr() override;

        /**
         * @brief 执行预处理语句
         */
        int execute() override;

        /**
         * @brief 获取上次插入记录的自增 ID
         */
        int64_t getLastInsertId() override;

        /**
         * @brief 执行查询操作
         */
        std::shared_ptr<ISQLData> query() override;

        /**
         * @brief 获取原始 MySQL 语句指针
         */
        MYSQL_STMT* getRaw() const;

    private:
        /**
         * @brief 构造函数，初始化 MySQL 语句对象
         * @param db MySQL 数据库对象的 shared_ptr
         * @param stmt MySQL 语句指针
         */
        MySQLStmt(const std::shared_ptr<MySQL>& db, MYSQL_STMT* stmt);

    private:
        std::shared_ptr<MySQL> m_mysql; ///< MySQL 数据库对象的 shared_ptr
        MYSQL_STMT* m_stmt; ///< MySQL 语句对象指针
        std::vector<MYSQL_BIND> m_binds; ///< 用于绑定参数的结构体
    };

    /**
     * @brief MySQL 连接管理类，用于管理多个 MySQL 实例和连接池
     */
    class MySQLManager
    {
    public:
        /**
         * @brief 构造函数，初始化 MySQL 管理器
         */
        MySQLManager();

        /**
         * @brief 析构函数，释放所有资源
         */
        ~MySQLManager();

        /**
         * @brief 获取指定名称的 MySQL 实例
         * @param name MySQL 实例的名称
         */
        std::shared_ptr<MySQL> get(const std::string& name);

        /**
         * @brief 注册一个新的 MySQL 实例并配置连接参数
         * @param name MySQL 实例的名称
         * @param params 连接参数（如主机名、端口、用户名、密码等）
         */
        void registerMySQL(const std::string& name, const std::unordered_map<std::string, std::string>& params);

        /**
         * @brief 检查所有 MySQL 连接的状态
         * @param sec 超时等待时间（默认 30 秒）
         */
        void checkConnection(int sec = 30);

        /**
         * @brief 获取最大连接数
         * @return 最大连接数
         */
        [[nodiscard]] uint32_t getMaxConn() const;

        /**
         * @brief 设置最大连接数
         * @param value 最大连接数
         */
        void setMaxConn(uint32_t value);

        /**
         * @brief 执行 SQL 语句（格式化字符串版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        int execute(const std::string& name, const char* format, ...);

        /**
         * @brief 执行 SQL 语句（va_list 版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        int execute(const std::string& name, const char* format, va_list ap);

        /**
         * @brief 执行 SQL 语句（字符串版本）
         * @param name MySQL 实例的名称
         * @param sql SQL 语句
         */
        int execute(const std::string& name, const std::string& sql);

        /**
         * @brief 执行查询操作（格式化字符串版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        std::shared_ptr<ISQLData> query(const std::string& name, const char* format, ...);

        /**
         * @brief 执行查询操作（va_list 版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        std::shared_ptr<ISQLData> query(const std::string& name, const char* format, va_list ap);

        /**
         * @brief 执行查询操作（字符串版本）
         * @param name MySQL 实例的名称
         * @param sql SQL 语句
         */
        std::shared_ptr<ISQLData> query(const std::string& name, const std::string& sql);

        /**
         * @brief 打开一个事务
         * @param name MySQL 实例的名称
         * @param auto_commit 是否自动提交事务
         */
        std::shared_ptr<MySQLTransaction> openTransaction(const std::string& name, bool auto_commit);

    private:
        /**
         * @brief 释放 MySQL 实例的连接
         * @param name MySQL 实例的名称
         * @param m MySQL 实例的指针
         */
        void freeMySQL(const std::string& name, MySQL* m);

    private:
        uint32_t m_maxConn; ///< 最大连接数
        std::mutex m_mutex; ///< 互斥锁，用于保证线程安全
        std::unordered_map<std::string, std::list<MySQL*>> m_conns; ///< 连接池，按名称存储多个 MySQL 连接
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_dbDefines; ///< 每个 MySQL 实例的配置信息
    };

    /**
     * @brief MySQL 工具类，提供对 MySQL 数据库的查询和执行操作的静态方法
     */
    class MySQLUtil
    {
    public:
        /**
         * @brief 执行查询操作（格式化字符串版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        static std::shared_ptr<ISQLData> Query(const std::string& name, const char* format, ...);

        /**
         * @brief 执行查询操作（va_list 版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        static std::shared_ptr<ISQLData> Query(const std::string& name, const char* format, va_list ap);

        /**
         * @brief 执行查询操作（字符串版本）
         * @param name MySQL 实例的名称
         * @param sql SQL 语句
         */
        static std::shared_ptr<ISQLData> Query(const std::string& name, const std::string& sql);

        /**
         * @brief 尝试执行查询操作（格式化字符串版本），最多执行指定次数
         * @param name MySQL 实例的名称
         * @param count 最大执行次数
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        static std::shared_ptr<ISQLData> TryQuery(const std::string& name, uint32_t count, const char* format, ...);

        /**
         * @brief 尝试执行查询操作（字符串版本），最多执行指定次数
         * @param name MySQL 实例的名称
         * @param count 最大执行次数
         * @param sql SQL 语句
         */
        static std::shared_ptr<ISQLData> TryQuery(const std::string& name, uint32_t count, const std::string& sql);

        /**
         * @brief 执行 SQL 语句（格式化字符串版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        static int Execute(const std::string& name, const char* format, ...);

        /**
         * @brief 执行 SQL 语句（va_list 版本）
         * @param name MySQL 实例的名称
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        static int Execute(const std::string& name, const char* format, va_list ap);

        /**
         * @brief 执行 SQL 语句（字符串版本）
         * @param name MySQL 实例的名称
         * @param sql SQL 语句
         */
        static int Execute(const std::string& name, const std::string& sql);

        /**
         * @brief 尝试执行 SQL 语句（格式化字符串版本），最多执行指定次数
         * @param name MySQL 实例的名称
         * @param count 最大执行次数
         * @param format 格式化字符串
         * @param ... 可变参数列表
         */
        static int TryExecute(const std::string& name, uint32_t count, const char* format, ...);

        /**
         * @brief 尝试执行 SQL 语句（va_list 版本），最多执行指定次数
         * @param name MySQL 实例的名称
         * @param count 最大执行次数
         * @param format 格式化字符串
         * @param ap 可变参数列表
         */
        static int TryExecute(const std::string& name, uint32_t count, const char* format, va_list ap);

        /**
         * @brief 尝试执行 SQL 语句（字符串版本），最多执行指定次数
         * @param name MySQL 实例的名称
         * @param count 最大执行次数
         * @param sql SQL 语句
         */
        static int TryExecute(const std::string& name, uint32_t count, const std::string& sql);
    };


    // 使用 MySQLManager 的单例模式
    using MySQLMgr = Singleton<MySQLManager>;


    namespace
    {
        /**
         * @brief MySQL 参数绑定结构体模板（递归终止版本）
         * 递归结束条件：当没有更多的参数时，返回 0 表示绑定完成。
         */
        template <size_t N, typename... Args>
        struct MySQLBinder
        {
            /**
             * @brief 绑定 MySQL 语句的参数（递归终止版本）
             * @param stmt MySQL 语句对象
             * @return 绑定结果，始终返回 0
             */
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt)
            {
                return 0;
            }
        };

        /**
         * @brief 使用递归模板将参数绑定到 MySQL 语句
         * @param stmt MySQL 语句对象
         * @param args 可变参数列表
         * @return 绑定的结果
         */
        template <typename... Args>
        int bindX(const std::shared_ptr<MySQLStmt>& stmt, Args&... args)
        {
            return MySQLBinder<1, Args...>::Bind(stmt, args...);
        }
    }


    template <typename... Args>
    int MySQL::execStmt(const char* stmt, Args&&... args)
    {
        // 创建 MySQL 语句对象
        auto st = MySQLStmt::Create(shared_from_this(), stmt);
        if (!st)
        {
            return -1; // 如果创建失败，返回错误
        }

        // 绑定参数
        if (const int result = bindX(st, args...); result != 0)
        {
            return result; // 如果绑定参数失败，返回错误
        }

        // 执行语句
        return st->execute();
    }

    template <class... Args>
    std::shared_ptr<ISQLData> MySQL::queryStmt(const char* stmt, Args&&... args)
    {
        // 创建 MySQL 语句对象
        auto st = MySQLStmt::Create(shared_from_this(), stmt);
        if (!st)
        {
            return nullptr; // 如果创建失败，返回空指针
        }

        // 绑定参数
        if (const int result = bindX(st, args...); result != 0)
        {
            return nullptr; // 如果绑定参数失败，返回空指针
        }

        // 执行查询并返回结果
        return st->query();
    }

    namespace
    {
        /**
         * @brief MySQL 参数绑定结构体模板（递归版本）
         */
        template <size_t N, typename Head, typename... Tail>
        struct MySQLBinder<N, Head, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const Head&, Tail&...)
            {
                static_assert(sizeof...(Tail) < 0, "invalid type");
                return 0;
            }
        };

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, char*, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const char* value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, const char*, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const char* value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, std::string, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const std::string& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, int8_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const int8_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, uint8_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const uint8_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, int16_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const int16_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, uint16_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const uint16_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, int32_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const int32_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, uint32_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const uint32_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, int64_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const int64_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, uint64_t, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const uint64_t& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, float, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const float& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };;

        template <size_t N, typename... Tail>
        struct MySQLBinder<N, double, Tail...>
        {
            static int Bind(const std::shared_ptr<MySQLStmt>& stmt, const double& value, Tail&... tail)
            {
                if (const int result = stmt->bind(N, value); result != 0)
                {
                    return result;
                }
                return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
            }
        };
    };
}


#endif
