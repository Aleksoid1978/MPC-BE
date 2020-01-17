// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 wh;
	uint   counter;
	float  clock;
	float2 dxdy;
};

float4 main(float4 Pos : SV_POSITION, float2 Tex : TEXCOORD) : SV_Target
{
	float4 c0 = 0;

	Tex.x += sin(Tex.x + clock / 0.3) / 20;
	Tex.y += sin(Tex.x + clock / 0.3) / 20;

	if (Tex.x >= 0 && Tex.x <= 1 && Tex.y >= 0 && Tex.y <= 1) {
		c0 = tex.Sample(samp, Tex);
	}

	return c0;
}
