#include "db/Mysql.h"
#include "base/Config.h"
#include "base/Log.h"
#include "base/Utils.h"

namespace Gyanis::db
{
    static auto g_logger = LOG_NAME("system");
    static auto g_mysql_dbs
        = base::Config::LookUp("mysql.dbs",
                               std::unordered_map<std::string, std::unordered_map<std::string, std::string>>()
                               , "mysql dbs");


    MySQLTime::MySQLTime(const time_t time): time(time)
    {
    }

    bool mysql_time_to_time_t(const MYSQL_TIME& mysql_time, time_t& time)
    {
        // 初始化 tm 结构
        tm tm = {};

        // 根据 mysql_time 设置 tm 结构的各个字段
        tm.tm_year = static_cast<int>(mysql_time.year - 1900);
        tm.tm_mon = static_cast<int>(mysql_time.month - 1);
        tm.tm_mday = static_cast<int>(mysql_time.day);
        tm.tm_hour = static_cast<int>(mysql_time.hour);
        tm.tm_min = static_cast<int>(mysql_time.minute);
        tm.tm_sec = static_cast<int>(mysql_time.second);

        // 转换为 time_t
        time = mktime(&tm);

        // 处理 mktime 失败的情况 (mktime 返回 -1 表示失败)
        if (time == static_cast<time_t>(-1))
        {
            time = 0; // 转换失败时将 time 设置为 0
        }

        return true;
    }

    bool time_t_to_mysql_time(const time_t& time, MYSQL_TIME& mysql_time)
    {
        tm tm = {}; // 使用 {} 初始化，保证所有字段为 0
        if (localtime_r(&time, &tm) == nullptr)
        {
            // 处理错误，例如返回 false
            return false;
        }

        mysql_time.year = tm.tm_year + 1900;
        mysql_time.month = tm.tm_mon + 1;
        mysql_time.day = tm.tm_mday;
        mysql_time.hour = tm.tm_hour;
        mysql_time.minute = tm.tm_min;
        mysql_time.second = tm.tm_sec;
        return true;
    }

    namespace
    {
        struct MySQLThreadIniter
        {
            MySQLThreadIniter()
            {
                mysql_thread_init();
            }

            ~MySQLThreadIniter()
            {
                mysql_thread_end();
            }
        };
    }


    static MYSQL* mysql_init(const std::unordered_map<std::string, std::string>& params,
                             const int timeout)
    {
        static thread_local MySQLThreadIniter s_thread_initer;

        MYSQL* mysql = ::mysql_init(nullptr);
        if (mysql == nullptr)
        {
            LOG_ERROR(g_logger) << "MySQL initialization failed.";
            return nullptr;
        }

        // 设置连接超时
        if (timeout > 0)
        {
            if (mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) != 0)
            {
                LOG_ERROR(g_logger) << "Failed to set connection timeout.";
                mysql_close(mysql);
                return nullptr;
            }
        }

        if (mysql_set_character_set(mysql, "utf8mb4") != 0)
        {
            LOG_ERROR(g_logger) << "Failed to set charset: " << mysql_error(mysql);
            mysql_close(mysql);
            return nullptr;
        }

        // 从参数表中提取连接参数
        const int port_param = Gyanis::base::GetParamValue<int>(params, "port", 0);
        if (port_param < 0)
        {
            LOG_ERROR(g_logger) << "Invalid port number: " << port_param;
            mysql_close(mysql);
            return nullptr;
        }
        const auto port = static_cast<unsigned int>(port_param);

        const auto host = Gyanis::base::GetParamValue<std::string>(params, "host");
        const auto user = Gyanis::base::GetParamValue<std::string>(params, "user");
        const auto passwd = Gyanis::base::GetParamValue<std::string>(params, "password");
        const auto dbname = Gyanis::base::GetParamValue<std::string>(params, "dbname");

        // 校验必要参数非空
        if (host.empty() || user.empty() || dbname.empty())
        {
            LOG_ERROR(g_logger) << "Missing required parameters (host/user/dbname).";
            mysql_close(mysql);
            return nullptr;
        }

        // 处理空密码场景（传递 nullptr 而非空字符串）

        // 发起数据库连接
        if (const char* passwd_ptr = passwd.empty() ? nullptr : passwd.c_str(); mysql_real_connect(
            mysql, host.c_str(), user.c_str(), passwd_ptr,
            dbname.c_str(), port, nullptr, 0) == nullptr)
        {
            LOG_ERROR(g_logger)
            << "MySQL connection failed. "
            << "Host: " << host
            << " | Port: " << port
            << " | Database: " << dbname
            << " | Error: " << mysql_error(mysql);
            mysql_close(mysql);
            return nullptr;
        }

        return mysql;
    }

    static MYSQL_RES* my_mysql_query(MYSQL* mysql, const char* sql)
    {
        if (mysql == nullptr)
        {
            LOG_ERROR(g_logger)
                << "MySQL query failed. "
                << "The MySQL connection is null.";
            return nullptr;
        }

        if (sql == nullptr)
        {
            LOG_ERROR(g_logger)
                << "MySQL query failed. "
                << "The SQL query string is null.";
            return nullptr;
        }

        if (::mysql_query(mysql, sql))
        {
            LOG_ERROR(g_logger)
                << "MySQL query execution failed. "
                << "SQL Query: " << sql
                << " | Error: " << mysql_error(mysql);
            return nullptr;
        }

        MYSQL_RES* res = mysql_store_result(mysql);
        if (res == nullptr)
        {
            LOG_ERROR(g_logger)
                << "MySQL result retrieval failed. "
                << "Error: " << mysql_error(mysql);
        }
        return res;
    }

    MySQLRes::MySQLRes(MYSQL_RES* result, const int err, const char* errStr) :
        m_errno(err)
        , m_errstr(errStr)
        , m_cur(nullptr)
        , m_curLength(nullptr)
    {
        if (result)
        {
            m_data.reset(result, mysql_free_result);
        }
    }

    MYSQL_RES* MySQLRes::get() const
    {
        return m_data.get();
    }

    int MySQLRes::getError() const
    {
        return m_errno;
    }

    const std::string& MySQLRes::getErrStr() const
    {
        return m_errstr;
    }

    bool MySQLRes::foreach(const data_cb& cb)
    {
        MYSQL_ROW row;
        const auto fields = getColumnCount();
        int i = 0;
        while ((row = mysql_fetch_row(m_data.get())))
        {
            if (!cb(row, fields, i++))
            {
                break;
            }
        }
        return true;
    }

    int MySQLRes::getDataCount()
    {
        return mysql_num_rows(m_data.get());
    }

    int MySQLRes::getColumnCount()
    {
        return mysql_num_fields(m_data.get());
    }

    int MySQLRes::getColumnBytes(const int index)
    {
        return m_curLength[index];
    }

    int MySQLRes::getColumnType(int index)
    {
        return 0;
    }

    std::string MySQLRes::getColumnName(int index)
    {
        return "";
    }

    bool MySQLRes::isNull(const int index)
    {
        if (m_cur[index] == nullptr)
        {
            return true;
        }
        return false;
    }

    int8_t MySQLRes::getInt8(const int index)
    {
        return getInt64(index);
    }

    uint8_t MySQLRes::getUint8(const int index)
    {
        return getInt64(index);
    }

    int16_t MySQLRes::getInt16(const int index)
    {
        return getInt64(index);
    }

    uint16_t MySQLRes::getUint16(const int index)
    {
        return getInt64(index);
    }

    int32_t MySQLRes::getInt32(const int index)
    {
        return getInt64(index);
    }

    uint32_t MySQLRes::getUint32(const int index)
    {
        return getInt64(index);
    }

    int64_t MySQLRes::getInt64(const int index)
    {
        return base::TypeUtil::Atoi(m_cur[index]);
    }

    uint64_t MySQLRes::getUint64(const int index)
    {
        return getInt64(index);
    }

    float MySQLRes::getFloat(const int index)
    {
        return getDouble(index);
    }

    double MySQLRes::getDouble(const int index)
    {
        return base::TypeUtil::Atof(m_cur[index]);
    }

    std::string MySQLRes::getString(const int index)
    {
        return {m_cur[index], m_curLength[index]};
    }

    std::string MySQLRes::getBlob(const int index)
    {
        return {m_cur[index], m_curLength[index]};
    }

    time_t MySQLRes::getTime(const int index)
    {
        if (!m_cur[index])
        {
            return 0;
        }
        return base::Str2Time(m_cur[index]);
    }

    bool MySQLRes::next()
    {
        m_cur = mysql_fetch_row(m_data.get());
        m_curLength = mysql_fetch_lengths(m_data.get());
        return m_cur;
    }

    std::shared_ptr<MySQLStmtRes> MySQLStmtRes::Create(const std::shared_ptr<MySQLStmt>& stmt)
    {
        const auto eno = mysql_stmt_errno(stmt->getRaw());
        const char* errstr = mysql_stmt_error(stmt->getRaw());
        std::shared_ptr<MySQLStmtRes> result(new MySQLStmtRes(stmt, eno, errstr));
        if (eno)
        {
            return result;
        }
        MYSQL_RES* res = mysql_stmt_result_metadata(stmt->getRaw());
        if (!res)
        {
            return std::shared_ptr<MySQLStmtRes>(new MySQLStmtRes(stmt, stmt->getErrno()
                                                                  , stmt->getErrStr()));
        }

        const auto num = mysql_num_fields(res);
        const MYSQL_FIELD* fields = mysql_fetch_fields(res);

        result->m_binds.resize(num);
        memset(&result->m_binds[0], 0, sizeof(result->m_binds[0]) * num);
        result->m_datas.resize(num);

        for (size_t i = 0; i < num; ++i)
        {
            result->m_datas[i].type = fields[i].type;
            switch (fields[i].type)
            {
            case MYSQL_TYPE_TINY:
                result->m_datas[i].alloc(sizeof(int8_t));
                break;
            case MYSQL_TYPE_SHORT:
                result->m_datas[i].alloc(sizeof(int16_t));
                break;
            case MYSQL_TYPE_LONG:
                result->m_datas[i].alloc(sizeof(int32_t));
                break;
            case MYSQL_TYPE_LONGLONG:
                result->m_datas[i].alloc(sizeof(int64_t));
                break;
            case MYSQL_TYPE_FLOAT:
                result->m_datas[i].alloc(sizeof(float));
                break;
            case MYSQL_TYPE_DOUBLE:
                result->m_datas[i].alloc(sizeof(double));
                break;
            case MYSQL_TYPE_TIMESTAMP:
                result->m_datas[i].alloc(sizeof(MYSQL_TIME));
                break;
            case MYSQL_TYPE_DATETIME:
                result->m_datas[i].alloc(sizeof(MYSQL_TIME));
                break;
            case MYSQL_TYPE_DATE:
                result->m_datas[i].alloc(sizeof(MYSQL_TIME));
                break;
            case MYSQL_TYPE_TIME:
                result->m_datas[i].alloc(sizeof(MYSQL_TIME));
                break;
            default:
                result->m_datas[i].alloc(fields[i].length);
                break;
            }

            result->m_binds[i].buffer_type = result->m_datas[i].type;
            result->m_binds[i].buffer = result->m_datas[i].data;
            result->m_binds[i].buffer_length = result->m_datas[i].data_length;
            result->m_binds[i].length = &result->m_datas[i].length;
            result->m_binds[i].is_null = &result->m_datas[i].is_null;
            result->m_binds[i].error = &result->m_datas[i].error;
        }

        if (mysql_stmt_bind_result(stmt->getRaw(), &result->m_binds[0]))
        {
            return std::shared_ptr<MySQLStmtRes>(new MySQLStmtRes(stmt, stmt->getErrno()
                                                                  , stmt->getErrStr()));
        }

        stmt->execute();

        if (mysql_stmt_store_result(stmt->getRaw()))
        {
            return std::shared_ptr<MySQLStmtRes>(new MySQLStmtRes(stmt, stmt->getErrno()
                                                                  , stmt->getErrStr()));
        }
        return result;
    }

    MySQLStmtRes::~MySQLStmtRes()
    {
        if (!m_errno)
        {
            mysql_stmt_free_result(m_stmt->getRaw());
        }
    }

    int MySQLStmtRes::getError() const
    {
        return m_errno;
    }

    const std::string& MySQLStmtRes::getErrStr() const
    {
        return m_errstr;
    }

    int MySQLStmtRes::getDataCount()
    {
        return mysql_stmt_num_rows(m_stmt->getRaw());
    }

    int MySQLStmtRes::getColumnCount()
    {
        return mysql_stmt_field_count(m_stmt->getRaw());
    }

    int MySQLStmtRes::getColumnBytes(const int index)
    {
        return static_cast<int>(m_datas[index].length);
    }

    int MySQLStmtRes::getColumnType(const int index)
    {
        return m_datas[index].type;
    }

    std::string MySQLStmtRes::getColumnName(int index)
    {
        return "";
    }

    bool MySQLStmtRes::isNull(const int index)
    {
        return m_datas[index].is_null;
    }

    int8_t MySQLStmtRes::getInt8(const int index)
    {
        return *reinterpret_cast<int8_t*>(m_datas[index].data);
    }

    uint8_t MySQLStmtRes::getUint8(const int index)
    {
        return *reinterpret_cast<uint8_t*>(m_datas[index].data);
    }

    int16_t MySQLStmtRes::getInt16(const int index)
    {
        return *reinterpret_cast<int16_t*>(m_datas[index].data);
    }

    uint16_t MySQLStmtRes::getUint16(const int index)
    {
        return *reinterpret_cast<uint16_t*>(m_datas[index].data);
    }

    int32_t MySQLStmtRes::getInt32(const int index)
    {
        return *reinterpret_cast<int32_t*>(m_datas[index].data);
    }

    uint32_t MySQLStmtRes::getUint32(const int index)
    {
        return *reinterpret_cast<uint32_t*>(m_datas[index].data);
    }

    int64_t MySQLStmtRes::getInt64(const int index)
    {
        return *reinterpret_cast<int64_t*>(m_datas[index].data);
    }

    uint64_t MySQLStmtRes::getUint64(const int index)
    {
        return *reinterpret_cast<uint64_t*>(m_datas[index].data);
    }

    float MySQLStmtRes::getFloat(const int index)
    {
        return *reinterpret_cast<float*>(m_datas[index].data);
    }

    double MySQLStmtRes::getDouble(const int index)
    {
        return *reinterpret_cast<double*>(m_datas[index].data);
    }

    std::string MySQLStmtRes::getString(const int index)
    {
        return {m_datas[index].data, m_datas[index].length};
    }

    std::string MySQLStmtRes::getBlob(const int index)
    {
        return {m_datas[index].data, m_datas[index].length};
    }

    time_t MySQLStmtRes::getTime(const int index)
    {
        const MYSQL_TIME* value = reinterpret_cast<MYSQL_TIME*>(m_datas[index].data);
        time_t ts = 0;
        mysql_time_to_time_t(*value, ts);
        return ts;
    }

    bool MySQLStmtRes::next()
    {
        return !mysql_stmt_fetch(m_stmt->getRaw());
    }

    MySQLStmtRes::MySQLStmtRes(const std::shared_ptr<MySQLStmt>& stmt, const int eno,
                               std::string errStr)
        : m_errno(eno)
          , m_errstr(std::move(errStr))
          , m_stmt(stmt)
    {
    }

    MySQLStmtRes::Data::Data()
        : is_null(false)
          , error(false)
          , type()
          , length(0)
          , data_length(0)
          , data(nullptr)
    {
    }

    MySQLStmtRes::Data::~Data()
    {
        delete[] data;
    }

    void MySQLStmtRes::Data::alloc(const size_t size)
    {
        delete[] data;
        data = new char[size]();
        length = size;
        data_length = size;
    }

    MySQL::MySQL(const std::unordered_map<std::string, std::string>& args)
        : m_params(args)
          , m_lastUsedTime(0)
          , m_hasError(false)
          , m_poolSize(10)
    {
    }

    bool MySQL::connect()
    {
        if (m_mysql && !m_hasError)
        {
            return true;
        }

        MYSQL* mysql = mysql_init(m_params, 5);
        if (!mysql)
        {
            m_hasError = true;
            return false;
        }
        m_hasError = false;
        m_poolSize = base::GetParamValue<int32_t>(m_params, "pool", 5);
        m_mysql.reset(mysql, mysql_close);
        return true;
    }

    bool MySQL::ping()
    {
        if (!m_mysql)
        {
            return false;
        }
        if (mysql_ping(m_mysql.get()))
        {
            m_hasError = true;
            return false;
        }
        m_hasError = false;
        return true;
    }

    int MySQL::execute(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        const int result = execute(format, ap);
        va_end(ap);
        return result;
    }

    int MySQL::execute(const char* format, va_list ap)
    {
        m_cmd = base::StringUtil::Formatv(format, ap);
        const int result = ::mysql_query(m_mysql.get(), m_cmd.c_str());
        if (result)
        {
            LOG_ERROR(g_logger)
                << "Command execution failed. "
                << "Command: " << cmd()
                << " | Error: " << getErrStr();
            m_hasError = true;
        }
        else
        {
            m_hasError = false;
        }
        return result;
    }

    int MySQL::execute(const std::string& sql)
    {
        m_cmd = sql;
        const int result = ::mysql_query(m_mysql.get(), m_cmd.c_str());
        if (result)
        {
            LOG_ERROR(g_logger)
                << "Command execution failed. "
                << "Command: " << cmd()
                << " | Error: " << getErrStr();
            m_hasError = true;
        }
        else
        {
            m_hasError = false;
        }
        return result;
    }

    int64_t MySQL::getLastInsertId()
    {
        return static_cast<int>(mysql_insert_id(m_mysql.get()));
    }

    std::shared_ptr<MySQL> MySQL::getMySQL()
    {
        return std::shared_ptr<MySQL>(this, base::nop<MySQL>);
    }

    std::shared_ptr<MYSQL> MySQL::getRaw()
    {
        return m_mysql;
    }

    uint64_t MySQL::getAffectedRows() const
    {
        if (!m_mysql)
        {
            return 0;
        }
        return mysql_affected_rows(m_mysql.get());
    }

    std::shared_ptr<ISQLData> MySQL::query(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        auto result = query(format, ap);
        va_end(ap);
        return result;
    }

    std::shared_ptr<ISQLData> MySQL::query(const char* format, va_list ap)
    {
        m_cmd = base::StringUtil::Formatv(format, ap);
        MYSQL_RES* res = my_mysql_query(m_mysql.get(), m_cmd.c_str());
        if (!res)
        {
            m_hasError = true;
            return nullptr;
        }
        m_hasError = false;
        return std::make_shared<MySQLRes>(res, mysql_errno(m_mysql.get()), mysql_error(m_mysql.get()));
    }

    std::shared_ptr<ISQLData> MySQL::query(const std::string& sql)
    {
        m_cmd = sql;
        MYSQL_RES* res = my_mysql_query(m_mysql.get(), m_cmd.c_str());
        if (!res)
        {
            m_hasError = true;
            return nullptr;
        }
        m_hasError = false;
        return std::make_shared<MySQLRes>(res, mysql_errno(m_mysql.get())
                                          , mysql_error(m_mysql.get()));
    }

    std::shared_ptr<ITransaction> MySQL::openTransaction(const bool auto_commit)
    {
        return MySQLTransaction::Create(shared_from_this(), auto_commit);
    }

    std::shared_ptr<IStmt> MySQL::prepare(const std::string& sql)
    {
        return MySQLStmt::Create(shared_from_this(), sql);
    }

    const char* MySQL::cmd() const
    {
        return m_cmd.c_str();
    }

    bool MySQL::use(const std::string& dbname)
    {
        if (!m_mysql)
        {
            return false;
        }
        if (m_dbname == dbname)
        {
            return true;
        }
        if (mysql_select_db(m_mysql.get(), dbname.c_str()) == 0)
        {
            m_dbname = dbname;
            m_hasError = false;
            return true;
        }
        m_dbname = "";
        m_hasError = true;
        return false;
    }

    int MySQL::getErrno()
    {
        if (!m_mysql)
        {
            return -1;
        }
        return mysql_errno(m_mysql.get());
    }

    std::string MySQL::getErrStr()
    {
        if (!m_mysql)
        {
            return "mysql is null";
        }
        if (const char* str = mysql_error(m_mysql.get()))
        {
            return str;
        }
        return "";
    }

    uint64_t MySQL::getInsertId() const
    {
        if (m_mysql)
        {
            return mysql_insert_id(m_mysql.get());
        }
        return 0;
    }

    bool MySQL::isNeedCheck() const
    {
        if (((time(nullptr) - m_lastUsedTime) < 5)
            && !m_hasError)
        {
            return false;
        }
        return true;
    }

    std::shared_ptr<MySQLTransaction> MySQLTransaction::Create(const std::shared_ptr<MySQL>& mysql, bool auto_commit)
    {
        if (std::shared_ptr<MySQLTransaction> result(new MySQLTransaction(mysql, auto_commit)); result->begin())
        {
            return result;
        }
        return nullptr;
    }

    MySQLTransaction::~MySQLTransaction()
    {
        if (m_autoCommit)
        {
            commit();
        }
        else
        {
            rollback();
        }
    }

    bool MySQLTransaction::begin()
    {
        const int result = execute("BEGIN");
        return result == 0;
    }

    bool MySQLTransaction::commit()
    {
        if (m_isFinished || m_hasError)
        {
            return !m_hasError;
        }
        const int result = execute("COMMIT");
        if (result == 0)
        {
            m_isFinished = true;
        }
        else
        {
            m_hasError = true;
        }
        return result == 0;
    }

    bool MySQLTransaction::rollback()
    {
        if (m_isFinished)
        {
            return true;
        }
        const int result = execute("ROLLBACK");
        if (result == 0)
        {
            m_isFinished = true;
        }
        else
        {
            m_hasError = true;
        }
        return result == 0;
    }

    int MySQLTransaction::execute(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        return execute(format, ap);
    }

    int MySQLTransaction::execute(const char* format, va_list ap)
    {
        if (m_isFinished)
        {
            LOG_ERROR(g_logger) << "Transaction has been completed. Format: " << format;
            return -1;
        }
        const int result = m_mysql->execute(format, ap);
        if (result)
        {
            m_hasError = true;
        }
        return result;
    }

    int MySQLTransaction::execute(const std::string& sql)
    {
        if (m_isFinished)
        {
            LOG_ERROR(g_logger) << "The transaction has been completed. SQL statement: " << sql;
            return -1;
        }
        const int result = m_mysql->execute(sql);
        if (result)
        {
            m_hasError = true;
        }
        return result;
    }

    int64_t MySQLTransaction::getLastInsertId()
    {
        return m_mysql->getLastInsertId();
    }

    std::shared_ptr<MySQL> MySQLTransaction::getMySQL()
    {
        return m_mysql;
    }

    bool MySQLTransaction::isAutoCommit() const
    {
        return m_autoCommit;
    }

    bool MySQLTransaction::isFinished() const
    {
        return m_isFinished;
    }

    bool MySQLTransaction::isError() const
    {
        return m_hasError;
    }

    MySQLTransaction::MySQLTransaction(const std::shared_ptr<MySQL>& mysql, const bool auto_commit)
        : m_mysql(mysql)
          , m_autoCommit(auto_commit)
          , m_isFinished(false)
          , m_hasError(false)
    {
    }

    std::shared_ptr<MySQLStmt> MySQLStmt::Create(const std::shared_ptr<MySQL>& db, const std::string& stmt)
    {
        auto st = mysql_stmt_init(db->getRaw().get());
        if (!st)
        {
            return nullptr;
        }
        if (mysql_stmt_prepare(st, stmt.c_str(), stmt.size()))
        {
            LOG_ERROR(g_logger) << "An error occurred while executing the statement. "
                                << "Statement: " << stmt
                                << ", Error Number: " << mysql_stmt_errno(st)
                                << ", Error Message: " << mysql_stmt_error(st);
            mysql_stmt_close(st);
            return nullptr;
        }
        const auto count = mysql_stmt_param_count(st);
        std::shared_ptr<MySQLStmt> result(new MySQLStmt(db, st));
        result->m_binds.resize(count);
        memset(&result->m_binds[0], 0, sizeof(result->m_binds[0]) * count);
        return result;
    }

    MySQLStmt::~MySQLStmt()
    {
        if (m_stmt)
        {
            mysql_stmt_close(m_stmt);
        }

        for (const auto& i : m_binds)
        {
            if (i.buffer)
            {
                free(i.buffer);
            }
        }
    }

    int MySQLStmt::bind(const int index, const int8_t& value)
    {
        return bindInt8(index, value);
    }

    int MySQLStmt::bind(const int index, const uint8_t& value)
    {
        return bindUint8(index, value);
    }

    int MySQLStmt::bind(const int index, const int16_t& value)
    {
        return bindInt16(index, value);
    }

    int MySQLStmt::bind(const int index, const uint16_t& value)
    {
        return bindUint16(index, value);
    }

    int MySQLStmt::bind(const int index, const int32_t& value)
    {
        return bindInt32(index, value);
    }

    int MySQLStmt::bind(const int index, const uint32_t& value)
    {
        return bindUint32(index, value);
    }

    int MySQLStmt::bind(const int index, const int64_t& value)
    {
        return bindInt64(index, value);
    }

    int MySQLStmt::bind(const int index, const uint64_t& value)
    {
        return bindUint64(index, value);
    }

    int MySQLStmt::bind(const int index, const float& value)
    {
        return bindFloat(index, value);
    }

    int MySQLStmt::bind(const int index, const double& value)
    {
        return bindDouble(index, value);
    }

    int MySQLStmt::bind(const int index, const std::string& value)
    {
        return bindString(index, value);
    }

    int MySQLStmt::bind(const int index, const char* value)
    {
        return bindString(index, value);
    }

    int MySQLStmt::bind(const int index, const void* value, const int len)
    {
        return bindBlob(index, value, len);
    }

    int MySQLStmt::bind(int index)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_NULL;
        return 0;
    }

    int MySQLStmt::bindInt8(int index, const int8_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_TINY;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = false;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindUint8(int index, const uint8_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_TINY;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = true;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindInt16(int index, const int16_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_SHORT;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = false;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindUint16(int index, const uint16_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_SHORT;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = true;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindInt32(int index, const int32_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_LONG;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = false;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindUint32(int index, const uint32_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_LONG;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = true;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindInt64(int index, const int64_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_LONGLONG;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = false;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindUint64(int index, const uint64_t& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_LONGLONG;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].is_unsigned = true;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindFloat(int index, const float& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_FLOAT;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindDouble(int index, const double& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_DOUBLE;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(sizeof(value));
        }
        memcpy(m_binds[index].buffer, &value, sizeof(value));;
        m_binds[index].buffer_length = sizeof(value);
        return 0;
    }

    int MySQLStmt::bindString(int index, const char* value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_STRING;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(strlen(value));
        }
        else if (m_binds[index].buffer_length < strlen(value))
        {
            free(m_binds[index].buffer);
            m_binds[index].buffer = malloc(strlen(value));
        }
        memcpy(m_binds[index].buffer, value, strlen(value));
        m_binds[index].buffer_length = strlen(value);;
        return 0;
    }

    int MySQLStmt::bindString(int index, const std::string& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_STRING;
        if (m_binds[index].buffer == nullptr) { m_binds[index].buffer = malloc(value.size()); }
        else if ((size_t)m_binds[index].buffer_length < (size_t)value.size())
        {
            free(m_binds[index].buffer);
            m_binds[index].buffer = malloc(value.size());
        }
        memcpy(m_binds[index].buffer, value.c_str(), value.size());
        m_binds[index].buffer_length = value.size();;
        return 0;
    }

    int MySQLStmt::bindBlob(int index, const void* value, const int64_t size)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_BLOB;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(size);
        }
        else if (m_binds[index].buffer_length < static_cast<size_t>(size))
        {
            free(m_binds[index].buffer);
            m_binds[index].buffer = malloc(size);
        }
        memcpy(m_binds[index].buffer, value, size);
        m_binds[index].buffer_length = size;;
        return 0;
    }

    int MySQLStmt::bindBlob(int index, const std::string& value)
    {
        index -= 1;
        m_binds[index].buffer_type = MYSQL_TYPE_BLOB;
        if (m_binds[index].buffer == nullptr)
        {
            m_binds[index].buffer = malloc(value.size());
        }
        else if (m_binds[index].buffer_length < value.size())
        {
            free(m_binds[index].buffer);
            m_binds[index].buffer = malloc(value.size());
        }
        memcpy(m_binds[index].buffer, value.c_str(), value.size());
        m_binds[index].buffer_length = value.size();;
        return 0;
    }

    int MySQLStmt::bindTime(const int index, const time_t& value)
    {
        return bindString(index, base::Time2Str(value));
    }

    int MySQLStmt::bindNull(const int index)
    {
        return bind(index);
    }

    int MySQLStmt::getErrno()
    {
        return mysql_stmt_errno(m_stmt);
    }

    std::string MySQLStmt::getErrStr()
    {
        if (const char* e = mysql_stmt_error(m_stmt))
        {
            return e;
        }
        return "";
    }

    int MySQLStmt::execute()
    {
        mysql_stmt_bind_param(m_stmt, &m_binds[0]);
        return mysql_stmt_execute(m_stmt);
    }

    int64_t MySQLStmt::getLastInsertId()
    {
        return mysql_stmt_insert_id(m_stmt);
    }

    std::shared_ptr<ISQLData> MySQLStmt::query()
    {
        mysql_stmt_bind_param(m_stmt, &m_binds[0]);
        return MySQLStmtRes::Create(shared_from_this());
    }

    MYSQL_STMT* MySQLStmt::getRaw() const
    {
        return m_stmt;
    }

    MySQLStmt::MySQLStmt(const std::shared_ptr<MySQL>& db, MYSQL_STMT* stmt)
        : m_mysql(db)
          , m_stmt(stmt)
    {
    }

    MySQLManager::MySQLManager()
        : m_maxConn(10)
    {
        mysql_library_init(0, nullptr, nullptr);
    }

    MySQLManager::~MySQLManager()
    {
        mysql_library_end();
        for (auto& [fst, snd] : m_conns)
        {
            for (const auto& n : snd)
            {
                delete n;
            }
        }
    }

    std::shared_ptr<MySQL> MySQLManager::get(const std::string& name)
    {
        std::unique_lock lock(m_mutex);
        const auto it = m_conns.find(name);
        if (it != m_conns.end())
        {
            if (!it->second.empty())
            {
                MySQL* rt = it->second.front();
                it->second.pop_front();
                lock.unlock();
                if (!rt->isNeedCheck())
                {
                    rt->m_lastUsedTime = time(nullptr);
                    return {
                        rt, std::bind(&MySQLManager::freeMySQL,
                                      this, name, std::placeholders::_1)
                    };
                }
                if (rt->ping())
                {
                    rt->m_lastUsedTime = time(nullptr);
                    return {
                        rt, std::bind(&MySQLManager::freeMySQL,
                                      this, name, std::placeholders::_1)
                    };
                }
                if (rt->connect())
                {
                    rt->m_lastUsedTime = time(nullptr);
                    return {
                        rt, std::bind(&MySQLManager::freeMySQL,
                                      this, name, std::placeholders::_1)
                    };
                }
                LOG_WARN(g_logger) << "Failed to reconnect to " << name << ".";
                return nullptr;
            }
        }
        auto config = g_mysql_dbs->getValue();
        auto sit = config.find(name);
        std::unordered_map<std::string, std::string> args;
        if (sit != config.end())
        {
            args = sit->second;
        }
        else
        {
            sit = m_dbDefines.find(name);
            if (sit != m_dbDefines.end())
            {
                args = sit->second;
            }
            else
            {
                return nullptr;
            }
        }
        lock.unlock();
        if (const auto result = new MySQL(args); result->connect())
        {
            result->m_lastUsedTime = time(nullptr);
            return {
                result, std::bind(&MySQLManager::freeMySQL,
                                  this, name, std::placeholders::_1)
            };
        }
        else
        {
            delete result;
            return nullptr;
        }
    }

    void MySQLManager::registerMySQL(const std::string& name,
                                     const std::unordered_map<std::string, std::string>& params)
    {
        std::lock_guard lock(m_mutex);
        m_dbDefines[name] = params;
    }

    void MySQLManager::checkConnection(const int sec)
    {
        const time_t now = time(nullptr);
        std::vector<MySQL*> conns;
        std::unique_lock lock(m_mutex);
        for (auto& [fst, snd] : m_conns)
        {
            for (auto it = snd.begin();
                 it != snd.end();)
            {
                if (static_cast<int>(now - (*it)->m_lastUsedTime) >= sec)
                {
                    auto tmp = *it;
                    snd.erase(it++);
                    conns.push_back(tmp);
                }
                else
                {
                    ++it;
                }
            }
        }
        lock.unlock();
        for (const auto& i : conns)
        {
            delete i;
        }
    }

    uint32_t MySQLManager::getMaxConn() const
    {
        return m_maxConn;
    }

    void MySQLManager::setMaxConn(const uint32_t value)
    {
        m_maxConn = value;
    }

    int MySQLManager::execute(const std::string& name, const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        const int result = execute(name, format, ap);
        va_end(ap);
        return result;
    }

    int MySQLManager::execute(const std::string& name, const char* format, va_list ap)
    {
        const auto conn = get(name);
        if (!conn)
        {
            LOG_ERROR(g_logger) << "MySQLManager::execute failed to retrieve data for " << name
                                << ". Format: " << format << ".";
            return -1;
        }
        return conn->execute(format, ap);
    }

    int MySQLManager::execute(const std::string& name, const std::string& sql)
    {
        const auto conn = get(name);
        if (!conn)
        {
            LOG_ERROR(g_logger) << "MySQLManager::execute failed to retrieve data for " << name
                                << ". SQL query: " << sql << ".";
            return -1;
        }
        return conn->execute(sql);
    }

    std::shared_ptr<ISQLData> MySQLManager::query(const std::string& name, const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        auto res = query(name, format, ap);
        va_end(ap);
        return res;
    }

    std::shared_ptr<ISQLData> MySQLManager::query(const std::string& name, const char* format, va_list ap)
    {
        const auto conn = get(name);
        if (!conn)
        {
            LOG_ERROR(g_logger) << "MySQLManager::query failed to retrieve data for " << name
                                << ". Format: " << format << ".";
            return nullptr;
        }
        return conn->query(format, ap);
    }

    std::shared_ptr<ISQLData> MySQLManager::query(const std::string& name, const std::string& sql)
    {
        const auto conn = get(name);
        if (!conn)
        {
            LOG_ERROR(g_logger) << "MySQLManager::query failed to retrieve data for " << name
                                << ". SQL query: " << sql << ".";
            return nullptr;
        }
        return conn->query(sql);
    }

    std::shared_ptr<MySQLTransaction> MySQLManager::openTransaction(const std::string& name, bool auto_commit)
    {
        const auto conn = get(name);
        if (!conn)
        {
            LOG_ERROR(g_logger) << "MySQLManager::openTransaction failed to retrieve data for " << name << ".";
            return nullptr;
        }
        auto trans(MySQLTransaction::Create(conn, auto_commit));
        return trans;
    }

    void MySQLManager::freeMySQL(const std::string& name, MySQL* m)
    {
        if (m->m_mysql)
        {
            std::unique_lock lock(m_mutex);
            if (m_conns[name].size() < static_cast<size_t>(m->m_poolSize))
            {
                m_conns[name].push_back(m);
                return;
            }
        }
        delete m;
    }

    std::shared_ptr<ISQLData> MySQLUtil::Query(const std::string& name, const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        auto rpy = Query(name, format, ap);
        va_end(ap);
        return rpy;
    }

    std::shared_ptr<ISQLData> MySQLUtil::Query(const std::string& name, const char* format, va_list ap)
    {
        const auto m = MySQLMgr::GetInstance()->get(name);
        if (!m)
        {
            return nullptr;
        }
        return m->query(format, ap);
    }

    std::shared_ptr<ISQLData> MySQLUtil::Query(const std::string& name, const std::string& sql)
    {
        const auto result = MySQLMgr::GetInstance()->get(name);
        if (!result)
        {
            return nullptr;
        }
        return result->query(sql);
    }

    std::shared_ptr<ISQLData> MySQLUtil::TryQuery(const std::string& name, uint32_t count, const char* format, ...)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            va_list ap;
            va_start(ap, format);
            auto rpy = Query(name, format, ap);
            va_end(ap);
            if (rpy)
            {
                return rpy;
            }
        }
        return nullptr;
    }

    std::shared_ptr<ISQLData> MySQLUtil::TryQuery(const std::string& name, uint32_t count, const std::string& sql)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            if (auto rpy = Query(name, sql))
            {
                return rpy;
            }
        }
        return nullptr;
    }

    int MySQLUtil::Execute(const std::string& name, const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        const auto result = Execute(name, format, ap);
        va_end(ap);
        return result;
    }

    int MySQLUtil::Execute(const std::string& name, const char* format, va_list ap)
    {
        const auto result = MySQLMgr::GetInstance()->get(name);
        if (!result)
        {
            return -1;
        }
        return result->execute(format, ap);
    }

    int MySQLUtil::Execute(const std::string& name, const std::string& sql)
    {
        const auto result = MySQLMgr::GetInstance()->get(name);
        if (!result)
        {
            return -1;
        }
        return result->execute(sql);
    }

    int MySQLUtil::TryExecute(const std::string& name, const uint32_t count, const char* format, ...)
    {
        int result = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            va_list ap;
            va_start(ap, format);
            result = Execute(name, format, ap);
            va_end(ap);
            if (!result)
            {
                return result;
            }
        }
        return result;
    }

    int MySQLUtil::TryExecute(const std::string& name, const uint32_t count, const char* format, va_list ap)
    {
        int result = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            result = Execute(name, format, ap);
            if (!result)
            {
                return result;
            }
        }
        return result;
    }

    int MySQLUtil::TryExecute(const std::string& name, const uint32_t count, const std::string& sql)
    {
        int result = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            result = Execute(name, sql);
            if (!result)
            {
                return result;
            }
        }
        return result;
    }
}
