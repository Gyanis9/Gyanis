/**
 * @file Db.h
 * @brief 数据库基类模块封装
 * @date 2025-04-11
 */


#ifndef DB_H
#define DB_H
#include <memory>
#include <string>


namespace Gyanis::db
{
    /**
     * @brief 用于从 SQL 查询结果集中获取数据的接口
     */
    class ISQLData
    {
    public:
        virtual ~ISQLData() = default;

        /**
         * @brief 获取 SQL 操作的错误码
         */
        [[nodiscard]] virtual int getError() const = 0;

        /**
         * @brief 获取 SQL 操作失败的错误信息
         */
        [[nodiscard]] virtual const std::string& getErrStr() const = 0;

        /**
         * @brief 获取数据集中的行数
         */
        virtual int getDataCount() = 0;

        /**
         * @brief 获取数据集中列的数量
         */
        virtual int getColumnCount() = 0;

        /**
         * @brief 获取指定列的字节大小
         * @param index 列的索引
         */
        virtual int getColumnBytes(int index) = 0;

        /**
         * @brief 获取指定列的数据类型
         * @param index 列的索引
         */
        virtual int getColumnType(int index) = 0;

        /**
         * @brief 获取指定列的列名
         * @param index 列的索引
         */
        virtual std::string getColumnName(int index) = 0;

        /**
         * @brief 检查指定列的值是否为 NULL
         * @param index 列的索引
         */
        virtual bool isNull(int index) = 0;

        /**
         * @brief 获取指定列的 int8 值
         * @param index 列的索引
         */
        virtual int8_t getInt8(int index) = 0;

        /**
         * @brief 获取指定列的 uint8 值
         * @param index 列的索引
         */
        virtual uint8_t getUint8(int index) = 0;

        /**
         * @brief 获取指定列的 int16 值
         * @param index 列的索引
         */
        virtual int16_t getInt16(int index) = 0;

        /**
         * @brief 获取指定列的 uint16 值
         * @param index 列的索引
         */
        virtual uint16_t getUint16(int index) = 0;

        /**
         * @brief 获取指定列的 int32 值
         * @param index 列的索引
         */
        virtual int32_t getInt32(int index) = 0;

        /**
         * @brief 获取指定列的 uint32 值
         * @param index 列的索引
         */
        virtual uint32_t getUint32(int index) = 0;

        /**
         * @brief 获取指定列的 int64 值
         * @param index 列的索引
         */
        virtual int64_t getInt64(int index) = 0;

        /**
         * @brief 获取指定列的 uint64 值
         * @param index 列的索引
         */
        virtual uint64_t getUint64(int index) = 0;

        /**
         * @brief 获取指定列的 float 值
         * @param index 列的索引
         */
        virtual float getFloat(int index) = 0;

        /**
         * @brief 获取指定列的 double 值
         * @param index 列的索引
         */
        virtual double getDouble(int index) = 0;

        /**
         * @brief 获取指定列的字符串值
         * @param index 列的索引
         */
        virtual std::string getString(int index) = 0;

        /**
         * @brief 获取指定列的 blob 值
         * @param index 列的索引
         */
        virtual std::string getBlob(int index) = 0;

        /**
         * @brief 获取指定列的时间戳（time_t）
         * @param index 列的索引
         */
        virtual time_t getTime(int index) = 0;

        /**
         * @brief 移动到结果集中的下一行
         */
        virtual bool next() = 0;
    };

    /**
     * @brief 执行 SQL 更新（插入、更新、删除）操作的接口
     */
    class ISQLUpdate
    {
    public:
        virtual ~ISQLUpdate() = default;

        /**
         * @brief 使用格式化参数执行 SQL 语句
         */
        virtual int execute(const char* format, ...) = 0;

        /**
         * @brief 执行原始 SQL 语句
         * @param sql SQL 语句
         */
        virtual int execute(const std::string& sql) = 0;

        /**
         * @brief 获取插入操作后的最后插入 ID
         */
        virtual int64_t getLastInsertId() = 0;
    };

    /**
     * @brief 执行 SQL 查询（SELECT）的接口
     */
    class ISQLQuery
    {
    public:
        virtual ~ISQLQuery() = default;

        /**
         * @brief 使用格式化参数执行 SQL 查询，并返回结果
         */
        virtual std::shared_ptr<ISQLData> query(const char* format, ...) = 0;

        /**
         * @brief 执行原始 SQL 查询，并返回结果
         * @param sql SQL 语句
         */
        virtual std::shared_ptr<ISQLData> query(const std::string& sql) = 0;
    };

    /**
     * @brief 用于 SQL 预处理语句的接口
     */
    class IStmt
    {
    public:
        virtual ~IStmt() = default;

        /**
         * @brief 将 int8 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindInt8(int index, const int8_t& value) = 0;

        /**
         * @brief 将 uint8 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindUint8(int index, const uint8_t& value) = 0;

        /**
         * @brief 将 int16 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindInt16(int index, const int16_t& value) = 0;

        /**
         * @brief 将 uint16 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindUint16(int index, const uint16_t& value) = 0;

        /**
         * @brief 将 int32 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindInt32(int index, const int32_t& value) = 0;

        /**
         * @brief 将 uint32 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindUint32(int index, const uint32_t& value) = 0;

        /**
         * @brief 将 int64 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindInt64(int index, const int64_t& value) = 0;

        /**
         * @brief 将 uint64 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindUint64(int index, const uint64_t& value) = 0;

        /**
         * @brief 将 float 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindFloat(int index, const float& value) = 0;

        /**
         * @brief 将 double 值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindDouble(int index, const double& value) = 0;

        /**
         * @brief 将字符串值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindString(int index, const char* value) = 0;

        /**
         * @brief 将字符串值绑定到语句的参数
         * @param index 参数索引
         * @param value 要绑定的值
         */
        virtual int bindString(int index, const std::string& value) = 0;

        /**
         * @brief 将 blob 数据绑定到语句的参数
         * @param index 参数索引
         * @param value blob 数据
         * @param size blob 数据的大小
         */
        virtual int bindBlob(int index, const void* value, int64_t size) = 0;

        /**
         * @brief 将字符串形式的 blob 数据绑定到语句的参数
         * @param index 参数索引
         * @param value blob 数据
         */
        virtual int bindBlob(int index, const std::string& value) = 0;

        /**
         * @brief 将时间戳绑定到语句的参数
         * @param index 参数索引
         * @param value 时间戳值（time_t）
         */
        virtual int bindTime(int index, const time_t& value) = 0;

        /**
         * @brief 将 NULL 值绑定到语句的参数
         * @param index 参数索引
         */
        virtual int bindNull(int index) = 0;

        /**
         * @brief 执行预处理语句
         */
        virtual int execute() = 0;

        /**
         * @brief 获取执行后的最后插入 ID
         */
        virtual int64_t getLastInsertId() = 0;

        /**
         * @brief 执行查询并返回结果
         */
        virtual std::shared_ptr<ISQLData> query() = 0;

        /**
         * @brief 获取预处理语句的错误号
         */
        virtual int getErrno() = 0;

        /**
         * @brief 获取预处理语句的错误信息
         */
        virtual std::string getErrStr() = 0;
    };

    /**
     * @brief SQL 事务的接口
     */
    class ITransaction : public ISQLUpdate
    {
    public:
        ~ITransaction() override = default;

        /**
         * @brief 开始事务
         */
        virtual bool begin() = 0;

        /**
         * @brief 提交事务
         */
        virtual bool commit() = 0;

        /**
         * @brief 回滚事务
         */
        virtual bool rollback() = 0;
    };

    /**
     * @brief 数据库连接和查询管理的接口
     */
    class IDB : public ISQLUpdate, public ISQLQuery
    {
    public:
        ~IDB() override = default;

        /**
         * @brief 准备执行 SQL 语句
         * @param stmt SQL 语句
         */
        virtual std::shared_ptr<IStmt> prepare(const std::string& stmt) = 0;

        /**
         * @brief 获取数据库的错误号
         */
        virtual int getErrno() = 0;

        /**
         * @brief 获取数据库的错误信息
         */
        virtual std::string getErrStr() = 0;

        /**
         * @brief 打开一个事务
         * @param auto_commit 是否自动提交
         */
        virtual std::shared_ptr<ITransaction> openTransaction(bool auto_commit) = 0;
    };
}
#endif
