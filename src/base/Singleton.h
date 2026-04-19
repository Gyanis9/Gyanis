#ifndef SINGLETON_H
#define SINGLETON_H
#include <memory>
#include <mutex>

#include "NonCopyable.h"

/**
 * @brief 单例模式封装类
 * @tparam T 要实现单例模式的类类型
 */
template<typename T>
class Singleton final : NonCopyable
{
public:
    /**
     * @brief 获取单例实例的引用
     */
    static T &GetReference()
    {
        return *GetInstance();
    }

    /**
     * @brief 获取单例实例的裸指针
     */
    static T *GetInstance()
    {
        /// 使用 std::call_once 确保单例对象只初始化一次
        std::call_once(initFlag, []()-> void
        {
            /// 第一次初始化时创建实例
            instance.reset(new T());
        });
        /// 返回单例实例的裸指针
        return instance.get();
    }

protected:
    /**
     * @brief 构造函数
     */
    Singleton() = default;

    /**
     * @brief 析构函数
     */
    ~Singleton() = default;

private:
    static std::unique_ptr<T> instance; ///< 使用 unique_ptr 管理单例对象的生命周期，确保实例在程序退出时被正确销毁
    static std::once_flag     initFlag; ///< 用于确保单例初始化线程安全的标志
};

/// 定义静态成员变量，实例化 unique_ptr 和 once_flag
template<typename T>
std::unique_ptr<T> Singleton<T>::instance = nullptr;

template<typename T>
std::once_flag Singleton<T>::initFlag;


#endif
