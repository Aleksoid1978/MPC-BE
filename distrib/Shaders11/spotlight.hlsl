// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 pxy;
	float2 wh;
	uint   counter;
	float  clock;
};

#define PI acos(-1)

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c0 = tex.Sample(samp, coord);
	float3 lightsrc = float3(sin(clock * PI / 1.5) / 2 + 0.5, cos(clock * PI) / 2 + 0.5, 1);
	float3 light = normalize(lightsrc - float3(coord.x, coord.y, 0));
	c0 *= pow(dot(light, float3(0, 0, 1)), 50);

	return c0;
}
