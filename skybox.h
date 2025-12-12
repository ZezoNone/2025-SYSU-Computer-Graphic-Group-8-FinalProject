#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 纯几何天空盒：古典庭院花园
class Skybox {
public:
    Skybox();
    ~Skybox();

    // 花园天空盒
    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    GLuint quadVAO = 0, quadVBO = 0;
    GLuint cubeVAO = 0, cubeVBO = 0;

    GLuint shaderProgram = 0;

    void initGeometry();
    void initShader();
    void renderQuad();
    void renderCube();

    GLuint compileShader(GLenum type, const char* src);
    GLuint linkProgram(GLuint vs, GLuint fs);
};

#endif 
