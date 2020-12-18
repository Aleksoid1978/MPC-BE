// Convert VideoPrimaries BT.2020 to BT.709

sampler image : register(s0);

#include "colorspace_gamut_conversion.hlsl"

float4 main(float2 tex : TEXCOORD0) : COLOR
{
    float4 pixel = tex2D(image, tex);

    // sRGB to to Linear
    pixel = saturate(pixel);
    pixel = pow(pixel, 2.2);

    // Colorspace Gamut Conversion
    pixel.rgb = Colorspace_Gamut_Conversion_2020_to_709(pixel.rgb);

    // Linear to sRGB
    pixel = saturate(pixel);
    pixel = pow(pixel, 1.0 / 2.2);

    return pixel;
}
