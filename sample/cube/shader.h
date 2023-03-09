#include "shader/shader_common.h"

#ifdef __cplusplus
#pragma once
#endif

VERTEX_INPUT(TriangleVertex)

VERTEX_ATTRIBUTE(0, vec3, pos)
VERTEX_ATTRIBUTE(1, vec3, color)
VERTEX_ATTRIBUTE(2, vec2, uv)

VERTEX_INPUT_END(TriangleVertex)


PUSH_CONSTANT(TrianglePushConstant)
    mat4 p_proj;
    mat4 p_model;
PUSH_CONSTANT_END(TrianglePushConstant)