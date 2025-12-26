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

    depthShader = std::make_shared<Shader>("assets/shaders/shadow_depth_vs.glsl", "assets/shaders/shadow_depth_fs.glsl");

    // 2. LightManager
    lightManager = std::make_shared<LightManager>();
    lightManager->init();
    // [!!! 必须添加这一行 !!!]
    // 没有这一行，显卡里根本没有创建阴影贴图，Shader 读到的全是 0
    lightManager->initShadows();
    // 3. Camera
    camera = std::make_shared<Camera>(glm::vec3(1.0f, 2.0f, 15.0f));

    // 4. Steve
    steve = std::make_shared<Steve>();
    steve->init("cornelia"); // 还是用原来的模型
    steve->setPosition(glm::vec3(0.0f, 1.141f, 10.0f)); // 初始位置

    alex = std::make_shared<Steve>();
    alex->init("minecraft_girl"); // 加载新模型 (确保 assets/models/minecraft_girl/ 存在)
    alex->setPosition(glm::vec3(2.0f, 1.141f, 10.0f)); // 站在旁边

    currentCharacter = steve;

    // 5. Scene (初始化资源)
    scene = std::make_shared<Scene>();
    scene->init();

    // [新增] 6. 加载地图 (生成物体和碰撞盒)
    // 以后这里可以改成 scene->loadMap("level1.txt");
    scene->loadMap(lightManager);

    // 7. Controller
    camController = std::make_shared<CameraController>(camera, currentCharacter);

    // 8. UI
    uiManager = std::make_unique<UIManager>();
    uiManager->Init(window);

    // [修改] 9. 获取碰撞数据
    // Game 不再自己算碰撞盒，直接问 Scene 要
    staticObstacles = scene->getObstacles();

    std::cout << "Game Init Complete." << std::endl;
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

    if (State == GAME_ACTIVE) {
        // [新增] 角色切换 (按 T 键)
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            if (!pressT) {
                // 切换指针
                if (currentCharacter == steve) {
                    currentCharacter = alex;
                } else {
                    currentCharacter = steve;
                }
                // 通知相机切换目标
                camController->setTarget(currentCharacter);
                pressT = true;
            }
        } else {
            pressT = false;
        }

        // B 键开关灯
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
            if (!pressB) {
                lightManager->toggleDayNight();
                pressB = true;
            }
        } else {
            pressB = false;
        }
        // Camera 自由移动模式下的 WASD ...
        camController->processKeyboard(window, dt);
    }
}

void Game::Update(float dt) {
    if (State == GAME_ACTIVE) {
        // 1. 构造当前玩家的输入指令
        SteveInput playerInput;

        // 只有在第三人称模式下，玩家的键盘才控制角色
        if (camController->getMode() == CameraMode::THIRD_PERSON) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) playerInput.moveDir.y += 1.0f;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) playerInput.moveDir.y -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) playerInput.moveDir.x -= 1.0f; // 左转 (注意你在Steve.cpp里的实现逻辑)
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) playerInput.moveDir.x += 1.0f; // 右转

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) playerInput.jump = true;
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) playerInput.attack = true;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) playerInput.shakeHeadLeft = true;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) playerInput.shakeHeadRight = true;
        }

        // 2. 更新所有角色
        // -> 当前角色接收玩家输入
        currentCharacter->update(dt, playerInput, staticObstacles);

        // -> 另一个角色接收空输入 (Idle)，但依然受重力影响
        SteveInput idleInput; // 默认全为 false/0
        if (currentCharacter == steve) {
            alex->update(dt, idleInput, staticObstacles);
        } else {
            steve->update(dt, idleInput, staticObstacles);
        }

        // 3. 更新相机
        camController->update(dt);
    }
}

void Game::Render() {
    // =========================================================
    // Pass 1: Shadow Map Generation (阴影生成阶段)
    // =========================================================

    // [修改] 获取 Steve 的位置作为阴影中心
    glm::vec3 centerPos = currentCharacter->getPosition();

    // 计算跟随玩家的光照矩阵
    glm::mat4 lightProjectionView = lightManager->getLightSpaceMatrix(centerPos);


    // 2. 配置管线
    depthShader->use();
    depthShader->setMat4("lightSpaceMatrix", lightProjectionView);

    glViewport(0, 0, lightManager->getShadowWidth(), lightManager->getShadowHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, lightManager->getShadowFBO());
    glClear(GL_DEPTH_BUFFER_BIT);

    // 3. 绘制场景几何体 (注意：需要修改 Steve 和 Scene 增加 drawShadow 接口)
    // 这里的 cull face 设置是为了防止彼得潘悬浮(Peter Panning)现象，可选
    glCullFace(GL_FRONT);
    steve->drawShadow(*depthShader);
    alex->drawShadow(*depthShader);
    scene->drawShadow(*depthShader);
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // =========================================================
    // Pass 2: Normal Rendering (正常渲染阶段)
    // =========================================================

    // 1. 重置视口和缓冲
    glViewport(0, 0, Width, Height);
    // 这里的 ClearColor 使用 SkyColor
    glm::vec3 sky = lightManager->getSkyColor();
    glClearColor(sky.r, sky.g, sky.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 配置 Lighting Shader 全局参数 (替代了 TriMesh 里的逻辑)
    lightingShader->use();

    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)Width / (float)Height, 0.1f, 100.0f);

    lightingShader->setMat4("view", view);
    lightingShader->setMat4("projection", projection);
    lightingShader->setVec3("viewPos", camera->Position);

    // 传入阴影相关参数
    lightingShader->setMat4("lightSpaceMatrix", lightProjectionView);

    // 绑定阴影贴图 (例如绑定到纹理单元 10，避免和模型纹理冲突)
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, lightManager->getShadowMap());
    lightingShader->setInt("shadowMap", 10);

    // 应用光照参数
    lightManager->apply(*lightingShader);

    // 3. 绘制物体 (使用修改后的 draw 接口，不再传 view/proj)
    steve->draw(*lightingShader);
    alex->draw(*lightingShader);
    scene->draw(*lightingShader, view, projection, lightManager.get());

    // 4. UI 绘制
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