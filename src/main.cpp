#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// 引入我们的封装类
#include "Shader.h"
#include "Camera.h"
#include "Steve.h" // 引入主角

// --- 函数声明 ---
void onWindowResize(GLFWwindow* window, int width, int height);
void onMouseMove(GLFWwindow* window, double xpos, double ypos);
void onMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// --- 全局变量 ---
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// 摄像机
Camera camera(glm::vec3(0.0f, 1.0f, 3.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 时间变量
float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::unique_ptr<Steve> steve;

int main()
{
    // 1. 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 2. 创建窗口
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Steve Walking - Elaine", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, onWindowResize);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetScrollCallback(window, onMouseScroll);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 3. 加载 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // 4. 初始化资源
    Shader lightingShader("assets/shaders/lighting_vs.glsl", "assets/shaders/lighting_fs.glsl");

    // 初始化 Steve
    steve = std::make_unique<Steve>();
    steve->init(); // 在这里加载所有模型

    // 5. 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        processInput(window);

        // 更新 Steve 状态 (头部旋转、举手逻辑都在这里面)
        steve->update(deltaTime, window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        // 光照设置
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -1.0f);
        lightingShader.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f);
        lightingShader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("dirLight.specular", 0.1f, 0.1f, 0.1f);

        // 关闭点光源和聚光灯干扰
        for(int i=0; i<4; i++) {
             std::string base = "pointLights[" + std::to_string(i) + "]";
             lightingShader.setVec3(base + ".diffuse", glm::vec3(0.0f));
             lightingShader.setVec3(base + ".specular", glm::vec3(0.0f));
             lightingShader.setFloat(base + ".constant", 1.0f);
             lightingShader.setFloat(base + ".linear", 0.0f);
             lightingShader.setFloat(base + ".quadratic", 0.0f);
        }
        lightingShader.setVec3("spotLight.diffuse", glm::vec3(0.0f));
        lightingShader.setFloat("spotLight.constant", 1.0f);

        // MVP 矩阵
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // 调用封装好的函数
        steve->draw(lightingShader, view, projection, camera.Position);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // 使用智能指针管理内存
    glfwTerminate();
    return 0;
}

// --- 回调函数 ---
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.ProcessKeyboard(DOWN, deltaTime);
}

void onWindowResize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void onMouseMove(GLFWwindow* window, double xposIn, double yposIn)
{
    auto xpos = static_cast<float>(xposIn);
    auto ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void onMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}