#ifndef _TRI_MESH_H_
#define _TRI_MESH_H_

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Texture {
	unsigned int id;
	std::string type;
	std::string path;
};

typedef struct vIndex {
	unsigned int x, y, z;
	vIndex(int ix, int iy, int iz) : x(ix), y(iy), z(iz) {}
} vec3i;

class TriMesh {
public:
	TriMesh();
	~TriMesh();

	// 支持自动加载贴图 + 自动烘焙材质颜色
	void readObjAssimp(const std::string &filename);

	void draw(GLuint program, const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj);
	void storeFacesPoints();
	void cleanData();

	// Setter
	void setAmbient(glm::vec4 a) { ambient = a; }
	void setDiffuse(glm::vec4 d) { diffuse = d; }
	void setSpecular(glm::vec4 s) { specular = s; }
	void setShininess(float s) { shininess = s; }

	// 获取模型的局部坐标系包围盒
	glm::vec3 getMinBound() const { return minBound; }
	glm::vec3 getMaxBound() const { return maxBound; }
	glm::vec3 getSize() const { return maxBound - minBound; } // 原始长宽高
protected:
	// 原始数据
	std::vector<glm::vec3> vertex_positions;
	std::vector<glm::vec3> vertex_colors;   // [关键] 存储材质颜色或白色
	std::vector<glm::vec3> vertex_normals;
	std::vector<glm::vec2> vertex_texcoords;

	std::vector<vec3i> faces;

	// GPU 数据
	std::vector<glm::vec3> points;
	std::vector<glm::vec3> colors;          // [关键] 传给 layout=3
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texcoords;

	std::vector<Texture> textures;
	glm::vec4 ambient, diffuse, specular;
	float shininess;

	GLuint vao, vbo;

	static unsigned int loadTexture(const std::string &path, const std::string &directory);

	// 存储原始边界
	glm::vec3 minBound;
	glm::vec3 maxBound;
};

#endif