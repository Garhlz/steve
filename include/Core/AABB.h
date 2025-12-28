#ifndef AABB_H
#define AABB_H

#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min; // 最小点 (左下后)
    glm::vec3 max; // 最大点 (右上前)

    // 构造函数：通过中心点和尺寸构造
    AABB(glm::vec3 center, glm::vec3 size) {
        glm::vec3 halfSize = size * 0.5f;
        min = center - halfSize;
        max = center + halfSize;
    }

    // 默认构造
    AABB() : min(0.0f), max(0.0f) {}

    // 检测两个 AABB 是否重叠
    // 如果在 X、Y、Z 三个轴上都重叠，则物体相撞
    bool checkCollision(const AABB& other) const {
        bool collisionX = max.x >= other.min.x && other.max.x >= min.x;
        bool collisionY = max.y >= other.min.y && other.max.y >= min.y;
        bool collisionZ = max.z >= other.min.z && other.max.z >= min.z;
        return collisionX && collisionY && collisionZ;
    }
};

#endif