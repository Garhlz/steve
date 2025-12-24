#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// 引入我们的封装类
#include "Shader.h"
#include "Camera.h"
#include "TriMesh.h"

// --- 函数声明 ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// --- 全局变量 ---
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// 摄像机 (使用 LearnOpenGL 的 Camera 类)
// 初始位置向后退一点(Z=3.0)，稍微抬高(Y=1.0)
Camera camera(glm::vec3(0.0f, 1.0f, 3.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 时间变量
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // 1. 初始化 GLFW
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 2. 创建窗口
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Steve MVP", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 设置回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 隐藏鼠标并捕获 (FPS 模式)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 3. 加载 GLAD
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 全局 OpenGL 配置
    glEnable(GL_DEPTH_TEST);

    // 4. 初始化资源 (Shader & Model)
    // ------------------------------------

    // 编译着色器
    Shader lightingShader("assets/shaders/lighting_vs.glsl", "assets/shaders/lighting_fs.glsl");
    // Shader lightSourceShader("assets/shaders/lighting_source_vs.glsl", "assets/shaders/lighting_source_fs.glsl"); // 暂时不用

    // 加载 Steve 的躯干 (MVP 测试核心)
    TriMesh* steveTorso = new TriMesh();
    // 这里的路径对应 cmake copy 后的 assets 目录
    std::cout << "Loading Model..." << std::endl;
    steveTorso->readObjAssimp("assets/models/minecraft_girl/body.obj");

    // 5. 渲染循环
    // -----------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // 计算时间差
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 输入处理
        processInput(window);

        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // 深灰色背景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 激活着色器
        lightingShader.use();

        // A. 设置光照 Uniforms (模拟一个简单的太阳光)
        // ---------------------------------------------------------
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        // 方向光 (DirLight)
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.2f, 0.2f, 0.2f);
        lightingShader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.8f); // 亮一点
        lightingShader.setVec3("dirLight.specular", 0.1f, 0.1f, 0.1f); // 衣服高光弱一点

        // 点光源 (PointLights) - 必须初始化，否则可能是垃圾值
        // 这里简单把所有点光源的强度设为 0，避免干扰
        for(int i=0; i<4; i++) {
             std::string base = "pointLights[" + std::to_string(i) + "]";
             lightingShader.setVec3(base + ".diffuse", glm::vec3(0.0f));
             lightingShader.setVec3(base + ".specular", glm::vec3(0.0f));
             lightingShader.setFloat(base + ".constant", 1.0f); // 避免除以0
             lightingShader.setFloat(base + ".linear", 0.0f);
             lightingShader.setFloat(base + ".quadratic", 0.0f);
        }
        // 聚光灯 (SpotLight) - 同理设为 0
        lightingShader.setVec3("spotLight.diffuse", glm::vec3(0.0f));
        lightingShader.setFloat("spotLight.constant", 1.0f);


        // B. 坐标变换 (MVP Matrix)
        // ---------------------------------------------------------
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // 模型矩阵：把物体放在原点
        glm::mat4 model = glm::mat4(1.0f);
        // 如果物体太大了，可以缩放一下，比如: model = glm::scale(model, glm::vec3(0.5f));

        // C. 绘制模型
        // ---------------------------------------------------------
        // TriMesh::draw 内部会绑定 VAO, Texture 并设置 "model", "view", "projection"
        steveTorso->draw(lightingShader.ID, model, view, projection);


        // 交换缓冲 & 事件轮询
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 资源释放
    delete steveTorso;
    glfwTerminate();
    return 0;
}

// --- 回调函数实现 ---

// 键盘输入处理
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 摄像机移动 (W,A,S,D)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// 窗口大小改变回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// 鼠标移动回调 (视角旋转)
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // 注意 Y 轴反转

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// 鼠标滚轮回调 (缩放)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}