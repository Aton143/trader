struct VS_Input_16
{
  float4 res0: IN0;
};

struct VS_Input_32
{
  float4 res0: IN0;
  float4 res1: IN1;
};

struct VS_Input_64
{
  float4 res0: IN0;
  float4 res1: IN1;
  float4 res2: IN2;
  float4 res3: IN3;
};

float4 VS_Main_16(VS_Input_16 input): SV_POSITION
{
  float output = input.res0;
  return(output);
}

float4 VS_Main_32(VS_Input_32 input): SV_POSITION
{
  float output = input.res0;
  return(output);
}

float4 VS_Main_64(VS_Input_64 input): SV_POSITION
{
  float output = input.res0;
  return(output);
}
