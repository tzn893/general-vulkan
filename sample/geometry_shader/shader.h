#include "shader/shader_common.h"

#ifdef __cplusplus
#pragma once
#endif

VERTEX_INPUT(GPoint)
VERTEX_ATTRIBUTE(0, vec3, pos)
VERTEX_ATTRIBUTE(1, vec3, color)
VERTEX_INPUT_END(GPoint)


PUSH_CONSTANT(Rotation)
	mat3 rotation;
PUSH_CONSTANT_END(Rotation)