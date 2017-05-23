
sampler s0   : register(s0); // input texture
float2 dxdy  : register(c0);
float2 scale : register(c2);

#ifdef PS20
    #define Get(pos) tex2D(s0, (pos))
#else
    #define Get(pos) tex2D(s0, (pos), 0, 0)
#endif

#ifndef AXIS
    #define AXIS 0
#endif

static const float support = 0.5 * scale[AXIS];

float4 main ( float2 tex : TEXCOORD0 ) : COLOR
{
    tex += 0.5;

    const float low = floor(tex[AXIS] + 0.5 - support);
    const int n = (int)(tex[AXIS] + 0.5 + support- low);

    float ww = 0.0;
    float4 avg = 0;

#ifdef PS20
    [unroll(6)]
#else
    [loop]
#endif
    for (int k = 0; k < n; k++) {
#if (AXIS == 0)
        avg += Get(float2(low + k + 0.5, tex.y) * dxdy);
#else
        avg += Get(float2(tex.x, low + k + 0.5) * dxdy);
#endif
    }
    avg /= n;

    return avg;
}
