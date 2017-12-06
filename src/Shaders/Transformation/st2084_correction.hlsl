// Fix incorrect display SMPTE ST 2084 after incorrect YUV to RGB conversion in EVR Mixer.

sampler image : register(s0);

#define ST2084_m1 ((2610.0 / 4096.0) / 4.0)
#define ST2084_m2 ((2523.0 / 4096.0) * 128.0)
#define ST2084_c1 ( 3424.0 / 4096.0)
#define ST2084_c2 ((2413.0 / 4096.0) * 32.0)
#define ST2084_c3 ((2392.0 / 4096.0) * 32.0)

#define SRC_LUMINANCE_PEAK     10000.0
#define DISPLAY_LUMINANCE_PEAK 125.0

#include "hdr_tone_mapping.hlsl"
#include "colorspace_gamut_conversion.hlsl"

#pragma warning(disable: 3571) // fix warning X3571 in pow().

float4 main(float2 tex : TEXCOORD0) : COLOR
{
    //float4 pixel = saturate(tex2D(image, tex)); // use mindless saturate() for fix warning X3571 in pow()
    float4 pixel = tex2D(image, tex);

    // ST2084 to Linear
    pixel.rgb = pow(pixel.rgb, 1.0 / ST2084_m2);
    pixel.rgb = max(pixel.rgb - ST2084_c1, 0.0) / (ST2084_c2 - ST2084_c3 * pixel.rgb);
    pixel.rgb = pow(pixel.rgb, 1.0 / ST2084_m1);

    // Peak luminance
    pixel.rgb = pixel.rgb * (SRC_LUMINANCE_PEAK / DISPLAY_LUMINANCE_PEAK);

    // HDR tone mapping
    pixel.rgb = ToneMappingHable(pixel.rgb);

    // Colorspace Gamut Conversion
    pixel.rgb = Colorspace_Gamut_Conversion_2020_to_709(pixel.rgb);

    // Linear to sRGB
    pixel.rgb = saturate(pixel.rgb);
    pixel.rgb = pow(pixel.rgb, 1.0 / 2.2);

    return pixel;
}
