#pragma once

#include <vector>
#include <string>

using uint = uint32_t;



#define DEFINE_VECTOR(n_dim) struct vec##n_dim {float a[n_dim];};
#define DEFINE_IVEC(n_dim) struct ivec##n_dim {int32_t a[n_dim];};
#define DEFINE_UVEC(n_dim) struct uvec##n_dim {uint32_t a[n_dim];};

DEFINE_VECTOR(4)
DEFINE_VECTOR(3)
DEFINE_VECTOR(2)

DEFINE_IVEC(4)
DEFINE_IVEC(3)
DEFINE_IVEC(2)

DEFINE_UVEC(4)
DEFINE_UVEC(3)
DEFINE_UVEC(2)

struct mat4 
{
	float a00, a01, a02, a03,
		a10, a11, a12, a13,
		a20, a21, a22, a23,
		a30, a31, a32, a33;
};

struct mat4x3
{
	float a00, a01, a02, a03,
		  a10, a11, a12, a13,
		  a20, a21, a22, a23;
};

struct mat4x2
{
	float a00, a01, a02, a03,
		  a10, a11, a12, a13;
};
