#version 450 core

layout(binding = 0) uniform sampler2D u_texture_ui;

in vec2 v2f_texcoord;

out vec4 out_color;

void main()
{
    float texture_value = texture(u_texture_ui, v2f_texcoord).r;
    out_color = vec4(1, 0, 0, texture_value);
}
