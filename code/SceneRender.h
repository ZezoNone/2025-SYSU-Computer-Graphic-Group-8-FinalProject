#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void setModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, Shader& shader);
void renderWall();
void renderSphere();
void renderGround();
void renderCube();
void renderQuad();

void setModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, Shader& shader)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    shader.setMat4("model", model);
    shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
}

// 球体生成函数
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

//地面渲染函数（带厚度，无纹理空隙）
unsigned int groundVAO = 0;
unsigned int groundIndexCount;
void renderGround()
{
    if (groundVAO == 0)
    {
        glGenVertexArrays(1, &groundVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> tangents; 
        std::vector<unsigned int> indices;

        // 地面参数
        const unsigned int SEGMENTS = 64;    // 分段数
        const float GROUND_THICKNESS = 0.2f; // 地面厚度
        const float GROUND_SCALE = 1.0f;     // 地面基础缩放

        for (unsigned int z = 0; z <= SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x <= SEGMENTS; ++x)
            {
                float xPos = ((float)x / SEGMENTS - 0.5f) * GROUND_SCALE;
                float zPos = ((float)z / SEGMENTS - 0.5f) * GROUND_SCALE;

                positions.push_back(glm::vec3(xPos, 0.0f, zPos));
                uv.push_back(glm::vec2((float)x / SEGMENTS, (float)z / SEGMENTS));
                normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
                tangents.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
            }
        }

        unsigned int bottomStartIdx = positions.size();
        for (unsigned int z = 0; z <= SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x <= SEGMENTS; ++x)
            {
                float xPos = ((float)x / SEGMENTS - 0.5f) * GROUND_SCALE;
                float zPos = ((float)z / SEGMENTS - 0.5f) * GROUND_SCALE;

                positions.push_back(glm::vec3(xPos, -GROUND_THICKNESS, zPos));
                uv.push_back(glm::vec2((float)x / SEGMENTS, (float)z / SEGMENTS));
                normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f)); 
                tangents.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
            }
        }

        for (unsigned int z = 0; z < SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x < SEGMENTS; ++x)
            {
                unsigned int idx0 = z * (SEGMENTS + 1) + x;          
                unsigned int idx1 = z * (SEGMENTS + 1) + x + 1;      
                unsigned int idx2 = (z + 1) * (SEGMENTS + 1) + x;    
                unsigned int idx3 = (z + 1) * (SEGMENTS + 1) + x + 1;

                indices.push_back(idx0);
                indices.push_back(idx1);
                indices.push_back(idx2);

                indices.push_back(idx1);
                indices.push_back(idx3);
                indices.push_back(idx2);
            }
        }

        for (unsigned int z = 0; z < SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x < SEGMENTS; ++x)
            {
                unsigned int idx0 = bottomStartIdx + z * (SEGMENTS + 1) + x;
                unsigned int idx1 = bottomStartIdx + z * (SEGMENTS + 1) + x + 1;
                unsigned int idx2 = bottomStartIdx + (z + 1) * (SEGMENTS + 1) + x;
                unsigned int idx3 = bottomStartIdx + (z + 1) * (SEGMENTS + 1) + x + 1;

                indices.push_back(idx0);
                indices.push_back(idx2);
                indices.push_back(idx1);
                indices.push_back(idx1);
                indices.push_back(idx2);
                indices.push_back(idx3);
            }
        }

        for (unsigned int z = 0; z < SEGMENTS; ++z)
        {
            unsigned int top0 = z * (SEGMENTS + 1);                
            unsigned int top1 = (z + 1) * (SEGMENTS + 1);          
            unsigned int bottom0 = bottomStartIdx + top0;         
            unsigned int bottom1 = bottomStartIdx + top1;         

            tangents[top0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[top1] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom1] = glm::vec3(0.0f, 0.0f, 1.0f);

            indices.push_back(top0);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(bottom1);
        }

        for (unsigned int z = 0; z < SEGMENTS; ++z)
        {
            unsigned int top0 = z * (SEGMENTS + 1) + SEGMENTS;
            unsigned int top1 = (z + 1) * (SEGMENTS + 1) + SEGMENTS;
            unsigned int bottom0 = bottomStartIdx + top0;
            unsigned int bottom1 = bottomStartIdx + top1;

            tangents[top0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[top1] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom1] = glm::vec3(0.0f, 0.0f, 1.0f);

            indices.push_back(top0);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(bottom1);
            indices.push_back(bottom0);
        }

        for (unsigned int x = 0; x < SEGMENTS; ++x)
        {
            unsigned int top0 = x;
            unsigned int top1 = x + 1;
            unsigned int bottom0 = bottomStartIdx + top0;
            unsigned int bottom1 = bottomStartIdx + top1;

            tangents[top0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[top1] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom1] = glm::vec3(1.0f, 0.0f, 0.0f);

            indices.push_back(top0);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(bottom1);
        }

        for (unsigned int x = 0; x < SEGMENTS; ++x)
        {
            unsigned int top0 = SEGMENTS * (SEGMENTS + 1) + x;
            unsigned int top1 = SEGMENTS * (SEGMENTS + 1) + x + 1;
            unsigned int bottom0 = bottomStartIdx + top0;
            unsigned int bottom1 = bottomStartIdx + top1;

            tangents[top0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[top1] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom1] = glm::vec3(1.0f, 0.0f, 0.0f);

            indices.push_back(top0);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(bottom1);
            indices.push_back(bottom0);
        }

        groundIndexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        data.reserve(positions.size() * 11);
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);

            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);

            data.push_back(uv[i].x + 0.0001f);
            data.push_back(uv[i].y + 0.0001f);

            glm::vec3 tan = glm::normalize(tangents[i]);
            data.push_back(tan.x);
            data.push_back(tan.y);
            data.push_back(tan.z);
        }

        glBindVertexArray(groundVAO);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        unsigned int stride = (3 + 3 + 2 + 3) * sizeof(float);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, stride, (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, stride, (void*)(8 * sizeof(float))); 

        glBindVertexArray(0);
    }

    glBindVertexArray(groundVAO);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDrawElements(GL_TRIANGLES, groundIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

//墙面生成函数
unsigned int wallVAO = 0;
unsigned int wallIndexCount;

const float WALL_THICKNESS = 0.2f;   // 墙面厚度
const float WALL_SCALE = 1.0f;       // 墙面基础缩放
const unsigned int WALL_SEGMENTS = 32; // 分段数

void renderWall()
{
    if (wallVAO == 0)
    {
        glGenVertexArrays(1, &wallVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> tangents;
        std::vector<unsigned int> indices;

        unsigned int frontStartIdx = 0;
        for (unsigned int z = 0; z <= WALL_SEGMENTS; ++z)  
        {
            for (unsigned int x = 0; x <= WALL_SEGMENTS; ++x) 
            {

                float xPos = ((float)x / WALL_SEGMENTS - 0.5f) * WALL_SCALE;
                float zPos = ((float)z / WALL_SEGMENTS - 0.5f) * WALL_SCALE;

                positions.push_back(glm::vec3(xPos, 0.0f, zPos));
                uv.push_back(glm::vec2((float)x / WALL_SEGMENTS, (float)z / WALL_SEGMENTS));
                normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
                tangents.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
            }
        }

        unsigned int backStartIdx = positions.size();
        for (unsigned int z = 0; z <= WALL_SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x <= WALL_SEGMENTS; ++x)
            {
                float xPos = ((float)x / WALL_SEGMENTS - 0.5f) * WALL_SCALE;
                float zPos = ((float)z / WALL_SEGMENTS - 0.5f) * WALL_SCALE;

                positions.push_back(glm::vec3(xPos, -WALL_THICKNESS, zPos));
                uv.push_back(glm::vec2((float)x / WALL_SEGMENTS, (float)z / WALL_SEGMENTS));
                normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
                tangents.push_back(glm::vec3(1.0f, 0.0f, 0.0f)); 
            }
        }

        for (unsigned int z = 0; z < WALL_SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x < WALL_SEGMENTS; ++x)
            {
                unsigned int idx0 = z * (WALL_SEGMENTS + 1) + x;          
                unsigned int idx1 = z * (WALL_SEGMENTS + 1) + x + 1;      
                unsigned int idx2 = (z + 1) * (WALL_SEGMENTS + 1) + x;    
                unsigned int idx3 = (z + 1) * (WALL_SEGMENTS + 1) + x + 1;

                indices.push_back(idx0);
                indices.push_back(idx1);
                indices.push_back(idx2);

                indices.push_back(idx1);
                indices.push_back(idx3);
                indices.push_back(idx2);
            }
        }

        for (unsigned int z = 0; z < WALL_SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x < WALL_SEGMENTS; ++x)
            {
                unsigned int idx0 = backStartIdx + z * (WALL_SEGMENTS + 1) + x;
                unsigned int idx1 = backStartIdx + z * (WALL_SEGMENTS + 1) + x + 1;
                unsigned int idx2 = backStartIdx + (z + 1) * (WALL_SEGMENTS + 1) + x;
                unsigned int idx3 = backStartIdx + (z + 1) * (WALL_SEGMENTS + 1) + x + 1;

                indices.push_back(idx0);
                indices.push_back(idx2);
                indices.push_back(idx1);
                indices.push_back(idx1);
                indices.push_back(idx2);
                indices.push_back(idx3);
            }
        }

        for (unsigned int z = 0; z < WALL_SEGMENTS; ++z)
        {
            unsigned int top0 = z * (WALL_SEGMENTS + 1);                
            unsigned int top1 = (z + 1) * (WALL_SEGMENTS + 1);          
            unsigned int bottom0 = backStartIdx + top0;                  
            unsigned int bottom1 = backStartIdx + top1;                  

            tangents[top0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[top1] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom1] = glm::vec3(0.0f, 0.0f, 1.0f);

            indices.push_back(top0);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(bottom1);
        }

        for (unsigned int z = 0; z < WALL_SEGMENTS; ++z)
        {
            unsigned int top0 = z * (WALL_SEGMENTS + 1) + WALL_SEGMENTS;
            unsigned int top1 = (z + 1) * (WALL_SEGMENTS + 1) + WALL_SEGMENTS;
            unsigned int bottom0 = backStartIdx + top0;
            unsigned int bottom1 = backStartIdx + top1;

            tangents[top0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[top1] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom0] = glm::vec3(0.0f, 0.0f, 1.0f);
            tangents[bottom1] = glm::vec3(0.0f, 0.0f, 1.0f);

            indices.push_back(top0);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(bottom1);
            indices.push_back(bottom0);
        }

        for (unsigned int x = 0; x < WALL_SEGMENTS; ++x)
        {
            unsigned int top0 = x;
            unsigned int top1 = x + 1;
            unsigned int bottom0 = backStartIdx + top0;
            unsigned int bottom1 = backStartIdx + top1;

            tangents[top0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[top1] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom1] = glm::vec3(1.0f, 0.0f, 0.0f);

            indices.push_back(top0);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(bottom1);
        }

        for (unsigned int x = 0; x < WALL_SEGMENTS; ++x)
        {
            unsigned int top0 = WALL_SEGMENTS * (WALL_SEGMENTS + 1) + x;
            unsigned int top1 = WALL_SEGMENTS * (WALL_SEGMENTS + 1) + x + 1;
            unsigned int bottom0 = backStartIdx + top0;
            unsigned int bottom1 = backStartIdx + top1;

            tangents[top0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[top1] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom0] = glm::vec3(1.0f, 0.0f, 0.0f);
            tangents[bottom1] = glm::vec3(1.0f, 0.0f, 0.0f);

            indices.push_back(top0);
            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(top1);
            indices.push_back(bottom1);
            indices.push_back(bottom0);
        }

        wallIndexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        data.reserve(positions.size() * 11);
        for (unsigned int i = 0; i < positions.size(); ++i)
        {

            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);

            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);

            data.push_back(uv[i].x + 0.0001f);
            data.push_back(uv[i].y + 0.0001f);

            glm::vec3 tan = glm::normalize(tangents[i]);
            data.push_back(tan.x);
            data.push_back(tan.y);
            data.push_back(tan.z);
        }

        glBindVertexArray(wallVAO);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        unsigned int stride = (3 + 3 + 2 + 3) * sizeof(float);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, stride, (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, stride, (void*)(8 * sizeof(float)));

        glBindVertexArray(0);
    }

    glBindVertexArray(wallVAO);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glDrawElements(GL_TRIANGLES, wallIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
             // bottom face
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
              1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             // top face
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
              1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
              1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
              1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}