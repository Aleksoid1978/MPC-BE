// Fix incorrect display ARIB STD-B67 (HLG) after incorrect YUV to RGB conversion in EVR Mixer.

sampler image : register(s0);

#include "colorspace_gamut_conversion.hlsl"

float4 main(float2 tex : TEXCOORD0) : COLOR
{
    float4 pixel = tex2D(image, tex);

    // Colorspace Gamut Conversion
    pixel.rgb = Colorspace_Gamut_Conversion_2020_to_709(pixel.rgb);

    return pixel;
}
