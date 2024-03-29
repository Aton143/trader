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

struct VS_Input_128
{
  float4 res0: IN0;
  float4 res1: IN1;
  float4 res2: IN2;
  float4 res3: IN3;
  float4 res4: IN4;
  float4 res5: IN5;
  float4 res6: IN6;
  float4 res7: IN7;
};

float4 VS_Main_16(VS_Input_16 input): SV_POSITION
{
  float4 output = input.res0;
  return(output);
}

float4 VS_Main_32(VS_Input_32 input): SV_POSITION
{
  float4 output = input.res0;
  return(output);
}

float4 VS_Main_64(VS_Input_64 input): SV_POSITION
{
  float4 output = input.res0;
  return(output);
}

float4 VS_Main_128(VS_Input_128 input): SV_POSITION
{
  float4 output = input.res0;
  return(output);
}
