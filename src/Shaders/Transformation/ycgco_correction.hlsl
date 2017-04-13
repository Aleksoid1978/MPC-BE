// Fix incorrect display YCgCo after incorrect YUV to RGB conversion in EVR Mixer.

sampler s0 : register(s0);

static float4x4 rgb2yuv709 = { // BT.709
	 0.2126,   0.7152,   0.0722,  0.0,
	-0.11457, -0.38543,  0.5,     0.0,
	 0.5,     -0.45415, -0.04585, 0.0,
	 0.0,      0.0,      0.0,     0.0
};

static float4x4 ycgco2rgb = {
	1.0, -1.0,  1.0, 0.0,
	1.0,  1.0,  0.0, 0.0,
	1.0, -1.0, -1.0, 0.0,
	0.0,  0.0,  0.0, 0.0
};

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	// original pixel
	float4 c0 = tex2D(s0, tex);

	c0 = mul(rgb2yuv709, c0); // convert RGB to YUV and get original YCgCo
	c0 = mul(ycgco2rgb, c0); // convert YCgCo to RGB

	return c0;
}
