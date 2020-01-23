// $MinimumShaderProfile: ps_4_0

// Run this shader before scaling.

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 pxy;
	float  width;
	float  height;
	uint   counter;
	float  clock;
};

#define const_1 ( 16.0 / 255.0)
#define const_2 (255.0 / 219.0)

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	// original pixel
	float4 c0 = tex.Sample(samp, coord);

	if (width <= 1024 && height <= 576) {
		return ((c0 - const_1) * const_2);
	} else {
		return c0;
	}
}
