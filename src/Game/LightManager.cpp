#include "Game/LightManager.h"
#include <string>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

LightManager::LightManager() : isNight(false)
{
}

void LightManager::init()
{
    // 移除所有关于 streetLamps 的初始化代码
    // 只保留太阳的初始化
    setDay();
}

int LightManager::addPointLight(glm::vec3 pos, glm::vec3 color)
{
    if (streetLamps.size() >= MAX_POINT_LIGHTS)
    {
        std::cout << "[LightManager] Warning: Max point lights reached!" << std::endl;
        return -1;
    }

    PointLight lamp;
    lamp.position = pos;
    lamp.constant = 1.0f;
    lamp.linear = 0.045f;
    lamp.quadratic = 0.0075f;

    // [关键修复 1] 永久保存这个灯的原始颜色配置
    lamp.baseColor = color;

    // 根据当前状态初始化
    if (isNight)
    {
        // 晚上：直接使用原始颜色
        lamp.ambient = lamp.baseColor * 0.05f;
        lamp.diffuse = lamp.baseColor; // 使用全亮度，不要乘 0.8，否则会变暗
        lamp.specular = lamp.baseColor;
    }
    else
    {
        // 白天：关灯
        lamp.ambient = glm::vec3(0.0f);
        lamp.diffuse = glm::vec3(0.0f);
        lamp.specular = glm::vec3(0.0f);
    }

    streetLamps.push_back(lamp);
    return streetLamps.size() - 1;
}
void LightManager::clearPointLights()
{
    streetLamps.clear();
}

// 实现接口
void LightManager::setLampPosition(int index, glm::vec3 pos)
{
    if (index >= 0 && index < streetLamps.size())
    {
        streetLamps[index].position = pos;
    }
}

void LightManager::toggleDayNight()
{
    isNight = !isNight;
    if (isNight)
        setNight();
    else
        setDay();
}

void LightManager::setDay()
{
    // 1. 全局光照设置 (保持你之前的修改)
    currentSkyColor = glm::vec3(0.53f, 0.81f, 0.92f);

    // 太阳方向
    sun.direction = glm::normalize(glm::vec3(-1.0f, -0.8f, -0.5f));
    sun.ambient = glm::vec3(0.12f, 0.15f, 0.20f);
    sun.diffuse = glm::vec3(1.3f, 1.25f, 1.15f);
    sun.specular = glm::vec3(0.4f, 0.4f, 0.4f);

    // 2. 路灯：白天关闭
    for (auto &lamp : streetLamps)
    {
        lamp.ambient = glm::vec3(0.0f);
        lamp.diffuse = glm::vec3(0.0f);
        lamp.specular = glm::vec3(0.0f);
    }

    // 3. 配置天体状态 (静态数据管理)
    // --- 太阳配置 ---
    sunConfig.visible = true;
    sunConfig.scale = 3.0f;              // 太阳比较大
    sunConfig.distance = 40.0f;          // 距离
    sunConfig.direction = sun.direction; // 太阳模型的位置跟随光照方向
    // 太阳发光参数 (高亮过曝)
    sunConfig.emissionAmbient = glm::vec3(0.8f);
    sunConfig.emissionDiffuse = glm::vec3(0.0f);

    // --- 月亮配置 ---
    moonConfig.visible = false; // 白天隐藏月亮
    // 给个默认值防止意外
    moonConfig.scale = 0.06f;
    moonConfig.distance = 40.0f;
    moonConfig.direction = glm::normalize(glm::vec3(1.0f, 0.8f, 0.5f)); // 随便给个背对太阳的方向
    moonConfig.emissionAmbient = glm::vec3(0.0f);
    moonConfig.emissionDiffuse = glm::vec3(0.0f);
}

void LightManager::setNight()
{
    // 1. 全局光照设置
    currentSkyColor = glm::vec3(0.02f, 0.02f, 0.08f);

    // 月亮方向 (模拟太阳落山后的主光源)
    sun.direction = glm::normalize(glm::vec3(-1.0f, -0.8f, -0.5f));
    sun.ambient = glm::vec3(0.01f, 0.01f, 0.02f);
    sun.diffuse = glm::vec3(0.1f, 0.12f, 0.2f);
    sun.specular = glm::vec3(0.1f, 0.1f, 0.15f);

    // 2. 开灯
    for (auto &lamp : streetLamps)
    {
        // 环境光：非常微弱
        lamp.ambient = lamp.baseColor * 0.01f;
        // 调整路灯强度乘数。这里乘 0.8 左右比较合适，既亮又不至于变成纯白光球
        lamp.diffuse = lamp.baseColor * 0.8f;
        lamp.specular = lamp.baseColor * 0.8f;
    }

    // 3. 配置天体状态
    // 太阳配置
    sunConfig.visible = false; // 晚上隐藏太阳

    // 月亮配置
    moonConfig.visible = true;
    moonConfig.scale = 0.06f; // 保持原定大小
    moonConfig.distance = 40.0f;
    moonConfig.direction = sun.direction; // 月亮模型位置跟随当前主光源
    // 月亮发光参数 (柔和光)
    moonConfig.emissionAmbient = glm::vec3(0.9f);
    moonConfig.emissionDiffuse = glm::vec3(0.3f);
}

void LightManager::apply(Shader &shader)
{
    // 1. 设置方向光 (Sun/moon)
    shader.setVec3("dirLight.direction", sun.direction);
    shader.setVec3("dirLight.ambient", sun.ambient);
    shader.setVec3("dirLight.diffuse", sun.diffuse);
    shader.setVec3("dirLight.specular", sun.specular);

    // 动态循环，不再写死 i < 4
    // 还要告诉 Shader 实际有多少个灯
    shader.setInt("nr_point_lights", (int)streetLamps.size());

    for (size_t i = 0; i < streetLamps.size(); i++)
    {
        std::string base = "pointLights[" + std::to_string(i) + "]";
        shader.setVec3(base + ".position", streetLamps[i].position);
        shader.setVec3(base + ".ambient", streetLamps[i].ambient);
        shader.setVec3(base + ".diffuse", streetLamps[i].diffuse);
        shader.setVec3(base + ".specular", streetLamps[i].specular);
        shader.setFloat(base + ".constant", streetLamps[i].constant);
        shader.setFloat(base + ".linear", streetLamps[i].linear);
        shader.setFloat(base + ".quadratic", streetLamps[i].quadratic);
    }

    // 3. 聚光灯 (暂时关闭)
    shader.setVec3("spotLight.diffuse", glm::vec3(0.0f));
    shader.setFloat("spotLight.constant", 1.0f);
}

// 初始化阴影 FBO
void LightManager::initShadows()
{
    glGenFramebuffers(1, &depthMapFBO);

    // 创建深度纹理
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // 防止纹理边缘重复产生奇怪的阴影，设为 CLAMP_TO_BORDER 并把边框设为白色 (深度1.0，即无阴影)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 绑定到 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    // 我们不需要颜色缓冲，显式告诉 OpenGL
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Shadow Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 LightManager::getLightSpaceMatrix(glm::vec3 centerPos) const
{
    // 1. 投影矩阵 (扩大范围)
    // 扩大到 -50 到 50 甚至更大
    float orthoSize = 50.0f;
    float near_plane = 1.0f;
    float far_plane = 100.0f; // 增加远平面，防止阴影在远出被切断

    glm::mat4 lightProjection = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        near_plane, far_plane);

    // 2. 视图矩阵 (让光追着玩家跑)
    glm::vec3 lightTarget = centerPos;
    // 稍微修正 target 的 Y，让其聚焦在地面附近，而不是玩家头顶（可选）
    lightTarget.y = 0.0f;

    // 逆推光源位置
    // sun.direction 指向地面，所以 -sun.direction 指向天空
    // 乘以 50.0f 是把光源推到这就远平面的一半位置，保证能容纳较高的树和山
    glm::vec3 lightPos = lightTarget - sun.direction * 50.0f;

    glm::mat4 lightView = glm::lookAt(
        lightPos,
        lightTarget,
        glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}