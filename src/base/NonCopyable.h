#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H
/**
 * @brief 对象无法拷贝,赋值
 */
class NonCopyable
{
public:
    /**
     * @brief 拷贝构造函数(禁用)
     */
    NonCopyable(const NonCopyable &other) = delete;

    /**
     * @brief 赋值函数(禁用)
     */
    NonCopyable &operator=(const NonCopyable &other) = delete;

protected:
    /**
     * @brief 默认构造函数
     */
    NonCopyable() = default;

    /**
     * @brief 默认析构函数
     */
    ~NonCopyable() = default;
};

#endif
