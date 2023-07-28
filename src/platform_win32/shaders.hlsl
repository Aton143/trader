struct Global_Data
{
  float4 texture_dimensions;
  float2 resolution;
};

struct VS_Input
{
  float2 top_left:             INSTANCE_SIZE0;
  float2 bottom_right:         INSTANCE_SIZE1;

  float4 color[4]:             INSTANCE_COLOR;

  float3 position:             INSTANCE_POSITION;

  float  corner_radius:        INSTANCE_CORNER_RADIUS;
  float  edge_softness:        INSTANCE_EDGE_SOFTNESS;
  float  border_thickness:     INSTANCE_BORDER_THICKNESS;

  float2 texture_top_left:     INSTANCE_UV0;
  float2 texture_bottom_right: INSTANCE_UV1;

  // NOTE(antonio): synthetic
  uint vertex_id:              SV_VertexID;
};

struct PS_Input
{
  float4 vertex:           SV_POSITION;
  float4 color:            COLOR;
  float2 uv:               TEXCOORD;

  float2 dst_pos:          POS0;
  float2 dst_center:       POS1;
  float2 dst_half_size:    POS2;

  float  corner_radius:    CORNER_RADIUS;
  float  edge_softness:    EDGE_SOFTNESS;
  float  border_thickness: BORDER_THICKNESS;
};

Global_Data  global_data;
Texture2D    global_texture: register(t0);
SamplerState global_sampler: register(s0);

// NOTE(antonio): ndc coordinates:
// 
//  (-1, -1)        (1, -1)
//     +-------------+z
//     |             |
//     |             |
//     |             |
//     |             |
//     |             |
//     |             |
//     +-------------+
//  (-1,  1)        (1,  1)

// >= 0 if outside
// 0 if inside
float sdf_rounded_rect(float2 sample_pos,
                       float2 rect_center,
                       float2 rect_half_size,
                       float  r)
{
  float2 d2 = abs(sample_pos - rect_center) - rect_half_size + float2(r, r);
  return length(max(d2, float2(0.0f, 0.0f))) /*+ min(max(d2.x, d2.y), 0.0) */ - r;
}

PS_Input VS_Main(VS_Input input)
{
  // NOTE(antonio): static vertex array that we can index into with our vertex ID
  static float2 vertices[] =
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

  PS_Input output;

  float2 dst_half_size = (input.bottom_right - input.top_left)     / 2.0f;
  float2 dst_center    = (input.top_left     + input.bottom_right) / 2.0f;
  float2 dst_position  = (vertices[input.vertex_id] * dst_half_size) + dst_center;

  dst_position.xy += input.position.xy;

  float2 unnorm_uv_half_size = (input.texture_bottom_right - input.texture_top_left)     / 2;
  float2 unnorm_uv_center    = (input.texture_top_left     + input.texture_bottom_right) / 2;
  float2 unnorm_uv_position  = ((vertices[input.vertex_id] * unnorm_uv_half_size) + unnorm_uv_center);

  float texture_width  = global_data.texture_dimensions.x;
  float texture_height = global_data.texture_dimensions.y;

  output.vertex = float4((2 * dst_position.x / global_data.resolution.x) - 1,
                         1 - (2 * dst_position.y / global_data.resolution.y),
                         input.position.z,
                         1);

  output.uv = float2(unnorm_uv_position.x / texture_width,
                     unnorm_uv_position.y / texture_height);

  output.color = input.color[input.vertex_id];

  output.dst_pos       = dst_position;
  output.dst_center    = dst_center + input.position.xy;
  output.dst_half_size = dst_half_size;

  output.border_thickness = input.border_thickness;
  output.corner_radius    = input.corner_radius;
  output.edge_softness    = input.edge_softness;

  return output;
}

float4 PS_Main(PS_Input input): SV_Target
{
  float alpha_sample = global_texture.Sample(global_sampler, input.uv).r;

  float border_factor = 1.0f;
  if(input.border_thickness != 0.0f)
  {
    float2 interior_half_size = input.dst_half_size - input.border_thickness;

    // reduction factor for the internal corner
    // radius. not 100% sure the best way to go
    // about this, but this is the best thing I've
    // found so far!
    //
    // this is necessary because otherwise it looks
    // weird
    float interior_radius_reduce_f = min(interior_half_size.x / input.dst_half_size.x,
                                         interior_half_size.y / input.dst_half_size.y);

    float interior_corner_radius = (input.corner_radius *
                                    interior_radius_reduce_f *
                                    interior_radius_reduce_f);

    // calculate sample distance from "interior"
    float inside_d = sdf_rounded_rect(input.dst_pos,
                                      input.dst_center,
                                      interior_half_size,
                                      interior_corner_radius);

    // map distance => factor
    float inside_f = 1.0f - smoothstep(0, 2.0f * input.edge_softness, inside_d);
    if (inside_d > 0.0f) border_factor = inside_f;
  }
  else
  {
    border_factor = 1.0f;
  }

  float  multiplied_alpha = alpha_sample * border_factor * input.color.a;
  float3 combined         = float3(input.color.rgb) * multiplied_alpha;
  float4 out_color        = float4(combined, multiplied_alpha);
  return out_color;
}

