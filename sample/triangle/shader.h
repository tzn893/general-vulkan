#include "shader/shader_common.h"

#ifdef __cplusplus
#pragma once
#endif

VERTEX_INPUT(TriangleVertex)

VERTEX_ATTRIBUTE(0, vec2, pos)
VERTEX_ATTRIBUTE(1, vec3, color)

VERTEX_INPUT_END(TriangleVertex)


PUSH_CONSTANT(TrianglePushConstant)

mat3  rotation;
float time;

PUSH_CONSTANT_END(TrianglePushConstant)