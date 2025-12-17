#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
out mat3 TBN;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normalMatrix;
uniform vec3 camPos;
uniform float texScale; // 纹理缩放因子

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    
    // 应用 UV 缩放
    TexCoords = aTexCoords * texScale;

    // 计算 TBN 矩阵
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(Normal);
    T = normalize(T - dot(T, N) * N); // 正交化
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);
    
    // 使用 TBN 的转置矩阵将 世界空间 -> 切线空间
    mat3 TBN_transposed = transpose(TBN);
    TangentViewPos = TBN_transposed * camPos;
    TangentFragPos = TBN_transposed * WorldPos;

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}