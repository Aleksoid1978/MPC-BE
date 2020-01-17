// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 main(float4 Pos : SV_POSITION, float2 Tex : TEXCOORD) : SV_Target
{
	float c = dot(tex.Sample(samp, Tex), float4(0.2, 0.6, 0.1, 0.1));

	return float4(0, c, 0, 0);
}
