#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <camera.h>
#include <model.h>

#include <iostream>
#include <functional>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void renderSphere();
void renderGround();
void initPointLights();
void renderAllObjectsToDepth(Shader& depthShader);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 1024;  // 立方体贴图分辨率（平衡性能）
const unsigned int SHADOW_HEIGHT = 1024;
const unsigned int MAX_POINT_LIGHTS = 4;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;

// light
struct PointLight {
    glm::vec3 position;    // 光源位置
    glm::vec3 color;       // 光源颜色/强度（PBR 高动态范围）
    float far_plane;       // 光照范围
    unsigned int depthCubemap; // 立方体贴图深度纹理
    unsigned int depthFBO;     // 深度 FBO
    glm::mat4 shadowMatrices[6]; // 6 方向的投影+视图矩阵
};
std::vector<PointLight> pointLights;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Model
vector<Model> Models;


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // lights
    pointLights.push_back({ glm::vec3(0.0f, 0.0f, 10.0f),glm::vec3(150.0f, 150.0f, 150.0f),50.0f });
    pointLights.push_back({ glm::vec3(10.0f, 1.0f, 10.0f),glm::vec3(300.0f, 300.0f, 300.0f),50.0f });
    initPointLights();

    // build and compile shaders
    // -------------------------
    Shader shader("pbr.vs", "pbr.fs");
    Shader shadowshader("depth.vs", "depth.fs", "depth.gs");

    shader.use();
    shader.setInt("albedoMap", 0);
    shader.setInt("normalMap", 1);
    shader.setInt("metallicMap", 2);
    shader.setInt("roughnessMap", 3);
    shader.setInt("aoMap", 4);
    shader.setInt("metallic_roughnessMap", 2);

    //载入模型
    Model book_shelf("model/book-shelf_obj/model.obj");
    book_shelf.SetScale(glm::vec3(0.05f));
    book_shelf.SetPosition(glm::vec3(0.0f, -9.0f, 10.0f));
    Models.push_back(book_shelf);

    Model book1("model/book1/scene.gltf", false, true);
    book1.SetScale(glm::vec3(0.1f));
    book1.SetPosition(glm::vec3(2.5f, -0.75f, 5.0f));
    Models.push_back(book1);

    Model book2("model/book2/scene.gltf", false, true);
    book2.SetScale(glm::vec3(0.1f));
    book2.SetPosition(glm::vec3(7.0f, -0.98f, 6.0f));
    book2.Rotate(glm::vec3(-90.0f, 0.0f, -90.0f));
    Models.push_back(book2);

    Model window1("model/glass_window/scene.gltf", false, true);
    window1.SetScale(glm::vec3(1.0f));
    window1.SetPosition(glm::vec3(10.0f, 0.0f, 7.0f));
    window1.Rotate(glm::vec3(90.0f, 0.0f,0.0f));
    Models.push_back(window1);

    Model light1("model/ceiling_light/scene.gltf", false, true);
    light1.SetScale(glm::vec3(0.5f));
    light1.SetPosition(glm::vec3(10.0f, 1.1f, 10.0f));
    light1.Rotate(glm::vec3(0.0f, 0.0f, 0.0f));
    Models.push_back(light1);
    // load PBR material textures
    // --------------------------
    unsigned int albedo = loadTexture("texture/rustediron/basecolor.png");
    unsigned int normal = loadTexture("texture/rustediron/normal.png");
    unsigned int metallic = loadTexture("texture/rustediron/metallic.png");
    unsigned int roughness = loadTexture("texture/rustediron/roughness.png");
    unsigned int ao = loadTexture("texture/rustediron/ao.jpg");

    unsigned int groundAlbedo = loadTexture("texture/floor/basecolor.png");       // 地面基础色
    unsigned int groundNormal = loadTexture("texture/floor/normal.png");       // 地面法线
    unsigned int groundMetallic = loadTexture("texture/floor/height.png");   // 地面金属度
    unsigned int groundRoughness = loadTexture("texture/floor/roughness.png"); // 地面粗糙度
    unsigned int groundAO = loadTexture("texture/floor/ao.png");               // 地面AO


    // initialize static shader uniforms before rendering
    // --------------------------------------------------
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    shader.use();
    shader.setMat4("projection", projection);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // light
        int validLightCount = min((int)pointLights.size(),4);
        shader.setInt("pointLightCount", validLightCount);

        for (int i = 0; i < validLightCount; ++i) 
        {
            // 1. 绑定立方体贴图采样器
            std::string samplerName = "pointLightDepthCubemaps[" + std::to_string(i) + "]";
            shader.setInt(samplerName.c_str(), 5 + i);

            // 2. 绑定光源基础参数
            std::string posName = "pointLightsBase[" + std::to_string(i) + "].position";
            std::string colorName = "pointLightsBase[" + std::to_string(i) + "].color";
            std::string farName = "pointLightsBase[" + std::to_string(i) + "].far_plane";

            shader.setVec3(posName.c_str(), pointLights[i].position);
            shader.setVec3(colorName.c_str(), pointLights[i].color);
            shader.setFloat(farName.c_str(), pointLights[i].far_plane);

            // 3. 绑定立方体贴图纹理
            glActiveTexture(GL_TEXTURE5 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, pointLights[i].depthCubemap);
        }

        //shadow
        renderAllObjectsToDepth(shadowshader);

        shader.use();
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);
        shader.setVec3("camPos", camera.Position);

        // 绑定地面PBR纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundAlbedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, groundNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, groundMetallic);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, groundRoughness);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, groundAO);

        // 设置地面模型矩阵（缩放+位置）
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -9.0f, 0.0f)); // 地面放在球体下方
        model = glm::scale(model, glm::vec3(20.0f, 1.0f, 20.0f));    // 地面缩放为40x40大小
        shader.setMat4("model", model);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        renderGround(); // 渲染地面

        // 绑定球体PBR纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, metallic);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, roughness);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ao);

        //渲染光源点小球
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 10.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        shader.setMat4("model", model);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        renderSphere();

        //加载载入模型的渲染
        book_shelf.Draw(shader);
        book1.Draw(shader);
        book2.Draw(shader);
        light1.Draw(shader);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Alpha混合
        glDepthMask(GL_FALSE); // 半透明物体关闭深度写入
        glDisable(GL_CULL_FACE); // 双面渲染

        window1.Draw(shader);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto& light : pointLights) {
        glDeleteTextures(1, &light.depthCubemap);
        glDeleteFramebuffers(1, &light.depthFBO);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
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
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
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

// -------------------------------------------------
// 新增：地面渲染函数（和球体保持相同的顶点属性布局）
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
        std::vector<unsigned int> indices;

        // 地面分段数（越大越精细）
        const unsigned int SEGMENTS = 64;

        // 生成地面顶点（单位平面，后续通过模型矩阵缩放）
        for (unsigned int z = 0; z <= SEGMENTS; ++z)
        {
            for (unsigned int x = 0; x <= SEGMENTS; ++x)
            {
                // 生成-0.5到0.5的单位平面
                float xPos = (float)x / (float)SEGMENTS - 0.5f;
                float zPos = (float)z / (float)SEGMENTS - 0.5f;

                positions.push_back(glm::vec3(xPos, 0.0f, zPos));
                uv.push_back(glm::vec2((float)x / 8.0f, (float)z / 8.0f)); // UV缩放，让纹理重复8次
                normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // 法线向上
            }
        }

        // 生成三角带索引（和球体相同的奇偶行处理）
        bool oddRow = false;
        for (unsigned int z = 0; z < SEGMENTS; ++z)
        {
            if (!oddRow)
            {
                for (unsigned int x = 0; x <= SEGMENTS; ++x)
                {
                    indices.push_back(z * (SEGMENTS + 1) + x);
                    indices.push_back((z + 1) * (SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((z + 1) * (SEGMENTS + 1) + x);
                    indices.push_back(z * (SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        groundIndexCount = static_cast<unsigned int>(indices.size());

        // 合并顶点数据（位置+法线+UV，和球体格式一致）
        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            // 位置 (3 floats)
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);

            // 法线 (3 floats)
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);

            // UV (2 floats)
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }

        // 绑定VAO和缓冲区
        glBindVertexArray(groundVAO);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 顶点属性配置（和球体完全一致）
        unsigned int stride = (3 + 3 + 2) * sizeof(float);

        // 位置属性 (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        // 法线属性 (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        // UV属性 (location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    // 绘制地面
    glBindVertexArray(groundVAO);
    glDrawElements(GL_TRIANGLE_STRIP, groundIndexCount, GL_UNSIGNED_INT, 0);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
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

void initPointLights()
{
    for (int i = 0; i < pointLights.size(); ++i) 
    {
        glActiveTexture(GL_TEXTURE5 + i);
    }
    for (auto& light : pointLights)
    {
        // 创建立方体深度纹理
        glGenTextures(1, &light.depthCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, light.depthCubemap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            // 修复：使用 GL_DEPTH_COMPONENT32F 提高深度精度，避免兼容性问题
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F,
                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // 创建深度 FBO
        glGenFramebuffers(1, &light.depthFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, light.depthFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light.depthCubemap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        // 关键：添加 FBO 状态检查，若失败直接报错
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "ERROR::FRAMEBUFFER:: Point light FBO incomplete! Error code: "
                << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
            // 修复：FBO 失败时不继续执行，避免后续渲染异常
            exit(-1);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // 解绑 FBO，确保默认 FBO 激活

        // 计算阴影矩阵
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / SHADOW_HEIGHT, 0.1f, light.far_plane);
        light.shadowMatrices[0] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        light.shadowMatrices[1] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        light.shadowMatrices[2] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        light.shadowMatrices[3] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        light.shadowMatrices[4] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        light.shadowMatrices[5] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    }
}

void renderAllObjectsToDepth(Shader& depthShader)
{
    depthShader.use();
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    for (auto& light : pointLights)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, light.depthFBO);
        glClear(GL_DEPTH_BUFFER_BIT); // 清除深度缓冲（每次渲染前清空）

        // 传递阴影矩阵和光源参数
        for (unsigned int i = 0; i < 6; ++i)
        {
            std::string matName = "shadowMatrices[" + std::to_string(i) + "]";
            depthShader.setMat4(matName.c_str(), light.shadowMatrices[i]);
        }
        depthShader.setVec3("lightPos", light.position);
        depthShader.setFloat("far_plane", light.far_plane);
        
        //非加载模型的渲染要在这里生成贴图深度缓冲
        // ========== 1. 渲染地面 ==========
        glm::mat4 modelMat;

        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -9.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(20.0f, 1.0f, 20.0f));
        depthShader.setMat4("model", modelMat);
        renderGround();

         //========== 2. 渲染模型 ==========
        for (auto model : Models)
            model.Draw(depthShader);
    }

    // 恢复默认FBO和视口
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
}

