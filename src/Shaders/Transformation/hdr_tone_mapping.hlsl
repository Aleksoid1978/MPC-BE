const float hable(const float x)
{
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;

    return ((x * (A * x + (C * B)) + (D * E)) / (x * (A * x + B) + (D * F))) - E / F;
}

const float3 hable(float3 rgb)
{
    rgb.r = hable(rgb.r);
    rgb.g = hable(rgb.g);
    rgb.b = hable(rgb.b);

    return rgb;
}

float3 ToneMappingHable(const float3 rgb)
{
    static const float HABLE_DIV = hable(4.8);

    return hable(rgb) / HABLE_DIV;
}
