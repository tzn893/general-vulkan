#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

layout(push_constant) uniform TrianglePushConstant {
    mat4  rotation;
    float time;
};

layout(location = 0) out vec2 o_uv;

void main()
{
    o_uv = uv;
    vec4 pos = rotation * vec4(pos, 0.5f, 1.f);
    gl_Position = pos;

}