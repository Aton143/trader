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

layout (location =  0) in vec2  top_left;
layout (location =  1) in vec2  bottom_right;
layout (location =  2) in vec4  color[4];
layout (location =  6) in vec3  position;
layout (location =  7) in float corner_radius;
layout (location =  8) in float edge_softness;
layout (location =  9) in float border_thickness;
layout (location = 10) in vec2  texture_top_left;
layout (location = 11) in vec2  texture_bottom_right;

out PS_Input vs_output;

uniform float uniform_scale;
layout (std140) uniform global_data
{
  vec4 texture_dimensions;
  vec2 resolution;
};
uniform mat4 transform;

void main()
{
  // NOTE(antonio): static vertex array that we can index into with our vertex ID
  const vec2 vertices[4] =
  vec2[4](
    vec2(-1.0f, -1.0f), // Bottom Left
    vec2(-1.0f, +1.0f), // Top Left
    vec2(+1.0f, -1.0f), // Bottom Right
    vec2(+1.0f, +1.0f)  // Top Right
  );

  vec2 dst_half_size = (bottom_right - top_left)     / 2.0f;
  vec2 dst_center    = (top_left     + bottom_right) / 2.0f;
  vec2 dst_position  = (vertices[gl_InstanceID] * dst_half_size) + dst_center;

  dst_position.xy += position.xy;

  vec2 unnorm_uv_half_size = (texture_bottom_right - texture_top_left)     / 2;
  vec2 unnorm_uv_center    = (texture_top_left     + texture_bottom_right) / 2;
  vec2 unnorm_uv_position  = ((vertices[gl_InstanceID] * unnorm_uv_half_size) + unnorm_uv_center);

  float texture_width  = texture_dimensions.x;
  float texture_height = texture_dimensions.y;

  gl_Position = vec4((2 * dst_position.x / resolution.x) - 1,
                     1 - (2 * dst_position.y / resolution.y),
                     position.z,
                     1);

  vs_output.uv = vec2(unnorm_uv_position.x / texture_width,
                   unnorm_uv_position.y / texture_height);

  vs_output.color = color[gl_InstanceID];

  vs_output.dst_pos       = dst_position;
  vs_output.dst_center    = dst_center + position.xy;
  vs_output.dst_half_size = dst_half_size;

  vs_output.border_thickness = border_thickness;
  vs_output.corner_radius    = corner_radius;
  vs_output.edge_softness    = edge_softness;
}
