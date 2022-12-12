#version 450

#include "shader.h"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3 i_col[];
layout(location = 0) out vec3 o_col;


void main()
{
    vec4 position = gl_in[0].gl_Position;
	
    gl_Position = position + vec4(-0.01f,-0.01f,0,0);
    o_col = i_col[0];
    EmitVertex();

    gl_Position = position + vec4( 0.01f,-0.01f,0,0);
    o_col = i_col[0];
    EmitVertex();
    
    gl_Position = position + vec4(-0.01f, 0.01f,0,0);
    o_col = i_col[0];
    EmitVertex();
    
    gl_Position = position + vec4( 0.01f, 0.01f,0,0);
    o_col = i_col[0];
    EmitVertex();
}
