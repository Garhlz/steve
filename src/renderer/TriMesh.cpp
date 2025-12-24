#include "TriMesh.h"
#include <iostream>

// 确保只在一个文件中定义 STB_IMAGE_IMPLEMENTATION
// 如果你还有其他文件用了 stb_image，请去掉这里的 define，确保全局只有一处
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

TriMesh::TriMesh() : vao(0), vbo(0), shininess(32.0f) {}

TriMesh::~TriMesh() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

// ==========================================================
// 核心函数：自适应读取 (融合贴图模型与纯色模型)
// ==========================================================
void TriMesh::readObjAssimp(const std::string &filename)
{
    Assimp::Importer importer;
    // 使用 CalcTangentSpace 以备未来扩展，Triangulate 是必须的
    const aiScene *scene = importer.ReadFile(filename,
                                             aiProcess_Triangulate |
                                             aiProcess_FlipUVs |
                                             aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return;
    }

    cleanData();
    std::string directory = filename.substr(0, filename.find_last_of('/'));

    // 记录合并 Mesh 时的索引偏移量
    unsigned int baseIndex = 0;

    // 1. 遍历所有子 Mesh (处理钻石剑那种多部件的情况)
    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh *mesh = scene->mMeshes[m];
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        // --- 策略：获取材质颜色 ---
        // 如果 obj 里没有贴图，通常会用 Kd 定义颜色
        aiColor3D color(1.f, 1.f, 1.f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

        // 遍历顶点
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            // 位置
            vertex_positions.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // 法线 (防崩处理)
            if (mesh->HasNormals())
                vertex_normals.emplace_back(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
            else
                vertex_normals.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f));

            // UV
            if (mesh->mTextureCoords[0])
                vertex_texcoords.emplace_back(glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y));
            else
                vertex_texcoords.emplace_back(glm::vec2(0.0f, 0.0f));

            // 颜色 (将材质 Kd 烘焙进顶点颜色)
            // 钻石剑：color 是蓝色/棕色...
            // Steve：color 通常是白色 (默认)
            vertex_colors.emplace_back(glm::vec3(color.r, color.g, color.b));
        }

        // 遍历面 (索引需要加上 baseIndex)
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            faces.emplace_back(
                face.mIndices[0] + baseIndex,
                face.mIndices[1] + baseIndex,
                face.mIndices[2] + baseIndex
            );
        }

        // 更新偏移量
        baseIndex += mesh->mNumVertices;

        // 加载贴图 (如果有的话)
        auto loadMaps = [&](aiTextureType type, const std::string& typeName) {
            for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
                aiString str;
                material->GetTexture(type, i, &str);

                // 简单去重
                bool skip = false;
                for(const auto& t : textures) if(t.path == str.C_Str()) skip = true;

                if(!skip) {
                    Texture tex;
                    tex.id = loadTexture(str.C_Str(), directory);
                    tex.type = typeName;
                    tex.path = str.C_Str();
                    textures.push_back(tex);
                }
            }
        };
        loadMaps(aiTextureType_DIFFUSE, "texture_diffuse");
        loadMaps(aiTextureType_SPECULAR, "texture_specular");
    }

    // 2. 兜底策略：如果没有 Diffuse 贴图，生成一张 1x1 的白色贴图
    // 这样 Shader 里的 texture() * vertexColor 才能正确显示 vertexColor
    bool hasDiffuse = false;
    for(const auto& t : textures) {
        if(t.type == "texture_diffuse") hasDiffuse = true;
    }

    if (!hasDiffuse) {
        Texture whiteTex;
        whiteTex.id = loadTexture("internal_white", ""); // 特殊标记
        whiteTex.type = "texture_diffuse";
        whiteTex.path = "internal_white";
        textures.push_back(whiteTex);
    }

    std::cout << "Loaded Model: " << filename << " | Meshes: " << scene->mNumMeshes
              << " | Textures: " << textures.size() << std::endl;

    storeFacesPoints();
}

// ==========================================================
// 展平数据传给 GPU (适配 Layout 0,1,2,3)
// ==========================================================
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

// ==========================================================
// 纹理加载 (含默认白图生成)
// ==========================================================
unsigned int TriMesh::loadTexture(const std::string &path, const std::string &directory)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // [新增逻辑] 特殊标记：生成 1x1 白色纹理
    // 用于给没有贴图的模型（如钻石剑）提供默认颜色乘数
    if (path == "internal_white") {
        unsigned char white[] = {255, 255, 255, 255}; // RGBA
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);

        // 必须设置过滤参数，否则纹理可能无法采样
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        return textureID;
    }

    // 标准文件加载逻辑
    std::string filename = directory + '/' + path;
    std::cout << "[Texture Debug] Trying to load: " << filename << std::endl;
    int width, height, nrComponents;

    // 确保 stb_image 加载时翻转 Y 轴，否则贴图是倒的
    // stbi_set_flip_vertically_on_load(true); // 如果你之前没加这句，建议加上

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

        // [关键] Minecraft 风格推荐使用 GL_NEAREST (邻近采样) 保持像素颗粒感
        // 如果想要平滑效果，保留 GL_LINEAR_MIPMAP_LINEAR
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

void TriMesh::draw(GLuint program, const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj)
{
    glUseProgram(program);

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

    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &proj[0][0]);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, points.size());
}

void TriMesh::cleanData()
{
    vertex_positions.clear(); vertex_normals.clear(); vertex_texcoords.clear(); vertex_colors.clear();
    faces.clear(); points.clear(); normals.clear(); texcoords.clear(); colors.clear();
    textures.clear();
}