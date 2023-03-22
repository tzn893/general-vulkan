#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D i_image;

layout(push_constant) uniform PushConstant
{
    vec2  p_resolution;
    float p_offset;
};

void main()
{
    vec2 inv_res = 1 / p_resolution;

    vec2 ruv = uv; 
    vec2 guv = ruv + vec2(inv_res.x * (1 + p_offset), 0);
    vec2 buv = ruv + vec2(0 , inv_res.y * (1 + p_offset));

    vec3 col;
    col.r = texture(i_image, ruv).r;
    col.g = texture(i_image, guv).g;
    col.b = texture(i_image, buv).b;

   o_color = vec4(col, 1.0f);
}