#include "geometry_generator.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 立方体顶点数据
const float GeometryGenerator::cubeVertices[] = {
    // 位置
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

const float GeometryGenerator::quadVertices[] = {
    // 位置         (x,    y,    z)
    -0.5f, -0.5f,  0.0f,
     0.5f, -0.5f,  0.0f,
     0.5f,  0.5f,  0.0f,

    -0.5f, -0.5f,  0.0f,
     0.5f,  0.5f,  0.0f,
    -0.5f,  0.5f,  0.0f
};

GeometryGenerator::GeometryGenerator()
    : cubeVAO(0), cubeVBO(0),
    quadVAO(0), quadVBO(0)  
{
}

GeometryGenerator::~GeometryGenerator() {
    // 清理资源
    if (cubeVAO != 0) glDeleteVertexArrays(1, &cubeVAO);
    if (cubeVBO != 0) glDeleteBuffers(1, &cubeVBO);

    if (quadVAO != 0) glDeleteVertexArrays(1, &quadVAO);   
    if (quadVBO != 0) glDeleteBuffers(1, &quadVBO);        
}

void GeometryGenerator::initCubeGeometry() {
    setupCubeBuffers();
}

void GeometryGenerator::initQuadGeometry() {
    setupQuadBuffers();
}

void GeometryGenerator::setupCubeBuffers() {
    // 生成并绑定VAO和VBO
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void GeometryGenerator::setupQuadBuffers() {
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void GeometryGenerator::renderCube() {
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void GeometryGenerator::renderQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void GeometryGenerator::drawCube(unsigned int shaderProgram) {
    renderCube();
}

void GeometryGenerator::drawColoredCube(unsigned int shaderProgram,
    glm::mat4 model, glm::vec3 color,
    int modelLoc, int colorLoc) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(colorLoc, color.x, color.y, color.z);
    renderCube();
}

void GeometryGenerator::drawBookshelf(unsigned int shaderProgram,
    glm::vec3 position, glm::vec3 color,
    int modelLoc, int colorLoc) {
    glm::mat4 model;

    // 书架基本尺寸
    float width = 2.0f;   // X方向
    float height = 3.0f;  // Y方向
    float depth = 0.5f;   // Z方向

    // 左侧板
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(-width / 2, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.05f, height, 0.05f));
    drawColoredCube(shaderProgram, model, color, modelLoc, colorLoc);

    // 右侧板
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(width / 2, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.05f, height, 0.05f));
    drawColoredCube(shaderProgram, model, color, modelLoc, colorLoc);

    // 底板
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, -height / 2, 0.0f));
    model = glm::scale(model, glm::vec3(width, 0.05f, depth));
    drawColoredCube(shaderProgram, model, color, modelLoc, colorLoc);

    // 顶板
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, height / 2, 0.0f));
    model = glm::scale(model, glm::vec3(width, 0.05f, depth));
    drawColoredCube(shaderProgram, model, color, modelLoc, colorLoc);

    // 背板
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.0f, -depth / 2));
    model = glm::scale(model, glm::vec3(width, height, 0.05f));
    drawColoredCube(shaderProgram, model, color, modelLoc, colorLoc);

    // 三层水平隔板
    float shelfSpacing = height / 4.0f;

    for (int i = 1; i <= 3; i++) {
        float yPos = -height / 2 + i * shelfSpacing;

        model = glm::mat4(1.0f);
        model = glm::translate(model, position + glm::vec3(0.0f, yPos, 0.0f));
        model = glm::scale(model, glm::vec3(width - 0.1f, 0.05f, depth - 0.1f));
        drawColoredCube(shaderProgram, model, color, modelLoc, colorLoc);
    }
}

void GeometryGenerator::drawBooksOnShelf(unsigned int shaderProgram,
    glm::vec3 position, int shelfLevel,
    int numBooks, int modelLoc, int colorLoc,
    bool isRightSide) {
    glm::mat4 model;

    // 书架内部尺寸
    float innerWidth = 1.8f;
    float shelfHeight = 0.05f;
    float shelfPositions[3] = { -0.75f, 0.0f, 0.75f };

    // 书本在X方向的位置
    float bookSpacing = innerWidth / (numBooks + 1);

    // 当前隔板的Y位置
    float shelfY = shelfPositions[shelfLevel];
    float shelfTopY = shelfY + shelfHeight / 2;

    for (int i = 0; i < numBooks; i++) {
        // 随机书本颜色
        glm::vec3 bookColor;
        int colorIdx = (shelfLevel * 7 + i) % 10;
        switch (colorIdx) {
        case 0: bookColor = glm::vec3(0.7f, 0.2f, 0.2f); break;
        case 1: bookColor = glm::vec3(0.2f, 0.2f, 0.6f); break;
        case 2: bookColor = glm::vec3(0.2f, 0.5f, 0.2f); break;
        case 3: bookColor = glm::vec3(0.5f, 0.3f, 0.1f); break;
        case 4: bookColor = glm::vec3(0.3f, 0.2f, 0.1f); break;
        case 5: bookColor = glm::vec3(0.8f, 0.6f, 0.2f); break;
        case 6: bookColor = glm::vec3(0.9f, 0.4f, 0.3f); break;
        case 7: bookColor = glm::vec3(0.4f, 0.3f, 0.8f); break;
        case 8: bookColor = glm::vec3(0.2f, 0.7f, 0.7f); break;
        case 9: bookColor = glm::vec3(0.8f, 0.2f, 0.8f); break;
        }

        // 书本在X方向的位置
        float bookX = -innerWidth / 2 + bookSpacing * (i + 1);

        // 书本尺寸
        float bookThickness = 0.08f + 0.03f * ((shelfLevel + i) % 3);
        float bookHeight = 0.2f + 0.05f * ((shelfLevel * i) % 3);
        float bookWidth = 0.15f + 0.03f * (i % 5);

        // 书本位置：书本底部接触隔板
        float bookY = shelfTopY + bookHeight / 2;
        float bookZ = -0.15f;

        model = glm::mat4(1.0f);
        model = glm::translate(model, position + glm::vec3(bookX, bookY, bookZ));

        // 调整书脊方向
        if (isRightSide) {
            // 右侧书架：旋转书本，使书脊朝左
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(bookWidth, bookHeight, bookThickness));
        }
        else {
            // 左侧书架：旋转书本，使书脊朝右
            model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(bookWidth, bookHeight, bookThickness));
        }

        drawColoredCube(shaderProgram, model, bookColor, modelLoc, colorLoc);
    }
}

void GeometryGenerator::drawPainting(unsigned int shaderProgram,
    glm::vec3 position, glm::vec3 frameColor,
    glm::vec3 paintingColor,
    float width, float height, float rotationAngle,
    int modelLoc, int colorLoc) {
    glm::mat4 model;

    // 画框
    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.025f));
    model = glm::scale(model, glm::vec3(width, height, 0.05f));
    drawColoredCube(shaderProgram, model, frameColor, modelLoc, colorLoc);

    // 画布
    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.045f));
    model = glm::scale(model, glm::vec3(width - 0.1f, height - 0.1f, 0.01f));
    drawColoredCube(shaderProgram, model, paintingColor, modelLoc, colorLoc);
}

void GeometryGenerator::drawWallLamp(unsigned int shaderProgram,
    glm::vec3 position, glm::vec3 lampColor,
    float rotationAngle,
    int modelLoc, int colorLoc) {
    glm::mat4 model;

    // 壁灯底座
    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.05f));
    model = glm::scale(model, glm::vec3(0.15f, 0.08f, 0.1f));
    drawColoredCube(shaderProgram, model, glm::vec3(0.7f, 0.5f, 0.3f), modelLoc, colorLoc);

    // 灯罩
    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.15f));
    model = glm::scale(model, glm::vec3(0.2f, 0.15f, 0.15f));
    drawColoredCube(shaderProgram, model, glm::vec3(1.0f, 0.95f, 0.8f), modelLoc, colorLoc);

    // 灯泡部分
    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.22f));
    model = glm::scale(model, glm::vec3(0.1f, 0.08f, 0.1f));
    drawColoredCube(shaderProgram, model, glm::vec3(1.0f, 1.0f, 0.9f), modelLoc, colorLoc);
}

void GeometryGenerator::drawTableChairLampSet(unsigned int shaderProgram,
    glm::vec3 position, glm::vec3 tableColor,
    glm::vec3 chairColor, glm::vec3 lampColor,
    int modelLoc, int colorLoc) {
    glm::mat4 model;

    // 桌子
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.3f, 0.0f));
    model = glm::scale(model, glm::vec3(0.9f, 0.1f, 2.5f));
    drawColoredCube(shaderProgram, model, tableColor, modelLoc, colorLoc);

    // 桌子腿
    for (int i = 0; i < 4; i++) {
        float x = (i % 2 == 0) ? -0.4f : 0.4f;
        float z = (i < 2) ? -1.1f : 1.1f;

        model = glm::mat4(1.0f);
        model = glm::translate(model, position + glm::vec3(x, 0.1f, z));
        model = glm::scale(model, glm::vec3(0.08f, 0.2f, 0.08f));
        drawColoredCube(shaderProgram, model, tableColor * 0.8f, modelLoc, colorLoc);
    }

    // 台灯
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.35f, 0.0f));
    model = glm::scale(model, glm::vec3(0.1f, 0.15f, 0.1f));
    drawColoredCube(shaderProgram, model, lampColor, modelLoc, colorLoc);

    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.55f, 0.0f));
    model = glm::scale(model, glm::vec3(0.05f, 0.25f, 0.05f));
    drawColoredCube(shaderProgram, model, lampColor, modelLoc, colorLoc);

    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.8f, 0.0f));
    model = glm::scale(model, glm::vec3(0.25f, 0.1f, 0.25f));
    drawColoredCube(shaderProgram, model, lampColor * 1.1f, modelLoc, colorLoc);

    // 第一个椅子（桌子长边左侧，-Z方向，调整高度和靠背位置）
    // 坐凳
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.2f, -1.8f));
    model = glm::scale(model, glm::vec3(0.6f, 0.06f, 0.6f));
    drawColoredCube(shaderProgram, model, chairColor, modelLoc, colorLoc);

    // 靠背
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.45f, -1.8f - 0.33f));
    model = glm::scale(model, glm::vec3(0.6f, 0.5f, 0.05f));
    drawColoredCube(shaderProgram, model, chairColor, modelLoc, colorLoc);

    // 第二个椅子（桌子长边右侧，+Z方向，调整高度和靠背位置）
    // 坐凳
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.2f, 1.8f));
    model = glm::scale(model, glm::vec3(0.6f, 0.06f, 0.6f));
    drawColoredCube(shaderProgram, model, chairColor, modelLoc, colorLoc);

    // 靠背
    model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0.0f, 0.45f, 1.8f + 0.33f));
    model = glm::scale(model, glm::vec3(0.6f, 0.5f, 0.05f));
    drawColoredCube(shaderProgram, model, chairColor, modelLoc, colorLoc);

    // 椅子腿
    for (int chair = 0; chair < 2; chair++) {
        float chairZ = (chair == 0) ? -1.8f : 1.8f;

        for (int i = 0; i < 4; i++) {
            float x = (i % 2 == 0) ? -0.25f : 0.25f;
            float z = chairZ + ((i < 2) ? -0.3f : 0.3f);

            model = glm::mat4(1.0f);
            model = glm::translate(model, position + glm::vec3(x, 0.05f, z));
            model = glm::scale(model, glm::vec3(0.05f, 0.2f, 0.05f));
            drawColoredCube(shaderProgram, model, chairColor * 0.8f, modelLoc, colorLoc);
        }
    }
}