#ifndef COMMON_GLSL
#define COMMON_GLSL

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct Vertex
{
	vec3 pos;
	vec3 color;
};


struct ObjDesc
{
    uint64_t vertexBufferAddr;
    uint64_t indiceBufferAddr;
};

// clang-format off
struct hitPayload
{
    vec3 color;
};

#endif