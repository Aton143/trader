#version 330 core
in  vec4 vs_color;
in  vec2 vs_uv;

uniform sampler2D texture_sampler;

out vec4 out_color;

void main()
{
  float alpha_sample = texture(texture_sampler, vs_uv).r;
  out_color          = vec4(alpha_sample, alpha_sample, alpha_sample, alpha_sample) * vs_color;
} 
