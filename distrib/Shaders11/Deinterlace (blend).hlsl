// $MinimumShaderProfile: ps_4_0

// Run this shader before scaling.

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float  px;
	float  py;
	float  width;
	float  height;
	uint   counter;
	float  clock;
};

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c0 = tex.Sample(samp, coord);

	float4 c1 = tex.Sample(samp, coord - float2(0, py));
	float4 c2 = tex.Sample(samp, coord + float2(0, py));
	c0 = (c0 * 2 + c1 + c2) / 4;

	return c0;
}
