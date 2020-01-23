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

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float dx = 4 * px;
	float dy = 4 * py;

	float4 c2 = tex.Sample(samp, coord + float2(  0, -dy));
	float4 c4 = tex.Sample(samp, coord + float2(-dx,   0));
	float4 c5 = tex.Sample(samp, coord + float2(  0,   0));
	float4 c6 = tex.Sample(samp, coord + float2( dx,   0));
	float4 c8 = tex.Sample(samp, coord + float2(  0,  dy));

	float4 c0 = (-c2 - c4 + c5 * 4 - c6 - c8);
	if (length(c0) < 1.0) {
		c0 = float4(0, 0, 0, 0);
	} else {
		c0 = float4(1, 1, 1, 0);
	}

	return c0;
}
