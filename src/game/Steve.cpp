#include "Steve.h"
#include <iostream>

Steve::Steve() 
    : position(0.0f,0.4f,0.0f), headYaw(0.0f), isArmRaised(false),
      walkTime(0.0f), walkSpeed(5.0f), swingRange(45.0f) 
{
}

void Steve::init(const std::string& characterName) {
    std::cout << "Initializing Character: " << characterName << "..." << std::endl;

    // 构造基础路径，例如 "assets/models/minecraft_girl/"
    std::string basePath = "assets/models/" + characterName + "/";

    torso = std::make_shared<TriMesh>();
    // 拼接路径：basePath + "body.obj"
    torso->readObjAssimp(basePath + "body.obj");

    head = std::make_shared<TriMesh>();
    head->readObjAssimp(basePath + "head.obj");

    leftArm = std::make_shared<TriMesh>();
    leftArm->readObjAssimp(basePath + "left_arm.obj");

    rightArm = std::make_shared<TriMesh>();
    rightArm->readObjAssimp(basePath + "right_arm.obj");

    leftLeg = std::make_shared<TriMesh>();
    leftLeg->readObjAssimp(basePath + "left_leg.obj");

    rightLeg = std::make_shared<TriMesh>();
    rightLeg->readObjAssimp(basePath + "right_leg.obj");

    // 钻石剑通常是通用的，可以写死或者也参数化
    sword = std::make_shared<TriMesh>();
    sword->readObjAssimp("assets/models/diamond_sword/model.obj");
}

void Steve::update(float dt, GLFWwindow* window) {
    // 1. 动画计时
    walkTime += dt;

    // 2. 头部控制 (Q/E)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) headYaw += 100.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) headYaw -= 100.0f * dt;
    // 限制头部角度
    if (headYaw > 60.0f) headYaw = 60.0f;
    if (headYaw < -60.0f) headYaw = -60.0f;

    // 3. 手臂控制 (R)
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        isArmRaised = true;
    else
        isArmRaised = false;
}
void Steve::draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {

    // --- 1. 动画计算 ---
    // 保持和你 main.cpp 完全一致的参数
    float swingAngle = sin(walkTime * walkSpeed) * swingRange;

    // 右臂的目标角度 (解决同手同脚问题)
    float rightArmTargetAngle = swingAngle;
    if (isArmRaised) {
        rightArmTargetAngle = -70.0f; // 举平
    }

    // --- 2. 基础 Model 矩阵 ---
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    // 转身 180 度，面朝相机
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // 定义旋转轴
    // armRotateAxis: 专门用于右臂及其子层级 (因为身体转了180度，视觉上X轴反了)
    glm::vec3 armRotateAxis = glm::vec3(-1.0f, 0.0f, 0.0f);
    // standardAxis:  用于其他肢体 (左臂、腿)，保持默认逻辑
    glm::vec3 standardAxis  = glm::vec3(1.0f, 0.0f, 0.0f);

    // -------------------------------------------
    // Level 1: 躯干
    // -------------------------------------------
    torso->draw(shader.ID, model, view, projection);

    // -------------------------------------------
    // 头部
    // -------------------------------------------
    glm::mat4 headModel = model;
    headModel = glm::translate(headModel, glm::vec3(0.0f, 0.37f, 0.0f)); // 0.37 是调试好的参数
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