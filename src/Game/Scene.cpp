#include "Game/Scene.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Core/ResourceManager.h"
#include "Game/LightManager.h" // 需要引用完整定义以访问 getStreetLamps
#include "Core/Skybox.h"

Scene::Scene() {}

void Scene::init() {
    // 1. 地面
    ground = ResourceManager::getInstance().getMesh("assets/models/scene/plane.obj");

    // 2. 太阳 & 月亮 (动态物体单独保留)
    sunMesh = ResourceManager::getInstance().getMesh("assets/models/sun/model.obj");
    moonMesh = ResourceManager::getInstance().getMesh("assets/models/moon/moon.obj");

    // 3. Skybox
    skybox = std::make_shared<Skybox>();
    skybox->init();

    std::cout << "Scene initialized." << std::endl;
}

// [核心] 地图加载逻辑
void Scene::loadMap(const std::shared_ptr<LightManager>& lightManager) {
    // 清空旧数据
    renderQueue.clear();
    collisionBoxes.clear();

    // === 1. 加载路灯 ===
    // 从 LightManager 获取位置 (数据源)
    const auto& lamps = lightManager->getStreetLamps();
    for (const auto& lamp : lamps) {
        // LightManager 里的 position.y 是灯泡的高度 (4.0f)
        // 但我们绘制模型时，需要指定的是底座的位置 (通常是地面 0.0f)
        glm::vec3 drawPos = lamp.position;
        drawPos.y = 0.0f; // 强制落地！
        // 调用通用函数：路径, 位置, 缩放, 碰撞半径(0.5)
        // 注意：addStaticObject 会自动处理贴地逻辑
        addStaticObject("assets/models/street_lamp/model.obj", drawPos, 2.5f, 0.5f);
    }

    // === 2. 加载树木 ===
    // 原来的逻辑：位置(0,0,0), 缩放 5.0, 碰撞半径 1.0
    addStaticObject("assets/models/pine_tree/model.obj", glm::vec3(0.0f, 0.0f, 0.0f), 5.0f, 1.0f);

    std::cout << "Map Loaded: " << renderQueue.size() << " objects, "
              << collisionBoxes.size() << " colliders." << std::endl;
}

// [核心] 通用物体添加函数 (自动计算贴地和碰撞)
void Scene::addStaticObject(const std::string& path, glm::vec3 pos, float scale, float colliderWidth) {
    auto mesh = ResourceManager::getInstance().getMesh(path);

    // 1. 获取边界用于计算对齐
    glm::vec3 minB = mesh->getMinBound();
    glm::vec3 maxB = mesh->getMaxBound();

    // 2. 计算贴地偏移 (Auto-Grounding)
    // 模型的最低点 (minB.y) * 缩放后，应该是 0 (或者 pos.y)
    // 所以我们需要把模型向上抬 -minB.y * scale 的距离
    float yOffset = -minB.y * scale;

    // 3. 构建渲染矩阵
    glm::mat4 model = glm::mat4(1.0f);
    // 先移到目标位置 (并加上贴地偏移)
    model = glm::translate(model, glm::vec3(pos.x, pos.y + yOffset, pos.z));
    model = glm::scale(model, glm::vec3(scale));

    // 4. 存入渲染队列
    renderQueue.push_back({mesh, model});

    // 5. 生成碰撞盒 (如果需要)
    if (colliderWidth > 0.0f) {
        float height = (maxB.y - minB.y) * scale;

        // 碰撞盒中心：
        // X/Z: 就是传入的 pos (强制中心对齐，忽略模型几何偏差)
        // Y: 贴地放置，所以中心高度是 height / 2
        glm::vec3 center = pos;
        center.y += height / 2.0f;

        // 存入碰撞列表
        collisionBoxes.emplace_back(center, glm::vec3(colliderWidth, height, colliderWidth));
    }
}

// 正常渲染 Pass
void Scene::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const LightManager* lights) {
    // 1. 绘制地面
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(5.0f));
    // [修改] 只传 ID 和 Model，不再传 View/Proj
    ground->draw(shader.ID, model);

    // 2. 绘制天体 (太阳/月亮)
    if (lights) {
        // [修改] 内部实现也去掉了 View/Proj 的传递
        drawCelestialBody(sunMesh, shader, lights, true);
        drawCelestialBody(moonMesh, shader, lights, false);
    }

    // 3. 绘制所有静态物体
    for (const auto& obj : renderQueue) {
        // [修改] 只传 ID 和 Model
        obj.mesh->draw(shader.ID, obj.modelMatrix);
    }

    // 4. Skybox
    // Skybox 使用独立的 Shader，所以仍然需要手动传 View/Proj
    if (lights) {
        skybox->draw(view, projection, lights->isNightMode());
    }
}

// [新增] 阴影生成 Pass
void Scene::drawShadow(Shader& shader) {
    // 1. 地面投射阴影
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(5.0f));
    ground->drawGeometry(shader.ID, model);

    // 2. 静态物体投射阴影
    for (const auto& obj : renderQueue) {
        obj.mesh->drawGeometry(shader.ID, obj.modelMatrix);
    }

    // 注意：天体和天空盒不需要投射阴影，这里跳过
}

// [修改] 辅助函数：天体绘制
// 移除了 view 和 projection 参数，因为不需要传给 mesh->draw 了
void Scene::drawCelestialBody(std::shared_ptr<TriMesh> mesh, Shader& shader,
                              const LightManager* lights, bool isSun)
{
    bool isNight = lights->isNightMode();
    if (isSun && isNight) return;
    if (!isSun && !isNight) return;

    glm::vec3 celestialDir = lights->getSunDirection();
    // 稍微放远一点，保证在天空盒范围内
    glm::vec3 pos = -celestialDir * 40.0f;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);

    // 自转动画
    float time = (float)glfwGetTime();
    model = glm::rotate(model, time * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
    // 1. 调整大小和位置
    if (!isSun) { // 月亮
        // [修改] 之前是 0.1f，如果还大，就继续缩小，比如 0.02f
        // 这取决于你的 moon.obj 原始大小，请根据视觉效果微调
        model = glm::scale(model, glm::vec3(0.06f));
        pos.y *= 0.4f;

        // [修改] 月亮亮度：稍微柔和一点，不要纯白 (1.0 -> 0.7)
        shader.setVec3("dirLight.ambient", glm::vec3(0.9f));
        shader.setVec3("dirLight.diffuse", glm::vec3(0.3f)); // 关闭漫反射，只用环境光显示纹理

    } else { // 太阳
        model = glm::scale(model, glm::vec3(3.0f));

        // [新增] 让太阳“发光”的关键
        // 我们把环境光设为 > 1.0 的值（过曝），这样它看起来就是耀眼的白色/黄色
        shader.setVec3("dirLight.ambient", glm::vec3(0.8f));
        shader.setVec3("dirLight.diffuse", glm::vec3(0.0f)); // 太阳不受光照方向影响
    }

    mesh->draw(shader.ID, model);

    // [关键] 恢复默认的光照参数，否则绘制完天体后，下一个物体的光照会乱掉
    // 恢复为当前时间段（白天/晚上）的参数
    // 这里我们简单粗暴地重置为一个很小的值，或者让 LightManager::apply 在下一帧重新覆盖
    // 但为了安全，最好重置一下：
    if(lights->isNightMode()) {
        shader.setVec3("dirLight.ambient", glm::vec3(0.05f)); // 恢复夜晚环境光
    } else {
        shader.setVec3("dirLight.ambient", glm::vec3(0.35f)); // 恢复白天环境光(与setDay一致)
    }
}