#ifndef SCENE_H
#define SCENE_H

#include "TriMesh.h"
#include "Shader.h"
#include <memory>
#include <vector>

#include "LightManager.h"
#include "Skybox.h"
class Scene {
public:
    Scene();
    void init();
    void draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const LightManager* lights);

private:
    std::shared_ptr<TriMesh> ground;

    std::shared_ptr<TriMesh> sunMesh;       // 太阳模型
    std::shared_ptr<TriMesh> moonMesh;
    std::shared_ptr<TriMesh> lampMesh;      // 路灯模型
    std::shared_ptr<TriMesh> treeMesh;

    std::shared_ptr<Skybox> skybox;
};

#endif