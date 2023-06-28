#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 o_uv;
layout(location = 1) out vec3 o_color;

layout(push_constant) uniform TrianglePushConstant
{
    mat4 p_proj;
    mat4 p_model;
};

void main()
{
    o_uv = uv;
    gl_Position = p_proj * p_model * vec4(pos,1.f);
    o_color = color;
}