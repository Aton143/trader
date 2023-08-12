#version 330 core
in  vec4 vs_color;
in  vec2 vs_uv;

uniform sampler2D wall_sampler;
uniform sampler2D smile_sampler;
uniform float     mix_factor;

out vec4 out_color;

void main()
{
  out_color = mix(texture(wall_sampler, vs_uv), texture(smile_sampler, vs_uv), mix_factor);//  * vs_color;
} 
