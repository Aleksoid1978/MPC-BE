// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

#define const_1 ( 16.0 / 255.0)
#define const_2 (219.0 / 255.0)

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	// original pixel
	float4 c0 = tex.Sample(samp, coord);

	return (c0 * const_2) + const_1;
}
