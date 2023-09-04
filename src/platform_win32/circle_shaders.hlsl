#pragma pack_matrix(row_major)

struct VS_Input
{
  float4 position: POSITION;
  float4 color:    COLOR;
  float4 normal:   NORMAL;
  float2 uv:       TEXCOORD;
};

struct PS_Input
{
  float4 vertex:   SV_POSITION;
  float4 color:    COLOR;
  float2 position: POSITION;
  float2 center:   CENTER;
  float  radius:   RADIUS;
};

cbuffer CBUF: register(b0)
{
  float2   texture_dimensions;
  float2   client_dimensions;
  float4x4 model;
  float4x4 view;
  float4x4 projection;
}

Texture2D          global_texture: register(t0);
SamplerState       global_sampler: register(s0);

// < 0 if inside
// 0 if on edge
// > 0 if outside
float sdf_circle(float2 pos, float2 center, float radius)
{
  float dist_from_center = length(pos - center);
  return(dist_from_center - radius);
}

PS_Input VS_Main(VS_Input input) {
  PS_Input output;

  output.vertex   = mul(projection, mul(view, input.position));

  output.color    = input.color;
  output.center   = input.normal.xy;
  output.radius   = input.normal.w;
  output.position = output.vertex;

  output.vertex.x = (client_dimensions.y / client_dimensions.x) * output.vertex.x;

  return(output);
}

float4 PS_Main(PS_Input input): SV_Target
{
  float4 out_color = input.color;
  out_color.rgb *= out_color.a;

  float circle_sdf = sdf_circle(input.position.xy, input.center.xy, input.radius);
  if (circle_sdf >= 0.0f)
  {
    out_color *= 0.0f;
  }

  out_color = pow(out_color, 1.0f / 2.2f);
  return out_color;
}
