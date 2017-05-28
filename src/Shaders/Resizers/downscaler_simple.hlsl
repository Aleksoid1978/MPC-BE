// downscaler "Simple Average"

#ifdef PS20
    #define GetFrom(s, t) tex2D(s, t)
    #define MAXSTEPS 8
#else
    #define GetFrom(s, t) tex2Dlod(s, float4(t, 0, 0))
    #define MAXSTEPS 10
#endif

#ifndef AXIS
    #define AXIS 0
#endif

sampler s0 : register(s0);
float2 dxdy : register(c0);
float2 steps : register(c1);

static const int k = clamp(int(steps[AXIS]), 2, MAXSTEPS);
static const int start = k / 2 - k;

float4 main(float2 tex : TEXCOORD0) : COLOR0
{
    float t = frac(tex[AXIS]);
#if (AXIS == 0)
    float2 pos = tex - float2(t, 0);
#else
    float2 pos = tex - float2(0, t);
#endif

    float4 result = 0;
    for (int i = 0; i < k; i++) {
#if (AXIS == 0)
        result += GetFrom(s0, (pos + float2(start+i+0.5, 0.5))*dxdy);
#else
        result += GetFrom(s0, (pos + float2(0.5, start+i+0.5))*dxdy);
#endif
    }
    result /= k;

    return result;
}
