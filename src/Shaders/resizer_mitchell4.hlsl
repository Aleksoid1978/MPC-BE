// Mitchell-Netravali spline4

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

#define sp(a, b, c) float4 a = tex2D(s0, tex+dxdy*float2(b, c));

sampler s0 : register(s0);
float2 dxdy : register(c0);

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float2 t = frac(tex); // calculate the difference between the output pixel and the original surrounding two pixels
	tex = (tex-t+.5)*dxdy; // adjust sampling matrix to put the ouput pixel in the interval [Q1, Q2)
	// weights
	float2 t2 = pow(t, 2);
	float2 t3 = pow(t, 3);
	float2 w0 = 1/18.+t2*5/6.-t3*7/18.-t/2.;
	float2 w1 = 8/9.+t3*7/6.-t2*2;
	float2 w2 = 1/18.+t2*1.5+t/2.-t3*7/6.;
	float2 w3 = t3*7/18.-t2/3.;

	// original pixels
	sp(M0, -1, -1) sp(M1, -1, 0) sp(M2, -1, 1) sp(M3, -1, 2)
	sp(L0,  0, -1) sp(L1,  0, 0) sp(L2,  0, 1) sp(L3,  0, 2)
	sp(K0,  1, -1) sp(K1,  1, 0) sp(K2,  1, 1) sp(K3,  1, 2)
	sp(J0,  2, -1) sp(J1,  2, 0) sp(J2,  2, 1) sp(J3,  2, 2)

	// vertical interpolation
	float4 Q0 = M0*w0.y+M1*w1.y+M2*w2.y+M3*w3.y;
	float4 Q1 = L0*w0.y+L1*w1.y+L2*w2.y+L3*w3.y;
	float4 Q2 = K0*w0.y+K1*w1.y+K2*w2.y+K3*w3.y;
	float4 Q3 = J0*w0.y+J1*w1.y+J2*w2.y+J3*w3.y;

	return Q0*w0.x+Q1*w1.x+Q2*w2.x+Q3*w3.x; // horizontal interpolation and output
}
