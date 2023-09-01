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
  float4 position: POSITION;
  float4 color:  COLOR;
  float4 normal: NORMAL;
  float2 uv:     TEXCOORD;
};

Vertex_Global_Data global_data;
Texture2D          global_texture: register(t0);
TextureCube        cubemap:        register(t1);

SamplerState       global_sampler: register(s0);
SamplerState       global_cubemap_sampler: register(s1);

float4x4 inverse(float4x4 m);

PS_Input VS_Main(VS_Input input)
{
  PS_Input output;

  output.vertex   = mul(global_data.projection, mul(global_data.view, mul(global_data.model, input.position)));
  output.position = mul(global_data.model, input.position);

  output.uv     = float2(input.uv.x, input.uv.y);
  output.color  = input.color;
  output.normal = float4(mul(
                             transpose(inverse(
                                                 global_data.model
                             )),
                             input.normal).xyz, 1.0f);

  return(output);
}

float4 PS_Main(PS_Input input): SV_Target
{
  float3 camera_pos = float3(0.0f, 0.0f, 1.0f);
  float3 dir        = normalize(input.position.xyz - camera_pos);
  float3 reflected  = reflect(dir, normalize(input.normal.xyz));
  float4 out_color  = float4(cubemap.Sample(global_cubemap_sampler, reflected).xyz, 1.0f);

/*
  float alpha_sample = global_texture.Sample(global_sampler, input.uv).r;
  float3 combined    = float3(input.color.rgb) * alpha_sample;
  out_color = float4(input.normal.xyz, alpha_sample);
*/

  out_color = pow(out_color, 1.0f / 2.2f);
  return out_color;
}


float4x4 inverse(float4x4 m)
{
  float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
  float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
  float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
  float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

  float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
  float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
  float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
  float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

  float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
  float idet = 1.0f / det;

  float4x4 ret;

  ret[0][0] = t11 * idet;
  ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
  ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
  ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

  ret[1][0] = t12 * idet;
  ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
  ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
  ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

  ret[2][0] = t13 * idet;
  ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
  ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
  ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

  ret[3][0] = t14 * idet;
  ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
  ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
  ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}
