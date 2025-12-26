#include "Core/Skybox.h"
#include <iostream>
#include <stb_image.h>

Skybox::Skybox() : dayTextureID(0), nightTextureID(0), VAO(0), VBO(0) {}

Skybox::~Skybox() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    // texture 删除略
}

void Skybox::init() {
    // 1. 编译 Shader
    skyboxShader = std::make_shared<Shader>("assets/shaders/skybox_vs.glsl", "assets/shaders/skybox_fs.glsl");

    // 2. 设置立方体顶点 (仅仅是位置)
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // 3. 加载贴图 (注意顺序: Right, Left, Top, Bottom, Front, Back)
    std::vector<std::string> dayFaces {
        "assets/textures/skybox/day/px.png",
        "assets/textures/skybox/day/nx.png",
        "assets/textures/skybox/day/py.png",
        "assets/textures/skybox/day/ny.png",
        "assets/textures/skybox/day/pz.png",
        "assets/textures/skybox/day/nz.png"
    };
    std::cout << "[Skybox] Loading Day Texture..." << std::endl;
    dayTextureID = loadCubemap(dayFaces);

    std::vector<std::string> nightFaces {
        "assets/textures/skybox/night/px.png",
        "assets/textures/skybox/night/nx.png",
        "assets/textures/skybox/night/py.png",
        "assets/textures/skybox/night/ny.png",
        "assets/textures/skybox/night/pz.png",
        "assets/textures/skybox/night/nz.png"
    };
    std::cout << "[Skybox] Loading Night Texture ..." << std::endl;
    nightTextureID = loadCubemap(nightFaces);
    
    // 配置 shader 纹理单元
    skyboxShader->use();
    skyboxShader->setInt("skybox", 0);
}

void Skybox::draw(const glm::mat4& view, const glm::mat4& projection, bool isNight) {
    // 1. 改变深度测试函数
    // 默认是 GL_LESS，我们要用 GL_LEQUAL，因为我们在 Shader 里把深度强制设为了 1.0
    // 这样天空盒就会画在所有物体的“后面”
    glDepthFunc(GL_LEQUAL);
    
    skyboxShader->use();
    
    // 2. 去掉 View 矩阵的平移部分
    // 天空盒不应该随玩家移动而移动，只随旋转而旋转
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view)); 
    
    skyboxShader->setMat4("view", viewNoTrans);
    skyboxShader->setMat4("projection", projection);

    // 3. 绑定贴图
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, isNight ? nightTextureID : dayTextureID);

    // 4. 绘制立方体
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // 5. 恢复深度测试函数
    glDepthFunc(GL_LESS);
}

unsigned int Skybox::loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            // GL_TEXTURE_CUBE_MAP_POSITIVE_X 是起始枚举值，+i 依次对应右左上下前后
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    
    // 必须设置的参数
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}