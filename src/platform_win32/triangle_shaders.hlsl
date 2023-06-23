#pragma pack_matrix(row_major)
struct Vertex_Global_Data
{
  float4   texture_dimensions;
  float4x4 model_view_projection;
};

struct VS_Input
{
  float4 position: POSITION;
  float4 color:    COLOR;
  float2 uv:       TEXCOORD;
};

struct PS_Input
{
  float4 vertex: SV_POSITION;
  float4 color:  COLOR;
  float2 uv:     TEXCOORD;
};

Vertex_Global_Data global_data;
Texture2D          global_texture: register(t0);
SamplerState       global_sampler: register(s0);

PS_Input VS_Main(VS_Input input)
{
  PS_Input output;


  float4 centered_vertex = float4((2 * input.position.x) - 1,
                                  1 - (2 * input.position.y),
                                  input.position.z,
                                  input.position.w);

  output.vertex = mul(global_data.model_view_projection, centered_vertex);
  output.vertex.z = 0.5f;
  output.uv     = float2(input.uv.x / global_data.texture_dimensions.x, input.uv.y / global_data.texture_dimensions.y);
  output.color  = input.color;

  return(output);
}

float4 PS_Main(PS_Input input): SV_Target
{
  float alpha_sample = global_texture.Sample(global_sampler, input.uv).r;
  float3 combined    = float3(input.color.rgb) * alpha_sample;
  float4 out_color   = float4(combined, alpha_sample);
  out_color = pow(out_color, 1.0 / 2.2f);
  return out_color;
}
