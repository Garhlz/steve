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

void Steve::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {

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
    auto model = glm::mat4(1.0f);

    // (1) 移动到世界坐标 (position)
    model = glm::translate(model, position);

    // (2) 应用身体整体旋转 (bodyYaw)
    // 注意：旋转轴是 Y 轴
    model = glm::rotate(model, glm::radians(bodyYaw), glm::vec3(0.0f, 1.0f, 0.0f));

    // 定义旋转轴辅助变量 (Minecraft 模型的局部坐标系可能不一样，这里使用了修正轴)
    glm::vec3 armRotateAxis = glm::vec3(-1.0f, 0.0f, 0.0f); // 手臂绕 X 轴转
    glm::vec3 standardAxis  = glm::vec3(1.0f, 0.0f, 0.0f);  // 标准轴

    // -------------------------------------------
    // [Level 1] 绘制躯干
    // -------------------------------------------
    torso->draw(shader.ID, model, view, projection);

    // -------------------------------------------
    // [Level 2] 绘制头部
    // 头部是躯干的子节点，随身体转动，还可以自己摇头
    // -------------------------------------------
    glm::mat4 headModel = model;
    headModel = glm::translate(headModel, glm::vec3(0.0f, 0.37f, 0.0f)); // 移到脖子位置
    headModel = glm::rotate(headModel, glm::radians(headYaw), glm::vec3(0.0f, 1.0f, 0.0f)); // 摇头
    head->draw(shader.ID, headModel, view, projection);

    // ===========================================
    // 右臂层级建模 (关键部分)
    // 层级：躯干 -> 右大臂 -> 右小臂 -> 剑
    // ===========================================

    // --- [Level 2] 右大臂 ---
    glm::mat4 rightUpperModel = model;
    // 1. 移动到肩膀关节
    rightUpperModel = glm::translate(rightUpperModel, glm::vec3(0.375f, 0.375f, 0.0f));
    // 2. 应用旋转 (这里是动画的核心)
    rightUpperModel = glm::rotate(rightUpperModel, glm::radians(rightArmTargetAngle), armRotateAxis);

    // 绘制大臂 (稍微缩放一下让它细一点，可选)
    glm::mat4 upperDrawModel = glm::scale(rightUpperModel, glm::vec3(1.0f, 0.5f, 1.0f));
    rightArm->draw(shader.ID, upperDrawModel, view, projection);

    // --- [Level 3] 右小臂 ---
    glm::mat4 rightLowerModel = rightUpperModel; // 继承大臂的变换
    // 1. 移动到肘部关节 (相对大臂向下 0.375)
    rightLowerModel = glm::translate(rightLowerModel, glm::vec3(0.0f, -0.375f, 0.0f));

    // 计算肘部弯曲
    float elbowBend = -20.0f + sin(walkTime * 10.0f) * 10.0f; // 走路时的自然摆动
    if (isArmRaised) {
        elbowBend = -10.0f; // 挥剑时手臂保持微弯，看起来更有力量感
    }
    // 2. 应用肘部旋转
    rightLowerModel = glm::rotate(rightLowerModel, glm::radians(elbowBend), armRotateAxis);

    // 绘制小臂
    glm::mat4 lowerDrawModel = glm::scale(rightLowerModel, glm::vec3(1.0f, 0.5f, 1.0f));
    rightArm->draw(shader.ID, lowerDrawModel, view, projection);

    // --- [Level 4] 钻石剑 ---
    glm::mat4 swordModel = rightLowerModel; // 继承小臂的变换
    // 1. 移动到手腕 (相对小臂向下 0.375)
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, -0.375f, 0.0f));

    // 2. 调整握姿 (让剑垂直于手臂)
    swordModel = glm::rotate(swordModel, glm::radians(90.0f), armRotateAxis);

    swordModel = glm::rotate(swordModel, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // 3. [可选] 挥剑时的额外前倾
    // 让剑尖稍微指向前方，符合人体工学
    if (isArmRaised) {
         swordModel = glm::rotate(swordModel, glm::radians(45.0f), armRotateAxis);
    }

    // 4. 微调剑柄位置 (让它看起来握在手里，而不是嵌在肉里)
    swordModel = glm::translate(swordModel, glm::vec3(0.0f, 0.35f, 0.0f));

    // 5. 放大剑 (让它看起来霸气一点)
    swordModel = glm::scale(swordModel, glm::vec3(1.5f));

    sword->draw(shader.ID, swordModel, view, projection);

    // ===========================================
    // 其他肢体 (左臂、双腿)
    // 它们只需要简单的摆动动画
    // ===========================================

    // 左臂 (swingAngle)
    drawLimb(leftArm, shader, model, glm::vec3(-0.375f, 0.375f, 0.0f), swingAngle, view, projection, standardAxis);

    // 左腿 (-swingAngle，与左臂相反)
    drawLimb(leftLeg, shader, model, glm::vec3(-0.125f, -0.375f, 0.0f), -swingAngle, view, projection, standardAxis);

    // 右腿 (swingAngle，与左腿相反)
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