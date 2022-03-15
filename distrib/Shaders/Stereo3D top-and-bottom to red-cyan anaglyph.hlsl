// $MinimumShaderProfile: ps_2_0
// This shader should be run as a pre-resize pixel shader.
sampler s0 : register(s0);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float2 coordT = float2(tex.x, tex.y*0.5);
	float2 coordB = float2(tex.x, tex.y*0.5+0.5);

	return float4(tex2D(s0, coordT).r, tex2D(s0, coordB).gbb);
}
