#version 450
#include "shader.h"

layout(location = 0) out vec2 o_uv;

void main()
{
    o_uv = uv;
    vec3 pos = rotation * vec3(pos,1.f);
    gl_Position = vec4(pos,1.0f);
}