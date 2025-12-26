#include "Scene.h"
#include <iostream>

#include "ResourceManager.h"

Scene::Scene() {}

void Scene::init() {
    // 1. 地面
    ground = ResourceManager::getInstance().getMesh("assets/models/scene/plane.obj");

    // 2. 太阳
    sunMesh = ResourceManager::getInstance().getMesh("assets/models/sun/model.obj");

    // 3. 月亮
    moonMesh = ResourceManager::getInstance().getMesh("assets/models/moon/moon.obj");

    // 4. 路灯
    // 即使你在场景里画了 4 个路灯，这里只加载一次
    lampMesh = ResourceManager::getInstance().getMesh("assets/models/street_lamp/model.obj");

    // 5. 树
    treeMesh = ResourceManager::getInstance().getMesh("assets/models/pine_tree/model.obj");

    skybox = std::make_shared<Skybox>();
    skybox->init();

    std::cout << "Scene initialized with ResourceManager." << std::endl;
}

void Scene::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const LightManager* lights) {

    // --- 1. 绘制地面 ---
    glm::mat4 model = glm::mat4(1.0f);
    // 原来是 -0.75f，现在改为 0.0f
    // model = glm::translate(model, glm::vec3(0.0f, -0.75f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f));
    ground->draw(shader.ID, model, view, projection);

    // --- 2. 绘制 太阳 OR 月亮 ---
    if (lights) {
        // 计算天体位置 (光源的反方向 * 距离)
        glm::vec3 celestialDir = lights->getSunDirection();
        glm::vec3 celestialPos = -celestialDir * 40.0f; // 40.0f 是距离

        glm::mat4 celestialModel = glm::mat4(1.0f);
        celestialModel = glm::translate(celestialModel, celestialPos);

        // 旋转动画 (自转)
        float time = (float)glfwGetTime();
        celestialModel = glm::rotate(celestialModel, time * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));

        // [核心修改] 判断昼夜
        if (lights->isNightMode()) {
            // === 晚上画月亮 ===
            celestialPos.y *= 0.4f;
            // 高亮月亮
            // 1. 临时把环境光设为全白 (1.0)，让月亮纹理显示原色，不受黑夜影响
            shader.setVec3("dirLight.ambient", glm::vec3(1.0f, 1.0f, 1.0f));
            // 2. 甚至可以增加漫反射让它更亮
            shader.setVec3("dirLight.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));

            celestialModel = glm::scale(celestialModel, glm::vec3(0.1f));
            moonMesh->draw(shader.ID, celestialModel, view, projection);

            // 恢复夜色
            // 画完月亮后，必须立刻把光照参数还原回黑夜模式
            // 这里的值要和 LightManager::setNight() 里设置的一致
            shader.setVec3("dirLight.ambient", glm::vec3(0.05f, 0.05f, 0.1f));
            shader.setVec3("dirLight.diffuse", glm::vec3(0.1f, 0.1f, 0.2f));

        } else {
            // === 白天画太阳 ===
            celestialModel = glm::scale(celestialModel, glm::vec3(3.0f)); // 太阳大一点
            sunMesh->draw(shader.ID, celestialModel, view, projection);
        }
    }

    // --- 3. 绘制路灯 (4个) ---
    if (lights) {
        const auto& lamps = lights->getStreetLamps();

        for (const auto& lampData : lamps) {
            glm::mat4 lampModel = glm::mat4(1.0f);

            float scaleFactor = 2.5f;
            // 调整好的 yOffset 参数
            float yOffset = 6.03f;
            float finalY = 0.0f + yOffset;

            lampModel = glm::translate(lampModel, glm::vec3(lampData.position.x, finalY, lampData.position.z));
            lampModel = glm::scale(lampModel, glm::vec3(scaleFactor));

            lampMesh->draw(shader.ID, lampModel, view, projection);
        }
    }

    if (lights) {
        skybox->draw(view, projection, lights->isNightMode());
    }

    // [新增] 绘制树木
    {
        glm::mat4 treeModel = glm::mat4(1.0f);

        // 1. 定义缩放 (树通常比较大)
        float scale = 5.0f;

        // 2. 自动计算贴地偏移
        // 公式：Y = 目标地面高度(0) - (模型最低点 * 缩放)
        float minY = treeMesh->getMinBound().y;
        float yOffset = 0.0f - (minY * scale);

        // 3. 放置位置 (正中央 0,0)
        treeModel = glm::translate(treeModel, glm::vec3(0.0f, yOffset, 0.0f));
        treeModel = glm::scale(treeModel, glm::vec3(scale));

        treeMesh->draw(shader.ID, treeModel, view, projection);
    }
}