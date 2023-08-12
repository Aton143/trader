#version 330 core
struct PS_Input
{
  vec4 vertex;
  vec4 color;
  vec2 uv;

  vec2 dst_pos;
  vec2 dst_center;
  vec2 dst_half_size;

  float  corner_radius;
  float  edge_softness;
  float  border_thickness;
};

in PS_Input vs_output;

uniform sampler2D texture_sampler;

out vec4 out_color;

void main()
{
  float alpha_sample = texture(texture_sampler, vs_uv).r;
  out_color          = vec4(alpha_sample, alpha_sample, alpha_sample, alpha_sample) * vs_color;
} 
