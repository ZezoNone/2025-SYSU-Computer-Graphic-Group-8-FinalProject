#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>

#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
	// position
	glm::vec3 Position;
	// normal
	glm::vec3 Normal;
	// texCoords
	glm::vec2 TexCoords;
	// tangent
	glm::vec3 Tangent;
	// bitangent
	glm::vec3 Bitangent;
	//bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	//weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
	unsigned int id;
	string type;
	string path;
};

class Mesh {
public:
	// mesh Data
	vector<Vertex>       vertices;
	vector<unsigned int> indices;
	vector<Texture>      textures;
	unsigned int VAO;

	glm::vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;

	bool isGlass = false;
	bool doubleSided = false;
	bool isblend = false;

	// constructor
	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures,
		glm::vec4 baseColor = glm::vec4(1.0f), float metallic = 1.0f, float roughness = 1.0f,
		bool glass = false, bool doubleSide = false, bool isblend = false)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		this->baseColorFactor = baseColor;
		this->metallicFactor = metallic;
		this->roughnessFactor = roughness;

		this->isGlass = glass;
		this->doubleSided = doubleSide;
		this->isblend = isblend;

		// now that we have all the required data, set the vertex buffers and its attribute pointers.
		setupMesh();
	}

	// render the mesh
	void Draw(Shader& shader, bool depth = false)
	{
		if (depth)
		{
			if (isGlass) return; // 玻璃不参与深度贴图渲染
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			return;
		}

		GLboolean oldBlend, oldCullFace, oldDepthMask;
		GLint oldBlendSrc, oldBlendDst;
		GLint oldDepthFunc;
		// 保存混合状态
		glGetBooleanv(GL_BLEND, &oldBlend);
		glGetIntegerv(GL_BLEND_SRC, &oldBlendSrc);
		glGetIntegerv(GL_BLEND_DST, &oldBlendDst);
		// 保存面剔除状态
		glGetBooleanv(GL_CULL_FACE, &oldCullFace);
		// 保存深度写入状态
		glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
		// 保存深度测试函数（天空盒依赖GL_LEQUAL）
		glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);

		shader.setBool("useAlbedoMap", false);
		shader.setBool("useNormalMap", false);
		shader.setBool("useMetallicRoughnessMap", false);
		shader.setBool("useAOMap", false);
		shader.setBool("useMetallicMap", false);
		shader.setBool("useRoughnessMap", false);
		shader.setBool("isGlass", false);
		shader.setBool("doubleSided", false);
		shader.setVec4("materialBaseColor", glm::vec4(1.0f));
		shader.setFloat("materialMetallic", 1.0f);
		shader.setFloat("materialRoughness", 1.0f);

		// 传递PBR材质因子
		shader.setVec4("materialBaseColor", baseColorFactor);
		shader.setFloat("materialMetallic", metallicFactor);
		shader.setFloat("materialRoughness", roughnessFactor);

		// 绑定纹理并设置标志位
		bool hasAlbedo = false;
		bool hasNormal = false;
		bool hasMetallicRoughness = false;
		bool hasAO = false;
		bool hasMetallic = false;
		bool hasRoughness = false;

		//初始化玻璃材质参数
		shader.setBool("isGlass", isGlass);

		// 绑定纹理
		if (!isGlass)
		{
			//unsigned int albedoNr = 1;
			//unsigned int metallicNr = 1;
			//unsigned int normalNr = 1;
			//unsigned int aoNr = 1;
			//unsigned int roughnessNr = 1;
			//unsigned int gltfmetallroughNr = 1;
			for (unsigned int i = 0; i < textures.size(); i++)
			{
				// 纹理单元从3开始，避开IBL/天空盒的0-2号单元
				glActiveTexture(GL_TEXTURE3 + i);
				string number;
				string name = textures[i].type;
				if (name == "albedoMap")
				{
					//number = std::to_string(albedoNr++);
					hasAlbedo = true;
				}
				else if (name == "metallicMap")
				{
					//number = std::to_string(metallicNr++);
					hasMetallic = true;
				}
				else if (name == "normalMap")
				{
					//number = std::to_string(normalNr++);
					hasNormal = true;
				}
				else if (name == "aoMap")
				{
					//number = std::to_string(aoNr++);
					hasAO = true;
				}
				else if (name == "roughnessMap")
				{
					//number = std::to_string(roughnessNr++);
					hasRoughness = true;
				}
				else if (name == "metallic_roughnessMap")
				{
					//number = std::to_string(gltfmetallroughNr++);
					hasMetallicRoughness = true;
				}

				glBindTexture(GL_TEXTURE_2D, textures[i].id);
				glUniform1i(glGetUniformLocation(shader.ID, (name).c_str()), i + 3);

			}
		}
		else
		{
			shader.setVec4("materialBaseColor", baseColorFactor);
			shader.setFloat("materialMetallic", metallicFactor);
			shader.setFloat("materialRoughness", roughnessFactor);
			shader.setBool("doubleSided", doubleSided);
		}

		shader.setBool("useAlbedoMap", hasAlbedo);
		shader.setBool("useNormalMap", hasNormal);
		shader.setBool("useMetallicRoughnessMap", hasMetallicRoughness);
		shader.setBool("useAOMap", hasAO);
		shader.setBool("useMetallicMap", hasMetallic);
		shader.setBool("useRoughnessMap", hasRoughness);

		//绘制逻辑
		glBindVertexArray(VAO);
		if (isGlass)
		{
			// 玻璃材质：开启混合、关闭深度写入、双面渲染
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			if (doubleSided) glDisable(GL_CULL_FACE);
		}
		else
		{
			if (isblend)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			if (doubleSided) glDisable(GL_CULL_FACE);
			// 非玻璃材质：正常绘制  
		}

		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// 恢复所有原始OpenGL状态
		// 恢复混合状态
		if (oldBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
		glBlendFunc(oldBlendSrc, oldBlendDst);
		// 恢复面剔除
		if (oldCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		// 恢复深度写入
		glDepthMask(oldDepthMask);
		// 恢复深度测试函数
		glDepthFunc(oldDepthFunc);
		// 恢复默认纹理单元
		glActiveTexture(GL_TEXTURE3);
		// 重置玻璃标记
		shader.setBool("isGlass", false);
	}

private:
	// render data 
	unsigned int VBO, EBO;

	// initializes all the buffer objects/arrays
	void setupMesh()
	{
		// create buffers/arrays
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		// load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
		// again translates to 3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
		// vertex tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
		// vertex bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		// ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
		// weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
		glBindVertexArray(0);
	}
};
#endif
