// Correcting bad chroma upscale in EVR Mixer for Nvidia graphics cards

sampler s0 : register(s0);
float4 p0  : register(c0);

#define dx (p0[0])

#include "conv_matrix.hlsl"

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float4 ycbcr0 = mul(rgb_ycbcr709, tex2D(s0, tex + float2(-dx, 0)));
	float4 ycbcr1 = mul(rgb_ycbcr709, tex2D(s0, tex));
	float4 ycbcr2 = mul(rgb_ycbcr709, tex2D(s0, tex + float2(dx, 0)));

	float4 ycbcr = (ycbcr0 + ycbcr1 * 2 + ycbcr2) / 4;
	ycbcr[0] = ycbcr1[0];

	return mul(ycbcr709_rgb, ycbcr);
}
