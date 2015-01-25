/*
 * (C) 2015 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Some shaders are taken from Jan-Willem Krans "Video pixel shader pack v1.4" and "MPC-HC tester builds".

#pragma once

// Bilinear
char const shader_resizer_bilinear[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float2 t = frac(tex);"
	"float2 pos = tex-t;"

	"return lerp("
		"lerp(tex2D(s0, (pos+.5)*dxdy), tex2D(s0, (pos+float2(1.5, .5))*dxdy), t.x),"
		"lerp(tex2D(s0, (pos+float2(.5, 1.5))*dxdy), tex2D(s0, (pos+1.5)*dxdy), t.x),"
		"t.y);"// interpolate and output
"}";

// Perlin Smootherstep
char const shader_resizer_smootherstep[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float2 t = frac(tex);"
	"float2 pos = tex-t;"
	"t *= ((6.*t-15.)*t+10.)*t*t;"// redistribute weights

	"return lerp("
		"lerp(tex2D(s0, (pos+.5)*dxdy), tex2D(s0, (pos+float2(1.5, .5))*dxdy), t.x),"
		"lerp(tex2D(s0, (pos+float2(.5, 1.5))*dxdy), tex2D(s0, (pos+1.5)*dxdy), t.x),"
		"t.y);"// interpolate and output
"}";

// Bicubic
char const shader_resizer_bicubic[] =
// "#define A -0.6, -0.8 or -1.0
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy :   register(c0);"
"float4 dxdy05 : register(c1);"
"float2 dx :     register(c2);"
"float2 dy :     register(c3);"

"static float4x4 tco = {"
"	0, A, -2 * A, A,"
"	1, 0, -A - 3, A + 2,"
"	0, -A, 2 * A + 3, -A - 2,"
"	0, 0, A, -A"
"};"

"float4 taps(float t)"
"{"
"	return mul(tco, float4(1, t, t * t, t * t * t));"
"}"

"float4 SampleX(float4 tx, float2 t0)"
"{"
"	return mul(tx, float4x4("
"		tex2D(s0, t0 - dx),"
"		tex2D(s0, t0),"
"		tex2D(s0, t0 + dx),"
"		tex2D(s0, t0 + dx + dx)"
"	));"
"}"

"float4 SampleY(float4 tx, float4 ty, float2 t0)"
"{"
"	return mul(ty, float4x4("
"		SampleX(tx, t0 - dy),"
"		SampleX(tx, t0),"
"		SampleX(tx, t0 + dy),"
"		SampleX(tx, t0 + dy + dy)"
"	));"
"}"

"float4 main(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float2 dd = frac(tex);"
"	float2 ExactPixel = tex - dd;"
"	float2 samplePos = ExactPixel * dxdy + dxdy05;"
"	return SampleY(taps(dd.x), taps(dd.y), samplePos);"
"}"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.x);"
"	float2 pos = tex-float2(t, 0.);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"

"	return mul(mul(tco, float4(1., t, t*t, t*t*t)), float4x4(Q0, Q1, Q2, Q3));"
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.y);"
"	float2 pos = tex-float2(0., t);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"

"	return mul(mul(tco, float4(1., t, t*t, t*t*t)), float4x4(Q0, Q1, Q2, Q3));"
"}";

// B-spline4
char const shader_resizer_bspline4[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"#define sp(a, b, c) float4 a = tex2D(s0, tex+dxdy*float2(b, c));\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float2 t = frac(tex);" // calculate the difference between the output pixel and the original surrounding two pixels
"	tex = (tex-t+.5)*dxdy;" // adjust sampling matrix to put the ouput pixel in the interval [Q1, Q2)
// weights
"	float2 t2 = pow(t, 2);"
"	float2 t3 = pow(t, 3);"
"	float2 w0 = (1.-t3)/6.+(t2-t)/2.;"
"	float2 w1 = t3/2.+2/3.-t2;"
"	float2 w2 = (t2+t-t3)/2.+1/6.;"
"	float2 w3 = t3/6.;"

// original pixels
"	sp(M0, -1, -1) sp(M1, -1, 0) sp(M2, -1, 1) sp(M3, -1, 2)"
"	sp(L0, 0, -1) sp(L1, 0, 0) sp(L2, 0, 1) sp(L3, 0, 2)"
"	sp(K0, 1, -1) sp(K1, 1, 0) sp(K2, 1, 1) sp(K3, 1, 2)"
"	sp(J0, 2, -1) sp(J1, 2, 0) sp(J2, 2, 1) sp(J3, 2, 2)"

// vertical interpolation
"	float4 Q0 = M0*w0.y+M1*w1.y+M2*w2.y+M3*w3.y;"
"	float4 Q1 = L0*w0.y+L1*w1.y+L2*w2.y+L3*w3.y;"
"	float4 Q2 = K0*w0.y+K1*w1.y+K2*w2.y+K3*w3.y;"
"	float4 Q3 = J0*w0.y+J1*w1.y+J2*w2.y+J3*w3.y;"

"	return Q0*w0.x+Q1*w1.x+Q2*w2.x+Q3*w3.x;" // horizontal interpolation and output
"}"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.x);"
"	float2 pos = tex - float2(t, 0.);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos + float2(-.5, .5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos + .5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos + float2(1.5, .5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos + float2(2.5, .5))*dxdy);"

	// calculate weights
"	float t2 = t*t, t3 = t*t2;"
"	float4 w0123 = float4(1., 4., 1., 0.) / 6. + float4(-.5, 0., .5, 0.)*t + float4(.5, -1., .5, 0.)*t2 + float4(-1., 3., -3., 1.) / 6.*t3;"

"	return w0123.x*Q0 + w0123.y*Q1 + w0123.z*Q2 + w0123.w*Q3;" // interpolation output
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.y);"
"	float2 pos = tex - float2(0., t);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos + float2(.5, -.5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos + .5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos + float2(.5, 1.5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos + float2(.5, 2.5))*dxdy);"

	// calculate weights
"	float t2 = t*t, t3 = t*t2;"
"	float4 w0123 = float4(1., 4., 1., 0.) / 6. + float4(-.5, 0., .5, 0.)*t + float4(.5, -1., .5, 0.)*t2 + float4(-1., 3., -3., 1.) / 6.*t3;"

"	return w0123.x*Q0 + w0123.y*Q1 + w0123.z*Q2 + w0123.w*Q3;" // interpolation output
"}";

// Mitchell-Netravali spline4
char const shader_resizer_mitchell4[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"#define sp(a, b, c) float4 a = tex2D(s0, tex+dxdy*float2(b, c));\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float2 t = frac(tex);" // calculate the difference between the output pixel and the original surrounding two pixels
"	tex = (tex-t+.5)*dxdy;" // adjust sampling matrix to put the ouput pixel in the interval [Q1, Q2)
	// weights
"	float2 t2 = pow(t, 2);"
"	float2 t3 = pow(t, 3);"
"	float2 w0 = 1/18.+t2*5/6.-t3*7/18.-t/2.;"
"	float2 w1 = 8/9.+t3*7/6.-t2*2;"
"	float2 w2 = 1/18.+t2*1.5+t/2.-t3*7/6.;"
"	float2 w3 = t3*7/18.-t2/3.;"

	// original pixels
"	sp(M0, -1, -1) sp(M1, -1, 0) sp(M2, -1, 1) sp(M3, -1, 2)"
"	sp(L0, 0, -1) sp(L1, 0, 0) sp(L2, 0, 1) sp(L3, 0, 2)"
"	sp(K0, 1, -1) sp(K1, 1, 0) sp(K2, 1, 1) sp(K3, 1, 2)"
"	sp(J0, 2, -1) sp(J1, 2, 0) sp(J2, 2, 1) sp(J3, 2, 2)"

	// vertical interpolation
"	float4 Q0 = M0*w0.y+M1*w1.y+M2*w2.y+M3*w3.y;"
"	float4 Q1 = L0*w0.y+L1*w1.y+L2*w2.y+L3*w3.y;"
"	float4 Q2 = K0*w0.y+K1*w1.y+K2*w2.y+K3*w3.y;"
"	float4 Q3 = J0*w0.y+J1*w1.y+J2*w2.y+J3*w3.y;"

"	return Q0*w0.x+Q1*w1.x+Q2*w2.x+Q3*w3.x;" // horizontal interpolation and output
"}"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.x);"
"	float2 pos = tex-float2(t, 0.);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"

	// calculate weights
"	float t2 = t*t, t3 = t*t2;"
"	float4 w0123 = float4(1., 16., 1., 0.)/18.+float4(-.5, 0., .5, 0.)*t+float4(5., -12., 9., -2.)/6.*t2+float4(-7., 21., -21., 7.)/18.*t3;"

"	return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.y);"
"	float2 pos = tex-float2(0., t);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"

	// calculate weights
"	float t2 = t*t, t3 = t*t2;"
"	float4 w0123 = float4(1., 16., 1., 0.)/18.+float4(-.5, 0., .5, 0.)*t+float4(5., -12., 9., -2.)/6.*t2+float4(-7., 21., -21., 7.)/18.*t3;"

"	return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}";

// Catmull-Rom spline4
char const shader_resizer_catmull4[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"#define sp(a, b, c) float4 a = tex2D(s0, frac(tex+dxdy*float2(b, c)));\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float2 t = frac(tex);" // calculate the difference between the output pixel and the original surrounding two pixels
"	tex = (tex-t+.5)*dxdy;" // adjust sampling matrix to put the ouput pixel in the interval [Q1, Q2)
	// weights
"	float2 t2 = pow(t, 2);"
"	float2 t3 = pow(t, 3);"
"	float2 w0 = t2-(t3+t)/2.;"
"	float2 w1 = t3*1.5+1.-t2*2.5;"
"	float2 w2 = t2*2+t/2.-t3*1.5;"
"	float2 w3 = (t3-t2)/2.;"

	// original pixels
"	sp(M0, -1, -1) sp(M1, -1, 0) sp(M2, -1, 1) sp(M3, -1, 2)"
"	sp(L0, 0, -1) sp(L1, 0, 0) sp(L2, 0, 1) sp(L3, 0, 2)"
"	sp(K0, 1, -1) sp(K1, 1, 0) sp(K2, 1, 1) sp(K3, 1, 2)"
"	sp(J0, 2, -1) sp(J1, 2, 0) sp(J2, 2, 1) sp(J3, 2, 2)"

	// vertical interpolation
"	float4 Q0 = M0*w0.y+M1*w1.y+M2*w2.y+M3*w3.y;"
"	float4 Q1 = L0*w0.y+L1*w1.y+L2*w2.y+L3*w3.y;"
"	float4 Q2 = K0*w0.y+K1*w1.y+K2*w2.y+K3*w3.y;"
"	float4 Q3 = J0*w0.y+J1*w1.y+J2*w2.y+J3*w3.y;"
"	return Q0*w0.x+Q1*w1.x+Q2*w2.x+Q3*w3.x;" // horizontal interpolation and output
"}"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.x);"
"	float2 pos = tex-float2(t, 0.);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"

	// calculate weights
"	float t2 = t*t, t3 = t*t2;"
"	float4 w0123 = float4(-.5, 0., .5, 0.)*t+float4(1., -2.5, 2., -.5)*t2+float4(-.5, 1.5, -1.5, .5)*t3;"
"	w0123.y += 1.;"

"	return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.y);"
"	float2 pos = tex-float2(0., t);"
	// original pixels
"	float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
"	float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
"	float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"

	// calculate weights
"	float t2 = t*t, t3 = t*t2;"
"	float4 w0123 = float4(-.5, 0., .5, 0.)*t+float4(1., -2.5, 2., -.5)*t2+float4(-.5, 1.5, -1.5, .5)*t3;"
"	w0123.y += 1.;"

"	return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}";

// compensated Lanczos2
char const shader_resizer_lanczos2[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

// compensated Lanczos2
"#define PI acos(-1.)\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.x);"
"	float2 pos = tex-float2(t, 0.);"

"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);" // nearest original pixel to the left
"	if(t) {"
// original pixels
"		float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
"		float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
"		float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"
"		float4 wset = float3(0., 1., 2.).yxyz+float2(t, -t).xxyy;"
"		float4 w = sin(wset*PI)*sin(wset*PI*.5)/(wset*wset*PI*PI*.5);"

"		float wc = 1.-dot(1., w);" // compensate truncated window factor by bilinear factoring on the two nearest samples
"		w.y += wc*(1.-t);"
"		w.z += wc*t;"
"		return w.x*Q0+w.y*Q1+w.z*Q2+w.w*Q3;}" // interpolation output

"	return Q1;" // case t == 0. is required to return sample Q1, because of a possible division by 0.
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
"	float t = frac(tex.y);"
"	float2 pos = tex-float2(0., t);"

"	float4 Q1 = tex2D(s0, (pos+.5)*dxdy);" // nearest original pixel to the top
"	if(t) {"
// original pixels
"		float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
"		float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
"		float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"
"		float4 wset = float3(0., 1., 2.).yxyz+float2(t, -t).xxyy;"
"		float4 w = sin(wset*PI)*sin(wset*PI*.5)/(wset*wset*PI*PI*.5);"

"		float wc = 1.-dot(1., w);" // compensate truncated window factor by bilinear factoring on the two nearest samples
"		w.y += wc*(1.-t);"
"		w.z += wc*t;"
"		return w.x*Q0+w.y*Q1+w.z*Q2+w.w*Q3;}" // interpolation output

"	return Q1;" // case t == 0. is required to return sample Q1, because of a possible division by 0.
"}";

// downscaling (experimental)
char const shader_resizer_downscaling[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#define MAXSTEPS 32\n"
"#else\n"
"#define MAXSTEPS 9\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"
"float rx :    register(c1);"
"float ry :    register(c2);"

//static const int kx = clamp(int(ceil(rx)), 2, MAXSTEPS);
//static const int ky = clamp(int(ceil(ry)), 2, MAXSTEPS);
"static const int kx = clamp(int(rx+0.5), 2, MAXSTEPS);"
"static const int ky = clamp(int(ry+0.5), 2, MAXSTEPS);"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR0"
"{"
"	float t = frac(tex.x);"
"	float2 pos = tex - float2(t, 0.);"

"	float4 result = 0;"
"	int start = kx / 2 - kx;"
"	for (int i = 0; i < kx; i++) {"
"		result = result + tex2D(s0, (pos + float2(start+i+0.5, .5))*dxdy) / kx;"
"	}"

"	return result;"
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR0"
"{"
"	float t = frac(tex.y);"
"	float2 pos = tex-float2(0., t);"

"	float4 result = 0;"
"	int start = ky / 2 - ky;"
"	for (int i = 0; i < ky; i++) {"
"		result = result + tex2D(s0, (pos + float2(.5, start+i+0.5))*dxdy) / ky;"
"	}"

"	return result;"
"}";

// Final pass
char const shader_final[] =
// #define QUANTIZATION 255.0 or 1023 (Maximum quantized integer value)
// #define LUT3D_ENABLED 0 or 1
// #define LUT3D_SIZE ...
"sampler image : register(s0);"
"sampler ditherMatrix : register(s1);"
"float2 ditherMatrixCoordScale : register(c0);\n"

"#if LUT3D_ENABLED\n"
"sampler lut3D : register(s2);"

// 3D LUT texture coordinate scale and offset required for correct linear interpolation
"static const float LUT3D_SCALE = (LUT3D_SIZE - 1.0f) / LUT3D_SIZE;"
"static const float LUT3D_OFFSET = 1.0f / (2.0f * LUT3D_SIZE);\n"
"#endif\n"

"float4 main(float2 imageCoord : TEXCOORD0) : COLOR {"
"	float4 pixel = tex2D(image, imageCoord);\n"

"#if LUT3D_ENABLED\n"
"	pixel = tex3D(lut3D, pixel.rgb * LUT3D_SCALE + LUT3D_OFFSET);\n"
"#endif\n"

"	float4 ditherValue = tex2D(ditherMatrix, imageCoord * ditherMatrixCoordScale);"
"	return floor(pixel * QUANTIZATION + ditherValue) / QUANTIZATION;"
"}";
