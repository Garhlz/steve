#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <memory>
#include "Camera.h"
#include "Steve.h"

enum class CameraMode {
    FIRST_PERSON,   // 第一人称 / 自由观察者 (原 Observer Mode)
    THIRD_PERSON    // 第三人称跟随
};

class CameraController {
public:
    CameraController(std::shared_ptr<Camera> cam, std::shared_ptr<Steve> player);

    // 每帧调用：根据模式更新相机位置
    void update(float dt);

    // 处理输入：专门处理模式切换和鼠标
    void processKeyboard(GLFWwindow* window, float dt);
    void processMouse(float xoffset, float yoffset);
    void processScroll(float yoffset);

    // 模式切换
    void toggleMode();
    CameraMode getMode() const { return currentMode; }

private:
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Steve> steve;

    CameraMode currentMode;
    bool isTabPressed; // 防抖动

    // 第三人称参数
    float distance = 4.0f;  // 距离人物多远
    float height = 2.0f;    // 高度偏移

    // 轨道相机的角度 (独立于人物)
    float orbitYaw;   // 鼠标左右转动控制这个
    float orbitPitch; // 鼠标上下转动控制这个
};

#endif