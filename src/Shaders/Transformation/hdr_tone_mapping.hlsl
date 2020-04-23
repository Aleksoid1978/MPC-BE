inline float3 hable(float3 x)
{
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;

    return ((x * (A * x + (C * B)) + (D * E)) / (x * (A * x + B) + (D * F))) - E / F;
}

float3 ToneMappingHable(const float3 rgb)
{
    static const float3 HABLE_DIV = hable(4.8);

    return hable(rgb) / HABLE_DIV;
}
