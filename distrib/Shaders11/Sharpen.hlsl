// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float  px;
	float  py;
	float2 wh;
	uint   counter;
	float  clock;
};

#define val0 (2.0)
#define val1 (-0.125)
#define effect_width (1.6)

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float dx = effect_width * px;
	float dy = effect_width * py;

	float4 c1 = tex.Sample(samp, coord + float2(-dx, -dy)) * val1;
	float4 c2 = tex.Sample(samp, coord + float2(  0, -dy)) * val1;
	float4 c3 = tex.Sample(samp, coord + float2(-dx,   0)) * val1;
	float4 c4 = tex.Sample(samp, coord + float2( dx,   0)) * val1;
	float4 c5 = tex.Sample(samp, coord + float2(  0,  dy)) * val1;
	float4 c6 = tex.Sample(samp, coord + float2( dx,  dy)) * val1;
	float4 c7 = tex.Sample(samp, coord + float2(-dx, +dy)) * val1;
	float4 c8 = tex.Sample(samp, coord + float2(+dx, -dy)) * val1;
	float4 c9 = tex.Sample(samp, coord) * val0;

	float4 c0 = (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9);

	return c0;
}
