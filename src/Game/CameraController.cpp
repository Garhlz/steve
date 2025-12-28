#include "Game/CameraController.h"
#include <iostream>

CameraController::CameraController(std::shared_ptr<Camera> cam, std::shared_ptr<Steve> initialTarget)
    : camera(cam), target(initialTarget), currentMode(CameraMode::FIRST_PERSON), isTabPressed(false)
{
    orbitYaw = 180.0f;
    orbitPitch = 10.0f;
}

void CameraController::setTarget(std::shared_ptr<Steve> newTarget) {
    this->target = newTarget;
    // 切换目标时，为了防止相机突然跳变，可以重置 orbitYaw 为当前目标的背后
    orbitYaw = target->getBodyYaw() + 180.0f;
    std::cout << "[Camera] Target Switched!" << std::endl;
}

void CameraController::toggleMode() {
    if (currentMode == CameraMode::FIRST_PERSON) {
        currentMode = CameraMode::THIRD_PERSON;

        // 切换瞬间，将相机“瞬移”到 Steve 的正后方
        // Steve 面向 bodyYaw，相机要在他的后面，所以是 bodyYaw + 180 度
        // 这样无论 Steve 之前转向哪里，切视角的瞬间相机都会正对着他的后背
        orbitYaw = target->getBodyYaw() + 180.0f;

        // 重置俯仰角，保证每次切过来都是平视
        orbitPitch = 0.0f;

        std::cout << "[Camera] Switched to Third Person (Orbit)" << std::endl;
    } else {
        currentMode = CameraMode::FIRST_PERSON;

        // 切回第一人称时，把相机放回眼睛的位置
        camera->Position = target->getPosition() + glm::vec3(0.0f, 1.7f, 0.0f);

        // 让相机看向 Steve 当前面向的方向
        camera->Yaw = target->getBodyYaw() - 90.0f; // Camera类通常 0度是向右，所以要-90指向前方
        camera->Pitch = 0.0f;

        std::cout << "[Camera] Switched to First Person" << std::endl;
    }
}

void CameraController::processKeyboard(GLFWwindow* window, float dt) {
    // 1. 模式切换 (TAB)
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!isTabPressed) {
            toggleMode();
            isTabPressed = true;
        }
    } else {
        isTabPressed = false;
    }

    // 2. 根据模式决定谁响应键盘（角色移动的逻辑移走了）
    if (currentMode == CameraMode::FIRST_PERSON) {
        // 自由模式：键盘控制相机移动
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera->ProcessKeyboard(FORWARD, dt);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera->ProcessKeyboard(BACKWARD, dt);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera->ProcessKeyboard(LEFT, dt);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera->ProcessKeyboard(RIGHT, dt);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera->ProcessKeyboard(UP, dt);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera->ProcessKeyboard(DOWN, dt);
    } 
    else {
       // 改为在Game中控制steve->update
    }
}

void CameraController::processMouse(float xoffset, float yoffset) {
    if (currentMode == CameraMode::FIRST_PERSON) {
        camera->ProcessMouseMovement(xoffset, yoffset);
    }
    else {
        float sensitivity = 0.1f; // 灵敏度与方向控制

        // 鼠标左移(负) -> Yaw增加 -> 相机顺时针绕 -> 视觉向左转
        orbitYaw   -= xoffset * sensitivity;

        // 鼠标上移(正) -> Pitch减少 -> 相机下降 -> 视觉抬头
        orbitPitch -= yoffset * sensitivity;

        // 限制角度范围 (防止穿模或角度太怪)
        if (orbitPitch > 80.0f) orbitPitch = 80.0f;
        if (orbitPitch < -40.0f) orbitPitch = -40.0f;
    }
}

void CameraController::processScroll(float yoffset) {
    if (currentMode == CameraMode::FIRST_PERSON) {
        camera->ProcessMouseScroll(yoffset);
    } else {
        // 第三人称：滚轮控制距离 (拉近拉远)
        distance -= yoffset * 0.5f;
        if (distance < 1.0f) distance = 1.0f;
        if (distance > 10.0f) distance = 10.0f;
    }
}

void CameraController::update(float dt) {
    if (currentMode == CameraMode::THIRD_PERSON) {
        if(!target) return;

        glm::vec3 targetPos = target->getPosition();

        // 目标注视点 (Steve 的头部)
        glm::vec3 lookAtTarget = targetPos + glm::vec3(0.0f, 1.5f, 0.0f);

        // 球坐标转笛卡尔坐标，计算相机相对于注视点的偏移向量
        float yawRad   = glm::radians(orbitYaw);
        float pitchRad = glm::radians(orbitPitch);

        // 标准球坐标公式 (Y轴向上)
        // x = r * cos(pitch) * sin(yaw)
        // z = r * cos(pitch) * cos(yaw)
        // y = r * sin(pitch)

        float horizDist = distance * cos(pitchRad);
        float vertDist  = distance * sin(pitchRad);

        float offsetX = horizDist * sin(yawRad);
        float offsetZ = horizDist * cos(yawRad);

        // 相机位置 = 目标位置 + 偏移
        glm::vec3 cameraPos;
        cameraPos.x = lookAtTarget.x - offsetX;
        cameraPos.z = lookAtTarget.z - offsetZ;
        cameraPos.y = lookAtTarget.y + vertDist; // 加上高度偏移

        // 应用位置
        camera->Position = cameraPos;

        // 重新计算相机的方向向量 (LookAt)
        camera->Front = glm::normalize(lookAtTarget - camera->Position);
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        camera->Right = glm::normalize(glm::cross(camera->Front, worldUp));
        camera->Up    = glm::normalize(glm::cross(camera->Right, camera->Front));
    }
}