// Bicubic, pass Y

// #define A -0.6, -0.8 or -1.0
#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

sampler s0 : register(s0);
float2 dxdy : register(c0);

static float4x4 tco = {
	0,  A,  -2*A,    A,
	1,  0,  -A-3,  A+2,
	0, -A, 2*A+3, -A-2,
	0,  0,     A,   -A
};

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float t = frac(tex.y);
	float2 pos = tex-float2(0., t);
	// original pixels
	float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);
	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);
	float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);
	float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);

	return mul(mul(tco, float4(1., t, t*t, t*t*t)), float4x4(Q0, Q1, Q2, Q3));
}
