// Nearest neighbor

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

sampler s0 : register(s0);
float2 dxdy : register(c0);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	return tex2D(s0, (tex+.5)*dxdy); // output nearest neighbor
}
