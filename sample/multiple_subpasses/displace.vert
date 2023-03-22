#version 450

layout(location = 0) out vec2 o_uv;


vec2 screen_pos[6] =
{
        {1.0f, -1.0f}, 
		{1.0f, 1.0f}, 
		{-1.0f, 1.0f}, 
		{-1.0f, -1.0f}, 
		{1.0f, -1.0f}, 
		{-1.0f, 1.0f} 
};

vec2 screen_uv[6] =
{
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f},
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {0.0f, 1.0f}
};


void main()
{
    uint vertId = gl_VertexIndex;

    o_uv = screen_uv[vertId];
    gl_Position = vec4(screen_pos[vertId],0.0f,1.0f);
}