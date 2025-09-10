#version 450 core

in vec2 v2f_uv;
in vec3 v2f_normal;

out vec4 o_color;

uniform sampler2D u_tex;

void main()
{
    o_color = texture(u_tex, v2f_uv) + vec4(v2f_normal * 0.001, 0.1);
}
