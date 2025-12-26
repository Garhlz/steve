#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <map>
#include <string>
#include <memory>
#include <iostream>
#include "Core/TriMesh.h"

class ResourceManager {
public:
    // 获取单例实例
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    // 禁止拷贝和赋值
    ResourceManager(const ResourceManager&) = delete;
    void operator=(const ResourceManager&) = delete;

    // 核心接口：获取模型
    // 如果缓存里有，直接返回；如果没有，加载后放入缓存再返回
    std::shared_ptr<TriMesh> getMesh(const std::string& path);

    // 清理所有资源 (通常在游戏结束时调用，或者智能指针自动释放)
    void clear();

private:
    ResourceManager() = default; // 私有构造

    // 缓存池：路径 -> 模型指针
    std::map<std::string, std::shared_ptr<TriMesh>> meshes;
};

#endif