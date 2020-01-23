// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float4 c = float4(1, 1, 1, 1) - tex.Sample(samp, coord);

	return c;
}
