#version 450


layout(location = 0) in vec2  uv;
layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D my_texture;

void main()
{
    o_color =  texture(my_texture,uv);
}