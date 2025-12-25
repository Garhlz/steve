#include "Game.h"
#include <iostream>

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
    // 所有具体的 Update 逻辑都放在这里
    
    // 更新相机位置 (Controller 内部会处理第三人称跟随)
    camController->update(dt);
    
    // 如果有物理碰撞检测、怪物AI等，也放在这里
    // monster->update(dt);
}

void Game::Render() {
    // 1. 设置背景色 (从 LightManager 获取)
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