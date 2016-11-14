// compensated Lanczos2, pass X

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

#define PI acos(-1.)

sampler s0 : register(s0);
float2 dxdy : register(c0);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float t = frac(tex.x);
	float2 pos = tex-float2(t, 0.);

	float4 Q1 = tex2D(s0, (pos+.5)*dxdy); // nearest original pixel to the left
	if(t) {
		// original pixels
		float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);
		float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);
		float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);
		float4 wset = float3(0., 1., 2.).yxyz+float2(t, -t).xxyy;
		float4 w = sin(wset*PI)*sin(wset*PI*.5)/(wset*wset*PI*PI*.5);

		float wc = 1.-dot(1., w); // compensate truncated window factor by bilinear factoring on the two nearest samples
		w.y += wc*(1.-t);
		w.z += wc*t;

		return w.x*Q0+w.y*Q1+w.z*Q2+w.w*Q3; // interpolation output
	}

	return Q1; // case t == 0. is required to return sample Q1, because of a possible division by 0.
}
