#include "Game/Game.h"
#include "Game/UIManager.h"
#include "Core/ResourceManager.h"
#include <iostream>

Game::Game(unsigned int width, unsigned int height)
    : State(GAME_MENU), Width(width), Height(height), pressB(false), window(nullptr)
{
}

Game::~Game() {
    // uiManager 使用 unique_ptr，会自动释放，也会自动调用 ImGui Cleanup
}

void Game::Init() {
    // 1. Shader
    lightingShader = std::make_shared<Shader>("assets/shaders/lighting_vs.glsl", "assets/shaders/lighting_fs.glsl");

    // 2. LightManager
    lightManager = std::make_shared<LightManager>();
    lightManager->init();

    // 3. Camera
    camera = std::make_shared<Camera>(glm::vec3(0.0f, 1.0f, 3.0f));

    // 4. Steve
    steve = std::make_shared<Steve>();
    // steve->init("minecraft_girl");
    steve->init("cornelia");

    // 5. Scene
    scene = std::make_shared<Scene>();
    scene->init();

    // 6. Controller
    camController = std::make_shared<CameraController>(camera, steve);

    // 7. [新增] 初始化 UI 管理器
    uiManager = std::make_unique<UIManager>();
    uiManager->Init(window); // 传入窗口句柄

    // === 生成碰撞数据 (保持你的代码不变) ===
    staticObstacles.clear();
    {
        auto lampMesh = ResourceManager::getInstance().getMesh("assets/models/street_lamp/model.obj");
        glm::vec3 minB = lampMesh->getMinBound();
        glm::vec3 maxB = lampMesh->getMaxBound();
        float scale = 2.5f;
        const auto& lamps = lightManager->getStreetLamps();
        for (const auto& lamp : lamps) {
            glm::vec3 worldPos = lamp.position;
            glm::vec3 colliderCenter;
            colliderCenter.x = worldPos.x;
            colliderCenter.z = worldPos.z;
            float height = (maxB.y - minB.y) * scale;
            colliderCenter.y = height / 2.0f;
            float poleThickness = 0.5f;
            staticObstacles.emplace_back(colliderCenter, glm::vec3(poleThickness, height, poleThickness));
        }
    }
    {
        auto treeMesh = ResourceManager::getInstance().getMesh("assets/models/tree/model.obj");
        glm::vec3 minB = treeMesh->getMinBound();
        glm::vec3 maxB = treeMesh->getMaxBound();
        float scale = 5.0f;
        glm::vec3 position(0.0f, 0.0f, 0.0f);
        float height = (maxB.y - minB.y) * scale;
        glm::vec3 center = position;
        center.y = height / 2.0f;
        float trunkThickness = 1.0f;
        staticObstacles.emplace_back(center, glm::vec3(trunkThickness, height, trunkThickness));
    }

    std::cout << "Generated " << staticObstacles.size() << " static obstacles." << std::endl;
}

void Game::ProcessInput(float dt) {
    if (!window) return;

    static bool pressEsc = false;

    // ESC 状态机切换
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!pressEsc) {
            if (State == GAME_ACTIVE) {
                State = GAME_PAUSED;
                SetMouseMode(false);
            }
            else if (State == GAME_PAUSED) {
                State = GAME_ACTIVE;
                SetMouseMode(true);
            }
            else if (State == GAME_MENU) {
                glfwSetWindowShouldClose(window, true);
            }
            pressEsc = true;
        }
    } else {
        pressEsc = false;
    }

    // 只有 ACTIVE 时处理输入
    if (State == GAME_ACTIVE) {
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
            if (!pressB) { lightManager->toggleDayNight(); pressB = true; }
        } else { pressB = false; }

        camController->processKeyboard(window, dt);
    }
}

void Game::Update(float dt) {
    if (State == GAME_ACTIVE) {
        bool steveInput = (camController->getMode() == CameraMode::THIRD_PERSON);
        steve->update(dt, window, staticObstacles, steveInput);
        camController->update(dt);
    }
}

void Game::Render() {
    // 1. 背景
    glm::vec3 sky = lightManager->getSkyColor();
    glClearColor(sky.r, sky.g, sky.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 场景渲染
    lightingShader->use();
    lightingShader->setVec3("viewPos", camera->Position);
    lightingShader->setFloat("material.shininess", 32.0f);
    lightManager->apply(*lightingShader);

    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)Width / (float)Height, 0.1f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();

    steve->draw(*lightingShader, view, projection, camera->Position);
    scene->draw(*lightingShader, view, projection, lightManager.get());

    // 3. [修改] 调用 UI 管理器进行绘制
    // 传入 *this (Game的引用)，让 UIManager 可以读取 State 和控制游戏
    uiManager->Render(*this);
}

void Game::HandleMouse(float xoffset, float yoffset) {
    if (State == GAME_ACTIVE) {
        camController->processMouse(xoffset, yoffset);
    }
}

void Game::HandleScroll(float yoffset) {
    if (State == GAME_ACTIVE) {
        camController->processScroll(yoffset);
    }
}

void Game::SetMouseMode(bool capture) {
    if (capture) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}