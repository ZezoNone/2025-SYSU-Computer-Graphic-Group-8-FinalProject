#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <mesh.h>
#include <shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;	// 存储已加载的所有纹理，避免重复加载
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
    bool gltf;
    
    // 新增：模型变换相关属性
    glm::vec3 position;       // 模型位置
    glm::vec3 rotation;       // 模型旋转（欧拉角）
    glm::vec3 scale;          // 模型缩放
    glm::mat4 modelMatrix;    // 模型矩阵

    // 构造函数，传入模型文件路径和伽马校正参数，确定是否加载gltf模型
    Model(string const& path, bool gamma = false, bool gltf = false) : gammaCorrection(gamma), gltf(gltf)
    {
        // 初始化变换属性
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        scale = glm::vec3(1.0f, 1.0f, 1.0f);
        modelMatrix = glm::mat4(1.0f);
        
        loadModel(path);
        updateModelMatrix(); // 初始化模型矩阵
    }

    // 绘制模型（更新版本，自动传递模型矩阵）
    void Draw(Shader& shader, float metallic = 0.5, float roughness = 0.5, float ao = 1.0)
    {
        // 设置模型矩阵到着色器
        updateModelMatrix();
        shader.setBool("gltf", gltf);
        shader.setMat4("model", modelMatrix);
        shader.setFloat("defaultMetallic", metallic);
        shader.setFloat("defaultRoughness", roughness);
        shader.setFloat("defaultAO", ao);
        // 绘制所有网格
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    //移动模型（相对移动）
    void Move(glm::vec3 offset)
    {
        position += offset;
    }

    // 设置模型位置（绝对位置）
    void SetPosition(glm::vec3 newPosition)
    {
        position = newPosition;
    }

    // 获取当前位置
    glm::vec3 GetPosition() const
    {
        return position;
    }

    //旋转模型
    void Rotate(glm::vec3 eulerAngles)
    {
        rotation += eulerAngles;
    }

    //设置模型缩放
    void SetScale(glm::vec3 newScale)
    {
        scale = newScale;
    }

    //重置模型变换
    void ResetTransform()
    {
        position = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
    }

private:
    // 加载模型
    void loadModel(string const& path)
    {
        Assimp::Importer importer;
        if (!gltf)
        {
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
                return;
            }

            directory = path.substr(0, path.find_last_of('/'));
            processNode(scene->mRootNode, scene);
        }
        else
        {
            Assimp::Importer importer;
            // 关键：Assimp后处理选项（适配GLTF）
            const aiScene* scene = importer.ReadFile(path,
                aiProcess_Triangulate           // 三角化
                | aiProcess_GenSmoothNormals    // 生成法线
                | aiProcess_CalcTangentSpace    // 生成切线/副切线
                | aiProcess_JoinIdenticalVertices // 合并重复顶点
                | aiProcess_PopulateArmatureData // 骨骼数据（若有）
                | aiProcess_ValidateDataStructure// 验证GLTF结构
                | aiProcess_FlipUVs //UV分解
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                std::cerr << "Assimp加载GLTF失败: " << importer.GetErrorString() << std::endl;
                return;
            }
            directory = path.substr(0, path.find_last_of('/'));
            processNode(scene->mRootNode, scene);
        }
    }

    // 处理节点
    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    // 处理网格
    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            // 位置
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // 法线
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // 纹理坐标
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

                // 切线
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;

                // 副切线
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        // 索引
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }



        // 材质
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        float metallicFactor = 0.0f;
        float roughnessFactor = 0.5f;
        bool isGlass = false;
        bool doubleSided = false;

        if (gltf) {
            // 解析GLTF材质的doubleSided
            bool doubleSide;
            if (material->Get(AI_MATKEY_TWOSIDED, doubleSide) == AI_SUCCESS) {
                doubleSided = (bool)doubleSide;
            }

            // 解析baseColorFactor（GLTF PBR）
            aiColor4D baseColor;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) {
                baseColorFactor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            }

            // 解析metallicFactor
            float metallic;
            if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
                metallicFactor = metallic;
            }

            // 解析roughnessFactor
            float roughness;
            if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
                roughnessFactor = roughness;
            }

            // 判断是否为玻璃材质（根据名称/参数特征）
            aiString matName;
            material->Get(AI_MATKEY_NAME, matName);
            if (matName.C_Str() == string("glass") || (metallicFactor < 0.1 && roughnessFactor < 0.2 && baseColorFactor.a < 0.2)) {
                isGlass = true;
            }
        }

        if (!gltf)
        {
            // 基础颜色贴图
            vector<Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_albedo");
            textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());

            // 法线贴图
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // 金属度贴图
            vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_metallic");
            textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

            //粗糙度贴图
            std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_SHININESS, "texture_roughness");
            textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

            // AO贴图
            std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_ao");
            textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
        }
        else
        {
            // 基础颜色贴图
            vector<Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_albedo");
            textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());

            // 法线贴图
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // 加载粗糙度纹理和金属度纹理（GLTF金属度/粗糙度通常合并为一张纹理，需拆分）
            std::vector<Texture> roughness_metallicMaps = loadMaterialTextures(material, aiTextureType_GLTF_METALLIC_ROUGHNESS, "gltf_texture_metallic_roughness");
            textures.insert(textures.end(), roughness_metallicMaps.begin(), roughness_metallicMaps.end());
        }

        return Mesh(vertices, indices, textures, baseColorFactor, metallicFactor, roughnessFactor, isGlass, doubleSided);
    }

    // 加载材质纹理
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            
            if (!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

    //更新模型矩阵
    void updateModelMatrix()
    {
        // 重置矩阵
        modelMatrix = glm::mat4(1.0f);
        
        // 应用变换：先缩放，再旋转，最后平移
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, scale);
    }
};

// 纹理加载函数
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif