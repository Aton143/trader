#pragma pack_matrix(row_major)
struct Vertex_Global_Data
{
  float4   texture_dimensions;
  float4x4 model;
  float4x4 view;
  float4x4 projection;
};

struct VS_Input
{
  float4 position: POSITION;
  float4 color:    COLOR;
  float4 normal:   NORMAL;
  float2 uv:       TEXCOORD;
};

struct PS_Input
{
  float4 vertex: SV_POSITION;
  float4 color:  COLOR;
  float3 center: CENTER;
  float  radius: RADIUS;
};

Vertex_Global_Data global_data;
Texture2D          global_texture: register(t0);

SamplerState       global_sampler: register(s0);

// < 0 if inside
// 0 if on edge
// > 0 if outside
float sdf_circle(float3 pos, float3 center, float radius)
{
  float dist_from_center = length(pos - center);
  return(dist_from_center - radius);
}

PS_Input VS_Main(VS_Input input) {
  PS_Input output;

  output.vertex   = mul(global_data.projection, mul(global_data.view, input.position));

  output.color  = input.color;
  output.center = input.normal.xyz;
  output.radius = input.normal.w;

  return(output);
}

float4 PS_Main(PS_Input input): SV_Target
{
  float4 out_color = input.color;

  float circle_sdf = sdf_circle(input.vertex.xyz, input.center, input.radius);
  if (circle_sdf >= 0.0f)
  {
    out_color *= 0.0f;
  }

  out_color = pow(out_color, 1.0f / 2.2f);
  return out_color;
}
