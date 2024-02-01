#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3 i_col[];
layout(location = 1) out vec3 o_col;


struct B
{
  float a;
  float b;
  vec2  c;
  mat4  d;
};

layout(binding = 0, set = 1) uniform SomeBuffer
{
   B b;
   mat4 c;
} someBuffer;


void main()
{
    vec4 position = gl_in[0].gl_Position;
	
    gl_Position = someBuffer.b.d * (position + vec4(-0.05f,-0.05f,0,0));
    o_col = i_col[0];
    EmitVertex();

    gl_Position = position + vec4( 0.05f,-0.05f,0,0);
    o_col = i_col[0];
    EmitVertex();
    
    gl_Position = position + vec4(-0.05f, 0.05f,0,0);
    o_col = i_col[0];
    EmitVertex();
    
    gl_Position = position + vec4( 0.05f, 0.05f,0,0);
    o_col = i_col[0];
    EmitVertex();
}
