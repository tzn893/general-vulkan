#version 450

layout(location = 0) in vec3 i_col;
layout(location = 0) out vec4 o_col;


void main()
{
	o_col = vec4(i_col,1.0f);
}