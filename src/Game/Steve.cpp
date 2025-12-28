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
      moveSpeed(5.0f),
      rotateSpeed(180.0f),     // 转身速度
      walkTime(0.0f),
      swingTime(0.0f),
      swingRange(45.0f),
      // 物理参数初始化
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
    // 使用 ResourceManager 获取资源
    // 即使你创建 10 个 Steve，它们现在共享同一份内存中的 Vertex Data
    torso    = ResourceManager::getInstance().getMesh(basePath + "body.obj");
    head     = ResourceManager::getInstance().getMesh(basePath + "head.obj");
    leftArm  = ResourceManager::getInstance().getMesh(basePath + "left_arm.obj");
    rightArm = ResourceManager::getInstance().getMesh(basePath + "right_arm.obj");
    leftLeg  = ResourceManager::getInstance().getMesh(basePath + "left_leg.obj");
    rightLeg = ResourceManager::getInstance().getMesh(basePath + "right_leg.obj");

    sword    = ResourceManager::getInstance().getMesh("assets/models/diamond_sword/model.obj");
}

AABB Steve::getBoundingBox() const {
    // 保持和 applyCollisionAndMove 里的大小一致
    float w = 0.6f;
    float h = 1.8f;
    return AABB(position, glm::vec3(w, h, w));
}

void Steve::update(float dt, const SteveInput& input,
                   const std::vector<AABB>& obstacles,
                   AABB otherPlayerBox) {
    state = SteveState::IDLE;

    // 1. 处理移动
    glm::vec3 velocity = processMovement(input, dt);

    // 2. 处理跳跃
    if (input.jump && isGrounded) {
        verticalVelocity = jumpForce;
        isGrounded = false;
    }

    // 3. 应用水平物理
    applyCollisionAndMove(velocity, obstacles, otherPlayerBox);

    // 4. 处理其他动作
    processActions(input, dt);

    // 5. 垂直物理 (重力)
    verticalVelocity -= gravity * dt;
    position.y += verticalVelocity * dt;
    if (position.y < groundLevel) {
        position.y = groundLevel;
        verticalVelocity = 0.0f;
        isGrounded = true;
    } else if (position.y > groundLevel + 0.001f) {
        // 简单的离地检测
        // isGrounded = false; // (如果需要更精确的滞空判断)
    }

    // 6. 动画
    updateAnimation(dt);
}

// --- 辅助函数实现 ---
// 直接读取 input 结构体
glm::vec3 Steve::processMovement(const SteveInput& input, float dt) {
    // 1. 计算当前的前向向量
    glm::vec3 dirVector;
    dirVector.x = -sin(glm::radians(bodyYaw));
    dirVector.z = -cos(glm::radians(-bodyYaw));
    dirVector.y = 0.0f;
    front = glm::normalize(dirVector);

    // 2. 处理旋转 (Input.moveDir.x 对应 A/D)
    if (abs(input.moveDir.x) > 0.1f) {
        state = SteveState::WALK;
        // 如果 x 是 -1 (按A)， bodyYaw 增加
        // 如果 x 是 1 (按D)， bodyYaw 减少
        bodyYaw -= input.moveDir.x * rotateSpeed * dt;
    }

    // 3. 处理前后移动 (Input.moveDir.y 对应 W/S)
    glm::vec3 velocity(0.0f);
    if (abs(input.moveDir.y) > 0.1f) {
        state = SteveState::WALK;
        // y = 1 (前), y = -1 (后)
        velocity += front * input.moveDir.y;
    }

    if (glm::length(velocity) > 0.0f) {
        return glm::normalize(velocity) * moveSpeed * dt;
    }
    return glm::vec3(0.0f);
}

// 碰撞检测逻辑
void Steve::applyCollisionAndMove(glm::vec3 velocity,
                                  const std::vector<AABB>& obstacles,
                                  AABB otherPlayerBox)
{
    if (glm::length(velocity) < 0.0001f) return;

    float characterHeight = 1.8f;
    float characterWidth = 0.6f;
    glm::vec3 currentCenter = position;

    // X 轴检测
    glm::vec3 nextPosX = currentCenter;
    nextPosX.x += velocity.x;
    AABB nextBoxX(nextPosX, glm::vec3(characterWidth, characterHeight, characterWidth));

    bool collisionX = false;
    if (nextPosX.x > 25.0f || nextPosX.x < -25.0f) collisionX = true;

    // 1. 检查地图静态障碍物
    if (!collisionX) {
        for (const auto& box : obstacles) {
            if (nextBoxX.checkCollision(box)) {
                collisionX = true;
                break;
            }
        }
    }

    // 2. 检查另一个玩家
    if (!collisionX) {
        if (nextBoxX.checkCollision(otherPlayerBox)) {
            collisionX = true;
        }
    }

    if (!collisionX) position.x += velocity.x;


    // Z 轴检测
    // glm::vec3 nextPosZ = currentCenter;
    glm::vec3 posWithNewX = position;
    posWithNewX.z += velocity.z;

    AABB nextBoxZ(posWithNewX, glm::vec3(characterWidth, characterHeight, characterWidth));

    bool collisionZ = false;
    if (posWithNewX.z > 25.0f || posWithNewX.z < -25.0f) collisionZ = true;

    // 1. 检查地图静态障碍物
    if (!collisionZ) {
        for (const auto& box : obstacles) {
            if (nextBoxZ.checkCollision(box)) {
                collisionZ = true;
                break;
            }
        }
    }

    // 2. 检查另一个玩家
    if (!collisionZ) {
        if (nextBoxZ.checkCollision(otherPlayerBox)) {
            collisionZ = true;
        }
    }

    if (!collisionZ) position.z += velocity.z;
}

void Steve::processActions(const SteveInput& input, float dt) {
    // 头部
    if (input.shakeHeadLeft) headYaw += 100.0f * dt;
    if (input.shakeHeadRight) headYaw -= 100.0f * dt;

    if (headYaw > 60.0f) headYaw = 60.0f;
    if (headYaw < -60.0f) headYaw = -60.0f;

    // 手臂
    isArmRaised = input.attack;
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
        // 挥剑速度
        swingTime += dt * 16.0f;
    } else {
        // 松开时重置，让手臂回到默认位置
        swingTime = 0.0f;
    }
}

void Steve::draw(Shader& shader) {

    // 1. 动画参数计算
    // 基础行走摆动 (基于 walkTime)
    float swingAngle = 0.0f;
    if (state == SteveState::WALK) {
        swingAngle = sin(walkTime * moveSpeed * 2.0f) * swingRange;
    }

    // 右臂的目标角度 (决定是摆动还是挥剑)
    float rightArmTargetAngle = swingAngle;

    if (isArmRaised) {
        // 挥剑：手臂会在高举和劈下之间快速往复
        rightArmTargetAngle = -65.0f + sin(swingTime) * 40.0f;
    }

    // 2. 基础模型矩阵 (Root: 身体中心)
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(bodyYaw), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 armRotateAxis = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 standardAxis  = glm::vec3(1.0f, 0.0f, 0.0f);

    // [Level 1] 躯干
    // 只传 ID 和 Model
    torso->draw(shader.ID, model);

    // [Level 2] 头部
    glm::mat4 headModel = model;
    headModel = glm::translate(headModel, glm::vec3(0.0f, 0.37f, 0.0f));
    headModel = glm::rotate(headModel, glm::radians(headYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    head->draw(shader.ID, headModel);

    // [Level 2] 右大臂
    glm::mat4 rightUpperModel = model;
    rightUpperModel = glm::translate(rightUpperModel, glm::vec3(0.375f, 0.375f, 0.0f));
    rightUpperModel = glm::rotate(rightUpperModel, glm::radians(rightArmTargetAngle), armRotateAxis);

    glm::mat4 upperDrawModel = glm::scale(rightUpperModel, glm::vec3(1.0f, 0.5f, 1.0f));
    rightArm->draw(shader.ID, upperDrawModel);

    // [Level 3] 右小臂
    glm::mat4 rightLowerModel = rightUpperModel;
    rightLowerModel = glm::translate(rightLowerModel, glm::vec3(0.0f, -0.375f, 0.0f));

    float elbowBend = -20.0f + sin(walkTime * 10.0f) * 10.0f;
    if (isArmRaised) elbowBend = -10.0f;

    rightLowerModel = glm::rotate(rightLowerModel, glm::radians(elbowBend), armRotateAxis);

    glm::mat4 lowerDrawModel = glm::scale(rightLowerModel, glm::vec3(1.0f, 0.5f, 1.0f));
    rightArm->draw(shader.ID, lowerDrawModel);

    // [Level 4] 钻石剑
    glm::mat4 swordModel = rightLowerModel;
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, -0.375f, 0.0f));
    swordModel = glm::rotate(swordModel, glm::radians(90.0f), armRotateAxis);
    swordModel = glm::rotate(swordModel, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    if (isArmRaised) swordModel = glm::rotate(swordModel, glm::radians(45.0f), armRotateAxis);
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, 0.35f, 0.0f));
    swordModel = glm::scale(swordModel, glm::vec3(1.5f));
    sword->draw(shader.ID, swordModel);

    // 其他肢体
    drawLimb(leftArm, shader, model, glm::vec3(-0.375f, 0.375f, 0.0f), swingAngle, standardAxis);
    drawLimb(leftLeg, shader, model, glm::vec3(-0.125f, -0.375f, 0.0f), -swingAngle, standardAxis);
    drawLimb(rightLeg, shader, model, glm::vec3(0.125f, -0.375f, 0.0f), swingAngle, standardAxis);
}

// 阴影生成 Pass
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

    // 其他肢体。手动展开 drawGeometry 调用
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

// 辅助函数
void Steve::drawLimb(const std::shared_ptr<TriMesh>& mesh, Shader& shader,
                     glm::mat4 parentModel, glm::vec3 offset, float angle,
                     glm::vec3 rotateAxis)
{
    glm::mat4 limbModel = parentModel;
    limbModel = glm::translate(limbModel, offset);
    limbModel = glm::rotate(limbModel, glm::radians(angle), rotateAxis);
    // 只传 ID 和 Model
    mesh->draw(shader.ID, limbModel);
}