#include "Game/Steve.h"
#include <iostream>

#include "Core/ResourceManager.h"

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
      swingTime(0.0f),
      swingRange(45.0f),
      // [新增] 物理参数初始化
      verticalVelocity(0.0f),
      // 游戏里的重力通常要比现实(9.8)大，才有打击感，否则会像在月球
      gravity(31.0f),
      jumpForce(11.0f),
      isGrounded(true),
      groundLevel(1.141f) // 记录初始高度作为“地面高度”
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

void Steve::update(float dt, GLFWwindow* window, const std::vector<AABB>& obstacles, bool enableInput) {
    state = SteveState::IDLE;

    if (enableInput) {
        // 1. 处理水平移动输入 (获取 WASD 意图)
        glm::vec3 velocity = processMovementInput(window, dt);

        // [新增] 处理跳跃输入
        // 只有在地面上才能跳
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
            verticalVelocity = jumpForce;
            isGrounded = false;
            // 可以在这里切换状态，比如 state = SteveState::JUMP;
        }

        // 2. 应用水平碰撞并移动 X/Z
        applyCollisionAndMove(velocity, obstacles);

        // 3. 处理动作 (头部/手臂)
        processActions(window, dt);
    }

    // ==========================================
    // [新增] 4. 垂直物理模拟 (Gravity & Jump)
    // ==========================================

    // v = v0 - g * t
    verticalVelocity -= gravity * dt;

    // y = y0 + v * t
    position.y += verticalVelocity * dt;

    // 地面检测 (简单版：只检测 Y < groundLevel)
    if (position.y < groundLevel) {
        position.y = groundLevel;   // 修正位置不掉下去
        verticalVelocity = 0.0f;    // 落地后速度归零
        isGrounded = true;
    } else {
        // 如果高度由于某些原因(比如从台阶走下来)高于地面，就视为滞空
        // 注意：这里可能会在起跳的第一帧有一点冲突，但加上容差通常没事
        // 为了严谨，可以用 epsilon，或者只在下降时检测
        if(position.y > groundLevel + 0.001f) {
            isGrounded = false;
        }
    }

    // 5. 更新动画计时器
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

    // Steve 的物理尺寸
    float characterHeight = 1.8f;
    float characterWidth = 0.6f; // 稍微改瘦一点，手感好

    // [修改] 现在的中心 Y 不再是固定的，而是跟随当前 position.y
    // position 是脚底(或者模型原点)，AABB 中心通常在半高处
    // 假设 position.y 是模型原点 (在腰部或脚底)，根据构造函数 1.141f 来看，
    // 原点似乎就是模型的几何中心附近？
    // 如果 position.y 代表“在空中的位置”，那碰撞盒中心就是 position.y + (如果原点在脚底则 +height/2)

    // 既然之前的 groundLevel 是 1.141，说明 position 就是中心点。
    // 所以直接用 position 作为碰撞盒中心即可。
    glm::vec3 currentCenter = position;

    // --- X 轴检测 ---
    glm::vec3 nextPosX = currentCenter;
    nextPosX.x += velocity.x;

    AABB nextBoxX(nextPosX, glm::vec3(characterWidth, characterHeight, characterWidth));

    bool collisionX = false;
    // 简单的地图边界
    if (nextPosX.x > 25.0f || nextPosX.x < -25.0f) collisionX = true;
    if (!collisionX) {
        for (const auto& box : obstacles) {
            // [优化] 如果我们跳得比障碍物还高，就不应该算碰撞！
            // 简单的 AABB 检测 `checkCollision` 已经包含了 Y 轴判断
            // 所以只要 nextBoxX 的 Y 跟着跳起来了，checkCollision 就会返回 false (如果障碍物很矮)
            if (nextBoxX.checkCollision(box)) {
                collisionX = true;
                break;
            }
        }
    }
    if (!collisionX) position.x += velocity.x;

    // --- Z 轴检测 ---
    glm::vec3 nextPosZ = currentCenter; // 注意：用 currentCenter (包含最新的 Y)
    nextPosZ.z += velocity.z;

    AABB nextBoxZ(nextPosZ, glm::vec3(characterWidth, characterHeight, characterWidth));

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
    // 1. 行走动画
    if (state == SteveState::WALK) {
        walkTime += dt;
    } else {
        walkTime = 0.0f;
    }

    // 2. 挥剑动画
    if (isArmRaised) {
        // 15.0f 是挥剑速度，你可以调得更快或更慢
        swingTime += dt * 16.0f;
    } else {
        // 松开时重置，让手臂回到默认位置
        swingTime = 0.0f;
    }
}

void Steve::draw(Shader& shader) {

    // ===========================================
    // 1. 动画参数计算
    // ===========================================

    // 基础行走摆动 (基于 walkTime)
    float swingAngle = 0.0f;
    if (state == SteveState::WALK) {
        // * 2.0f 是为了让摆动频率快一点，看起来更有活力
        swingAngle = sin(walkTime * moveSpeed * 2.0f) * swingRange;
    }

    // 右臂的目标角度 (决定是摆动还是挥剑)
    float rightArmTargetAngle = swingAngle;

    if (isArmRaised) {
        // [挥剑逻辑]
        // 基础姿态：-90度 (抬平)
        // 动态叠加：正弦波 (sin(swingTime) * 45度)
        // 结果：手臂会在 "高举(-135度)" 和 "劈下(-45度)" 之间快速往复
        rightArmTargetAngle = -65.0f + sin(swingTime) * 40.0f;
    }

    // ===========================================
    // 2. 基础模型矩阵 (Root: 身体中心)
    // ===========================================
   // ===========================================
    // 2. 基础模型矩阵
    // ===========================================
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(bodyYaw), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 armRotateAxis = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 standardAxis  = glm::vec3(1.0f, 0.0f, 0.0f);

    // [Level 1] 躯干
    // [修改] 只传 ID 和 Model
    torso->draw(shader.ID, model);

    // [Level 2] 头部
    glm::mat4 headModel = model;
    headModel = glm::translate(headModel, glm::vec3(0.0f, 0.37f, 0.0f));
    headModel = glm::rotate(headModel, glm::radians(headYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    // [修改] 只传 ID 和 Model
    head->draw(shader.ID, headModel);

    // [Level 2] 右大臂
    glm::mat4 rightUpperModel = model;
    rightUpperModel = glm::translate(rightUpperModel, glm::vec3(0.375f, 0.375f, 0.0f));
    rightUpperModel = glm::rotate(rightUpperModel, glm::radians(rightArmTargetAngle), armRotateAxis);

    glm::mat4 upperDrawModel = glm::scale(rightUpperModel, glm::vec3(1.0f, 0.5f, 1.0f));
    // [修改] 只传 ID 和 Model
    rightArm->draw(shader.ID, upperDrawModel);

    // [Level 3] 右小臂
    glm::mat4 rightLowerModel = rightUpperModel;
    rightLowerModel = glm::translate(rightLowerModel, glm::vec3(0.0f, -0.375f, 0.0f));

    float elbowBend = -20.0f + sin(walkTime * 10.0f) * 10.0f;
    if (isArmRaised) elbowBend = -10.0f;

    rightLowerModel = glm::rotate(rightLowerModel, glm::radians(elbowBend), armRotateAxis);

    glm::mat4 lowerDrawModel = glm::scale(rightLowerModel, glm::vec3(1.0f, 0.5f, 1.0f));
    // [修改] 只传 ID 和 Model
    rightArm->draw(shader.ID, lowerDrawModel);

    // [Level 4] 钻石剑
    glm::mat4 swordModel = rightLowerModel;
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, -0.375f, 0.0f));
    swordModel = glm::rotate(swordModel, glm::radians(90.0f), armRotateAxis);
    swordModel = glm::rotate(swordModel, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    if (isArmRaised) swordModel = glm::rotate(swordModel, glm::radians(45.0f), armRotateAxis);
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, 0.35f, 0.0f));
    swordModel = glm::scale(swordModel, glm::vec3(1.5f));
    // [修改] 只传 ID 和 Model
    sword->draw(shader.ID, swordModel);

    // 其他肢体
    // [修改] drawLimb 内部实现也需要改，去掉 view/proj
    drawLimb(leftArm, shader, model, glm::vec3(-0.375f, 0.375f, 0.0f), swingAngle, standardAxis);
    drawLimb(leftLeg, shader, model, glm::vec3(-0.125f, -0.375f, 0.0f), -swingAngle, standardAxis);
    drawLimb(rightLeg, shader, model, glm::vec3(0.125f, -0.375f, 0.0f), swingAngle, standardAxis);
}

// [新增] 阴影生成 Pass
// 逻辑与 draw 完全一致，只是调用 drawGeometry
void Steve::drawShadow(Shader& shader) {
    // 必须重复计算一遍矩阵，因为阴影 Pass 里的 Steve 也要动！
    float swingAngle = 0.0f;
    if (state == SteveState::WALK) {
        swingAngle = sin(walkTime * moveSpeed * 2.0f) * swingRange;
    }
    float rightArmTargetAngle = swingAngle;
    if (isArmRaised) {
        rightArmTargetAngle = -65.0f + sin(swingTime) * 40.0f;
    }

    auto model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(bodyYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 armRotateAxis = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 standardAxis  = glm::vec3(1.0f, 0.0f, 0.0f);

    // 绘制身体部件 (使用 drawGeometry)
    torso->drawGeometry(shader.ID, model);

    glm::mat4 headModel = model;
    headModel = glm::translate(headModel, glm::vec3(0.0f, 0.37f, 0.0f));
    headModel = glm::rotate(headModel, glm::radians(headYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    head->drawGeometry(shader.ID, headModel);

    // 右臂层级
    glm::mat4 rightUpperModel = model;
    rightUpperModel = glm::translate(rightUpperModel, glm::vec3(0.375f, 0.375f, 0.0f));
    rightUpperModel = glm::rotate(rightUpperModel, glm::radians(rightArmTargetAngle), armRotateAxis);
    rightArm->drawGeometry(shader.ID, glm::scale(rightUpperModel, glm::vec3(1.0f, 0.5f, 1.0f)));

    glm::mat4 rightLowerModel = rightUpperModel;
    rightLowerModel = glm::translate(rightLowerModel, glm::vec3(0.0f, -0.375f, 0.0f));
    float elbowBend = -20.0f + sin(walkTime * 10.0f) * 10.0f;
    if (isArmRaised) elbowBend = -10.0f;
    rightLowerModel = glm::rotate(rightLowerModel, glm::radians(elbowBend), armRotateAxis);
    rightArm->drawGeometry(shader.ID, glm::scale(rightLowerModel, glm::vec3(1.0f, 0.5f, 1.0f)));

    glm::mat4 swordModel = rightLowerModel;
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, -0.375f, 0.0f));
    swordModel = glm::rotate(swordModel, glm::radians(90.0f), armRotateAxis);
    swordModel = glm::rotate(swordModel, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    if (isArmRaised) swordModel = glm::rotate(swordModel, glm::radians(45.0f), armRotateAxis);
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, 0.35f, 0.0f));
    swordModel = glm::scale(swordModel, glm::vec3(1.5f));
    sword->drawGeometry(shader.ID, swordModel);

    // 其他肢体 (因为 drawLimb 现在调用 draw，我们需要一个专门用于 shadow 的 limb helper，或者手动展开)
    // 这里为了简单，手动展开 drawGeometry 调用
    auto drawLimbShadow = [&](std::shared_ptr<TriMesh> mesh, glm::vec3 offset, float angle) {
        glm::mat4 m = model;
        m = glm::translate(m, offset);
        m = glm::rotate(m, glm::radians(angle), standardAxis);
        mesh->drawGeometry(shader.ID, m);
    };

    drawLimbShadow(leftArm, glm::vec3(-0.375f, 0.375f, 0.0f), swingAngle);
    drawLimbShadow(leftLeg, glm::vec3(-0.125f, -0.375f, 0.0f), -swingAngle);
    drawLimbShadow(rightLeg, glm::vec3(0.125f, -0.375f, 0.0f), swingAngle);
}

// 辅助函数实现
// [修改] 辅助函数 drawLimb (移除 view, proj)
void Steve::drawLimb(const std::shared_ptr<TriMesh>& mesh, Shader& shader,
                     glm::mat4 parentModel, glm::vec3 offset, float angle,
                     glm::vec3 rotateAxis)
{
    glm::mat4 limbModel = parentModel;
    limbModel = glm::translate(limbModel, offset);
    limbModel = glm::rotate(limbModel, glm::radians(angle), rotateAxis);
    // [修改] 只传 ID 和 Model
    mesh->draw(shader.ID, limbModel);
}