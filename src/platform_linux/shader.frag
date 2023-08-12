#version 330 core
in  vec4 vs_color;
in  vec2 vs_uv;

uniform sampler2D texture_sampler;

out vec4 out_color;

void main()
{
  out_color = texture(texture_sampler, vs_uv);
} 
