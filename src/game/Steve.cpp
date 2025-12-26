#include "Steve.h"
#include <iostream>

#include "ResourceManager.h"

Steve::Steve()
    : state(SteveState::IDLE),
      position(0.0f, 1.141f, 5.0f),
// 脚部原本的最低位置加上偏移量即可
      front(0.0f, 0.0f, 1.0f), // 初始朝向
      bodyYaw(180.0f),         // 初始背对Z轴，设为180让他面朝相机(初始+Z)
      headYaw(0.0f),
      isArmRaised(false),
      moveSpeed(4.0f),
      rotateSpeed(175.0f),     // 转身速度
      walkTime(0.0f),
      swingRange(45.0f)
{
}

void Steve::init(const std::string& characterName) {
    std::cout << "Initializing Character: " << characterName << "..." << std::endl;

    std::string basePath = "assets/models/" + characterName + "/";
    // [修改] 使用 ResourceManager 获取资源
    // 即使你创建 10 个 Steve，它们现在共享同一份内存中的 Vertex Data
    torso    = ResourceManager::getInstance().getMesh(basePath + "body.obj");
    head     = ResourceManager::getInstance().getMesh(basePath + "head.obj");
    leftArm  = ResourceManager::getInstance().getMesh(basePath + "left_arm.obj");
    rightArm = ResourceManager::getInstance().getMesh(basePath + "right_arm.obj");
    leftLeg  = ResourceManager::getInstance().getMesh(basePath + "left_leg.obj");
    rightLeg = ResourceManager::getInstance().getMesh(basePath + "right_leg.obj");

    sword    = ResourceManager::getInstance().getMesh("assets/models/diamond_sword/model.obj");
}

// [重构] 主 Update 函数变得非常清爽
void Steve::update(float dt, GLFWwindow* window, const std::vector<AABB>& obstacles, bool enableInput) {
    state = SteveState::IDLE; // 默认重置状态

    if (enableInput) {
        // 1. 处理移动输入并计算速度向量
        glm::vec3 velocity = processMovementInput(window, dt);

        // 2. 应用物理碰撞并移动
        applyCollisionAndMove(velocity, obstacles);

        // 3. 处理其他动作 (头部、手臂)
        processActions(window, dt);
    }

    // 4. 更新动画计时器
    updateAnimation(dt);
}

// --- 辅助函数实现 ---

glm::vec3 Steve::processMovementInput(GLFWwindow* window, float dt) {
    // A. 计算前向向量
    glm::vec3 dirVector;
    dirVector.x = -sin(glm::radians(bodyYaw));
    dirVector.z = -cos(glm::radians(-bodyYaw));
    dirVector.y = 0.0f;
    front = glm::normalize(dirVector);

    // B. 处理旋转 (A/D)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        state = SteveState::WALK;
        bodyYaw += rotateSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        state = SteveState::WALK;
        bodyYaw -= rotateSpeed * dt;
    }

    // C. 计算目标速度 (W/S)
    glm::vec3 velocity(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) velocity += front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) velocity -= front;

    // 归一化并应用速度
    if (glm::length(velocity) > 0.0f) {
        state = SteveState::WALK;
        return glm::normalize(velocity) * moveSpeed * dt;
    }

    return glm::vec3(0.0f);
}

void Steve::applyCollisionAndMove(glm::vec3 velocity, const std::vector<AABB>& obstacles) {
    if (glm::length(velocity) < 0.0001f) return;

    // Steve 的物理参数
    float characterHeight = 1.8f;
    float characterWidth = 0.8f;

    // [关键修正] AABB 的中心 Y 坐标应该是固定的
    // 无论模型怎么上下偏移，物理上 Steve 就是站在地面(Y=0)上的
    // 所以中心 Y 永远是 Height / 2 = 0.9f
    float fixedCenterY = characterHeight / 2.0f;

    // --- X 轴检测 ---
    glm::vec3 nextPosX = position;
    nextPosX.x += velocity.x;

    // [修改] 构造 AABB 时，忽略 nextPosX.y，强制使用 fixedCenterY
    glm::vec3 centerX = nextPosX;
    centerX.y = fixedCenterY; // 强制钉在地面上方 0.9 米处

    AABB nextBoxX(centerX, glm::vec3(characterWidth, characterHeight, characterWidth));

    bool collisionX = false;
    if (nextPosX.x > 25.0f || nextPosX.x < -25.0f) collisionX = true;
    if (!collisionX) {
        for (const auto& box : obstacles) {
            if (nextBoxX.checkCollision(box)) {
                collisionX = true;
                break;
            }
        }
    }
    if (!collisionX) position.x += velocity.x;

    // --- Z 轴检测 ---
    glm::vec3 nextPosZ = position;
    nextPosZ.z += velocity.z;

    // [修改] 同理，Z 轴检测也强制固定高度
    glm::vec3 centerZ = nextPosZ;
    centerZ.y = fixedCenterY;

    AABB nextBoxZ(centerZ, glm::vec3(characterWidth, characterHeight, characterWidth));

    bool collisionZ = false;
    if (nextPosZ.z > 25.0f || nextPosZ.z < -25.0f) collisionZ = true;
    if (!collisionZ) {
        for (const auto& box : obstacles) {
            if (nextBoxZ.checkCollision(box)) {
                collisionZ = true;
                break;
            }
        }
    }
    if (!collisionZ) position.z += velocity.z;
}

void Steve::processActions(GLFWwindow* window, float dt) {
    // 头部控制 (Q/E)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) headYaw += 100.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) headYaw -= 100.0f * dt;

    // 限制头部角度
    if (headYaw > 60.0f) headYaw = 60.0f;
    if (headYaw < -60.0f) headYaw = -60.0f;

    // 手臂控制 (R)
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        isArmRaised = true;
    else
        isArmRaised = false;
}

void Steve::updateAnimation(float dt) {
    if (state == SteveState::WALK) {
        walkTime += dt;
    } else {
        // 让动画自然归位 (这里简单重置)
        walkTime = 0.0f;
    }
}

void Steve::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {

    // --- 1. 动画计算 ---
    // 摆动角度由状态决定
    float swingAngle = 0.0f;
    if (state == SteveState::WALK) {
        swingAngle = sin(walkTime * moveSpeed * 2.0f) * swingRange; // *2.0f 加快一点频率
    }

    // 右臂举手覆盖
    float rightArmTargetAngle = swingAngle;
    if (isArmRaised) {
        rightArmTargetAngle = -90.0f; // 举平
    }

    // --- 2. 基础 Model 矩阵 ---
    auto model = glm::mat4(1.0f);

    // (1) 移动到世界坐标
    model = glm::translate(model, position);

    // (2) 应用身体旋转 (bodyYaw)
    model = glm::rotate(model, glm::radians(bodyYaw), glm::vec3(0.0f, 1.0f, 0.0f));

    // (3) [不再需要硬编码的180度转身]
    // 之前为了让它一开始面朝相机强行加了180度。
    // 现在我们在构造函数里把 bodyYaw 初始化为 180.0f 即可达到同样效果。
    // 如果这里再转，逻辑会变乱。直接由 bodyYaw 控制一切。

    // 定义旋转轴
    glm::vec3 armRotateAxis = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 standardAxis  = glm::vec3(1.0f, 0.0f, 0.0f);

    // -------------------------------------------
    // Level 1: 躯干
    // -------------------------------------------
    torso->draw(shader.ID, model, view, projection);

    // -------------------------------------------
    // 头部 (注意：headYaw 是叠加在 bodyYaw 之上的)
    // -------------------------------------------
    glm::mat4 headModel = model;
    headModel = glm::translate(headModel, glm::vec3(0.0f, 0.37f, 0.0f));
    headModel = glm::rotate(headModel, glm::radians(headYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    head->draw(shader.ID, headModel, view, projection);

    // ===========================================
    // 右臂 4 层层级建模 (完全复刻你的 main.cpp 逻辑)
    // ===========================================

    // 1. [Level 2] 右大臂
    glm::mat4 rightUpperModel = model;
    rightUpperModel = glm::translate(rightUpperModel, glm::vec3(0.375f, 0.375f, 0.0f));
    // 使用修正轴 (-1, 0, 0)
    rightUpperModel = glm::rotate(rightUpperModel, glm::radians(rightArmTargetAngle), armRotateAxis);

    glm::mat4 upperDrawModel = glm::scale(rightUpperModel, glm::vec3(1.0f, 0.5f, 1.0f));
    rightArm->draw(shader.ID, upperDrawModel, view, projection);

    // 2. [Level 3] 右小臂
    glm::mat4 rightLowerModel = rightUpperModel;
    rightLowerModel = glm::translate(rightLowerModel, glm::vec3(0.0f, -0.375f, 0.0f));

    float elbowBend = -20.0f + sin(walkTime * 10.0f) * 10.0f;
    if (isArmRaised) elbowBend = 0.0f;
    // 使用修正轴 (-1, 0, 0)
    rightLowerModel = glm::rotate(rightLowerModel, glm::radians(elbowBend), armRotateAxis);

    glm::mat4 lowerDrawModel = glm::scale(rightLowerModel, glm::vec3(1.0f, 0.5f, 1.0f));
    rightArm->draw(shader.ID, lowerDrawModel, view, projection);

    // 3. [Level 4] 钻石剑
    glm::mat4 swordModel = rightLowerModel;
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, -0.375f, 0.0f)); // 移到手腕

    // A. 基础握姿：配合修正轴旋转 90 度
    swordModel = glm::rotate(swordModel, glm::radians(90.0f), armRotateAxis);

    // B. 挥剑动画
    if (!isArmRaised) {
        // 你在 main.cpp 里注释掉了这行，这里我也先注释掉，保持一致
        // float hackAngle = sin(walkTime * 8.0f) * 20.0f;
        // swordModel = glm::rotate(swordModel, glm::radians(hackAngle), armRotateAxis);
    } else {
         swordModel = glm::rotate(swordModel, glm::radians(45.0f), armRotateAxis);
    }

    // C. 调整握柄位置 (你调整后的参数 0.35f)
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, 0.35f, 0.0f));

    // D. 放大剑 (你调整后的参数 1.5f)
    swordModel = glm::scale(swordModel, glm::vec3(1.5f));

    sword->draw(shader.ID, swordModel, view, projection);

    // ===========================================
    // 其他肢体 (使用 standardAxis 修正反向问题)
    // ===========================================

    // 左臂 (swingAngle) - 使用 standardAxis (1,0,0)
    drawLimb(leftArm, shader, model, glm::vec3(-0.375f, 0.375f, 0.0f), swingAngle, view, projection, standardAxis);

    // 左腿 (-swingAngle) - 使用 standardAxis (1,0,0)
    drawLimb(leftLeg, shader, model, glm::vec3(-0.125f, -0.375f, 0.0f), -swingAngle, view, projection, standardAxis);

    // 右腿 (swingAngle) - 使用 standardAxis (1,0,0)
    drawLimb(rightLeg, shader, model, glm::vec3(0.125f, -0.375f, 0.0f), swingAngle, view, projection, standardAxis);
}


// 辅助函数实现
void Steve::drawLimb(const std::shared_ptr<TriMesh>& mesh, Shader& shader,
                     glm::mat4 parentModel, glm::vec3 offset, float angle, 
                     const glm::mat4& view, const glm::mat4& projection, 
                     glm::vec3 rotateAxis) 
{
    glm::mat4 limbModel = parentModel;
    limbModel = glm::translate(limbModel, offset);
    limbModel = glm::rotate(limbModel, glm::radians(angle), rotateAxis);
    mesh->draw(shader.ID, limbModel, view, projection);
}