// $MinimumShaderProfile: ps_2_0
sampler s0 : register(s0);

#define threshold 0.5

float4 main(float2 tex : TEXCOORD0) : COLOR {
	float c0 = dot(tex2D(s0, tex), float4(0.2126, 0.7152, 0.0722, 0)); // grayscale

	return c0 < threshold;
}
