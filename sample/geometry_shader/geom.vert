#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(push_constant) uniform Rotation
{
	mat4 rotation;
};

layout(location = 0) out vec3 o_col;

void main()
{
	o_col			= color;
	gl_Position		= rotation * vec4(pos, 1);
}
