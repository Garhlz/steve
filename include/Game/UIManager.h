#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "Vendor/glad/glad.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// 前向声明，告诉编译器 "Game" 是个类，具体细节在 .cpp 里看
class Game;

class UIManager {
public:
    UIManager();
    ~UIManager();

    // 初始化 ImGui
    void Init(GLFWwindow* window);

    // 渲染所有 UI (传入 Game 引用以便读取状态和控制游戏)
    void Render(Game& game);

private:
    // 内部状态：是否显示按键说明
    bool showControls;

    // 辅助函数：绘制具体的子菜单
    void RenderMainMenu(Game& game);
    void RenderPauseMenu(Game& game);
    void RenderHUD(Game& game);

    // 辅助函数：绘制按键列表 (复用逻辑)
    void RenderControlsList();
};

#endif