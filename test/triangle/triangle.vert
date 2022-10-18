#version 450
#include "shader.h"

layout(location = 0) out vec3 o_color;

void main()
{
    o_color = vec3(color);
    gl_Position = vec4(pos,0.f,1.0f);
}