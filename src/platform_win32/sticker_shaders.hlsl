#pragma pack_matrix(row_major)
struct Global_Data
{
  float2   texture_dimensions;
  float2   client_dimensions;
  float4x4 model;
  float4x4 view;
  float4x4 projection;
};

struct VS_Input
{
  float4 position: IN0;
  float4 color:    IN1;
  float4 normal:   IN2;
  float4 uv:       IN3;

  float4 center:   IN4;
  float4 x_axis:   IN5;
  float4 y_axis:   IN6;
};

struct PS_Input
{
  float4 vertex:   SV_POSITION;
  float4 position: POSITION;
  float4 color:    COLOR;
  float4 center:   CENTER;
  float2 uv:       TEXCOORD;

  float4 x_axis:   XAXIS;
  float4 y_axis:   YAXIS;
};

Global_Data global_data;
Texture2D          global_texture: register(t0);
TextureCube        cubemap:        register(t1);

SamplerState global_sampler:         register(s0);
SamplerState global_cubemap_sampler: register(s1);

float4x4 inverse(float4x4 m);

PS_Input VS_Main(VS_Input input)
{
  PS_Input output;

  output.vertex   = mul(global_data.projection, mul(global_data.view, mul(global_data.model, input.position)));
  output.position = mul(global_data.model, input.position);

  output.uv.xy  = input.uv.xy;
  output.color  = input.color;

  output.center   = input.center;
  output.x_axis   = input.x_axis;
  output.y_axis   = input.y_axis;

  return(output);
}

float sdf_rounded_rect(float3 sample_pos,
                       float3 center,
                       float3 x_axis,
                       float3 y_axis,
                       float2 half_size,
                       float  r)
{
  float3 difference = sample_pos - center;
  float2 extents = float2(abs(dot(difference, x_axis)), abs(dot(difference, y_axis)));
  float2 d2 = abs(extents - center.xy) - half_size + float2(r, r);
  return(length(max(d2, float2(0.0f, 0.0f))) - r);
}

float4 PS_Main(PS_Input input): SV_Target
{
  float4 out_color;

  float alpha_sample = global_texture.Sample(global_sampler, input.uv).r;
  float3 combined    = float3(input.color.rgb) * alpha_sample;
  out_color = (alpha_sample > 0.0f) ? float4(combined, alpha_sample) : float4(0.0f, 0.0f, 0.0f, 1.0f);

  out_color = pow(out_color, 1.0f / 2.2f);
  return out_color;
}
