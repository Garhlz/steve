#include "Scene.h"
#include <iostream>

Scene::Scene() {}

void Scene::init() {
    // 1. 地面
    ground = std::make_shared<TriMesh>();
    ground->readObjAssimp("assets/models/scene/plane.obj");

    // 2. 太阳
    sunMesh = std::make_shared<TriMesh>();
    std::cout << "Loading Sun..." << std::endl;
    sunMesh->readObjAssimp("assets/models/sun/model.obj");

    // 3. [新增] 月亮
    // 确保你的路径是对的，如果有材质记得检查 mtl
    moonMesh = std::make_shared<TriMesh>();
    std::cout << "Loading Moon..." << std::endl;
    moonMesh->readObjAssimp("assets/models/moon/moon.obj");

    // 4. 路灯
    lampMesh = std::make_shared<TriMesh>();
    std::cout << "Loading Street Lamp..." << std::endl;
    lampMesh->readObjAssimp("assets/models/street_lamp/model.obj");
}

void Scene::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const LightManager* lights) {

    // --- 1. 绘制地面 ---
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -0.75f, 0.0f));
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
            // 【核心修改：高亮月亮】
            // 1. 临时把环境光设为全白 (1.0)，让月亮纹理显示原色，不受黑夜影响
            shader.setVec3("dirLight.ambient", glm::vec3(1.0f, 1.0f, 1.0f));
            // 2. 甚至可以增加漫反射让它更亮
            shader.setVec3("dirLight.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));

            celestialModel = glm::scale(celestialModel, glm::vec3(0.1f));
            moonMesh->draw(shader.ID, celestialModel, view, projection);

            // 【核心修改：恢复夜色】
            // 画完月亮后，必须立刻把光照参数还原回“黑夜模式”，否则后面的路灯会亮得像白天！
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
            // 你调整好的 yOffset 参数
            float yOffset = 5.5f;
            float finalY = -0.75f + yOffset;

            lampModel = glm::translate(lampModel, glm::vec3(lampData.position.x, finalY, lampData.position.z));
            lampModel = glm::scale(lampModel, glm::vec3(scaleFactor));

            lampMesh->draw(shader.ID, lampModel, view, projection);
        }
    }
}