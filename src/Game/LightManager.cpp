#include "Game/LightManager.h"
#include <string>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

LightManager::LightManager() : isNight(false) {
}

void LightManager::init() {
    // [修改] 移除所有关于 streetLamps 的初始化代码
    // 只保留太阳的初始化
    setDay();
}

int LightManager::addPointLight(glm::vec3 pos, glm::vec3 color) {
    if (streetLamps.size() >= MAX_POINT_LIGHTS) {
        std::cout << "[LightManager] Warning: Max point lights reached!" << std::endl;
        return -1;
    }

    PointLight lamp;
    lamp.position = pos;

    // 设置物理参数 (你可以把这些也做成参数传进来，或者用默认值)
    lamp.constant = 1.0f;
    lamp.linear = 0.045f;    // 覆盖距离 30-50米
    lamp.quadratic = 0.0075f;

    // 设置颜色强度
    lamp.ambient = color * 0.1f;
    lamp.diffuse = color;
    lamp.specular = color;

    streetLamps.push_back(lamp);

    // 如果当前是晚上，新加的灯要根据夜晚逻辑调整一下亮度吗？
    // 简单起见，我们假设添加时都是“开启状态”的基础参数
    // 或者你可以调用一下 setNight() 来刷新它们的状态

    return streetLamps.size() - 1;
}

void LightManager::clearPointLights() {
    streetLamps.clear();
}

// [新增] 实现接口
void LightManager::setLampPosition(int index, glm::vec3 pos) {
    if (index >= 0 && index < streetLamps.size()) {
        streetLamps[index].position = pos;
    }
}

void LightManager::toggleDayNight() {
    isNight = !isNight;
    if (isNight) setNight();
    else setDay();
}

void LightManager::setDay() {
    // --- 天空颜色 ---
    currentSkyColor = glm::vec3(0.53f, 0.81f, 0.92f); // 天蓝

    // --- 太阳 (方向光) ---
    // 白天：强烈的白光，从上方斜射下来
    // sun.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    sun.direction = glm::normalize(glm::vec3(-1.0f, -0.5f, -1.0f));
    sun.ambient   = glm::vec3(0.35f, 0.35f, 0.35f);
    sun.diffuse = glm::vec3(1.2f, 1.2f, 1.1f);
    sun.specular  = glm::vec3(0.3f, 0.3f, 0.3f);

    // --- 路灯 (点光源) ---
    // 白天：路灯关闭 (黑色)
    for(auto& lamp : streetLamps) {
        lamp.ambient  = glm::vec3(0.0f);
        lamp.diffuse  = glm::vec3(0.0f);
        lamp.specular = glm::vec3(0.0f);
    }
}

void LightManager::setNight() {
    // --- 天空颜色 ---
    currentSkyColor = glm::vec3(0.05f, 0.05f, 0.15f); // 深邃的夜空蓝

    // --- 月亮 (方向光) ---
    // 晚上：微弱的蓝调冷光
    // sun.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    sun.direction = glm::normalize(glm::vec3(-1.0f, -0.5f, -1.0f));
    sun.ambient   = glm::vec3(0.15f, 0.15f, 0.25f); // 偏蓝的暗光

    // [关键 3] 大幅提高漫反射 (Diffuse)：
    // 这是让影子出现的关键！必须拉开和 Ambient 的差距。
    // 使用冷青色/银白色，强度设为 0.4 ~ 0.6，这样影子就有了“深度”
    sun.diffuse   = glm::vec3(0.4f, 0.45f, 0.6f);   // 皎洁的月光

    // 高光可以弱一点，月光通常比较柔和
    sun.specular  = glm::vec3(0.3f, 0.3f, 0.4f);

    // --- 路灯 (点光源) ---
    // 晚上：暖黄色的灯光开启
    for(auto& lamp : streetLamps) {
        glm::vec3 lightColor = glm::vec3(2.0f, 1.8f, 1.0f);
        lamp.ambient  = lightColor * 0.1f;
        lamp.diffuse  = lightColor;
        lamp.specular = glm::vec3(1.0f);
    }
}

void LightManager::apply(Shader& shader) {
    // 1. 设置方向光 (Sun/moon)
    shader.setVec3("dirLight.direction", sun.direction);
    shader.setVec3("dirLight.ambient",   sun.ambient);
    shader.setVec3("dirLight.diffuse",   sun.diffuse);
    shader.setVec3("dirLight.specular",  sun.specular);

    // [修改] 动态循环，不再写死 i < 4
    // 还要告诉 Shader 实际有多少个灯
    shader.setInt("nr_point_lights", (int)streetLamps.size());

    for(size_t i = 0; i < streetLamps.size(); i++) {
        std::string base = "pointLights[" + std::to_string(i) + "]";
        shader.setVec3(base + ".position",  streetLamps[i].position);
        shader.setVec3(base + ".ambient",   streetLamps[i].ambient);
        shader.setVec3(base + ".diffuse",   streetLamps[i].diffuse);
        shader.setVec3(base + ".specular",  streetLamps[i].specular);
        shader.setFloat(base + ".constant", streetLamps[i].constant);
        shader.setFloat(base + ".linear",   streetLamps[i].linear);
        shader.setFloat(base + ".quadratic",streetLamps[i].quadratic);
    }
    
    // 3. 聚光灯 (暂时关闭)
    shader.setVec3("spotLight.diffuse", glm::vec3(0.0f));
    shader.setFloat("spotLight.constant", 1.0f);
}

// [新增] 初始化阴影 FBO
void LightManager::initShadows() {
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
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
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

glm::mat4 LightManager::getLightSpaceMatrix(glm::vec3 centerPos) const {
    // 1. 投影矩阵 (扩大范围)
    // 之前的 -20 到 20 可能太小，对于 RPG 游戏，建议扩大到 -50 到 50 甚至更大
    // 这样能覆盖玩家周围 100x100 的区域
    float orthoSize = 50.0f;
    float near_plane = 1.0f;
    float far_plane = 100.0f; // 增加远平面，防止阴影在远出被切断

    glm::mat4 lightProjection = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        near_plane, far_plane
    );

    // 2. 视图矩阵 (让光追着玩家跑)
    // 关键点：LightPos 应该是 "玩家位置 + 逆光方向 * 距离"
    // 这样太阳看起来就像永远在那个角度照着玩家

    // 我们可以只跟随 X 和 Z 轴，Y 轴保持不动或者也跟随，
    // 为了稳定性，通常让光的高度稍微高一点
    glm::vec3 lightTarget = centerPos;
    // 稍微修正 target 的 Y，让其聚焦在地面附近，而不是玩家头顶（可选）
    lightTarget.y = 0.0f;

    // 逆推光源位置
    // sun.direction 指向地面，所以 -sun.direction 指向天空
    // 乘以 50.0f 是为了把光源推到这就远平面的一半位置，保证能容纳较高的树和山
    glm::vec3 lightPos = lightTarget - sun.direction * 50.0f;

    glm::mat4 lightView = glm::lookAt(
        lightPos,
        lightTarget,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    return lightProjection * lightView;
}