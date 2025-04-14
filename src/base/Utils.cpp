#include "base/Utils.h"
#include "base/Log.h"

#include <ifaddrs.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstdarg>
#include <csignal>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <execinfo.h>

namespace Gyanis::base
{
    static auto g_logger = LOG_NAME("system");

    pid_t GetThreadID()
    {
        return static_cast<int>(syscall(SYS_gettid));
    }

    std::string demangle(const char* str)
    {
        size_t buffer_size = 0;
        int status = 0;
        std::string result;
        result.resize(256);

        if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &result[0]))
        {
            if (char* demangled_name = abi::__cxa_demangle(&result[0], nullptr, &buffer_size, &status))
            {
                std::string demangled_result(demangled_name);
                free(demangled_name);
                return demangled_result;
            }
        }

        if (1 == sscanf(str, "%255s", &result[0]))
        {
            return result;
        }

        return str;
    }

    void captureBacktrace(std::vector<std::string>& backtrace_output, const int max_depth, const int skip_frames)
    {
        const auto address_array = static_cast<void**>(malloc(sizeof(void*) * max_depth));
        const size_t trace_depth = ::backtrace(address_array, max_depth);

        char** symbol_strings = backtrace_symbols(address_array, static_cast<int>(trace_depth));
        if (symbol_strings == nullptr)
        {
            LOG_ERROR(g_logger)
                << "Error: Failed to retrieve backtrace symbols. "
                << "This may indicate an issue with the backtrace generation process.";
            free(address_array);
            return;
        }

        for (size_t i = skip_frames; i < trace_depth; ++i)
        {
            backtrace_output.push_back(demangle(symbol_strings[i]));
        }

        free(symbol_strings);
        free(address_array);
    }

    std::string backtraceToString(const int max_depth, const int skip_frames, const std::string& prefix)
    {
        std::vector<std::string> backtrace_output;
        captureBacktrace(backtrace_output, max_depth, skip_frames);

        std::stringstream result_stream;
        for (const auto& trace_line : backtrace_output)
        {
            result_stream << prefix << trace_line << std::endl;
        }

        return result_stream.str();
    }

    // 辅助函数：将十六进制字符转换为对应的整数值
    int hex_digit_to_value(const char c)
    {
        if (std::isdigit(c))
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        return 0;
    }

    std::string UrlDecode(const std::string& str, const bool space_as_plus)
    {
        std::string result;
        result.reserve(str.size()); // 预分配内存，减少多次分配

        const char* end = str.c_str() + str.length();
        for (const char* c = str.c_str(); c < end; ++c)
        {
            if (*c == '+' && space_as_plus)
            {
                result.push_back(' '); // 将 "+" 转换为空格
            }
            else if (*c == '%' && (c + 2) < end
                && std::isxdigit(*(c + 1)) && std::isxdigit(*(c + 2)))
            {
                // 解析百分号编码
                const char hex_value = static_cast<char>((hex_digit_to_value(*(c + 1)) << 4) | hex_digit_to_value(
                    *(c + 2)));
                result.push_back(hex_value);
                c += 2; // 跳过已处理的两个字符
            }
            else
            {
                result.push_back(*c);
            }
        }

        return result;
    }

    std::string Trim(const std::string& str, const std::string& delimit)
    {
        const auto begin = str.find_first_not_of(delimit);
        if (begin == std::string::npos)
        {
            return "";
        }
        const auto end = str.find_last_not_of(delimit);
        return str.substr(begin, end - begin + 1);
    }

    std::string Time2Str(const time_t ts, const std::string& format)
    {
        std::stringstream ss;
        ss << std::put_time(std::localtime(&ts), format.c_str());
        return ss.str();
    }

    time_t Str2Time(const char* str, const char* format)
    {
        if (str == nullptr || format == nullptr)
        {
            return -1; // 返回 -1 表示输入参数无效
        }

        tm t = {};
        if (strptime(str, format, &t) == nullptr)
        {
            return -1;
        }

        const time_t result = mktime(&t);
        if (result == -1)
        {
            return -1; // 如果 mktime 失败，返回 -1
        }

        return result;
    }


    bool YamlToJson(const YAML::Node& ynode, nlohmann::json& jnode)
    {
        try
        {
            // 如果是标量类型（字符串、数字等）
            if (ynode.IsScalar())
            {
                jnode = ynode.Scalar();
                return true;
            }

            // 如果是序列（数组）
            if (ynode.IsSequence())
            {
                for (const auto& i : ynode)
                {
                    if (nlohmann::json value; YamlToJson(i, value))
                    {
                        jnode.push_back(value);
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            // 如果是映射（键值对）
            else if (ynode.IsMap())
            {
                for (auto it = ynode.begin(); it != ynode.end(); ++it)
                {
                    if (nlohmann::json value; YamlToJson(it->second, value))
                    {
                        jnode[it->first.Scalar()] = value;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
        catch (...)
        {
            return false;
        }
        return true;
    }

    bool JsonToYaml(const nlohmann::json& jnode, YAML::Node& ynode)
    {
        try
        {
            // 如果是数组类型
            if (jnode.is_array())
            {
                for (const auto& i : jnode)
                {
                    if (YAML::Node n; JsonToYaml(i, n))
                    {
                        ynode.push_back(n);
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            // 如果是对象类型
            else if (jnode.is_object())
            {
                for (auto it = jnode.begin(); it != jnode.end(); ++it)
                {
                    if (YAML::Node n; JsonToYaml(it.value(), n))
                    {
                        ynode[it.key()] = n;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            // 如果是基本类型（如字符串、数字等）
            else
            {
                ynode = jnode.get<std::string>(); // 将基本类型转换为字符串
            }
        }
        catch (...)
        {
            return false; // 处理任何异常并返回失败
        }
        return true; // 转换成功
    }

    std::string StringUtil::Format(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::string result = Formatv(fmt, args);
        va_end(args);
        return result;
    }

    std::string GetHostName()
    {
        std::array<char, 512> host{};
        if (gethostname(host.data(), host.size() - 1) == 0)
        {
            return host.data();
        }
        return ""; // 或者可以抛出异常，表示获取主机名失败
    }

    in_addr_t GetIPv4Inet()
    {
        ifaddrs* ifas = nullptr;
        const in_addr_t localhost = inet_addr("127.0.0.1");

        // 获取网络接口列表
        if (getifaddrs(&ifas) != 0)
        {
            LOG_ERROR(g_logger)
                << "getifaddrs failed. "
                << "Error number: " << errno
                << " | Error string: " << strerror(errno);
            return localhost; // 出错时返回回环地址
        }

        in_addr_t ipv4 = localhost; // 默认设置为回环地址

        // 遍历所有接口地址
        for (const ifaddrs* ifa = ifas; ifa != nullptr; ifa = ifa->ifa_next)
        {
            // 只处理IPv4地址
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
            {
                // 排除环回接口
                if (strncasecmp(ifa->ifa_name, "lo", 2) != 0)
                {
                    ipv4 = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr;
                    // 如果发现有效IPv4地址且不为回环地址，更新 ipv4
                    if (ipv4 != localhost)
                    {
                        break; // 找到有效的 IPv4 地址，提前返回
                    }
                }
            }
        }
        freeifaddrs(ifas);
        return ipv4;
    }

    std::string _GetIPv4()
    {
        // 使用 std::array 来管理内存
        std::array<char, INET_ADDRSTRLEN> ipv4{};

        // 获取 IPv4 地址
        const auto ia = GetIPv4Inet();

        // 转换 IPv4 地址为字符串格式
        if (inet_ntop(AF_INET, &ia, ipv4.data(), INET_ADDRSTRLEN) == nullptr)
        {
            return ""; // 如果转换失败，返回空字符串
        }

        // 返回转换后的 IPv4 地址
        return ipv4.data();
    }

    std::string GetIPv4()
    {
        static const std::string ip = _GetIPv4();
        return ip;
    }

    std::string StringUtil::Formatv(const char* fmt, va_list ap)
    {
        // 计算格式化后的字符串所需的缓冲区大小
        va_list ap_copy;
        va_copy(ap_copy, ap);
        const int size = std::vsnprintf(nullptr, 0, fmt, ap_copy) + 1; // +1 for null terminator
        va_end(ap_copy);

        if (size <= 0)
        {
            throw std::runtime_error("Runtime Error: An error occurred during the formatting process.");
        }

        std::vector<char> buf(size);
        if (std::vsnprintf(buf.data(), size, fmt, ap) < 0)
        {
            throw std::runtime_error("Runtime Error: An error occurred during the formatting process.");
        }

        return buf.data();
    }

    std::string StringUtil::UrlEncode(const std::string& str, bool space_as_plus)
    {
        std::ostringstream encoded;

        for (const unsigned char c : str)
        {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                encoded.put(c); // 直接添加字母或数字
            }
            else if (c == ' ' && space_as_plus)
            {
                encoded.put('+'); // 空格转换为 '+'
            }
            else
            {
                // 转换为十六进制格式并写入输出流
                encoded << '%' << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(c);
            }
        }

        return encoded.str();
    }

    std::string StringUtil::UrlDecode(const std::string& str, const bool space_as_plus)
    {
        std::string result;
        result.reserve(str.size()); // 预分配内存，减少多次分配

        const char* end = str.c_str() + str.length();
        for (const char* c = str.c_str(); c < end; ++c)
        {
            if (*c == '+' && space_as_plus)
            {
                result.push_back(' '); // 将 "+" 转换为空格
            }
            else if (*c == '%' && (c + 2) < end
                && std::isxdigit(*(c + 1)) && std::isxdigit(*(c + 2)))
            {
                // 解析百分号编码
                const char hex_value = static_cast<char>((hex_digit_to_value(*(c + 1)) << 4) | hex_digit_to_value(
                    *(c + 2)));
                result.push_back(hex_value);
                c += 2; // 跳过已处理的两个字符
            }
            else
            {
                result.push_back(*c);
            }
        }
        return result;
    }

    std::string StringUtil::Trim(const std::string& str, const std::string& delimit)
    {
        const auto begin = str.find_first_not_of(delimit);
        if (begin == std::string::npos)
        {
            return "";
        }
        const auto end = str.find_last_not_of(delimit);
        return str.substr(begin, end - begin + 1);
    }

    std::string StringUtil::TrimLeft(const std::string& str, const std::string& delimit)
    {
        const size_t start = str.find_first_not_of(delimit);
        return (start == std::string::npos) ? "" : str.substr(start);
    }

    std::string StringUtil::TrimRight(const std::string& str, const std::string& delimit)
    {
        const size_t end = str.find_last_not_of(delimit);
        return (end == std::string::npos) ? "" : str.substr(0, end + 1);
    }

    std::string StringUtil::WStringToString(const std::wstring& ws)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(ws);
    }

    std::wstring StringUtil::StringToWString(const std::string& s)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(s);
    }

    int8_t TypeUtil::ToChar(const std::string& str)
    {
        // 如果字符串为空，返回 0；否则返回第一个字符的 int8_t 值
        return str.empty() ? 0 : static_cast<int8_t>(static_cast<unsigned char>(str[0]));
    }

    int64_t TypeUtil::Atoi(const std::string& str)
    {
        if (str.empty())
        {
            return 0; // 如果字符串为空，直接返回 0
        }

        try
        {
            // 使用 std::stoll 进行类型安全的转换，指定基数为 10
            size_t pos = 0;
            const int64_t result = std::stoll(str, &pos, 10);

            // 检查是否整个字符串都被成功解析为数字
            if (pos != str.size())
            {
                LOG_ERROR(g_logger) << "TypeUtil::Atoi() fail : invalid characters in string";
                return 0; // 返回 0 如果字符串中有无效字符
            }

            return result;
        }
        catch (const std::invalid_argument& e)
        {
            LOG_ERROR(g_logger) << "TypeUtil::Atoi() fail : " << e.what();
        }
        catch (const std::out_of_range& e)
        {
            LOG_ERROR(g_logger) << "TypeUtil::Atoi() fail : " << e.what();
        }

        return 0; // 出现任何异常或错误时返回 0
    }


    double TypeUtil::Atof(const std::string& str)
    {
        if (str.empty())
        {
            return 0.0; // 如果字符串为空，直接返回 0.0
        }

        try
        {
            size_t pos = 0;
            const double result = std::stod(str, &pos);

            // 检查是否整个字符串都成功转换为数字
            if (pos != str.size())
            {
                LOG_ERROR(g_logger) << "TypeUtil::Atof() fail : invalid characters in string";
                return 0.0; // 如果字符串中有无效字符，则返回 0.0
            }

            return result;
        }
        catch (const std::invalid_argument& e)
        {
            LOG_ERROR(g_logger) << "TypeUtil::Atof() fail : " << e.what();
        }
        catch (const std::out_of_range& e)
        {
            LOG_ERROR(g_logger) << "TypeUtil::Atof() fail : " << e.what();
        }

        return 0.0; // 如果出现任何异常或错误，返回 0.0
    }

    int8_t TypeUtil::ToChar(const char* str)
    {
        return (str && *str) ? static_cast<int8_t>(*str) : 0;
    }


    int64_t TypeUtil::Atoi(const char* str)
    {
        if (str == nullptr || *str == '\0')
        {
            return 0; // 空字符串或空指针返回 0
        }

        char* endptr;
        errno = 0;

        const long long result = std::strtoll(str, &endptr, 10);

        // 检查是否转换了整个字符串，或发生了溢出
        if (endptr == str || *endptr != '\0' || errno == ERANGE || result < LLONG_MIN || result > LLONG_MAX)
        {
            return 0; // 无效转换或溢出返回 0
        }

        return result;
    }

    double TypeUtil::Atof(const char* str)
    {
        if (str == nullptr || *str == '\0')
        {
            return 0.0; // 空字符串或空指针返回 0.0
        }

        // 重置 errno 以便我们检查溢出错误
        errno = 0;

        char* endptr;
        const double result = std::strtod(str, &endptr);

        // 检查是否转换失败或是否有额外字符
        if (endptr == str || *endptr != '\0' || errno == ERANGE)
        {
            return 0.0; // 如果有错误，返回 0.0
        }

        // 检查是否是无效值（NaN）或无穷大（inf）
        if (std::isnan(result) || std::isinf(result))
        {
            return 0.0; // 如果是无效值或无穷大，返回 0.0
        }

        return result;
    }

    void FSUtil::ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix)
    {
        try
        {
            // 使用 std::filesystem 遍历目录
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                const auto& filename = entry.path().filename().string();

                if (is_directory(entry.status()))
                {
                    // 忽略 . 和 .. 目录
                    if (filename != "." && filename != "..")
                    {
                        // 递归处理子目录
                        ListAllFile(files, entry.path().string(), subfix);
                    }
                }
                else if (is_regular_file(entry.status()))
                {
                    // 处理文件，检查后缀
                    if ((subfix.empty() || filename.size() >= subfix.size()) &&
                        (filename.compare(filename.size() - subfix.size(), subfix.size(), subfix) == 0))
                    {
                        files.push_back(entry.path().string());
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_WARN(g_logger) << "Error while accessing directory: " << e.what();
        }
    }

    bool FSUtil::Mkdir(const std::string& dirname)
    {
        if (std::filesystem::exists(dirname))
        {
            return true;
        }

        try
        {
            if (std::filesystem::create_directories(dirname))
            {
                return true;
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR(g_logger) << "Error creating directory: " << e.what();
        }
        return false;
    }

    bool FSUtil::IsRunningPidfile(const std::string& pidfile)
    {
        if (!std::filesystem::exists(pidfile))
        {
            return false;
        }

        std::ifstream ifs(pidfile);
        std::string line;

        if (!std::getline(ifs, line) || line.empty())
        {
            return false;
        }

        pid_t pid = 0;
        try
        {
            pid = std::stoi(line);
        }
        catch (const std::invalid_argument& e)
        {
            LOG_ERROR(g_logger) << "Error while creating pidfile: " << e.what();
            return false;
        }
        catch (const std::out_of_range& e)
        {
            LOG_ERROR(g_logger) << "Error while creating pidfile: " << e.what();
            return false;
        }

        if (pid <= 1)
        {
            return false;
        }
        if (kill(pid, 0) != 0)
        {
            return false;
        }
        return true;
    }

    bool FSUtil::Unlink(const std::string& filename, const bool exist)
    {
        try
        {
            if (!exist && !std::filesystem::exists(filename))
            {
                return true;
            }

            std::filesystem::remove(filename);
            return true; // 删除成功返回 true
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            // 如果出现错误（例如权限问题），可以记录错误并返回 false
            LOG_ERROR(g_logger) << "Error removing file: " << e.what();
            return false;
        }
    }

    bool FSUtil::Rm(const std::string& path)
    {
        try
        {
            // 如果路径不存在，则返回 true（表示没有错误）
            if (!std::filesystem::exists(path))
            {
                return true;
            }

            // 如果是文件，使用 remove 删除文件
            if (std::filesystem::is_regular_file(path))
            {
                std::filesystem::remove(path);
                return true;
            }

            // 如果是目录，使用 remove_all 递归删除目录及其内容
            if (std::filesystem::is_directory(path))
            {
                std::filesystem::remove_all(path); // remove_all 会递归删除目录及其内部的所有文件和子目录
                return true;
            }

            // 如果是其他类型的路径，返回 false
            return false;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR(g_logger) << "Error removing file: " << e.what();
            return false;
        }
    }

    bool FSUtil::Mv(const std::string& from, const std::string& to)
    {
        try
        {
            if (!Rm(to))
            {
                return false;
            }
            std::filesystem::rename(from, to);
            return true; // 成功重命名
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR(g_logger) << "Error removing file: " << e.what();
            return false; // 如果出现错误，返回 false
        }
    }

    bool FSUtil::Realpath(const std::string& path, std::string& rpath)
    {
        try
        {
            // 检查路径是否存在
            if (!std::filesystem::exists(path))
            {
                return false;
            }

            // 使用 std::filesystem::canonical 获取规范化的路径
            rpath = std::filesystem::canonical(path).string();
            return true;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR(g_logger) << "Error creating file: " << e.what();
            // 捕获文件系统错误（例如路径无效等）
            return false;
        }
    }

    bool FSUtil::Symlink(const std::string& from, const std::string& to)
    {
        try
        {
            // 如果目标符号链接已存在，先删除它
            if (std::filesystem::exists(to))
            {
                std::filesystem::remove(to); // 删除已存在的目标
            }
            // 创建符号链接
            std::filesystem::create_symlink(from, to);
            return true; // 成功创建符号链接返回 true
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            // 捕获文件系统错误并输出
            LOG_ERROR(g_logger) << "Error creating file: " << e.what();
            return false; // 创建符号链接失败时返回 false
        }
    }

    std::string FSUtil::Dirname(const std::string& filename)
    {
        // 使用 std::filesystem::path 来处理路径
        const std::filesystem::path p(filename);

        // 如果路径为空或是根路径，返回 "."
        if (p.empty() || p == "/")
        {
            return ".";
        }

        // 返回路径的父目录
        return p.parent_path().string();
    }

    std::string FSUtil::Basename(const std::string& filename)
    {
        std::filesystem::path p(filename);
        return p.filename().string();
    }

    bool FSUtil::OpenForRead(std::ifstream& ifs, const std::string& filename, const std::ios_base::openmode mode)
    {
        ifs.open(filename.c_str(), mode);
        return ifs.is_open();
    }

    bool FSUtil::OpenForWrite(std::ofstream& ofs, const std::string& filename, const std::ios_base::openmode mode)
    {
        ofs.open(filename.c_str(), mode);
        if (!ofs.is_open())
        {
            const std::string dir = Dirname(filename);
            Mkdir(dir);
            ofs.open(filename.c_str(), mode);
        }
        return ofs.is_open();
    }
}
