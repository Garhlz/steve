#ifndef STEVE_H
#define STEVE_H

#include <memory> // 引入智能指针
#include <vector>
#include <glad/glad.h>

// 然后再包含 GLM 和 GLFW
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "TriMesh.h"
#include "Shader.h"

class Steve {
public:
    Steve();
    ~Steve() = default; // 智能指针会自动释放内存，析构函数可以用默认的

    // 初始化：加载模型
    void init(const std::string& characterName);

    // 更新：处理输入和动画计时
    void update(float deltaTime, GLFWwindow* window);

    // 绘制：核心层级建模逻辑
    void draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);

    // 设置位置（如果以后想移动 Steve）
    void setPosition(glm::vec3 pos) { position = pos; }

private:
    // 使用 shared_ptr 管理模型
    std::shared_ptr<TriMesh> torso;
    std::shared_ptr<TriMesh> head;
    std::shared_ptr<TriMesh> leftArm;
    std::shared_ptr<TriMesh> rightArm;
    std::shared_ptr<TriMesh> leftLeg;
    std::shared_ptr<TriMesh> rightLeg;
    std::shared_ptr<TriMesh> sword;

    // 状态变量
    glm::vec3 position;
    float headYaw;
    bool isArmRaised;
    
    // 动画变量
    float walkTime;
    float walkSpeed;
    float swingRange;

    // 辅助绘制函数
    static void drawLimb(const std::shared_ptr<TriMesh>& mesh, Shader& shader,
                  glm::mat4 parentModel, glm::vec3 offset, float angle, 
                  const glm::mat4& view, const glm::mat4& projection, 
                  glm::vec3 rotateAxis = glm::vec3(1.0f, 0.0f, 0.0f));
};

#endif