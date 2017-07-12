// Fix incorrect display YCgCo after incorrect YUV to RGB conversion in EVR Mixer.

sampler s0 : register(s0);

#include "conv_matrix.hlsl"

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	// original pixel
	float4 c0 = tex2D(s0, tex);

	c0 = mul(rgb_ycbcr709, c0); // convert RGB to YUV and get original YCgCo
	c0 = mul(ycgco_rgb, c0); // convert YCgCo to RGB

	return c0;
}
