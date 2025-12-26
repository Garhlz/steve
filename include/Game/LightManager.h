#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include <glm/glm.hpp>
#include <vector>
#include "Core/Shader.h"

// 对应 Shader 中的 DirLight
struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

// 对应 Shader 中的 PointLight
struct PointLight {
    glm::vec3 position;
    
    // 衰减系数
    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

class LightManager {
public:
    LightManager();
    
    // 初始化：设置默认的灯光位置
    void init();

    // 核心功能：将光照数据传给 Shader
    void apply(Shader& shader);

    // 切换白天/黑夜
    void toggleDayNight();
    
    // 获取当前天空颜色 (用于 main 中设置 glClearColor)
    glm::vec3 getSkyColor() const { return currentSkyColor; }

    bool isNightMode() const { return isNight; }

    const std::vector<PointLight>& getStreetLamps() const { return streetLamps; }

    glm::vec3 getSunDirection() const { return sun.direction; }

    // [新增] 初始化阴影贴图 (FBO)
    void initShadows();

    // [新增] 获取阴影相关的 ID 和 尺寸
    unsigned int getShadowMap() const { return depthMap; }
    unsigned int getShadowFBO() const { return depthMapFBO; }
    unsigned int getShadowWidth() const { return SHADOW_WIDTH; }
    unsigned int getShadowHeight() const { return SHADOW_HEIGHT; }

    // [修改] 接收中心点位置（通常是玩家位置）
    glm::mat4 getLightSpaceMatrix(glm::vec3 centerPos) const;
private:
    bool isNight;
    glm::vec3 currentSkyColor{};

    DirLight sun{};
    std::vector<PointLight> streetLamps; // 存储4个路灯

    // 内部函数：设置白天参数
    void setDay();
    // 内部函数：设置黑夜参数
    void setNight();

    // [新增] 阴影资源
    unsigned int depthMapFBO;
    unsigned int depthMap;
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; // 高分辨率阴影
};

#endif