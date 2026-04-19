#ifndef SINGLETON_H
#define SINGLETON_H

#include <concepts>
#include <memory>
#include <type_traits>

#include "NonCopyable.h"

/**
 * @brief 单例模式封装类
 * @tparam T 要实现单例模式的类类型 
 */
template<typename T> requires std::default_initializable<T>
class Singleton final : NonCopyable
{
public:
    /**
     * @brief 获取单例实例的引用
     */
    [[nodiscard]] static T &GetReference() noexcept(std::is_nothrow_default_constructible_v<T>)
    {
        static T instance{};
        return instance;
    }

    /**
     * @brief 获取单例实例的裸指针
     */
    [[nodiscard]] static T *GetInstance() noexcept(std::is_nothrow_default_constructible_v<T>)
    {
        return std::addressof(GetReference());
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
};

#endif
