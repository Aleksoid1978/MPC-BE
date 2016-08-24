// Mitchell-Netravali spline4, pass Y

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

#define sp(a, b, c) float4 a = tex2D(s0, tex+dxdy*float2(b, c));

sampler s0 : register(s0);
float2 dxdy : register(c0);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float t = frac(tex.y);
	float2 pos = tex-float2(0., t);
	// original pixels
	float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);
	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);
	float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);
	float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);

	// calculate weights
	float t2 = t*t, t3 = t*t2;
	float4 w0123 = float4(1., 16., 1., 0.)/18.+float4(-.5, 0., .5, 0.)*t+float4(5., -12., 9., -2.)/6.*t2+float4(-7., 21., -21., 7.)/18.*t3;

	return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3; // interpolation output
}
