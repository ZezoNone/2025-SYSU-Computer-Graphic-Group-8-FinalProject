#ifndef GEOMETRY_GENERATOR_H
#define GEOMETRY_GENERATOR_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class GeometryGenerator {
public:
    // 构造函数和析构函数
    GeometryGenerator();
    ~GeometryGenerator();

    // 初始化函数：设置VAO和VBO
    void initCubeGeometry();
    void initQuadGeometry();         
    void renderCube();
    void renderQuad();                

    // 获取立方体VAO和VBO
    unsigned int getCubeVAO() const { return cubeVAO; }
    unsigned int getCubeVBO() const { return cubeVBO; }

    // 基础绘制函数
    void drawCube(unsigned int shaderProgram);

    void drawColoredCube(unsigned int shaderProgram,
        glm::mat4 model, glm::vec3 color,
        int modelLoc, int colorLoc);

    // 复杂物体绘制函数
    void drawBookshelf(unsigned int shaderProgram,
        glm::vec3 position, glm::vec3 color,
        int modelLoc, int colorLoc);

    void drawBooksOnShelf(unsigned int shaderProgram,
        glm::vec3 position, int shelfLevel, int numBooks,
        int modelLoc, int colorLoc, bool isRightSide);

    void drawPainting(unsigned int shaderProgram,
        glm::vec3 position, glm::vec3 frameColor,
        glm::vec3 paintingColor,
        float width, float height, float rotationAngle,
        int modelLoc, int colorLoc);

    void drawWallLamp(unsigned int shaderProgram,
        glm::vec3 position, glm::vec3 lampColor,
        float rotationAngle,
        int modelLoc, int colorLoc);

    void drawTableChairLampSet(unsigned int shaderProgram,
        glm::vec3 position, glm::vec3 tableColor,
        glm::vec3 chairColor, glm::vec3 lampColor,
        int modelLoc, int colorLoc);

private:
    unsigned int cubeVAO, cubeVBO;
    unsigned int quadVAO, quadVBO;   

    // 立方体顶点数据
    static const float cubeVertices[108];

    // 平面顶点数据
    static const float quadVertices[18];  

    // 内部函数
    void setupCubeBuffers();
    void setupQuadBuffers();          
};

#endif 
