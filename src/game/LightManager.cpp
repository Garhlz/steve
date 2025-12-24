#include "LightManager.h"
#include <string>

LightManager::LightManager() : isNight(false) {
    // 预留4个路灯的空间
    streetLamps.resize(4);
}

void LightManager::init() {
    // 1. 初始化路灯位置 (假设场景是一个 10x10 的草地)
    // 【修改 1】调整路灯分布
    // 之前是 3.0，太近了。地面大概是 25x25 的范围，我们把路灯放到 +/- 10.0 的位置
    // Y轴设为 4.0，因为我们要放大路灯，灯泡的高度自然也会变高
    float lampDist = 10.0f;
    float lampHeight = 4.0f;

    streetLamps[0].position = glm::vec3(-lampDist, lampHeight, -lampDist);
    streetLamps[1].position = glm::vec3( lampDist, lampHeight, -lampDist);
    streetLamps[2].position = glm::vec3(-lampDist, lampHeight,  lampDist);
    streetLamps[3].position = glm::vec3( lampDist, lampHeight,  lampDist);

    // 初始化所有路灯的衰减系数 (覆盖距离约 5-10 米)
    for(auto& lamp : streetLamps) {
        lamp.constant = 1.0f;
        lamp.linear = 0.09f;
        lamp.quadratic = 0.032f;
    }

    // 默认开始是白天
    setDay();
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
    sun.ambient   = glm::vec3(0.4f, 0.4f, 0.4f); // 环境光较亮
    sun.diffuse   = glm::vec3(1.0f, 1.0f, 0.9f); // 暖白阳光
    sun.specular  = glm::vec3(0.5f, 0.5f, 0.5f);

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
    sun.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    sun.ambient   = glm::vec3(0.05f, 0.05f, 0.1f); // 非常暗
    sun.diffuse   = glm::vec3(0.1f, 0.1f, 0.2f);   // 只有一点点月光
    sun.specular  = glm::vec3(0.1f, 0.1f, 0.1f);

    // --- 路灯 (点光源) ---
    // 晚上：暖黄色的灯光开启
    for(auto& lamp : streetLamps) {
        lamp.ambient  = glm::vec3(0.1f, 0.1f, 0.0f); 
        lamp.diffuse  = glm::vec3(1.0f, 0.8f, 0.4f); // 经典的钠灯/火把颜色
        lamp.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    }
}

void LightManager::apply(Shader& shader) {
    // 1. 设置方向光 (Sun/moon)
    shader.setVec3("dirLight.direction", sun.direction);
    shader.setVec3("dirLight.ambient",   sun.ambient);
    shader.setVec3("dirLight.diffuse",   sun.diffuse);
    shader.setVec3("dirLight.specular",  sun.specular);

    // 2. 设置点光源 (Street Lamps)
    for(int i = 0; i < 4; i++) {
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