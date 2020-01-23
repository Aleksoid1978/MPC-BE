// $MinimumShaderProfile: ps_4_0

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

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c0 = 0;

	float2 ar = float2(16, 9);
	float h = (1 - width / height * ar.y / ar.x) / 2;

	if (coord.y >= h && coord.y <= 1 - h) {
		c0 = tex.Sample(samp, coord);
	}

	return c0;
}
