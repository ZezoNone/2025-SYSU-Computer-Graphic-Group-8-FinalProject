#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"
#include "geometry_generator.h"
#include "skybox.h"

// 窗口设置
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 摄像机
Camera camera(glm::vec3(0.0f, 2.5f, 8.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 计时
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool showSkybox = true;

// 透明物体结构体
struct TransparentObject {
    glm::vec3 position;   // 世界坐标，用来算和摄像机的距离
    glm::mat4 model;      // 模型矩阵
    glm::vec3 color;      // 颜色
    float alpha;          // 透明度
};

// 着色器源码字符串
const char* vertexShaderSource = R"(
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

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform float alpha = 1.0;

void main()
{
    FragColor = vec4(objectColor, alpha);
}
)";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// 编译着色器的函数
unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

// 创建着色器程序的函数
unsigned int createShaderProgram() {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM_LINKING_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Library Scene with Skybox", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Creating library scene with skybox..." << std::endl;

    // 创建着色器程序
    unsigned int shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        std::cout << "Failed to create shader program!" << std::endl;
        return -1;
    }

    // 获取uniform位置
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    int alphaLoc = glGetUniformLocation(shaderProgram, "alpha");

    // 创建几何生成器并初始化
    GeometryGenerator geometryGen;
    geometryGen.initCubeGeometry();
    geometryGen.initQuadGeometry();  

    // 创建天空盒
    Skybox skybox;

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);

    // 帧率计数器
    int frameCount = 0;
    double lastTime = glfwGetTime();

    std::cout << "\n=== Library Scene Controls ===" << std::endl;
    std::cout << "WASD: Move camera" << std::endl;
    std::cout << "Mouse: Look around" << std::endl;
    std::cout << "ESC: Exit" << std::endl;
    std::cout << "R: Reset camera" << std::endl;
    std::cout << "SPACE: Toggle skybox visibility" << std::endl;
    std::cout << "\nDrawing library scene..." << std::endl;

    // 颜色定义
    glm::vec3 woodColor = glm::vec3(0.55f, 0.4f, 0.25f);
    glm::vec3 darkWood = glm::vec3(0.4f, 0.3f, 0.2f);
    glm::vec3 floorColor = glm::vec3(0.35f, 0.25f, 0.15f);
    glm::vec3 wallColor = glm::vec3(0.88f, 0.84f, 0.78f);
    glm::vec3 windowColor = glm::vec3(0.7f, 0.85f, 1.0f);
    glm::vec3 doorColor = glm::vec3(0.45f, 0.35f, 0.25f);
    glm::vec3 lampColor = glm::vec3(0.95f, 0.95f, 0.75f);
    glm::vec3 chairColor = glm::vec3(0.6f, 0.5f, 0.4f);
    glm::vec3 shelfColor = glm::vec3(0.45f, 0.35f, 0.25f);
    glm::vec3 frameColor = glm::vec3(0.6f, 0.45f, 0.25f);

    // 房间尺寸
    float roomWidth = 14.0f;
    float roomDepth = 10.0f;
    float roomHeight = 4.0f;
    float wallThickness = 0.2f;

    // 窗户尺寸
    float windowWidth = 3.5f;
    float windowHeight = 2.2f;

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 计算帧率
        frameCount++;
        if (currentFrame - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentFrame;
        }

        processInput(window);

        // 清屏
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 相机矩阵
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 300.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // ===画天空盒===
        if (showSkybox) {
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);       
            skybox.Draw(view, projection);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
        }

        // 使用室内场景的着色器
        glUseProgram(shaderProgram);

        // 设置全局 uniform
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // 默认不透明
        glUniform1f(alphaLoc, 1.0f);

        glm::mat4 model;

        // === 渲染所有不透明物体 ===
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // 地板
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(roomWidth, 0.1f, roomDepth));
        geometryGen.drawColoredCube(shaderProgram, model, floorColor, modelLoc, colorLoc);

        // 墙壁
        // ===== 后墙 (-Z方向)，挖出一个窗口洞 =====
        float wallZ = -roomDepth / 2 - wallThickness / 2;

        // 窗户在墙上的位置参数
        float windowYCenter = 1.5f;          
        float windowH = windowHeight;  // 2.2
        float windowYBottom = windowYCenter - windowH * 0.5f;  // 0.4
        float windowYTop = windowYCenter + windowH * 0.5f;  // 2.6

        // 整面墙的上下范围：[-0.5, 3.5]
        float wallYBottom = -0.5f;
        float wallYTop = wallYBottom + roomHeight; // -0.5 + 4 = 3.5

        // 1) 底部墙块（窗户下面）
        {
            float bottomHeight = windowYBottom - wallYBottom;             // 0.9
            float bottomCenterY = (windowYBottom + wallYBottom) * 0.5f;    // -0.05

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, bottomCenterY, wallZ));
            model = glm::scale(model, glm::vec3(roomWidth, bottomHeight, wallThickness));
            geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);
        }

        // 2) 顶部墙块（窗户上面）
        {
            float topHeight = wallYTop - windowYTop;             // 0.9
            float topCenterY = (wallYTop + windowYTop) * 0.5f;    // 3.05

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, topCenterY, wallZ));
            model = glm::scale(model, glm::vec3(roomWidth, topHeight, wallThickness));
            geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);
        }

        // 3) 左侧竖向墙块
        {
            float sideWidth = (roomWidth - windowWidth) * 0.5f; // (14 - 3.5) / 2 = 5.25
            float sideCenterY = roomHeight / 2.0f - 0.5f;         // 1.5
            float leftCenterX = -(windowWidth / 2.0f + sideWidth / 2.0f); // -4.375

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(leftCenterX, sideCenterY, wallZ));
            model = glm::scale(model, glm::vec3(sideWidth, roomHeight, wallThickness));
            geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);
        }

        // 4) 右侧竖向墙块
        {
            float sideWidth = (roomWidth - windowWidth) * 0.5f; // (14 - 3.5) / 2 = 5.25
            float leftCenterX = -(windowWidth / 2.0f + sideWidth / 2.0f); // -4.37
            float sideCenterY = roomHeight / 2.0f - 0.5f;
            float rightCenterX = -leftCenterX;  // 对称

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(rightCenterX, sideCenterY, wallZ));
            model = glm::scale(model, glm::vec3(sideWidth, roomHeight, wallThickness));
            geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);
        }
        // 前墙 (+Z方向，带门洞)
        float doorWidth = 1.8f;
        float doorHeight = 2.5f;
        float wallSegmentWidth = (roomWidth - doorWidth) / 2.0f;

        // 前墙左半部分
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-(roomWidth - doorWidth) / 4.0f - doorWidth / 2.0f,
            roomHeight / 2 - 0.5f,
            roomDepth / 2 + wallThickness / 2));
        model = glm::scale(model, glm::vec3(wallSegmentWidth, roomHeight, wallThickness));
        geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);

        // 前墙右半部分
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3((roomWidth - doorWidth) / 4.0f + doorWidth / 2.0f,
            roomHeight / 2 - 0.5f,
            roomDepth / 2 + wallThickness / 2));
        model = glm::scale(model, glm::vec3(wallSegmentWidth, roomHeight, wallThickness));
        geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);

        // 左墙 (-X方向)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-roomWidth / 2 - wallThickness / 2,
            roomHeight / 2 - 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(wallThickness, roomHeight, roomDepth));
        geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);

        // 右墙 (+X方向)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(roomWidth / 2 + wallThickness / 2,
            roomHeight / 2 - 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(wallThickness, roomHeight, roomDepth));
        geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);

        // 门
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, doorHeight / 2 - 0.5f, roomDepth / 2 + wallThickness / 2));
        model = glm::scale(model, glm::vec3(doorWidth, doorHeight, wallThickness));
        geometryGen.drawColoredCube(shaderProgram, model, doorColor, modelLoc, colorLoc);

        // 门上方墙体
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, (roomHeight + doorHeight) / 2 - 0.5f,
            roomDepth / 2 + wallThickness / 2));
        model = glm::scale(model, glm::vec3(doorWidth, roomHeight - doorHeight, wallThickness));
        geometryGen.drawColoredCube(shaderProgram, model, wallColor, modelLoc, colorLoc);

        // 书架 & 书
        float shelfSpacingZ = 2.5f;
        float shelfPosX = 4.5f;

        glm::vec3 shelfPos1 = glm::vec3(-shelfPosX, 1.0f, -shelfSpacingZ);
        geometryGen.drawBookshelf(shaderProgram, shelfPos1, shelfColor, modelLoc, colorLoc);
        for (int shelf = 0; shelf < 3; shelf++) {
            geometryGen.drawBooksOnShelf(shaderProgram, shelfPos1, shelf, 6, modelLoc, colorLoc, false);
        }

        glm::vec3 shelfPos2 = glm::vec3(-shelfPosX, 1.0f, shelfSpacingZ);
        geometryGen.drawBookshelf(shaderProgram, shelfPos2, shelfColor, modelLoc, colorLoc);
        for (int shelf = 0; shelf < 3; shelf++) {
            geometryGen.drawBooksOnShelf(shaderProgram, shelfPos2, shelf, 5, modelLoc, colorLoc, false);
        }

        glm::vec3 shelfPos3 = glm::vec3(shelfPosX, 1.0f, -shelfSpacingZ);
        geometryGen.drawBookshelf(shaderProgram, shelfPos3, shelfColor, modelLoc, colorLoc);
        for (int shelf = 0; shelf < 3; shelf++) {
            geometryGen.drawBooksOnShelf(shaderProgram, shelfPos3, shelf, 7, modelLoc, colorLoc, true);
        }

        glm::vec3 shelfPos4 = glm::vec3(shelfPosX, 1.0f, shelfSpacingZ);
        geometryGen.drawBookshelf(shaderProgram, shelfPos4, shelfColor, modelLoc, colorLoc);
        for (int shelf = 0; shelf < 3; shelf++) {
            geometryGen.drawBooksOnShelf(shaderProgram, shelfPos4, shelf, 6, modelLoc, colorLoc, true);
        }

        // 桌子组合（3个）
        geometryGen.drawTableChairLampSet(shaderProgram,
            glm::vec3(0.0f, 0.0f, 0.0f),
            woodColor, chairColor, lampColor,
            modelLoc, colorLoc);

        geometryGen.drawTableChairLampSet(shaderProgram,
            glm::vec3(-shelfPosX + 2.0f, 0.0f, 0.0f),
            darkWood, chairColor * 0.9f, lampColor * 0.9f,
            modelLoc, colorLoc);

        geometryGen.drawTableChairLampSet(shaderProgram,
            glm::vec3(shelfPosX - 2.0f, 0.0f, 0.0f),
            woodColor * 0.8f, chairColor * 1.1f, lampColor * 1.1f,
            modelLoc, colorLoc);

        // 天花板
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, roomHeight - 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(roomWidth, 0.1f, roomDepth));
        geometryGen.drawColoredCube(shaderProgram, model, wallColor * 0.8f, modelLoc, colorLoc);

        // 画作装饰
        glm::vec3 paintingColor1 = glm::vec3(0.8f, 0.3f, 0.3f);
        glm::vec3 paintingColor2 = glm::vec3(0.3f, 0.5f, 0.8f);
        glm::vec3 paintingColor3 = glm::vec3(0.3f, 0.8f, 0.4f);
        glm::vec3 paintingColor4 = glm::vec3(0.9f, 0.8f, 0.3f);
        glm::vec3 paintingColor5 = glm::vec3(0.7f, 0.3f, 0.8f);
        glm::vec3 paintingColor6 = glm::vec3(0.8f, 0.5f, 0.2f);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(-roomWidth / 2 + 0.01f, 1.5f, -roomDepth / 4),
            frameColor, paintingColor1, 1.2f, 0.8f, 90.0f, modelLoc, colorLoc);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(-roomWidth / 2 + 0.01f, 1.5f, roomDepth / 4),
            frameColor, paintingColor2, 1.0f, 1.0f, 90.0f, modelLoc, colorLoc);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(roomWidth / 2 - 0.01f, 1.5f, -roomDepth / 4),
            frameColor, paintingColor3, 1.5f, 0.9f, -90.0f, modelLoc, colorLoc);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(roomWidth / 2 - 0.01f, 1.5f, roomDepth / 4),
            frameColor, paintingColor4, 0.8f, 1.2f, -90.0f, modelLoc, colorLoc);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(-roomWidth / 4, 1.5f, -roomDepth / 2 + 0.01f),
            frameColor, paintingColor5, 0.9f, 0.7f, 0.0f, modelLoc, colorLoc);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(roomWidth / 4, 1.5f, -roomDepth / 2 + 0.01f),
            frameColor, paintingColor6, 1.1f, 0.8f, 0.0f, modelLoc, colorLoc);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(-doorWidth / 2 - 2.5f, 1.5f, roomDepth / 2 - 0.01f),
            frameColor, paintingColor1, 0.9f, 0.7f, 180.0f, modelLoc, colorLoc);

        geometryGen.drawPainting(shaderProgram,
            glm::vec3(doorWidth / 2 + 2.5f, 1.5f, roomDepth / 2 - 0.01f),
            frameColor, paintingColor2, 0.9f, 0.7f, 180.0f, modelLoc, colorLoc);

        // 壁灯装饰
        glm::vec3 wallLampColor = glm::vec3(1.0f, 0.95f, 0.7f);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(-roomWidth / 2 + 0.01f, 2.5f, -roomDepth / 3),
            wallLampColor, 90.0f, modelLoc, colorLoc);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(-roomWidth / 2 + 0.01f, 2.5f, roomDepth / 3),
            wallLampColor, 90.0f, modelLoc, colorLoc);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(roomWidth / 2 - 0.01f, 2.5f, -roomDepth / 3),
            wallLampColor, -90.0f, modelLoc, colorLoc);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(roomWidth / 2 - 0.01f, 2.5f, roomDepth / 3),
            wallLampColor, -90.0f, modelLoc, colorLoc);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(-roomWidth / 3, 2.5f, -roomDepth / 2 + 0.01f),
            wallLampColor, 0.0f, modelLoc, colorLoc);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(roomWidth / 3, 2.5f, -roomDepth / 2 + 0.01f),
            wallLampColor, 0.0f, modelLoc, colorLoc);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(-doorWidth / 2 - 2.0f, 2.5f, roomDepth / 2 - 0.01f),
            wallLampColor, 180.0f, modelLoc, colorLoc);

        geometryGen.drawWallLamp(shaderProgram,
            glm::vec3(doorWidth / 2 + 2.0f, 2.5f, roomDepth / 2 - 0.01f),
            wallLampColor, 180.0f, modelLoc, colorLoc);

        // === 构建透明物体列表===
        std::vector<TransparentObject> transparentObjects;

        

        // 窗户玻璃中心位置
        glm::vec3 windowCenterPos = glm::vec3(
            0.0f,
            1.5f,
            -roomDepth / 2 + 0.01f   
        );

        TransparentObject windowGlass;
        windowGlass.position = windowCenterPos;
        windowGlass.model = glm::mat4(1.0f);
        windowGlass.model = glm::translate(windowGlass.model, windowCenterPos);
     
        windowGlass.model = glm::scale(windowGlass.model, glm::vec3(windowWidth, windowHeight, 1.0f));
        windowGlass.color = windowColor;
        windowGlass.alpha = 0.4f; 
        transparentObjects.push_back(windowGlass);

        

        // 按摄像机距离从远到近排序
        std::sort(transparentObjects.begin(), transparentObjects.end(),
            [](const TransparentObject& a, const TransparentObject& b) {
                float da = glm::length(camera.Position - a.position);
                float db = glm::length(camera.Position - b.position);
                return da > db; 
            });

        // === 绘制透明物体 ===
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); 

        for (const auto& obj : transparentObjects) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(obj.model));
            glUniform3f(colorLoc, obj.color.r, obj.color.g, obj.color.b);
            glUniform1f(alphaLoc, obj.alpha);
            geometryGen.renderQuad();  
        }

        // 恢复状态
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glUniform1f(alphaLoc, 1.0f);

        // 交换缓冲区和轮询事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // 重置摄像机位置
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camera = Camera(glm::vec3(0.0f, 2.5f, 8.0f));
        std::cout << "Camera reset to initial position" << std::endl;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll((float)yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        showSkybox = !showSkybox;
        std::cout << "Skybox visibility: " << (showSkybox ? "ON" : "OFF") << std::endl;
    }
}
