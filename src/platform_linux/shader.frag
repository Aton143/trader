#version 330 core
struct PS_Input
{
  vec4  color;
  vec2  uv;

  vec2  dst_pos;
  vec2  dst_center;
  vec2  dst_half_size;

  float corner_radius;
  float edge_softness;
  float border_thickness;
};

in PS_Input vs_output;
uniform sampler2D texture_sampler;

out vec4 out_color;

// > 0 if outside
// 0 if inside
float sdf_rounded_rect(in vec2  sample_pos,
                       in vec2  rect_center,
                       in vec2  rect_half_size,
                       in float r)
{
  vec2 d2 = abs(sample_pos - rect_center) - rect_half_size + vec2(r, r);
  return length(max(d2, vec2(0.0f, 0.0f))) /*+ min(max(d2.x, d2.y), 0.0) */ - r;
}

void main()
{
  float alpha_sample = texture(texture_sampler, vs_output.uv).r;

  float border_factor = 1.0f;
  if (vs_output.border_thickness != 0.0f)
  {
    vec2 interior_half_size = vs_output.dst_half_size - vs_output.border_thickness;

    // reduction factor for the internal corner
    // radius. not 100% sure the best way to go
    // about this, but this is the best thing I've
    // found so far!
    //
    // this is necessary because otherwise it looks
    // weird
    float interior_radius_reduce_f = min(interior_half_size.x / vs_output.dst_half_size.x,
                                         interior_half_size.y / vs_output.dst_half_size.y);

    float interior_corner_radius = (vs_output.corner_radius *
                                    interior_radius_reduce_f *
                                    interior_radius_reduce_f);

    // calculate sample distance from "interior"
    float inside_d = sdf_rounded_rect(vs_output.dst_pos,
                                      vs_output.dst_center,
                                      interior_half_size,
                                      interior_corner_radius);

    // map distance => factor
    float inside_f = 1.0f - smoothstep(0, 2.0f * vs_output.edge_softness, inside_d);
    if (inside_d > 0.0f) border_factor = inside_f;
  }
  else
  {
    border_factor = 1.0f;
  }

  float multiplied_alpha = alpha_sample * border_factor * vs_output.color.a;
  vec3  combined         = vec3(vs_output.color.rgb) * multiplied_alpha;

  out_color = vec4(combined, multiplied_alpha);
} 
