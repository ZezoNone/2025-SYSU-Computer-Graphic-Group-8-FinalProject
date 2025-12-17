#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos; // 传递世界坐标到片段着色器

void main() 
{
    for(int face = 0; face < 6; ++face) 
    {
        gl_Layer = face; // 指定渲染到立方体贴图的第 face 个面
        for(int i = 0; i < 3; ++i) 
        { // 遍历三角形的 3 个顶点
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}