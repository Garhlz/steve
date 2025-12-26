#ifndef STEVE_H
#define STEVE_H

#include <memory> // 引入智能指针
#include <vector>
#include "Vendor/glad/glad.h"

// 然后再包含 GLM 和 GLFW
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// #include <GLFW/glfw3.h>

#include "Core/AABB.h"
#include "Core/TriMesh.h"
#include "Core/Shader.h"

enum class SteveState {
    IDLE,       // 站立 (只呼吸，不摆臂)
    WALK,       // 行走 (摆臂 + 移动)
    ATTACK      // 攻击 (播放挥剑动画，锁定移动)
};

struct SteveInput {
    // x: 左右移动 (-1 左, 1 右, 0 不动) -> 对应 A/D 控制转身
    // z: 前后移动 (1 前, -1 后, 0 不动)  -> 对应 W/S 控制前进
    glm::vec2 moveDir = glm::vec2(0.0f);
    bool jump = false;
    bool attack = false;
    bool shakeHeadLeft = false;  // Q
    bool shakeHeadRight = false; // E
};

class Steve {
public:
    Steve();
    ~Steve() = default;

    void init(const std::string& characterName);

    // [核心修改] update 不再接收 window，而是接收 input 结构体
    // 这样无论是 玩家控制 还是 AI控制，只需要构造这个结构体传进去即可
    void update(float dt, const SteveInput& input, const std::vector<AABB>& obstacles);

    void draw(Shader& shader);
    void drawShadow(Shader& shader);

    void setPosition(glm::vec3 pos) { position = pos; }
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    float getBodyYaw() const { return bodyYaw; }

    // 获取碰撞盒
    AABB getAABB() const {
        return AABB(position + glm::vec3(0.0f, 0.9f, 0.0f), glm::vec3(0.6f, 1.8f, 0.6f));
    }
private:
    // 使用 shared_ptr 管理模型
    std::shared_ptr<TriMesh> torso;
    std::shared_ptr<TriMesh> head;
    std::shared_ptr<TriMesh> leftArm;
    std::shared_ptr<TriMesh> rightArm;
    std::shared_ptr<TriMesh> leftLeg;
    std::shared_ptr<TriMesh> rightLeg;
    std::shared_ptr<TriMesh> sword;

    // --- 状态变量 ---
    SteveState state;       // 当前动作状态
    glm::vec3 position;     // 世界坐标位置
    glm::vec3 front;        // 面朝方向向量

    float bodyYaw;          // [新增] 身体的绝对旋转角度 (0度=初始方向)
    float headYaw;          // 头部相对身体的旋转角度
    bool isArmRaised;       // 举手状态

    // --- 属性设置 ---
    float moveSpeed;        // 移动速度
    float rotateSpeed;      // 转身速度 (度/秒)

    // --- 动画变量 ---
    float walkTime;         // 只有行走时才累加
    float swingRange;       // 摆动幅度
    float swingTime;        // [新增] 挥剑动画时间

    // [新增] 物理属性
    float verticalVelocity; // 当前垂直速度
    float gravity;          // 重力加速度 (建议 20.0f ~ 30.0f，游戏里通常比现实大)
    float jumpForce;        // 跳跃瞬间的向上速度 (建议 8.0f ~ 12.0f)
    bool isGrounded;        // 是否在地面上

    // 地面高度 (假设平地是 Y=0，如果你的模型中心在腰部，这里可能需要是 0.9f)
    // 根据你之前的代码，初始 Y 是 1.141f，我们暂定这就是“地面上的平衡高度”
    float groundLevel;

    // 辅助绘制函数
    static void drawLimb(const std::shared_ptr<TriMesh>& mesh, Shader& shader,
                     glm::mat4 parentModel, glm::vec3 offset, float angle,
                     glm::vec3 rotateAxis);


    // [修改] 内部处理函数也只需接收 input
    glm::vec3 processMovement(const SteveInput& input, float dt);
    void applyCollisionAndMove(glm::vec3 velocity, const std::vector<AABB>& obstacles);
    void processActions(const SteveInput& input, float dt);
    void updateAnimation(float dt);
};

#endif