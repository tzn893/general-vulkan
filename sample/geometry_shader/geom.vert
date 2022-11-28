#version 450
#include "shader.h"

layout(location = 0) out vec3 o_col;

void main()
{
	o_col			= color;
	gl_Position		= vec4(rotation * pos,0.5f);
}
