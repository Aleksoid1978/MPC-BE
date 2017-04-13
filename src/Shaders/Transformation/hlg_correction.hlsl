// Fix incorrect display ARIB STD-B67 (HLG) after incorrect YUV to RGB conversion in EVR Mixer.

sampler image : register(s0);

#define SRC_LUMINANCE_PEAK     1000.0
#define DISPLAY_LUMINANCE_PEAK 80.0

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

inline float inverse_HLG(float x)
{
    const float B67_a = 0.17883277;
    const float B67_b = 0.28466892;
    const float B67_c = 0.55991073;
    const float B67_inv_r2 = 4.0;
    if (x <= 0.5)
        x = x * x * B67_inv_r2;
    else
       x = exp((x - B67_c) / B67_a) + B67_b;

    return x;
}

float4 main(float2 tex : TEXCOORD0) : COLOR
{
    float4 pixel = tex2D(image, tex);

    // HLG to Linear
    pixel.r = inverse_HLG(pixel.r);
    pixel.g = inverse_HLG(pixel.g);
    pixel.b = inverse_HLG(pixel.b);
    pixel.rgb /= 12.0;

    // HDR tone mapping
    pixel.rgb = ToneMapping(pixel.rgb);

    // Peak luminance
    pixel.rgb = pixel.rgb * (SRC_LUMINANCE_PEAK / DISPLAY_LUMINANCE_PEAK);

    // Linear to sRGB
    pixel.rgb = saturate(pixel.rgb);
    pixel.rgb = pow(pixel.rgb, 1.0 / 2.2);

    return pixel;
}
