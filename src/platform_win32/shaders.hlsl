struct Global_Data
{
  float2 resolution;
  float2 texture_dimensions;
};

struct VS_Input
{
  float2 top_left:             INSTANCE_SIZE0;
  float2 bottom_right:         INSTANCE_SIZE1;

  float4 color:                INSTANCE_COLOR;
  float3 position:             INSTANCE_POSITION;

  float2 texture_top_left:     INSTANCE_UV0;
  float2 texture_bottom_right: INSTANCE_UV1;

  // NOTE(antonio): synthetic
  uint vertex_id:              SV_VertexID;
};

struct PS_Input
{
  float4 vertex: SV_POSITION;
  float2 uv:     TEXCOORD;
  float4 color:  COLOR;
};

Global_Data  global_data;
Texture2D    global_texture: register(t0);
SamplerState global_sampler: register(s0);

// NOTE(antonio): will be using OpenGL ndc coordinate:
// 
//  (0, 0)        (1, 0)
//    +-------------+
//    |             |
//    |             |
//    |             |
//    |             |
//    |             |
//    |             |
//    +-------------+
//  (0, 1)        (1, 1)


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

  float2 destination_half_size = (input.bottom_right - input.top_left)     / 2;
  float2 destination_center    = (input.top_left     + input.bottom_right) / 2;
  float2 destination_position  = (vertices[input.vertex_id] * destination_half_size) + destination_center;

  destination_position.xy += input.position.xy;

  output.vertex = float4((2 * destination_position.x / global_data.resolution.x) - 1,
                         1 - (2 * destination_position.y / global_data.resolution.y),
                         input.position.z,
                         1);

  float2 unnorm_uv_half_size = (input.texture_bottom_right - input.texture_top_left)     / 2;
  float2 unnorm_uv_center    = (input.texture_top_left     + input.texture_bottom_right) / 2;
  float2 unnorm_uv_position  = ((vertices[input.vertex_id] * unnorm_uv_half_size) + unnorm_uv_center);

  float texture_width  = global_data.texture_dimensions.x;
  float texture_height = global_data.texture_dimensions.y;

  output.uv = float2(unnorm_uv_position.x / texture_width,
                     unnorm_uv_position.y / texture_height);
  output.color = input.color;

  return output;
}

float4 PS_Main(PS_Input input): SV_Target
{
  float4 texture_sample = global_texture.Sample(global_sampler, input.uv);
  float4 out_color      = texture_sample * input.color;
  return out_color;
}

