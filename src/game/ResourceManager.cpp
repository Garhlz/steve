#include "ResourceManager.h"

std::shared_ptr<TriMesh> ResourceManager::getMesh(const std::string& path) {
    // 1. 先查表
    auto it = meshes.find(path);
    if (it != meshes.end()) {
        std::cout << "[Resource] Cache Hit: " << path << std::endl;
        return it->second;
    }

    // 2. 如果没找到，加载新模型
    std::cout << "[Resource] Loading New Model: " << path << std::endl;
    std::shared_ptr<TriMesh> newMesh = std::make_shared<TriMesh>();
    newMesh->readObjAssimp(path);

    // 3. 存入缓存
    meshes[path] = newMesh;

    return newMesh;
}

void ResourceManager::clear() {
    meshes.clear();
}