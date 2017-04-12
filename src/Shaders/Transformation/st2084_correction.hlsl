// Fix incorrect display SMPTE ST 2084 after incorrect YUV to RGB conversion in EVR Mixer.

sampler image : register(s0);

#define ST2084_m1 ((2610.0 / 4096.0) / 4.0)
#define ST2084_m2 ((2523.0 / 4096.0) * 128.0)
#define ST2084_c1 ( 3424.0 / 4096.0)
#define ST2084_c2 ((2413.0 / 4096.0) * 32.0)
#define ST2084_c3 ((2392.0 / 4096.0) * 32.0)

#define SRC_LUMINANCE_PEAK     7500.0
#define DISPLAY_LUMINANCE_PEAK 150.0

float hable1(float x)
{
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;

    return ((x * (A * x + (C * B)) + (D * E)) / (x * (A * x + B) + (D * F))) - E / F;
}

float3 hable3(float3 rgb)
{
    rgb.r = hable1(rgb.r);
    rgb.g = hable1(rgb.g);
    rgb.b = hable1(rgb.b);

    return rgb;
}

float3 ToneMapping(float3 rgb)
{
    const float HABLE_DIV = hable1(11.2);

    return hable3(rgb) / HABLE_DIV;
}

float4 main(float2 tex : TEXCOORD0) : COLOR
{
    float4 pixel = tex2D(image, tex);

    // ST2084 to Linear
    pixel.rgb = pow(pixel.rgb, 1.0 / ST2084_m2);
    pixel.rgb = max(pixel.rgb - ST2084_c1, 0.0) / (ST2084_c2 - ST2084_c3 * pixel.rgb);
    pixel.rgb = pow(pixel.rgb, 1.0 / ST2084_m1);

    // HDR tone mapping
    pixel.rgb = ToneMapping(pixel.rgb);

    // Peak luminance
    pixel.rgb = pixel.rgb * (SRC_LUMINANCE_PEAK / DISPLAY_LUMINANCE_PEAK);

    // Linear to sRGB
    pixel.rgb = saturate(pixel.rgb);
    pixel.rgb = pow(pixel.rgb, 1.0 / 2.2);

    return pixel;
}
