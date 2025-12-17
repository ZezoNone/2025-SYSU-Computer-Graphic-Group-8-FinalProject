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
#include <assimp/pbrmaterial.h>

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
    bool gltf;

    // 构造函数，传入模型文件路径，确定是否加载gltf模型
    Model(string const& path, bool gltf = false) : gltf(gltf)
    {    
        loadModel(path); // 初始化模型矩阵
    }

    // 绘制模型
    void Draw(Shader& shader,bool depth = false, float metallic = 1.0, float roughness = 1.0, float ao = 1.0)
    {
        if (depth)
        {
            for (unsigned int i = 0; i < meshes.size(); i++)
                meshes[i].Draw(shader,true);
            return;
        }
        shader.setBool("gltf", gltf);
        // 绘制所有网格
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
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
            // Assimp后处理选项（适配GLTF）
            const aiScene* scene = importer.ReadFile(path,
                  aiProcess_Triangulate           // 三角化
                | aiProcess_GenSmoothNormals    // 生成法线
                | aiProcess_CalcTangentSpace    // 生成切线/副切线
                | aiProcess_JoinIdenticalVertices // 合并重复顶点
                | aiProcess_PopulateArmatureData // 骨骼数据
                | aiProcess_ValidateDataStructure// 验证GLTF结构
                | aiProcess_PreTransformVertices //烘焙节点变换
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
            {
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

        // 处理顶点
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

        // 处理索引
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // 处理材质
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 默认材质因子
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        bool isGlass = false;
        bool doubleSided = false;
        bool isblend = false;


        if (gltf) 
        {
            // 解析GLTF材质的doubleSided
            bool doubleSide;
            if (material->Get(AI_MATKEY_TWOSIDED, doubleSide) == AI_SUCCESS) 
            {
                doubleSided = (bool)doubleSide;
            }

            // 解析baseColorFactor（GLTF PBR）
            aiColor4D baseColor;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) 
            {
                baseColorFactor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            }

            // 解析metallicFactor
            float metallic;
            if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) 
            {
                metallicFactor = metallic;
            }

            // 解析roughnessFactor
            float roughness;
            if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) 
            {
                roughnessFactor = roughness;
            }

            // 判断是否为玻璃材质（根据名称/参数特征）
            aiString alphaMode;
            aiString matName;
            material->Get(AI_MATKEY_NAME, matName);
            std::string nameStr = std::string(matName.C_Str());

            // 转为小写以忽略大小写差异
            std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);

            // 检测是否包含 "glass"
            if (nameStr.find("glass") != std::string::npos)
            {
                isGlass = true;
            }
            //检测材料是否启动混合
            if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) 
            {
                if (aiString("BLEND") == alphaMode) 
                {
                    isblend = true;
                }
            }
        }

        if (!gltf)
        {
            // 基础颜色贴图
            vector<Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "albedoMap");
            textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());

            // 法线贴图
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "normalMap");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // 金属度贴图
            vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "metallicMap");
            textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

            //粗糙度贴图
            std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_SHININESS, "roughnessMap");
            textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

            // AO贴图
            std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "aoMap");
            textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
        }
        else
        {
            // 基础颜色贴图
            vector<Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "albedoMap");
            textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
            if (albedoMaps.empty())
            {
                vector<Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "albedoMap");
                textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
            }

            // 法线贴图
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normalMap");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // 加载粗糙度纹理和金属度纹理（GLTF金属度/粗糙度通常合并为一张纹理，需拆分）
            std::vector<Texture> roughness_metallicMaps = loadMaterialTextures(material, aiTextureType_GLTF_METALLIC_ROUGHNESS, "metallic_roughnessMap");
            textures.insert(textures.end(), roughness_metallicMaps.begin(), roughness_metallicMaps.end());
        }

        return Mesh(vertices, indices, textures, baseColorFactor, metallicFactor, roughnessFactor, isGlass, doubleSided, isblend);
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
};

// 纹理加载函数
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    GLint oldTextureID;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureID);

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
    glBindTexture(GL_TEXTURE_2D, oldTextureID);

    return textureID;
}
#endif
