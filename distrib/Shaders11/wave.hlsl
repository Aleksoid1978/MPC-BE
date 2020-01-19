// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 wh;
	uint   counter;
	float  clock;
	float2 dxy;
};

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c0 = 0;

	coord.x += sin(coord.x + clock / 0.3) / 20;
	coord.y += sin(coord.x + clock / 0.3) / 20;

	if (coord.x >= 0 && coord.x <= 1 && coord.y >= 0 && coord.y <= 1) {
		c0 = tex.Sample(samp, coord);
	}

	return c0;
}
