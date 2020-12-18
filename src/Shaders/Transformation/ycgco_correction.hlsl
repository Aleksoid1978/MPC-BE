// Fix incorrect display YCgCo after incorrect YUV to RGB conversion in EVR Mixer.

sampler s0 : register(s0);

#include "conv_matrix.hlsl"

static const float4x4 rgb_ycbcr709_ycgco_rgb = mul(ycgco_rgb, rgb_ycbcr709);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
    float4 pixel = tex2D(s0, tex); // original pixel

    // convert RGB to YUV and get original YCgCo. convert YCgCo to RGB
    pixel = mul(rgb_ycbcr709_ycgco_rgb, pixel);

    return pixel;
}
