//for push constant reflection 

//for C++ project
#ifdef __cplusplus

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

//filling padding field is meaningless
//it is handy when pushing it to GPU
struct mat3
{
	float a00, a01, a02, __padding03,
		  a10, a11, a12, __padding13,
		  a20, a21, a22, __padding23;
};

struct mat3x2
{
	float a00, a01, a02, __padding03,
		  a10, a11, a12, __padding13;
};


#define PUSH_CONSTANT(PC) struct PC {

#define PUSH_CONSTANT_END(PC) };

#define VERTEX_INPUT(VT) struct VT{

//TODO : support multiple vertex input binding point 
#define VERTEX_ATTRIBUTE_BINDING(loca,binding,type,name) type name; 

#define VERTEX_INPUT_END(VT) };

#else

#define PUSH_CONSTANT(PC) layout(push_constant) uniform PC {

#define PUSH_CONSTANT_END(PC) };

#define VERTEX_INPUT(VT) 

#define VERTEX_ATTRIBUTE_BINDING(loca,binding,type,name) layout(location=loca) in type name; 

#define VERTEX_INPUT_END(VT) 

#endif

#define VERTEX_ATTRIBUTE(loca,type,name) VERTEX_ATTRIBUTE_BINDING(loca,0,type,name)
