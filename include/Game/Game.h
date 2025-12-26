#ifndef GAME_H
#define GAME_H

#include <Vendor/glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

// 子系统
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Game/Steve.h"
#include "Game/Scene.h"
#include "Game/LightManager.h"
#include "Game/CameraController.h"

// [新增] 引入 UIManager (前向声明即可，不需要包含头文件)
class UIManager;

enum GameState {
    GAME_MENU,
    GAME_ACTIVE,
    GAME_PAUSED
};

class Game {
public:
    GameState State;
    bool Keys[1024]{};
    unsigned int Width, Height;

    Game(unsigned int width, unsigned int height);
    ~Game();

    void Init();
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();

    void HandleMouse(float xoffset, float yoffset);
    void HandleScroll(float yoffset);

    void SetWindow(GLFWwindow* window) { this->window = window; }
    // [新增] 提供给 UIManager 使用，用于关闭游戏
    GLFWwindow* GetWindow() const { return window; }

    void SetMouseMode(bool capture);
    bool isFollowing = false;
private:
    GLFWwindow* window;

    std::shared_ptr<Shader> lightingShader;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Steve> steve;
    std::shared_ptr<Steve> alex;
    std::shared_ptr<Steve> currentCharacter;

    std::shared_ptr<Scene> scene;
    std::shared_ptr<LightManager> lightManager;
    std::shared_ptr<CameraController> camController;

    std::unique_ptr<UIManager> uiManager;

    std::shared_ptr<Shader> depthShader;

    std::vector<AABB> staticObstacles;
    bool pressB;

    bool pressT; // 用于防抖动切换角色

    bool pressF = false;      // F 键防抖

    // [新增] 辅助函数：计算 AI 的输入指令
    // 输入：追逐者(follower)，目标(target)
    // 输出：模拟的按键输入
    SteveInput calculateFollowInput(std::shared_ptr<Steve> follower, std::shared_ptr<Steve> target);
};

#endif