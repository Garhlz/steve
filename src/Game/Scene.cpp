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

void Scene::loadMap(const std::shared_ptr<LightManager>& lightManager) {
    renderQueue.clear();
    collisionBoxes.clear();
    lightManager->clearPointLights();

    // ==========================================
    // 1. 灯光布局
    // ==========================================
    struct LampConfig { glm::vec3 pos; glm::vec3 color; };
    std::vector<LampConfig> lamps = {
        // [灯1] 右前路引：稍微调亮一点，作为主光源
        { glm::vec3(5.0f, 4.0f, 6.0f), glm::vec3(1.2f, 1.1f, 1.0f) },
        // [灯2] 露营暖光
        { glm::vec3(16.0f, 4.5f, -2.0f), glm::vec3(1.0f, 0.6f, 0.3f) },
        // [灯3] 森林冷光
        { glm::vec3(-12.0f, 4.0f, -6.0f), glm::vec3(0.4f, 0.6f, 1.0f) },
        // [灯4] 远景背光
        { glm::vec3(0.0f, 5.0f, -25.0f), glm::vec3(0.8f, 0.8f, 0.9f) }
    };

    for (const auto& cfg : lamps) {
        lightManager->addPointLight(cfg.pos, cfg.color);
        glm::vec3 modelPos = cfg.pos;
        modelPos.y = 0.0f;
        addStaticObject("assets/models/street_lamp/model.obj", modelPos, 2.5f, 0.5f);
    }
    if (lightManager->isNightMode()) {
        lightManager->setNight();
    } else {
        lightManager->setDay();
    }

    // ==========================================
    // 2. 环境造景
    // ==========================================

    // ------------------------------------------
    // [Zone A] 出生点 (Spawn Area) - 重度装修
    // ------------------------------------------
    // 1. "背景墙"：在出生点身后(Z=13~15)堆砌掩体，消除背后空旷感
    // 左后方的大石头
    addStaticObject("assets/models/rock/model.obj", glm::vec3(-5.0f, 0.0f, 14.0f), 2.5f, 1.5f);
    // 右后方的巨型灌木墙 (Scale 15.0!)
    addStaticObject("assets/models/bush/model.obj", glm::vec3(6.0f, 0.0f, 14.0f), 15.0f, 2.0f);
    addStaticObject("assets/models/bush/model.obj", glm::vec3(1.0f, 0.0f, 15.0f), 12.0f, 2.0f);
    addStaticObject("assets/models/bush/model.obj", glm::vec3(9.0f, 0.0f, 18.0f), 14.0f, 2.0f);
    addStaticObject("assets/models/bush/model.obj", glm::vec3(15.0f, 0.0f, 12.0f), 12.0f, 2.0f);
    // 2. "侧翼掩护"：左边的一棵树，增加包围感
    addStaticObject("assets/models/another_tree/model.obj", glm::vec3(-9.0f, 0.0f, 10.0f), 4.5f, 0.6f);
    // 树下的灌木 (Scale 10.0)
    addStaticObject("assets/models/bush/model.obj", glm::vec3(-8.0f, 0.0f, 11.0f), 10.0f, 1.5f);

    // 3. "地面细节"：脚下的碎石路 (Pebbles)
    // 撒一把小石头在出生点 (-2 ~ 2, 8 ~ 10)
    addStaticObject("assets/models/rock/model.obj", glm::vec3(-1.5f, 0.0f, 9.5f), 0.4f, 0.0f);
    addStaticObject("assets/models/rock/model.obj", glm::vec3(0.5f, 0.0f, 8.5f), 0.3f, 0.0f);
    addStaticObject("assets/models/rock/model.obj", glm::vec3(2.5f, 0.0f, 7.0f), 0.5f, 0.0f);
    addStaticObject("assets/models/rock/model.obj", glm::vec3(-2.0f, 0.0f, 7.5f), 0.4f, 0.0f);

    // 路边的杂草 (Scale 6.0，不再用1.5这种微缩版了)
    addStaticObject("assets/models/bush/model.obj", glm::vec3(3.5f, 0.0f, 6.0f), 6.0f, 0.0f);


    // ------------------------------------------
    // [Zone B] 右侧：露营地 (The Campsite)
    // ------------------------------------------
    // 布局维持之前的三角形结构，微调灌木大小
    addStaticObject("assets/models/another_tree/model.obj", glm::vec3(15.0f, 0.0f, -8.0f), 5.5f, 0.8f);
    addStaticObject("assets/models/park_bench/model.obj", glm::vec3(13.0f, 0.0f, -5.0f), 2.0f, 3.0f);
    addStaticObject("assets/models/camp_fire/model.obj", glm::vec3(10.0f, 0.0f, -3.0f), 0.04f, 1.5f);

    // 外围保护圈
    addStaticObject("assets/models/rock/model.obj", glm::vec3(16.0f, 0.0f, -4.0f), 1.2f, 0.8f);
    // 这里的灌木也放大到 8.0
    addStaticObject("assets/models/bush/model.obj", glm::vec3(15.0f, 0.0f, -2.0f), 8.0f, 1.0f);


    // ------------------------------------------
    // [Zone C] 左侧：野生林地 (The Forest)
    // ------------------------------------------
    // 树木
    addStaticObject("assets/models/pine_tree/model.obj", glm::vec3(-12.0f, 0.0f, -6.0f), 5.5f, 1.0f);
    addStaticObject("assets/models/pine_tree/model.obj", glm::vec3(-18.0f, 0.0f, -10.0f), 6.0f, 1.2f);
    addStaticObject("assets/models/pine_tree/model.obj", glm::vec3(-10.0f, 0.0f, -14.0f), 4.5f, 0.8f);

    // 填充灌木 (Scale 10.0~12.0)
    // 这样灌木就有半人高了，甚至比Steve还高一点，很有野外探险的感觉
    addStaticObject("assets/models/bush/model.obj", glm::vec3(-14.0f, 0.0f, -8.0f), 10.0f, 1.5f);
    addStaticObject("assets/models/bush/model.obj", glm::vec3(-11.0f, 0.0f, -10.0f), 12.0f, 1.5f);
    addStaticObject("assets/models/bush/model.obj", glm::vec3(-19.0f, 0.0f, -8.0f), 10.0f, 1.5f);

    // 巨石
    addStaticObject("assets/models/rock/model.obj", glm::vec3(-8.0f, 0.0f, -8.0f), 2.0f, 1.5f);


    // ------------------------------------------
    // [Zone D] 远景球门
    // ------------------------------------------
    addStaticObject("assets/models/rock/model.obj", glm::vec3(0.0f, 0.0f, -22.0f), 3.0f, 2.0f);
    addStaticObject("assets/models/another_tree/model.obj", glm::vec3(7.0f, 0.0f, -23.0f), 4.0f, 0.6f);
    addStaticObject("assets/models/pine_tree/model.obj", glm::vec3(-7.0f, 0.0f, -23.0f), 5.0f, 1.0f);


    // [Center] 足球
    addStaticObject("assets/models/soccer_ball/model.obj", glm::vec3(0.0f, 0.0f, 2.0f), 0.000001f, 0.5f);

    std::cout << "Map Loaded: " << renderQueue.size() << " objects." << std::endl;
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

void Scene::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, LightManager* lights) {
    // 1. 绘制地面
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(5.0f));
    // 只传 ID 和 Model，不再传 View/Proj
    ground->draw(shader.ID, model);

    // 2. 绘制天体 (太阳/月亮)
    if (lights) {
        // 内部实现也去掉了 View/Proj 的传递
        drawCelestialBody(sunMesh, shader, lights, true);
        drawCelestialBody(moonMesh, shader, lights, false);
    }

    // 3. 绘制所有静态物体
    for (const auto& obj : renderQueue) {
        // 只传 ID 和 Model
        obj.mesh->draw(shader.ID, obj.modelMatrix);
    }

    // 4. Skybox
    // Skybox 使用独立的 Shader，所以仍然需要手动传 View/Proj
    if (lights) {
        skybox->draw(view, projection, lights->isNightMode());
    }
}

// 阴影生成
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

void Scene::drawCelestialBody(std::shared_ptr<TriMesh> mesh, Shader& shader,
                              LightManager* lights, bool isSun)
{
    // 1. 获取配置 (完全解耦！Scene 不再关心现在是白天还是晚上)
    const CelestialConfig& config = isSun ? lights->getSunConfig() : lights->getMoonConfig();

    // 2. 如果不可见，直接跳过
    if (!config.visible) return;

    // 3. 计算模型矩阵
    glm::vec3 pos = -config.direction * config.distance;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);

    // 自转动画 (保留一点动态效果)
    float time = (float)glfwGetTime();
    model = glm::rotate(model, time * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));

    model = glm::scale(model, glm::vec3(config.scale));

    // 4. 应用发光参数 (从 config 读取，不再硬编码 0.8/0.9)
    shader.setVec3("dirLight.ambient", config.emissionAmbient);
    shader.setVec3("dirLight.diffuse", config.emissionDiffuse);

    // 5. 绘制
    mesh->draw(shader.ID, model);

    // 6. 恢复现场 (依然调用 apply 重置为全局光照)
    lights->apply(shader);
}