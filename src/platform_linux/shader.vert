#version 330 core

struct VS_Input
{
  vec2 top_left;
  vec2 bottom_right;

  vec4 color[4];

  vec3 position;

  float  corner_radius;
  float  edge_softness;
  float  border_thickness;

  vec2 texture_top_left;
  vec2 texture_bottom_right;
};

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

layout (location = 0) in VS_Input vs_input;

out PS_Input vs_output;

layout (location = 0) uniform float uniform_scale;
layout (std140)       uniform global_data
{
  vec4 texture_dimensions;
  vec2 resolution;
};
uniform mat4 transform;

void main()
{
  // NOTE(antonio): static vertex array that we can index into with our vertex ID
  static vec2 vertices[] =
  {
    // NOTE(antonio): Bottom Left
    {-1.0f, -1.0f},
    // NOTE(antonio): Top    Left
    {-1.0f, +1.0f},
    // NOTE(antonio): Bottom Right
    {+1.0f, -1.0f},
    // NOTE(antonio): Top    Right
    {+1.0f, +1.0f},
  };
}
