/**
 * @file Library.h
 * @brief 动态加载和管理模块封装
 * @date 2025-04-03
 */

#ifndef LIBRARY_H
#define LIBRARY_H

#include "Module.h"

namespace Gyanis::net
{
    /**
     * @brief 该类用于加载和管理模块。通过该类，可以根据指定的路径动态加载模块，并获取模块的实例
     */
    class Library
    {
    public:
        /**
         * @brief 从指定路径获取模块
         * @param path 模块的文件路径
         * @return 返回一个指向 `Module` 对象的共享指针。如果加载失败，返回一个空指针
         */
        static std::shared_ptr<Module> GetModule(const std::string& path);
    };
}

#endif
