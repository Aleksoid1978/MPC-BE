// $MinimumShaderProfile: ps_4_0

// Correct video colorspace BT.601 [SD] to BT.709 [HD] for HD video input.
// Use this shader only if BT.709 [HD] encoded video is incorrectly matrixed to full range RGB with the BT.601 [SD] colorspace.
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

static float4x4 rgb_ycbcr601 = {
	 0.299,     0.587,     0.114,    0.0,
	-0.168736, -0.331264,  0.5,      0.0,
	 0.5,      -0.418688, -0.081312, 0.0,
	 0.0,       0.0,       0.0,      0.0
};
static float4x4 ycbcr709_rgb = {
	1.0,  0.0,       1.5748,   0.0,
	1.0, -0.187324, -0.468124, 0.0,
	1.0,  1.8556,    0.0,      0.0,
	0.0,  0.0,       0.0,      0.0
};

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c0 = tex.Sample(samp, coord); // original pixel
	
	if (width <= 1024 && height <= 576) {
		return c0; // this shader does not alter SD video
	}
	c0 = mul(rgb_ycbcr601, c0); // convert RGB to Y'CbCr
	c0 = mul(ycbcr709_rgb, c0); // convert Y'CbCr to RGB
	
	return c0;
}
