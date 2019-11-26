#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoor;

void main(void)
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoor = aTexCoor;
}
