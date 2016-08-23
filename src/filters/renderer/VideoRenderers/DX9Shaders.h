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


// Two-pass resizers

// Bicubic
char const shader_resizer_bicubic_2pass[] =
// "#define A -0.6, -0.8 or -1.0
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy :   register(c0);"

"static float4x4 tco = {"
	"0, A, -2*A, A,"
	"1, 0, -A-3, A+2,"
	"0, -A, 2*A+3, -A-2,"
	"0, 0, A, -A"
"};"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.x);"
	"float2 pos = tex-float2(t, 0.);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"

	"return mul(mul(tco, float4(1., t, t*t, t*t*t)), float4x4(Q0, Q1, Q2, Q3));"
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.y);"
	"float2 pos = tex-float2(0., t);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"

	"return mul(mul(tco, float4(1., t, t*t, t*t*t)), float4x4(Q0, Q1, Q2, Q3));"
"}";

// B-spline4
char const shader_resizer_bspline4_2pass[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.x);"
	"float2 pos = tex - float2(t, 0.);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos + float2(-.5, .5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos + .5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos + float2(1.5, .5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos + float2(2.5, .5))*dxdy);"

	// calculate weights
	"float t2 = t*t, t3 = t*t2;"
	"float4 w0123 = float4(1., 4., 1., 0.) / 6. + float4(-.5, 0., .5, 0.)*t + float4(.5, -1., .5, 0.)*t2 + float4(-1., 3., -3., 1.) / 6.*t3;"

	"return w0123.x*Q0 + w0123.y*Q1 + w0123.z*Q2 + w0123.w*Q3;" // interpolation output
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.y);"
	"float2 pos = tex - float2(0., t);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos + float2(.5, -.5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos + .5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos + float2(.5, 1.5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos + float2(.5, 2.5))*dxdy);"

	// calculate weights
	"float t2 = t*t, t3 = t*t2;"
	"float4 w0123 = float4(1., 4., 1., 0.) / 6. + float4(-.5, 0., .5, 0.)*t + float4(.5, -1., .5, 0.)*t2 + float4(-1., 3., -3., 1.) / 6.*t3;"

	"return w0123.x*Q0 + w0123.y*Q1 + w0123.z*Q2 + w0123.w*Q3;" // interpolation output
"}";

// Mitchell-Netravali spline4
char const shader_resizer_mitchell4_2pass[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"#define sp(a, b, c) float4 a = tex2D(s0, tex+dxdy*float2(b, c));\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.x);"
	"float2 pos = tex-float2(t, 0.);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"

	// calculate weights
	"float t2 = t*t, t3 = t*t2;"
	"float4 w0123 = float4(1., 16., 1., 0.)/18.+float4(-.5, 0., .5, 0.)*t+float4(5., -12., 9., -2.)/6.*t2+float4(-7., 21., -21., 7.)/18.*t3;"

	"return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.y);"
	"float2 pos = tex-float2(0., t);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"

	// calculate weights
	"float t2 = t*t, t3 = t*t2;"
	"float4 w0123 = float4(1., 16., 1., 0.)/18.+float4(-.5, 0., .5, 0.)*t+float4(5., -12., 9., -2.)/6.*t2+float4(-7., 21., -21., 7.)/18.*t3;"

	"return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}";

// Catmull-Rom spline4
char const shader_resizer_catmull4_2pass[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.x);"
	"float2 pos = tex-float2(t, 0.);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"

	// calculate weights
	"float t2 = t*t, t3 = t*t2;"
	"float4 w0123 = float4(-.5, 0., .5, 0.)*t+float4(1., -2.5, 2., -.5)*t2+float4(-.5, 1.5, -1.5, .5)*t3;"
	"w0123.y += 1.;"

	"return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.y);"
	"float2 pos = tex-float2(0., t);"
	// original pixels
	"float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);"
	"float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
	"float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"

	// calculate weights
	"float t2 = t*t, t3 = t*t2;"
	"float4 w0123 = float4(-.5, 0., .5, 0.)*t+float4(1., -2.5, 2., -.5)*t2+float4(-.5, 1.5, -1.5, .5)*t3;"
	"w0123.y += 1.;"

	"return w0123.x*Q0+w0123.y*Q1+w0123.z*Q2+w0123.w*Q3;" // interpolation output
"}";

// compensated Lanczos2
char const shader_resizer_lanczos2_2pass[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"#define PI acos(-1.)\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.x);"
	"float2 pos = tex-float2(t, 0.);"

	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);" // nearest original pixel to the left
	"if(t) {"
// original pixels
		"float4 Q0 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
		"float4 Q2 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
		"float4 Q3 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"
		"float4 wset = float3(0., 1., 2.).yxyz+float2(t, -t).xxyy;"
		"float4 w = sin(wset*PI)*sin(wset*PI*.5)/(wset*wset*PI*PI*.5);"

		"float wc = 1.-dot(1., w);" // compensate truncated window factor by bilinear factoring on the two nearest samples
		"w.y += wc*(1.-t);"
		"w.z += wc*t;"
		"return w.x*Q0+w.y*Q1+w.z*Q2+w.w*Q3;}" // interpolation output

	"return Q1;" // case t == 0. is required to return sample Q1, because of a possible division by 0.
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.y);"
	"float2 pos = tex-float2(0., t);"

	"float4 Q1 = tex2D(s0, (pos+.5)*dxdy);" // nearest original pixel to the top
	"if(t) {"
// original pixels
		"float4 Q0 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
		"float4 Q2 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
		"float4 Q3 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"
		"float4 wset = float3(0., 1., 2.).yxyz+float2(t, -t).xxyy;"
		"float4 w = sin(wset*PI)*sin(wset*PI*.5)/(wset*wset*PI*PI*.5);"

		"float wc = 1.-dot(1., w);" // compensate truncated window factor by bilinear factoring on the two nearest samples
		"w.y += wc*(1.-t);"
		"w.z += wc*t;"
		"return w.x*Q0+w.y*Q1+w.z*Q2+w.w*Q3;}" // interpolation output

	"return Q1;" // case t == 0. is required to return sample Q1, because of a possible division by 0.
"}";

// downscaling
char const shader_resizer_downscaling_2pass[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#define MAXSTEPS 16\n"
"#else\n"
"#define MAXSTEPS 9\n"
"#endif\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);"
"float rx : register(c1);"
"float ry : register(c2);"

"static const int kx = clamp(int(rx+0.5), 2, MAXSTEPS);"
"static const int ky = clamp(int(ry+0.5), 2, MAXSTEPS);"
"static const int kxy = kx*ky;"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR0"
"{"
	"float t = frac(tex.x);"
	"float2 pos = tex - float2(t, 0.);"

	"float4 result = 0;"
	"int start = kx / 2 - kx;"
	"for (int i = 0; i < kx; i++) {"
		"result = result + tex2D(s0, (pos + float2(start+i+0.5, .5))*dxdy) / kx;"
	"}"

	"return result;"
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR0"
"{"
	"float t = frac(tex.y);"
	"float2 pos = tex-float2(0., t);"

	"float4 result = 0;"
	"int start = ky / 2 - ky;"
	"for (int i = 0; i < ky; i++) {"
		"result = result + tex2D(s0, (pos + float2(.5, start+i+0.5))*dxdy) / ky;"
	"}"

	"return result;"
"}";

// compensated Lanczos3
char const shader_resizer_lanczos3_2pass[] =
"#if Ml\n"
"#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))\n"
"#endif\n"

"#define PI acos(-1.)\n"

"sampler s0 : register(s0);"
"float2 dxdy : register(c0);\n"

"float4 main_x(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.x);"
	"float2 pos = tex-float2(t, 0.);"

	"float4 Q2 = tex2D(s0, (pos+.5)*dxdy);" // nearest original pixel to the left
	"if(!t) return Q2;"// case t == 0. is required to return sample Q2, because of a possible division by 0.
	"else {"
		// original pixels
		"float4 Q0 = tex2D(s0, (pos+float2(-1.5, .5))*dxdy);"
		"float4 Q1 = tex2D(s0, (pos+float2(-.5, .5))*dxdy);"
		"float4 Q3 = tex2D(s0, (pos+float2(1.5, .5))*dxdy);"
		"float4 Q4 = tex2D(s0, (pos+float2(2.5, .5))*dxdy);"
		"float4 Q5 = tex2D(s0, (pos+float2(3.5, .5))*dxdy);"
		"float3 wset0 = float3(2., 1., 0.)*PI+t*PI;"
		"float3 wset1 = float3(1., 2., 3.)*PI-t*PI;"
		"float3 wset0s = wset0*.5;"
		"float3 wset1s = wset1*.5;"
		"float3 w0 = sin(wset0)*sin(wset0s)/(wset0*wset0s);"
		"float3 w1 = sin(wset1)*sin(wset1s)/(wset1*wset1s);"

		"float wc = 1.-dot(1., w0+w1);"// compensate truncated window factor by linear factoring on the two nearest samples
		"w0.z += wc*(1.-t);"
		"w1.x += wc*t;"
		"return w0.x*Q0+w0.y*Q1+w0.z*Q2+w1.x*Q3+w1.y*Q4+w1.z*Q5;}"// interpolation output
"}"

"float4 main_y(float2 tex : TEXCOORD0) : COLOR"
"{"
	"float t = frac(tex.y);"
	"float2 pos = tex-float2(0., t);"

	"float4 Q2 = tex2D(s0, (pos+.5)*dxdy);"// nearest original pixel to the top
	"if(!t) return Q2;"// case t == 0. is required to return sample Q2, because of a possible division by 0.
	"else {"
		// original pixels
		"float4 Q0 = tex2D(s0, (pos+float2(.5, -1.5))*dxdy);"
		"float4 Q1 = tex2D(s0, (pos+float2(.5, -.5))*dxdy);"
		"float4 Q3 = tex2D(s0, (pos+float2(.5, 1.5))*dxdy);"
		"float4 Q4 = tex2D(s0, (pos+float2(.5, 2.5))*dxdy);"
		"float4 Q5 = tex2D(s0, (pos+float2(.5, 3.5))*dxdy);"
		"float3 wset0 = float3(2., 1., 0.)*PI+t*PI;"
		"float3 wset1 = float3(1., 2., 3.)*PI-t*PI;"
		"float3 wset0s = wset0*.5;"
		"float3 wset1s = wset1*.5;"
		"float3 w0 = sin(wset0)*sin(wset0s)/(wset0*wset0s);"
		"float3 w1 = sin(wset1)*sin(wset1s)/(wset1*wset1s);"

		"float wc = 1.-dot(1., w0+w1);"// compensate truncated window factor by linear factoring on the two nearest samples
		"w0.z += wc*(1.-t);"
		"w1.x += wc*t;"
		"return w0.x*Q0+w0.y*Q1+w0.z*Q2+w1.x*Q3+w1.y*Q4+w1.z*Q5;}"// interpolation output
"}";
