// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 wh;
	uint   counter;
	float  clock;
	float2 dxy;
};

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c1 = tex.Sample(samp, coord + float2(-dxy.x, -dxy.y));
	float4 c2 = tex.Sample(samp, coord + float2(     0, -dxy.y));
	float4 c4 = tex.Sample(samp, coord + float2(-dxy.x,      0));
	float4 c6 = tex.Sample(samp, coord + float2( dxy.x,      0));
	float4 c8 = tex.Sample(samp, coord + float2(     0,  dxy.y));
	float4 c9 = tex.Sample(samp, coord + float2( dxy.x,  dxy.y));

	float4 c0 = (-c1 - c2 - c4 + c6 + c8 + c9);
	c0 = (c0.r + c0.g + c0.b) / 3 + 0.5;

	return c0;
}
