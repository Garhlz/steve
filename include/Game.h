#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

// 引入所有子系统
#include "Shader.h"
#include "Camera.h"
#include "Steve.h"
#include "Scene.h"
#include "LightManager.h"
#include "CameraController.h"

// 游戏状态枚举 (预留扩展)
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

class Game {
public:
    // 游戏状态
    GameState State;
    bool Keys[1024]; // 记录键盘状态
    unsigned int Width, Height;

    // 构造/析构
    Game(unsigned int width, unsigned int height);
    ~Game();

    // --- 核心循环 ---
    // 1. 初始化所有子系统
    void Init(); 
    // 2. 处理输入 (按键触发逻辑)
    void ProcessInput(float dt);
    // 3. 更新逻辑 (物理、移动、相机跟随)
    void Update(float dt);
    // 4. 渲染 (绘图)
    void Render();

    // --- 事件回调接口 ---
    void HandleMouse(float xoffset, float yoffset);
    void HandleScroll(float yoffset);
    
    // 设置窗口指针 (用于 ProcessInput 中查询按键)
    void SetWindow(GLFWwindow* window) { this->window = window; }

private:
    GLFWwindow* window; // 仅用于输入查询

    // --- 核心子系统 (全部转为成员变量) ---
    std::shared_ptr<Shader> lightingShader;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Steve> steve;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<LightManager> lightManager;
    std::shared_ptr<CameraController> camController;
    
    // 防抖动变量
    bool pressB;
};

#endif