//for push constant reflection 

//for C++ project
#ifdef __cplusplus

#pragma once

#include <vector>
#include <string>

using uint = uint32_t;

#define DEFINE_MATRIX(m_dim, n_dim) struct mat##m_dim##x##n_dim { float a[m_dim][n_dim];};
#define DEFINE_S_MATRIX(m_dim) struct mat##m_dim { float a[m_dim][m_dim];};

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

DEFINE_MATRIX(4, 3)
DEFINE_MATRIX(4, 2)
DEFINE_MATRIX(3, 2)

DEFINE_MATRIX(3, 4)
DEFINE_MATRIX(2, 4)
DEFINE_MATRIX(2, 3)

DEFINE_S_MATRIX(4)
DEFINE_S_MATRIX(3)
DEFINE_S_MATRIX(2)


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
