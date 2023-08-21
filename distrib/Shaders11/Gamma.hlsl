// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

#define Gamma 1.3
// gamma > 1.0 brightens the output; gamma < 1.0 darkens the output

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	return pow(tex.Sample(samp, coord), 1.0/Gamma);
}
