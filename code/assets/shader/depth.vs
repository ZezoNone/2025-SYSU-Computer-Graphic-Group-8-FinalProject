#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 4) in mat4 instanceMatrix;

uniform bool useInstance;
uniform mat4 model;

void main() 
{
    if(useInstance)
    {
        gl_Position = instanceMatrix * vec4(aPos, 1.0);
    }
    else
    {
        gl_Position = model * vec4(aPos, 1.0);
    }
}