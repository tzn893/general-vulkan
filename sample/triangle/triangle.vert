#version 450
#include "shader.h"

layout(location = 0) out vec3 o_color;

void main()
{
    o_color = vec3(color * (sin(time) * .5 + .5));
    vec3 pos = rotation * vec3(pos,1.f);
    gl_Position = vec4(pos,1.0f);
}