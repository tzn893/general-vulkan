#version 450


layout(location = 0) in vec2  uv;
layout(location = 1) in vec3  i_color;
layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D my_texture;

void main()
{
    o_color =  texture(my_texture,uv);
    o_color.xyz = vec3(o_color.x * i_color.x,o_color.y * i_color.y,i_color.z * i_color.z);
}