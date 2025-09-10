#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec2 v2f_uv;
out vec3 v2f_normal;

void main()
{
    v2f_uv = in_uv;
    v2f_normal = in_normal;
    gl_Position = u_proj * u_view * u_model * vec4(in_pos, 1.0);
}
