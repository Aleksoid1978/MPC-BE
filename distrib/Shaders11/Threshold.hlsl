// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

#define threshold 0.5

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float c0 = dot(tex.Sample(samp, coord), float4(0.2126, 0.7152, 0.0722, 0)); // grayscale

	return c0 < threshold ? 0 : 1;
}
