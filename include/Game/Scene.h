#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "Core/TriMesh.h"
#include "Core/Shader.h"
#include "Core/AABB.h"

// 前向声明
class LightManager;
class Skybox;

// [新增] 场景物体结构体
struct SceneObject {
    std::shared_ptr<TriMesh> mesh;
    glm::mat4 modelMatrix; // 预计算好的渲染矩阵
};

class Scene {
public:
    Scene();
    ~Scene() = default;

    void init();

    // [新增] 加载地图 (目前接收 LightManager 是为了放置路灯)
    void loadMap(const std::shared_ptr<LightManager>& lightManager);

    // [新增] 获取计算好的碰撞盒 (给 Game 类用于物理检测)
    const std::vector<AABB>& getObstacles() const { return collisionBoxes; }

    // 渲染
    void draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const LightManager* lights);

    void drawShadow(Shader& shader);
private:
    std::shared_ptr<TriMesh> ground;
    std::shared_ptr<TriMesh> sunMesh;
    std::shared_ptr<TriMesh> moonMesh;
    // std::shared_ptr<TriMesh> lampMesh; // 移入通用列表管理
    // std::shared_ptr<TriMesh> treeMesh; // 移入通用列表管理

    std::shared_ptr<Skybox> skybox;

    // [新增] 统一管理所有的静态场景物体 (路灯、树等)
    std::vector<SceneObject> renderQueue;

    // [新增] 统一存储所有的碰撞盒
    std::vector<AABB> collisionBoxes;

    // [新增] 核心工具函数：添加一个静态物体
    // path: 模型路径
    // pos: 世界坐标位置 (x, y, z)
    // scale: 缩放倍数
    // colliderWidth: 碰撞柱半径，如果 < 0 则不生成碰撞盒
    void addStaticObject(const std::string& path, glm::vec3 pos, float scale, float colliderWidth = -1.0f);

    // 辅助绘制天体
    void drawCelestialBody(std::shared_ptr<TriMesh> mesh, Shader& shader,
                              const LightManager* lights, bool isSun);

};

#endif