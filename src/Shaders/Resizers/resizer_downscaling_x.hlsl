// downscaling, pass X

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#define MAXSTEPS 10
#else
#define MAXSTEPS 8
#endif

sampler s0 : register(s0);
float2 dxdy : register(c0);
float2 steps : register(c1);

static const int kx = clamp(int(steps.x), 2, MAXSTEPS);
static const int start = kx / 2 - kx;

float4 main(float2 tex : TEXCOORD0) : COLOR0
{
	float t = frac(tex.x);
	float2 pos = tex - float2(t, 0.);

	float4 result = 0;
	for (int i = 0; i < kx; i++) {
		result += tex2D(s0, (pos + float2(start+i+0.5, .5))*dxdy);
	}
	result /= kx;

	return result;
}
