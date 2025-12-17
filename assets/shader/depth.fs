#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

uniform bool isGlass;

void main() 
{
     // 若为玻璃，强制设置深度为far_plane（超出光源范围，无阴影）
    if (isGlass) 
    {
        gl_FragDepth = far_plane; // 玻璃深度设为最远，不会被阴影采样命中
        return;
    }
    // 计算片段到光源的线性距离（PBR 阴影需要线性深度）
    float lightDistance = length(FragPos.xyz - lightPos);
    // 归一化到 [0,1] 范围（深度缓冲默认范围）
    lightDistance = lightDistance / far_plane;
    // 写入深度值
    gl_FragDepth = lightDistance;
}