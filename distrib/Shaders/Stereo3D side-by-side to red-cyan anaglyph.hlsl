// $MinimumShaderProfile: ps_2_0
// This shader should be run as a pre-resize pixel shader.
sampler s0 : register(s0);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float2 coordL = float2(tex.x*0.5, tex.y);
	float2 coordR = float2(tex.x*0.5+0.5, tex.y);

	return float4(tex2D(s0, coordL).r, tex2D(s0, coordR).gbb);
}
