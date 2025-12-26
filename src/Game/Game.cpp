#include "Game/Game.h"
#include "Game/UIManager.h"
#include "Core/ResourceManager.h"
#include <iostream>

Game::Game(unsigned int width, unsigned int height)
    : State(GAME_MENU), Width(width), Height(height), pressB(false), pressT(false), window(nullptr)
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

    // Camera 保持在后面
    camera = std::make_shared<Camera>(glm::vec3(0.0f, 3.0f, 18.0f)); // 稍微抬高一点视角，看得更清楚

    camera->MovementSpeed = 7.0f;

    // 4. Steve & Alex 出生点
    // 让他们站在“空地”的边缘，面对足球
    steve = std::make_shared<Steve>();
    steve->init("cornelia");
    steve->setPosition(glm::vec3(-2.0f, 1.141f, 8.0f)); // 左边一点

    alex = std::make_shared<Steve>();
    alex->init("minecraft_girl");
    alex->setPosition(glm::vec3(2.0f, 1.141f, 8.0f));   // 右边一点

    currentCharacter = steve;

    // 5. Scene (初始化资源)
    scene = std::make_shared<Scene>();
    scene->init();

    // 6. 加载地图 (生成物体和碰撞盒)
    // 以后这里可以改成 scene->loadMap("level1.txt");
    scene->loadMap(lightManager);

    // 7. Controller
    camController = std::make_shared<CameraController>(camera, currentCharacter);

    // 8. UI
    uiManager = std::make_unique<UIManager>();
    uiManager->Init(window);

    // 9. 获取碰撞数据
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
        // 角色切换 (按 T 键)
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

        // F 键切换跟随模式
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            if (!pressF) {
                isFollowing = !isFollowing; // 切换开关
                std::cout << "[Game] Follow Mode: " << (isFollowing ? "ON" : "OFF") << std::endl;
                pressF = true;
            }
        } else {
            pressF = false;
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
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                playerInput.attack = true;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) playerInput.shakeHeadLeft = true;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) playerInput.shakeHeadRight = true;
        }

        // 2. 更新所有角色

        // 获取两个角色的碰撞盒
        AABB steveBox = steve->getBoundingBox();
        AABB alexBox = alex->getBoundingBox();

        // -------------------------------------------------
        // 更新当前操控的角色
        // -------------------------------------------------
        // 如果当前是 steve，就把 alexBox 传给它作为障碍
        // 如果当前是 alex，就把 steveBox 传给它
        AABB currentObstaclePlayer = (currentCharacter == steve) ? alexBox : steveBox;

        currentCharacter->update(dt, playerInput, staticObstacles, currentObstaclePlayer);


        // -------------------------------------------------
        // 更新另一个角色 (AI / Idle)
        // -------------------------------------------------
        SteveInput idleInput;
        SteveInput aiInput;
        std::shared_ptr<Steve> otherCharacter = (currentCharacter == steve) ? alex : steve;

        // 另一个角色的障碍物，就是“我现在操控的角色”
        // 注意：这里用 getBoundingBox() 重新获取一下，因为 currentCharacter 刚刚可能移动了位置
        AABB otherObstaclePlayer = currentCharacter->getBoundingBox();

        if (isFollowing) {
            aiInput = calculateFollowInput(otherCharacter, currentCharacter);
        } else {
            aiInput = idleInput;
        }

        otherCharacter->update(dt, aiInput, staticObstacles, otherObstaclePlayer);

        // 3. 更新相机
        camController->update(dt);
    }
}

void Game::Render() {
    // =========================================================
    // Pass 1: Shadow Map Generation (阴影生成阶段)
    // =========================================================

    // 获取 Steve 的位置作为阴影中心
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

SteveInput Game::calculateFollowInput(std::shared_ptr<Steve> follower, std::shared_ptr<Steve> target) {
    SteveInput input; // 默认全是 false/0

    // 参数设置
    float stopDistance = 2.5f; // 停止距离
    float turnThreshold = 0.98f; // 角度容差 (点积)，越接近 1.0 走得越直

    // 1. 获取位置向量
    glm::vec3 followerPos = follower->getPosition();
    glm::vec3 targetPos = target->getPosition();

    // 2. 计算方向向量 (忽略高度差，只在 XZ 平面移动)
    glm::vec3 diff = targetPos - followerPos;
    diff.y = 0.0f; // 扁平化

    float dist = glm::length(diff);

    // 3. 距离检测：如果小于停止距离，直接返回空指令 (站立不动)
    if (dist <= stopDistance) {
        return input;
    }

    // --- 如果代码走到这里，说明需要移动 ---

    // 4. 模拟按下 "W" (前进)
    input.moveDir.y = 1.0f;

    // 5. 计算转向 (模拟按下 A/D)
    // 我们需要判断目标在我的左边还是右边
    if (dist > 0.01f) {
        glm::vec3 targetDir = glm::normalize(diff); // 目标方向
        glm::vec3 currentFront = follower->getFront(); // 当前面朝方向

        // 使用 叉乘 (Cross Product) 判断左右
        // 在 XZ 平面，叉乘的 Y 分量可以告诉我们旋转方向
        // cross(A, B).y > 0 表示 B 在 A 的左侧 (逆时针)
        // cross(A, B).y < 0 表示 B 在 A 的右侧 (顺时针)
        float crossY = currentFront.z * targetDir.x - currentFront.x * targetDir.z;
        // 或者直接用 glm: float crossY = glm::cross(currentFront, targetDir).y;

        // 使用 点积 (Dot Product) 判断是否已经对准了
        float dotVal = glm::dot(currentFront, targetDir);

        // 只有当方向偏差较大时才转向 (避免震荡)
        if (dotVal < turnThreshold) {
            if (crossY > 0) {
                // 目标在左边 -> 模拟按下 A (MoveDir.x = -1)
                // 记得之前的逻辑：A键(-1)会让 bodyYaw 增加 (左转)
                input.moveDir.x = -1.0f;
            } else {
                // 目标在右边 -> 模拟按下 D (MoveDir.x = 1)
                input.moveDir.x = 1.0f;
            }
        }

        // [可选优化] 如果需要跳跃 (比如目标比我高很多)
        if (targetPos.y > followerPos.y + 0.5f) {
            input.jump = true;
        }
    }

    return input;
}