#version 450
#include "shader.h"

layout(location = 0) out vec2 o_uv;
layout(location = 1) out vec3 o_color;

void main()
{
    o_uv = uv;
    gl_Position = p_proj * p_model * vec4(pos,1.f);
    o_color = color;
}