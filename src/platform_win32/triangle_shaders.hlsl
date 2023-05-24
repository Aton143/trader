struct Global_Data
{
  float2 resolution;
  float2 texture_dimensions;
}

struct VS_Input
{
  float3 position:   POSITION;
  float2 texture_uv: TEXCOORD;
}

struct PS_Input
{
  float4 vertex: SV_POSITION;
  float2 uv:     TEXCOORD;
  float4 color:  COLOR;
}

Global_Data  global_data;
Texture2D    global_texture: register(t0);
SamplerState global_sampler: register(s0);

PS_Input VS_Main(VS_Input input)
{
  PS_Input output;

  output.vertex = float4((2 * input.position.x / global_data.resolution.x) - 1,
                         1 - (2 * input.position.y / global_data.resolution.y),
                         input.position.z,
                         1);

  output.uv     = float2(input.uv.x / texture_dimensions.x, input.uv.y / texture_dimensions.y);
  output.color  = input.color;

  return(output);
}

float4 PS_Main(PS_Input input): SV_Target
{
  float4 texture_sample    = global_texture.Sample(global_sampler, input.uv);
  float4 alpha_from_sample = texture_sample.a;
  float4 out_color         = alpha_from_sample * input.color;
  return out_color;
}
