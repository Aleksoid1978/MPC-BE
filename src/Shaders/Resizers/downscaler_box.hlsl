
sampler s0   : register(s0); // input texture
float2 dxdy  : register(c0);
float2 scale : register(c2);

#ifndef AXIS
    #define AXIS 0
#endif

#define filter_support (0.5)

static const float support = filter_support * scale[AXIS];
static const float ss = 1.0 / scale[AXIS];

float4 main ( float2 tex : TEXCOORD0 ) : COLOR
{
    tex += 0.5;

    int low = (int)floor(tex[AXIS] - support);
    int high = (int)ceil(tex[AXIS] + support);

    float ww = 0.0;
    float4 avg = 0;

    [loop] for (int n = low; n < high; n++) {
        float d = (n - tex[AXIS] + 0.5) * ss;
        if (d >= -0.5 && d < 0.5) {
#if (AXIS == 0)
            avg += tex2D(s0, (tex + float2(n - tex[AXIS] + 0.5, 0)) * dxdy, 0, 0);
#else
            avg += tex2D(s0, (tex + float2(0, n - tex[AXIS] + 0.5)) * dxdy, 0, 0);
#endif
            ww++;
        }
    }
    avg /= ww;

    return avg;
}
