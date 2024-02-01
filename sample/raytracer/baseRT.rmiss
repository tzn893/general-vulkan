#version 460
#extension GL_EXT_ray_tracing : require
#include "common.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

// do nothing
void main()
{
    //prd.color = vec3(1, 0, 0);
    prd.color = vec3(0.3, 0.3, 0.3);
}