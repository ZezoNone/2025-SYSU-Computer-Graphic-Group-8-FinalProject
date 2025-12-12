#include "skybox.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Quad：XY 平面 1x1 正方形，两三角形
static const float SKYBOX_QUAD_VERTICES[] = {
    -0.5f, -0.5f,  0.0f,
     0.5f, -0.5f,  0.0f,
     0.5f,  0.5f,  0.0f,

    -0.5f, -0.5f,  0.0f,
     0.5f,  0.5f,  0.0f,
    -0.5f,  0.5f,  0.0f
};

// Cube：和 GeometryGenerator 里一致的 36 个顶点
static const float SKYBOX_CUBE_VERTICES[] = {
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,

    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,

     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,

    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f
};

static const char* SKYBOX_VERTEX_SHADER_SRC = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char* SKYBOX_FRAGMENT_SHADER_SRC = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main()
{
    FragColor = vec4(color, 1.0);
}
)";

Skybox::Skybox() {
    initGeometry();
    initShader();
    std::cout << "Procedural classical garden skybox initialized (no HDR / textures)." << std::endl;
}

Skybox::~Skybox() {
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (cubeVAO) glDeleteVertexArrays(1, &cubeVAO);
    if (cubeVBO) glDeleteBuffers(1, &cubeVBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

void Skybox::initGeometry() {
    // Quad
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_QUAD_VERTICES), SKYBOX_QUAD_VERTICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Cube
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_CUBE_VERTICES), SKYBOX_CUBE_VERTICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Skybox::initShader() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, SKYBOX_VERTEX_SHADER_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, SKYBOX_FRAGMENT_SHADER_SRC);
    shaderProgram = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

GLuint Skybox::compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Skybox shader compile error:\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint Skybox::linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(prog, 512, nullptr, infoLog);
        std::cout << "Skybox program link error:\n" << infoLog << std::endl;
    }
    return prog;
}

void Skybox::renderQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Skybox::renderCube() {
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// ===================== 按 6 面设计搭出花园盒子 =====================

void Skybox::Draw(const glm::mat4& view, const glm::mat4& projection)
{
    glUseProgram(shaderProgram);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    // 相机无平移版 view
    glm::mat4 viewRot = glm::mat4(glm::mat3(view));

    auto drawCubeColored = [&](const glm::mat4& model, const glm::vec3& color) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));
        renderCube();
    };

    auto drawQuadColored = [&](const glm::mat4& model, const glm::vec3& color) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));
        renderQuad();
    };

    // 世界内花园区域
    const float groundY = -0.5f;
    const float gardenStartZ = -7.0f;
    const float gardenEndZ = -25.0f;
    const float gardenCenterZ = (gardenStartZ + gardenEndZ) * 0.5f;
    const float gardenDepth = gardenStartZ - gardenEndZ;
    const float gardenWidth = 24.0f;

    // 草地范围
    const float GROUND_RADIUS = 60.0f;

    // 无限天空的
    const float SKY_RADIUS = 120.0f;

    // 颜色
    const glm::vec3 skyColor = glm::vec3(0.65f, 0.80f, 0.95f);
    const glm::vec3 horizonColor = skyColor;
    const glm::vec3 grassColor = glm::vec3(0.30f, 0.55f, 0.30f);
    const glm::vec3 pathColor = glm::vec3(0.70f, 0.70f, 0.68f);
    const glm::vec3 hedgeColor = glm::vec3(0.20f, 0.45f, 0.25f);
    const glm::vec3 trunkColor = glm::vec3(0.40f, 0.25f, 0.15f);
    const glm::vec3 leavesColor = glm::vec3(0.20f, 0.45f, 0.25f);
    const glm::vec3 stoneColor = glm::vec3(0.70f, 0.72f, 0.75f);
    const glm::vec3 benchColor = glm::vec3(0.45f, 0.30f, 0.18f);
    const glm::vec3 ivyColor = glm::vec3(0.16f, 0.40f, 0.22f);
    const glm::vec3 archColor = glm::vec3(0.55f, 0.38f, 0.25f);
    const glm::vec3 roseColor = glm::vec3(0.85f, 0.35f, 0.45f);
    const glm::vec3 leafColor = glm::vec3(0.80f, 0.55f, 0.25f);

    
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewRot));

    // 顶部天空板（+Y）：一块很大的淡蓝色天幕
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, SKY_RADIUS * 0.6f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0)); // XY -> XZ 朝下
        model = glm::scale(model, glm::vec3(SKY_RADIUS * 2.5f, SKY_RADIUS * 2.5f, 1.0f));
        drawQuadColored(model, skyColor);
    }

    {
        float midY = 0.0f;
        float height = SKY_RADIUS * 1.2f;

        // -Z 背面
        {
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, midY, -SKY_RADIUS));
            model = glm::scale(model, glm::vec3(SKY_RADIUS * 2.5f, height, 1.0f));
            drawCubeColored(model, horizonColor);
        }
        // +Z 前面
        {
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, midY, SKY_RADIUS));
            model = glm::scale(model, glm::vec3(SKY_RADIUS * 2.5f, height, 1.0f));
            drawCubeColored(model, horizonColor );
        }
        // -X 左侧
        {
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(-SKY_RADIUS, midY, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, height, SKY_RADIUS * 2.5f));
            drawCubeColored(model, horizonColor);
        }
        // +X 右侧
        {
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(SKY_RADIUS, midY, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, height, SKY_RADIUS * 2.5f));
            drawCubeColored(model, horizonColor);
        }
    }

    
    //  花园细节
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // ---------- 1. 大草地 ----------
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, groundY, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0)); // XY -> XZ
        model = glm::scale(model, glm::vec3(GROUND_RADIUS * 2.0f, GROUND_RADIUS * 2.0f, 1.0f));
        drawQuadColored(model, grassColor);
    }

    // ---------- 2. 从窗户看出去的主石板小径 ----------
    {
        float pathWidth = 4.0f;
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, groundY + 0.02f, gardenCenterZ));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        model = glm::scale(model, glm::vec3(pathWidth, gardenDepth, 1.0f));
        drawQuadColored(model, pathColor);
    }

    // ---------- 3. 小花坛 ----------
    {
        glm::vec3 flowerColors[4] = {
            glm::vec3(0.9f, 0.4f, 0.4f),
            glm::vec3(0.9f, 0.8f, 0.4f),
            glm::vec3(0.6f, 0.4f, 0.8f),
            glm::vec3(0.4f, 0.8f, 0.6f)
        };

        float firstZ = gardenStartZ - 2.0f;
        int colorIndex = 0;
        for (int i = 0; i < 6; ++i) {
            float z = firstZ - i * 2.6f;

            // 左花坛
            glm::mat4 modelL(1.0f);
            modelL = glm::translate(modelL, glm::vec3(-4.2f, groundY + 0.03f, z));
            modelL = glm::rotate(modelL, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            modelL = glm::scale(modelL, glm::vec3(2.0f, 1.2f, 1.0f));
            drawQuadColored(modelL, flowerColors[colorIndex % 4]);

            // 右花坛
            glm::mat4 modelR(1.0f);
            modelR = glm::translate(modelR, glm::vec3(4.2f, groundY + 0.03f, z));
            modelR = glm::rotate(modelR, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            modelR = glm::scale(modelR, glm::vec3(2.0f, 1.2f, 1.0f));
            drawQuadColored(modelR, flowerColors[(colorIndex + 1) % 4]);

            colorIndex += 2;
        }
    }

    // ---------- 4. 落叶 ----------
    {
        for (int i = 0; i < 10; ++i) {
            float x = -3.0f + (i % 5) * 0.8f;
            float z = gardenStartZ - 1.5f - (i / 5) * 0.9f;

            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(x, groundY + 0.025f, z));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(0.35f, 0.18f, 1.0f));
            drawQuadColored(model, leafColor);
        }
    }

    // ---------- 5. 对称矮灌木 ----------
    {
        float hedgeHeight = 0.8f;
        float hedgeWidth = 0.8f;
        float firstZ = gardenStartZ - 2.0f;

        for (int i = 0; i < 7; ++i) {
            float z = firstZ - i * 2.4f;

            glm::mat4 modelL(1.0f);
            modelL = glm::translate(modelL, glm::vec3(-2.8f, groundY + hedgeHeight * 0.5f, z));
            modelL = glm::scale(modelL, glm::vec3(hedgeWidth, hedgeHeight, 1.8f));
            drawCubeColored(modelL, hedgeColor);

            glm::mat4 modelR(1.0f);
            modelR = glm::translate(modelR, glm::vec3(2.8f, groundY + hedgeHeight * 0.5f, z));
            modelR = glm::scale(modelR, glm::vec3(hedgeWidth, hedgeHeight, 1.8f));
            drawCubeColored(modelR, hedgeColor);
        }
    }

    // ---------- 6. 远处长椅一角 ----------
    {
        float benchZ = gardenEndZ + 2.0f;

        glm::mat4 seat(1.0f);
        seat = glm::translate(seat, glm::vec3(-2.0f, groundY + 0.45f, benchZ));
        seat = glm::scale(seat, glm::vec3(1.8f, 0.15f, 0.4f));
        drawCubeColored(seat, benchColor);

        glm::mat4 back(1.0f);
        back = glm::translate(back, glm::vec3(-2.0f, groundY + 0.9f, benchZ - 0.15f));
        back = glm::scale(back, glm::vec3(1.8f, 0.7f, 0.12f));
        drawCubeColored(back, benchColor * 0.9f);
    }

    // ---------- 7. 花园深处喷泉 + 高树背景 ----------
    {
        float centerZ = gardenEndZ + 2.5f;

        glm::mat4 base(1.0f);
        base = glm::translate(base, glm::vec3(0.0f, groundY + 0.25f, centerZ));
        base = glm::scale(base, glm::vec3(3.0f, 0.5f, 3.0f));
        drawCubeColored(base, stoneColor);

        glm::mat4 column(1.0f);
        column = glm::translate(column, glm::vec3(0.0f, groundY + 1.5f, centerZ));
        column = glm::scale(column, glm::vec3(0.8f, 2.5f, 0.8f));
        drawCubeColored(column, stoneColor * 0.95f);

        glm::mat4 bowl(1.0f);
        bowl = glm::translate(bowl, glm::vec3(0.0f, groundY + 2.3f, centerZ));
        bowl = glm::scale(bowl, glm::vec3(1.6f, 0.3f, 1.6f));
        drawCubeColored(bowl, stoneColor * 1.05f);

        // 树
        for (int i = -3; i <= 3; ++i) {
            float x = i * 3.5f;
            float z = gardenEndZ + 0.5f - (i % 2) * 1.0f;

            glm::mat4 trunk(1.0f);
            trunk = glm::translate(trunk, glm::vec3(x, groundY + 2.0f, z));
            trunk = glm::scale(trunk, glm::vec3(0.6f, 4.0f, 0.6f));
            drawCubeColored(trunk, trunkColor);

            glm::mat4 crown(1.0f);
            crown = glm::translate(crown, glm::vec3(x, groundY + 5.0f, z));
            crown = glm::scale(crown, glm::vec3(3.0f, 2.5f, 3.0f));
            drawCubeColored(crown, leavesColor);
        }
    }

    // ---------- 8. 右侧：常春藤外墙 ----------
    {
        float wallX = gardenWidth / 2.0f + 2.5f;
        float wallHeight = 5.0f;
        float wallThickness = 1.0f;
        float wallCenterZ = gardenCenterZ;

        glm::mat4 wall(1.0f);
        wall = glm::translate(wall, glm::vec3(wallX, groundY + wallHeight / 2.0f, wallCenterZ));
        wall = glm::scale(wall, glm::vec3(wallThickness, wallHeight, gardenDepth));
        glm::vec3 wallColor(0.82f, 0.80f, 0.76f);
        drawCubeColored(wall, wallColor);

        // 常春藤条带
        for (int i = 0; i < 6; ++i) {
            glm::mat4 ivy(1.0f);
            float y = groundY + 0.6f + i * 0.6f;
            float z = wallCenterZ + (i - 3) * 2.3f;
            ivy = glm::translate(ivy, glm::vec3(wallX - 0.55f, y, z));
            ivy = glm::scale(ivy, glm::vec3(0.12f, 1.2f, 1.4f));
            drawCubeColored(ivy, ivyColor);
        }
    }

    // ---------- 9. 左侧：玫瑰花架 + 吊篮 ----------
    {
        float archX = -gardenWidth / 2.0f - 2.5f;
        float archZ = gardenStartZ - 8.0f;
        float archHeight = 3.0f;

        glm::mat4 pillarL(1.0f);
        pillarL = glm::translate(pillarL, glm::vec3(archX - 1.0f, groundY + archHeight / 2.0f, archZ));
        pillarL = glm::scale(pillarL, glm::vec3(0.25f, archHeight, 0.4f));
        drawCubeColored(pillarL, archColor);

        glm::mat4 pillarR(1.0f);
        pillarR = glm::translate(pillarR, glm::vec3(archX + 1.0f, groundY + archHeight / 2.0f, archZ));
        pillarR = glm::scale(pillarR, glm::vec3(0.25f, archHeight, 0.4f));
        drawCubeColored(pillarR, archColor);

        glm::mat4 topBeam(1.0f);
        topBeam = glm::translate(topBeam, glm::vec3(archX, groundY + archHeight + 0.15f, archZ));
        topBeam = glm::scale(topBeam, glm::vec3(2.4f, 0.3f, 0.4f));
        drawCubeColored(topBeam, archColor * 0.9f);

        for (int i = -1; i <= 1; ++i) {
            glm::mat4 basket(1.0f);
            basket = glm::translate(basket,
                glm::vec3(archX + i * 0.7f, groundY + archHeight - 0.5f, archZ - 0.25f));
            basket = glm::scale(basket, glm::vec3(0.4f, 0.5f, 0.4f));
            drawCubeColored(basket, roseColor);
        }
    }

    glUseProgram(0);
}
