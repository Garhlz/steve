#include "Game.h"
#include <iostream>

#include "ResourceManager.h"

Game::Game(unsigned int width, unsigned int height) 
    : State(GAME_ACTIVE), Width(width), Height(height), pressB(false), window(nullptr)
{
}

Game::~Game() {
    // shared_ptr 会自动释放，这里不需要 delete
}

void Game::Init() {
    // 1. 初始化 Shader
    lightingShader = std::make_shared<Shader>("assets/shaders/lighting_vs.glsl", "assets/shaders/lighting_fs.glsl");

    // 2. 初始化 LightManager
    lightManager = std::make_shared<LightManager>();
    lightManager->init();

    // 3. 初始化 Camera
    camera = std::make_shared<Camera>(glm::vec3(0.0f, 1.0f, 3.0f));

    // 4. 初始化 Steve
    steve = std::make_shared<Steve>();
    steve->init("minecraft_girl");

    // 5. 初始化 Scene
    scene = std::make_shared<Scene>();
    scene->init();

    // 6. 初始化 Controller
    camController = std::make_shared<CameraController>(camera, steve);

    // === 生成碰撞数据 ===
    staticObstacles.clear();

    {
        // 1. 获取路灯模型数据 (为了计算高度)
        auto lampMesh = ResourceManager::getInstance().getMesh("assets/models/street_lamp/model.obj");
        glm::vec3 minB = lampMesh->getMinBound();
        glm::vec3 maxB = lampMesh->getMaxBound();

        // [修改] 不再计算 localCenter，因为我们要的是柱子的碰撞，不是整个模型的碰撞
        // 柱子通常在 (0,0) 附近，而灯罩会导致几何中心偏移

        float scale = 2.5f;

        const auto& lamps = lightManager->getStreetLamps();
        for (const auto& lamp : lamps) {
            glm::vec3 worldPos = lamp.position;

            glm::vec3 colliderCenter;

            // [核心修正] 强制 X/Z 中心对齐到放置点 (忽略模型本身的几何偏差)
            // 假设柱子就是立在放置点上的
            colliderCenter.x = worldPos.x;
            colliderCenter.z = worldPos.z;

            // Y 轴高度保持原算法
            float height = (maxB.y - minB.y) * scale;
            colliderCenter.y = height / 2.0f;

            // [优化] 手动指定柱子的粗细
            // 不要用 (Max-Min)*Scale，因为那包含了宽大的灯罩
            // 直接给一个合理的柱子宽度，比如 0.5 米 (世界尺寸)
            float poleThickness = 0.5f;

            staticObstacles.emplace_back(colliderCenter, glm::vec3(poleThickness, height, poleThickness));
        }
    }

    // --- 2. [新增] 树的碰撞 ---
    {
        auto treeMesh = ResourceManager::getInstance().getMesh("assets/models/tree/model.obj");

        // 获取模型原始边界
        glm::vec3 minB = treeMesh->getMinBound();
        glm::vec3 maxB = treeMesh->getMaxBound();

        float scale = 5.0f; // 必须和 Scene::draw 里的缩放一致！
        glm::vec3 position(0.0f, 0.0f, 0.0f); // 必须和 Scene::draw 里的位置一致！

        // A. 计算碰撞盒中心
        // X/Z: 就在树的中心 (0,0)
        // Y: 高度的一半
        float height = (maxB.y - minB.y) * scale;
        glm::vec3 center = position;
        center.y = height / 2.0f;

        // B. [关键] 手动定义树干粗细
        // 不要用 (maxB.x - minB.x) * scale，那样会把树冠也算进去
        // 我们假设树干直径大概 0.8 米
        float trunkThickness = 1.0f;

        // C. 添加碰撞盒
        // 中心在 (0, height/2, 0)，尺寸是 (0.8, height, 0.8)
        staticObstacles.emplace_back(center, glm::vec3(trunkThickness, height, trunkThickness));
    }

    std::cout << "Generated " << staticObstacles.size() << " static obstacles." << std::endl;
}

void Game::ProcessInput(float dt) {
    if (!window) return;

    // 系统按键
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 昼夜切换 (B键)
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        if (!pressB) {
            lightManager->toggleDayNight();
            pressB = true;
        }
    } else {
        pressB = false;
    }

    // 角色/相机控制
    camController->processKeyboard(window, dt);


}

void Game::Update(float dt) {
    // 1. 确定 Steve 是否允许输入
    bool steveInput = (camController->getMode() == CameraMode::THIRD_PERSON);

    // 2. 统一更新 Steve (带物理检测)
    steve->update(dt, window, staticObstacles, steveInput);

    // 3. 更新相机
    camController->update(dt);
}

void Game::Render() {
    // 1. 设置背景色
    // 虽然有 Skybox，但为了防止接缝或加载失败，保留一个底色
    glm::vec3 sky = lightManager->getSkyColor();
    glClearColor(sky.r, sky.g, sky.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 激活 Shader
    lightingShader->use();
    
    // 3. 设置全局 Uniforms (ViewPos, Shininess)
    lightingShader->setVec3("viewPos", camera->Position);
    lightingShader->setFloat("material.shininess", 32.0f);

    // 4. 应用光照数据 (LightManager -> Shader)
    lightManager->apply(*lightingShader); // 注意解引用 shared_ptr

    // 5. 计算矩阵
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)Width / (float)Height, 0.1f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();

    // 6. 绘制实体
    // 这里的管理逻辑：Game 告诉各个实体 "用这个 Shader 和矩阵把自己画出来"
    steve->draw(*lightingShader, view, projection, camera->Position);
    scene->draw(*lightingShader, view, projection, lightManager.get());
}

// 回调桥接
void Game::HandleMouse(float xoffset, float yoffset) {
    camController->processMouse(xoffset, yoffset);
}

void Game::HandleScroll(float yoffset) {
    camController->processScroll(yoffset);
}