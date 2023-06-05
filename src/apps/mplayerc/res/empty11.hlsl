Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float  px;
	float  py;
	float  w;
	float  h;
	uint   counter;
	float  clock;
};

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c = tex.Sample(samp, coord);

	return c;
}
