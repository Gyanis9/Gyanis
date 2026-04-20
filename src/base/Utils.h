/**
 * @file Utils.h
 * @brief 常用工具封装
 * @date 2025-03-13
 */
#ifndef UTILS_H
#define UTILS_H

#include <ctime>
#include <string>
#include <cxxabi.h>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <boost/lexical_cast.hpp>

namespace Gyanis::base
{
    /**
     * @brief 获取类型的可读名称
     */
    template <typename T>
    const char* TypeToName()
    {
        static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
        return s_name;
    }

    /**
     * @brief 将 C++ 类型名从 mangled 格式转换为可读格式
     */
    static std::string demangle(const char* str);

    /**
     * @brief 获取当前线程的ID
     */
    pid_t GetThreadID();

    /**
     * @brief 获取当前的调用栈
     * @param[out] backtrace_output 保存调用栈
     * @param[in] max_depth 最多返回层数
     * @param[in] skip_frames 跳过栈顶的层数
     */
    void captureBacktrace(std::vector<std::string>& backtrace_output, int max_depth, int skip_frames);

    /**
     * @brief 获取当前栈信息的字符串
     * @param[in] max_depth 栈的最大层数
     * @param[in] skip_frames 跳过栈顶的层数
     * @param[in] prefix 栈信息前输出的内容
     */
    std::string backtraceToString(int max_depth, int skip_frames, const std::string& prefix);

    /**
     * @brief 从映射中获取指定键的值，并尝试将其转换为指定的类型
     */
    template <class ValueType, class MapType, class KeyType>
    ValueType GetParamValue(const MapType& param_map, const KeyType& key, const ValueType& default_value = ValueType())
    {
        // 查找键值对是否存在
        auto iterator = param_map.find(key);

        // 如果找不到指定的键，返回默认值
        if (iterator == param_map.end())
        {
            return default_value;
        }

        try
        {
            // 使用 boost::lexical_cast 将找到的值转换为期望的类型
            return boost::lexical_cast<ValueType>(iterator->second);
        }
        catch (...)
        {
            // 捕获所有异常，若转换失败则返回默认值
        }

        // 转换失败时返回默认值
        return default_value;
    }

    template <class T>
    void nop(T*)
    {
    }

    /**
     * @brief 删除动态分配的数组
     */
    template <class T>
    void delete_array(T* ptr)
    {
        delete[] ptr;
    }

    /**
     * @brief 将单个十六进制字符转换为对应的数值
     */
    int hex_digit_to_value(char c);

    /**
     * @brief URL解码
     */
    std::string UrlDecode(const std::string& str, bool space_as_plus = true);

    /**
     * @brief 字符串修剪
     */
    std::string Trim(const std::string& str, const std::string& delimit = " \t\r\n");

    std::string Time2Str(time_t ts = time(nullptr), const std::string& format = "%Y-%m-%d %H:%M:%S");

    time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

    bool YamlToJson(const YAML::Node& ynode, nlohmann::json& jnode);

    bool JsonToYaml(const nlohmann::json& jnode, YAML::Node& ynode);

    /**
     * @brief 获取当前主机的主机名（Hostname）
     */
    std::string GetHostName();

    /**
     * @brief 获取本地计算机的 IPv4 地址
     */
    std::string GetIPv4();


    class StringUtil
    {
    public:
        /**
         * @brief 格式化字符串，类似 printf 的功能
         * @param fmt 格式化字符串
         * @param ... 可变参数
         */
        static std::string Format(const char* fmt, ...);

        /**
         * @brief 格式化字符串，接收 va_list 参数
         * @param fmt 格式化字符串
         * @param ap va_list 参数
         */
        static std::string Formatv(const char* fmt, va_list ap);

        /**
         * @brief URL 编码
         * @param str 输入字符串
         * @param space_as_plus 是否将空格转换为 "+"（默认 true）
         */
        static std::string UrlEncode(const std::string& str, bool space_as_plus = true);

        /**
         * @brief URL 解码
         * @param str 编码后的字符串
         * @param space_as_plus 是否将 "+" 转换为空格（默认 true）
         */
        static std::string UrlDecode(const std::string& str, bool space_as_plus = true);

        /**
         * @brief 去掉字符串两边的指定字符（默认为空格、制表符、回车、换行）
         * @param str 输入字符串
         * @param delimit 要去掉的字符（默认值为 " \t\r\n"）
         */
        static std::string Trim(const std::string& str, const std::string& delimit = " \t\r\n");

        /**
         * @brief 去掉字符串左边的指定字符（默认为空格、制表符、回车、换行）
         * @param str 输入字符串
         * @param delimit 要去掉的字符（默认值为 " \t\r\n"）
         */
        static std::string TrimLeft(const std::string& str, const std::string& delimit = " \t\r\n");

        /**
         * @brief 去掉字符串右边的指定字符（默认为空格、制表符、回车、换行）
         * @param str 输入字符串
         * @param delimit 要去掉的字符（默认值为 " \t\r\n"）
         */
        static std::string TrimRight(const std::string& str, const std::string& delimit = " \t\r\n");

        /**
         * @brief 将 std::wstring 转换为 std::string
         * @param ws 输入宽字符串
         */
        static std::string WStringToString(const std::wstring& ws);

        /**
         * @brief 将 std::string 转换为 std::wstring
         * @param s 输入窄字符串
         */
        static std::wstring StringToWString(const std::string& s);
    };

    class TypeUtil
    {
    public:
        /**
         * @brief 将字符串的第一个字符转换为 int8_t 类型
         * @param str 输入字符串
         */
        static int8_t ToChar(const std::string& str);

        /**
         * @brief 将字符串转换为 int64_t 类型
         * @param str 输入字符串
         */
        static int64_t Atoi(const std::string& str);

        /**
         * @brief 将字符串转换为 double 类型
         * @param str 输入字符串
         */
        static double Atof(const std::string& str);

        /**
         * @brief 将字符串的第一个字符转换为 int8_t 类型
         * @param str 输入 C 字符串
         */
        static int8_t ToChar(const char* str);

        /**
         * @brief 将 C 字符串转换为 int64_t 类型
         * @param str 输入 C 字符串
         */
        static int64_t Atoi(const char* str);

        /**
         * @brief 将 C 字符串转换为 double 类型
         * @param str 输入 C 字符串
         */
        static double Atof(const char* str);
    };

    /**
     * @brief 提供一些常用的文件系统操作工具函数。
     */
    class FSUtil
    {
    public:
        /**
         * @brief 列出指定路径下所有符合条件的文件
         * @param files 存放符合条件的文件名的容器
         * @param path 要搜索的目录路径
         * @param subfix 要匹配的文件后缀名（如 ".txt"）
         */
        static void ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix);

        /**
         * @brief 创建一个新的目录。
         * @param dirname 需要创建的目录路径
         */
        static bool Mkdir(const std::string& dirname);

        /**
         * @brief 检查一个进程是否在运行（根据 PID 文件）
         * @param pidfile PID 文件的路径
         */
        static bool IsRunningPidfile(const std::string& pidfile);

        /**
         * @brief 删除指定路径的文件或目录
         * @param path 要删除的文件或目录路径
         * @return 如果成功删除文件或目录，返回 `true`，否则返回 `false`
         */
        static bool Rm(const std::string& path);

        /**
         * @brief 移动或重命名文件或目录
         * @param from 原始文件路径
         * @param to 目标文件路径
         */
        static bool Mv(const std::string& from, const std::string& to);

        /**
         * @brief 获取符号链接的真实路径
         * @param path 要检查的符号链接路径
         * @param rpath 返回的实际路径
         */
        static bool Realpath(const std::string& path, std::string& rpath);

        /**
         * @brief 创建符号链接。
         * @param from 源文件或目录路径
         * @param to 符号链接目标路径
         */
        static bool Symlink(const std::string& from, const std::string& to);

        /**
         * @brief 删除符号链接或文件
         * @param filename 要删除的文件或符号链接路径
         * @param exist 如果为 `true`，文件不存在时不会报错
         */
        static bool Unlink(const std::string& filename, bool exist = false);

        /**
         * @brief 获取文件路径的目录部分
         * @param filename 完整的文件路径
         * @return 返回文件路径的目录部分
         */
        static std::string Dirname(const std::string& filename);

        /**
         * @brief 获取文件路径的文件名部分
         * @param filename 完整的文件路径
         * @return 返回文件路径的文件名部分
         */
        static std::string Basename(const std::string& filename);

        /**
         * @brief 打开文件进行读取
         * @param ifs 输入文件流（`std::ifstream`），用于读取文件内容
         * @param filename 要打开的文件路径
         * @param mode 打开文件时的模式（如 `std::ios::in`）
         * @return 如果文件成功打开，返回 `true`，否则返回 `false`
         */
        static bool OpenForRead(std::ifstream& ifs, const std::string& filename, std::ios_base::openmode mode);

        /**
         * @brief 打开文件进行写入
         * @param ofs 输出文件流（`std::ofstream`），用于写入文件内容
         * @param filename 要打开的文件路径
         * @param mode 打开文件时的模式（如 `std::ios::out`）
         */
        static bool OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode);
    };

    class Atomic
    {
    public:
        template <class T, class S = T>
        static T addFetch(volatile T& t, S v = 1)
        {
            return __sync_add_and_fetch(&t, (T)v);
        }

        template <class T, class S = T>
        static T subFetch(volatile T& t, S v = 1)
        {
            return __sync_sub_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T orFetch(volatile T& t, S v)
        {
            return __sync_or_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T andFetch(volatile T& t, S v)
        {
            return __sync_and_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T xorFetch(volatile T& t, S v)
        {
            return __sync_xor_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T nandFetch(volatile T& t, S v)
        {
            return __sync_nand_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T fetchAdd(volatile T& t, S v = 1)
        {
            return __sync_fetch_and_add(&t, (T)v);
        }

        template <class T, class S>
        static T fetchSub(volatile T& t, S v = 1)
        {
            return __sync_fetch_and_sub(&t, (T)v);
        }

        template <class T, class S>
        static T fetchOr(volatile T& t, S v)
        {
            return __sync_fetch_and_or(&t, (T)v);
        }

        template <class T, class S>
        static T fetchAnd(volatile T& t, S v)
        {
            return __sync_fetch_and_and(&t, (T)v);
        }

        template <class T, class S>
        static T fetchXor(volatile T& t, S v)
        {
            return __sync_fetch_and_xor(&t, (T)v);
        }

        template <class T, class S>
        static T fetchNand(volatile T& t, S v)
        {
            return __sync_fetch_and_nand(&t, (T)v);
        }

        template <class T, class S>
        static T compareAndSwap(volatile T& t, S old_val, S new_val)
        {
            return __sync_val_compare_and_swap(&t, (T)old_val, (T)new_val);
        }

        template <class T, class S>
        static bool compareAndSwapBool(volatile T& t, S old_val, S new_val)
        {
            return __sync_bool_compare_and_swap(&t, (T)old_val, (T)new_val);
        }
    };
}

#endif
