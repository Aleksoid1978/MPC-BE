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

#define val0 (1.0)
#define val1 (0.125)
#define effect_width (0.1)

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float dx = 0.0f;
	float dy = 0.0f;
	float fTap = effect_width;

	float4 cAccum = tex.Sample(samp, coord) * val0;

	for (int iDx = 0; iDx < 16; ++iDx) {
		dx = fTap * px;
		dy = fTap * py;

		cAccum += tex.Sample(samp, coord + float2(-dx, -dy)) * val1;
		cAccum += tex.Sample(samp, coord + float2(  0, -dy)) * val1;
		cAccum += tex.Sample(samp, coord + float2(-dx,   0)) * val1;
		cAccum += tex.Sample(samp, coord + float2( dx,   0)) * val1;
		cAccum += tex.Sample(samp, coord + float2(  0,  dy)) * val1;
		cAccum += tex.Sample(samp, coord + float2( dx,  dy)) * val1;
		cAccum += tex.Sample(samp, coord + float2(-dx, +dy)) * val1;
		cAccum += tex.Sample(samp, coord + float2(+dx, -dy)) * val1;

		fTap  += 0.1f;
	}

	return (cAccum / 16.0f);
}
