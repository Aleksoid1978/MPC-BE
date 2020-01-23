// $MinimumShaderProfile: ps_2_0

// Run this shader before scaling.

sampler s0 : register(s0);
float4 p0 :  register(c0);

#define width  (p0[0])
#define height (p0[1])

#define const_1 ( 16.0 / 255.0)
#define const_2 (255.0 / 219.0)

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	// original pixel
	float4 c0 = tex2D(s0, tex);

	if (width <= 1024 && height <= 576) {
		return ((c0 - const_1) * const_2);
	} else {
		return c0;
	}
}
