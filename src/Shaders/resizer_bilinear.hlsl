// Bilinear

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

sampler s0 : register(s0);
float2 dxdy : register(c0);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float2 t = frac(tex);
	float2 pos = tex-t;

	return lerp(
		lerp(tex2D(s0, (pos+.5)*dxdy), tex2D(s0, (pos+float2(1.5, .5))*dxdy), t.x),
		lerp(tex2D(s0, (pos+float2(.5, 1.5))*dxdy), tex2D(s0, (pos+1.5)*dxdy), t.x),
		t.y); // interpolate and output
}
