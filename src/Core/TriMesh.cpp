#include "Core/TriMesh.h"
#include <iostream>
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <tiny_obj_loader.h>
#define TINYOBJLOADER_IMPLEMENTATION

TriMesh::TriMesh() : vao(0), vbo(0), shininess(32.0f) {}

TriMesh::~TriMesh() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

// 核心函数：自适应读取 (融合贴图模型与纯色模型)
void TriMesh::readObjTiny(const std::string &filename)
{
#ifndef NDEBUG
    std::cout << "[TinyObj] Loading: " << filename << std::endl;
#endif

    cleanData();
    std::string directory = filename.substr(0, filename.find_last_of('/'));

    // 1. 初始化 Loader
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = directory;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObj Reader Error: " << reader.Error() << std::endl;
        }
        return;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObj Reader Warning: " << reader.Warning() << std::endl;
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // 初始化边界
    minBound = glm::vec3(1e9f);
    maxBound = glm::vec3(-1e9f);

    // 2. 预加载所有材质贴图 (模仿 Assimp 逻辑)
    // TinyObj 的材质列表是全局的，先遍历一遍加载贴图
    for (const auto& mat : materials) {
        // 辅助 lambda: 加载并去重
        auto loadMap = [&](std::string texPath, std::string typeName) {
            if (texPath.empty()) return;

            // 检查去重
            for(const auto& t : textures) if(t.path == texPath) return;

            Texture tex;
            tex.id = loadTexture(texPath, directory); // 复用 loadTexture
            tex.type = typeName;
            tex.path = texPath;
            textures.push_back(tex);
        };

        loadMap(mat.diffuse_texname, "texture_diffuse");
        loadMap(mat.specular_texname, "texture_specular");
        // 如果 .mtl 里有 bump 贴图需要加载（我放弃了）
        // loadMap(mat.bump_texname, "texture_normal");
    }

    // 3. 遍历几何体
    // TinyObj 的数据是展平的数组，通过 index 访问
    unsigned int baseIndex = 0; // 用于 faces 的索引偏移

    for (const auto& shape : shapes) {
        // 遍历所有面
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            // 获取当前面的顶点数 (通常是 3，因为 tinyobj 默认 triangulate)
            int fv = shape.mesh.num_face_vertices[f];

            // 获取材质 ID
            int matId = shape.mesh.material_ids[f];
            glm::vec3 diffuseColor(1.0f);
            if (matId >= 0 && matId < materials.size()) {
                diffuseColor = glm::vec3(
                    materials[matId].diffuse[0],
                    materials[matId].diffuse[1],
                    materials[matId].diffuse[2]
                );
            }

            // 遍历面的每个顶点
            for (size_t v = 0; v < fv; v++) {
                // 获取索引
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                // --- 位置 (Position) ---
                glm::vec3 pos(
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                );
                vertex_positions.push_back(pos);

                // 更新 AABB
                minBound = glm::min(minBound, pos);
                maxBound = glm::max(maxBound, pos);

                // --- 法线 (Normal) ---
                if (idx.normal_index >= 0) {
                    vertex_normals.emplace_back(glm::vec3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    ));
                } else {
                    vertex_normals.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f));
                }

                // --- 纹理坐标 (UV) ---
                if (idx.texcoord_index >= 0) {
                    vertex_texcoords.emplace_back(glm::vec2(
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1] // [关键] 手动 Flip Y
                    ));
                } else {
                    vertex_texcoords.emplace_back(glm::vec2(0.0f, 0.0f));
                }

                // --- 颜色 (Color) ---
                vertex_colors.push_back(diffuseColor);

                // --- 切线 (Tangent) ---
                // TinyObj 不会自动计算切线。
                // 我放弃了 Normal Map，这里只给个默认值...
            }

            // --- 构建面索引 (Faces) ---
            // 因为是把所有顶点按顺序 push 进去的，所以索引是线性的
            faces.emplace_back(
                baseIndex + 0,
                baseIndex + 1,
                baseIndex + 2
            );

            baseIndex += 3;
            index_offset += fv;
        }
    }

    // 4. 兜底策略：白图
    bool hasDiffuse = false;
    for(const auto& t : textures) {
        if(t.type == "texture_diffuse") hasDiffuse = true;
    }

    if (!hasDiffuse) {
        Texture whiteTex;
        whiteTex.id = loadTexture("internal_white", "");
        whiteTex.type = "texture_diffuse";
        whiteTex.path = "internal_white";
        textures.push_back(whiteTex);
    }
#ifndef NDEBUG
    std::cout << "Loaded Model: " << filename << " | Shapes: " << shapes.size()
              << " | Textures: " << textures.size() << std::endl;
#endif
    // 提交 GPU
    storeFacesPoints();
}

// 展平数据传给 GPU (适配 Layout 0,1,2,3)
void TriMesh::storeFacesPoints()
{
    // 将索引数据展平
    for (auto & face : faces)
    {
        // P0, P1, P2
        points.push_back(vertex_positions[face.x]);
        points.push_back(vertex_positions[face.y]);
        points.push_back(vertex_positions[face.z]);

        normals.push_back(vertex_normals[face.x]);
        normals.push_back(vertex_normals[face.y]);
        normals.push_back(vertex_normals[face.z]);

        texcoords.push_back(vertex_texcoords[face.x]);
        texcoords.push_back(vertex_texcoords[face.y]);
        texcoords.push_back(vertex_texcoords[face.z]);

        colors.push_back(vertex_colors[face.x]);
        colors.push_back(vertex_colors[face.y]);
        colors.push_back(vertex_colors[face.z]);
    }

    if (!vao) glGenVertexArrays(1, &vao);
    if (!vbo) glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    size_t size_p = points.size() * sizeof(glm::vec3);
    size_t size_n = normals.size() * sizeof(glm::vec3);
    size_t size_t1 = texcoords.size() * sizeof(glm::vec2);
    size_t size_c = colors.size() * sizeof(glm::vec3);

    // 分配总内存
    glBufferData(GL_ARRAY_BUFFER, size_p + size_n + size_t1 + size_c, nullptr, GL_STATIC_DRAW);

    // 填充子数据
    glBufferSubData(GL_ARRAY_BUFFER, 0, size_p, &points[0]);
    glBufferSubData(GL_ARRAY_BUFFER, size_p, size_n, &normals[0]);
    glBufferSubData(GL_ARRAY_BUFFER, size_p + size_n, size_t1, &texcoords[0]);
    glBufferSubData(GL_ARRAY_BUFFER, size_p + size_n + size_t1, size_c, &colors[0]);

    // Layout 0: Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);

    // Layout 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)size_p);
    glEnableVertexAttribArray(1);

    // Layout 2: TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void *)(size_p + size_n));
    glEnableVertexAttribArray(2);

    // Layout 3: Color (新增)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void *)(size_p + size_n + size_t1));
    glEnableVertexAttribArray(3);
}

// 纹理加载 (含默认白图生成)
unsigned int TriMesh::loadTexture(const std::string &path, const std::string &directory)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // 特殊标记：生成 1x1 白色纹理
    // 用于给没有贴图的模型（如钻石剑）提供默认颜色乘数
    if (path == "internal_white") {
        unsigned char white[] = {255, 255, 255, 255}; // RGBA
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);

        // 设置过滤参数，否则纹理可能无法采样
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        return textureID;
    }

    // 标准文件加载逻辑
    std::string filename = directory + '/' + path;
#ifndef NDEBUG
    std::cout << "[Texture Debug] Trying to load: " << filename << std::endl;
#endif
    int width, height, nrComponents;
    // stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA; // [关键] Minecraft 皮肤必须支持 RGBA 透明通道

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Minecraft 风格使用 GL_NEAREST (邻近采样) 保持像素颗粒感
        // 如果要平滑效果则使用 GL_LINEAR_MIPMAP_LINEAR
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// 纯几何绘制：适用于阴影生成阶段 (Shadow Pass)
// 不需要传 View/Proj，也不需要绑定纹理，只需要 Model 矩阵
void TriMesh::drawGeometry(GLuint program, const glm::mat4 &model) {
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, points.size());
    glBindVertexArray(0);
}

// 标准绘制：适用于主渲染阶段 (已解耦 View/Proj)
void TriMesh::draw(GLuint program, const glm::mat4 &model)
{
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        std::string name = textures[i].type;
        std::string number;

        if (name == "texture_diffuse") number = std::to_string(diffuseNr++);
        else if (name == "texture_specular") number = std::to_string(specularNr++);

        glUniform1i(glGetUniformLocation(program, (name + number).c_str()), i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // 上传 Model 矩阵
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

    // 补回材质的高光系数
    glUniform1f(glGetUniformLocation(program, "material.shininess"), shininess);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, points.size());

    // 恢复默认
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}

void TriMesh::cleanData()
{
    vertex_positions.clear(); vertex_normals.clear(); vertex_texcoords.clear(); vertex_colors.clear();
    faces.clear(); points.clear(); normals.clear(); texcoords.clear(); colors.clear();
    textures.clear();
}