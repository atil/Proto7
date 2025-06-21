#version 450 core

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_texcoord;

out vec2 v2f_texcoord;

void main()
{
    v2f_texcoord = vec2(in_texcoord.x, in_texcoord.y);
    gl_Position = vec4(in_pos, 0.0, 1.0);
}
