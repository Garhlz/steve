#include <Vendor/glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Game/Game.h"
#include <stb_image.h>
// --- 全局变量 ---
Game* steveGame = nullptr;

constexpr unsigned int SCR_WIDTH = 1600;
constexpr unsigned int SCR_HEIGHT = 1200;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- 统一使用驼峰法命名函数 ---
void onWindowResize(GLFWwindow* window, int width, int height);
void onMouseMove(GLFWwindow* window, double xpos, double ypos);
void onMouseScroll(GLFWwindow* window, double xoffset, double yoffset);

int main()
{
    // 1. GLFW 初始化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Steve's Adventure - Elaine", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // ====================================================
    // [新增] 设置窗口图标
    // ====================================================
    {
        GLFWimage images[1];
        int channels;
        // 强制加载为 4 通道 (RGBA)，因为图标通常需要透明度
        // 请确保路径正确，比如放在 assets/icon.png
        images[0].pixels = stbi_load("assets/icon.png", &images[0].width, &images[0].height, &channels, 4);

        if (images[0].pixels) {
            // 设置图标
            glfwSetWindowIcon(window, 1, images);
            // 释放内存
            stbi_image_free(images[0].pixels);
        } else {
            std::cout << "Warning: Failed to load window icon." << std::endl;
        }
    }
    // ====================================================

    // [新增] 开启垂直同步 (V-Sync)
    // 0 = 不限制 (400 FPS+)
    // 1 = 锁定为屏幕刷新率 (通常 60 FPS)
    // 2 = 锁定为刷新率的一半 (30 FPS)
    glfwSwapInterval(1);

    // --- 注册回调函数 ---
    glfwSetFramebufferSizeCallback(window, onWindowResize);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetScrollCallback(window, onMouseScroll);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // 2. 游戏初始化
    steveGame = new Game(SCR_WIDTH, SCR_HEIGHT);
    steveGame->SetWindow(window);
    steveGame->Init();

    // 3. 游戏循环
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        steveGame->ProcessInput(deltaTime);
        steveGame->Update(deltaTime);
        steveGame->Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete steveGame;
    glfwTerminate();
    return 0;
}

// --- [修正] 回调函数实现 ---

void onWindowResize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    if(steveGame) {
        steveGame->Width = width;
        steveGame->Height = height;
    }
}

void onMouseMove(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;

    if(steveGame) steveGame->HandleMouse(xoffset, yoffset);
}

void onMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if(steveGame) steveGame->HandleScroll(static_cast<float>(yoffset));
}