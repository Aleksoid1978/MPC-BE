// Bicubic

// #define A -0.6, -0.8 or -1.0

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

sampler s0 : register(s0);
float2 dxdy : register(c0);
float2 dxdy05 : register(c1);
float2 dx : register(c2);
float2 dy : register(c3);

static float4x4 tco = {
	0,  A,  -2*A,    A,
	1,  0,  -A-3,  A+2,
	0, -A, 2*A+3, -A-2,
	0,  0,     A,   -A
};

float4 taps(float t)
{
	return mul(tco, float4(1, t, t*t, t*t*t));
}

float4 SampleX(float4 tx, float2 t0)
{
	return mul(tx, float4x4(
		tex2D(s0, t0 - dx),
		tex2D(s0, t0),
		tex2D(s0, t0 + dx),
		tex2D(s0, t0 + dx + dx)
	));
}

float4 SampleY(float4 tx, float4 ty, float2 t0)
{
	return mul(ty, float4x4(
		SampleX(tx, t0-dy),
		SampleX(tx, t0),
		SampleX(tx, t0+dy),
		SampleX(tx, t0+dy+dy)
	));
}

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float2 dd = frac(tex);
	float2 ExactPixel = tex - dd;
	float2 samplePos = ExactPixel * dxdy + dxdy05;

	return SampleY(taps(dd.x), taps(dd.y), samplePos);
}
