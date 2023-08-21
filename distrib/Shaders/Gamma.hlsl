// $MinimumShaderProfile: ps_2_0

sampler s0 : register(s0);

#define Gamma 1.3
// gamma > 1.0 brightens the output; gamma < 1.0 darkens the output

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	return pow(tex2D(s0, tex), 1.0/Gamma);
}
