#ifndef SKYBOX_H
#define SKYBOX_H

#include <vector>
#include <string>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class Skybox {
public:
    Skybox();
    ~Skybox();

    // 初始化：加载白天和晚上的贴图
    void init();

    // 绘制：根据昼夜状态选择贴图
    // 需要 view 和 projection 矩阵，以及是否是晚上的标志
    void draw(const glm::mat4& view, const glm::mat4& projection, bool isNight);

private:
    unsigned int dayTextureID;
    unsigned int nightTextureID;
    
    unsigned int VAO, VBO;
    std::shared_ptr<Shader> skyboxShader;

    // 内部工具：加载 Cubemap
    unsigned int loadCubemap(std::vector<std::string> faces);
};

#endif