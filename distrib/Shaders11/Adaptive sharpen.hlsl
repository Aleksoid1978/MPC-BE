// $MinimumShaderProfile: ps_4_0

// Copyright (c) 2015, bacondither
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer
//    in this position and unchanged.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Adaptive sharpen - version 2015-06-25 modified for ps_4_0
// Tuned for use post resize, EXPECTS FULL RANGE GAMMA LIGHT

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float  px;
	float  py;
	float2 wh;
	uint   counter;
	float  clock;
};

#define curve_height    1.0                  // Main sharpening strength, POSITIVE VALUE ONLY!
                                             // 0.3 <-> 1.5 is a reasonable range of values

#define video_level_out false                // True to preserve BTB & WTW (minor summation error)
                                             // Normally it should be set to false

// Defined values under this row are "optimal" DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING!

#define curveslope      (curve_height*1.5)   // Sharpening curve slope, edge region
#define D_overshoot     0.016                // Max dark overshoot before max compression
#define D_comp_ratio    0.250                // Max compression ratio, dark overshoot (1/0.25=4x)
#define L_overshoot     0.004                // Max light overshoot before max compression
#define L_comp_ratio    0.167                // Max compression ratio, light overshoot (1/0.167=6x)
#define max_scale_lim   10.0                 // Abs change before max compression (1/10=±10%)

// Colour to greyscale, fast approx gamma
float CtG(float3 RGB) { return  sqrt( (1.0/3.0)*((RGB*RGB).r + (RGB*RGB).g + (RGB*RGB).b) ); }

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	// Get points and saturate out of range values (BTB & WTW)
	// [                c22               ]
	// [           c24, c9,  c23          ]
	// [      c21, c1,  c2,  c3, c18      ]
	// [ c19, c10, c4,  c0,  c5, c11, c16 ]
	// [      c20, c6,  c7,  c8, c17      ]
	// [           c15, c12, c14          ]
	// [                c13               ]
	float3 c19 = saturate( tex.Sample(samp, coord + float2(-3*px,   0)).rgb );
	float3 c21 = saturate( tex.Sample(samp, coord + float2(-2*px,  -py)).rgb );
	float3 c10 = saturate( tex.Sample(samp, coord + float2(-2*px,   0)).rgb );
	float3 c20 = saturate( tex.Sample(samp, coord + float2(-2*px,   py)).rgb );
	float3 c24 = saturate( tex.Sample(samp, coord + float2(  -px,-2*py)).rgb );
	float3 c1  = saturate( tex.Sample(samp, coord + float2(  -px,  -py)).rgb );
	float3 c4  = saturate( tex.Sample(samp, coord + float2(  -px,   0)).rgb );
	float3 c6  = saturate( tex.Sample(samp, coord + float2(  -px,   py)).rgb );
	float3 c15 = saturate( tex.Sample(samp, coord + float2(  -px, 2*py)).rgb );
	float3 c22 = saturate( tex.Sample(samp, coord + float2(   0, -3*py)).rgb );
	float3 c9  = saturate( tex.Sample(samp, coord + float2(   0, -2*py)).rgb );
	float3 c2  = saturate( tex.Sample(samp, coord + float2(   0,   -py)).rgb );
	float3 c0  = saturate( tex.Sample(samp, coord).rgb );
	float3 c7  = saturate( tex.Sample(samp, coord + float2(   0,    py)).rgb );
	float3 c12 = saturate( tex.Sample(samp, coord + float2(   0,  2*py)).rgb );
	float3 c13 = saturate( tex.Sample(samp, coord + float2(   0,  3*py)).rgb );
	float3 c23 = saturate( tex.Sample(samp, coord + float2(   px,-2*py)).rgb );
	float3 c3  = saturate( tex.Sample(samp, coord + float2(   px,  -py)).rgb );
	float3 c5  = saturate( tex.Sample(samp, coord + float2(   px,   0)).rgb );
	float3 c8  = saturate( tex.Sample(samp, coord + float2(   px,   py)).rgb );
	float3 c14 = saturate( tex.Sample(samp, coord + float2(   px, 2*py)).rgb );
	float3 c18 = saturate( tex.Sample(samp, coord + float2( 2*px,  -py)).rgb );
	float3 c11 = saturate( tex.Sample(samp, coord + float2( 2*px,   0)).rgb );
	float3 c17 = saturate( tex.Sample(samp, coord + float2( 2*px,   py)).rgb );
	float3 c16 = saturate( tex.Sample(samp, coord + float2( 3*px,   0)).rgb );

	// Blur, gauss 3x3
	float3 blur   = (2*(c2 + c4 + c5 + c7) + (c1 + c3 + c6 +c8) + 4*c0)/16;
	float  blur_Y = (blur.r*(1.0/3.0) + blur.g*(1.0/3.0) + blur.b*(1.0/3.0));

	// Edge detection
	// Matrix, relative weights
	// [           1          ]
	// [       4,  4,  4      ]
	// [   1,  4,  4,  4,  1  ]
	// [       4,  4,  4      ]
	// [           1          ]
	float edge = length( abs(blur-c0) + abs(blur-c1) + abs(blur-c2) + abs(blur-c3)
	                   + abs(blur-c4) + abs(blur-c5) + abs(blur-c6) + abs(blur-c7) + abs(blur-c8)
	                   + 0.25*(abs(blur-c9) + abs(blur-c10) + abs(blur-c11) + abs(blur-c12)) )*(1.0/3.0);

	// Edge detect contrast compression, center = 0.5
	edge *= min((0.8+2.7*pow(2, (-7.4*blur_Y))), 3.2);

	// RGB to greyscale
	float c0_Y = CtG(c0);

	float kernel[25] = { c0_Y,  CtG(c1), CtG(c2), CtG(c3), CtG(c4), CtG(c5), CtG(c6), CtG(c7), CtG(c8),
	                     CtG(c9), CtG(c10), CtG(c11), CtG(c12), CtG(c13), CtG(c14), CtG(c15), CtG(c16),
	                     CtG(c17), CtG(c18), CtG(c19), CtG(c20), CtG(c21), CtG(c22), CtG(c23), CtG(c24) };

	// Partial laplacian outer pixel weighting scheme
	float mdiff_c0  = 0.03 + 4*( abs(kernel[0]-kernel[2]) + abs(kernel[0]-kernel[4])
	                           + abs(kernel[0]-kernel[5]) + abs(kernel[0]-kernel[7])
	                           + 0.25*(abs(kernel[0]-kernel[1]) + abs(kernel[0]-kernel[3])
	                                  +abs(kernel[0]-kernel[6]) + abs(kernel[0]-kernel[8])) );

	float mdiff_c9  = ( abs(kernel[9]-kernel[2])   + abs(kernel[9]-kernel[24])
	                  + abs(kernel[9]-kernel[23])  + abs(kernel[9]-kernel[22])
	                  + 0.5*(abs(kernel[9]-kernel[1]) + abs(kernel[9]-kernel[3])) );

	float mdiff_c10 = ( abs(kernel[10]-kernel[20]) + abs(kernel[10]-kernel[19])
	                  + abs(kernel[10]-kernel[21]) + abs(kernel[10]-kernel[4])
	                  + 0.5*(abs(kernel[10]-kernel[1]) + abs(kernel[10]-kernel[6])) );

	float mdiff_c11 = ( abs(kernel[11]-kernel[17]) + abs(kernel[11]-kernel[5])
	                  + abs(kernel[11]-kernel[18]) + abs(kernel[11]-kernel[16])
	                  + 0.5*(abs(kernel[11]-kernel[3]) + abs(kernel[11]-kernel[8])) );

	float mdiff_c12 = ( abs(kernel[12]-kernel[13]) + abs(kernel[12]-kernel[15])
	                  + abs(kernel[12]-kernel[7])  + abs(kernel[12]-kernel[14])
	                  + 0.5*(abs(kernel[12]-kernel[6]) + abs(kernel[12]-kernel[8])) );

	float4 weights  = float4( (min((mdiff_c0/mdiff_c9), 2)), (min((mdiff_c0/mdiff_c10),2)),
	                          (min((mdiff_c0/mdiff_c11),2)), (min((mdiff_c0/mdiff_c12),2)) );

	// Negative laplace matrix
	// Matrix, relative weights, *Varying 0<->8
	// [          8*         ]
	// [      4,  1,  4      ]
	// [  8*, 1,      1,  8* ]
	// [      4,  1,  4      ]
	// [          8*         ]
	float neg_laplace = ( 0.25 * (kernel[2] + kernel[4] + kernel[5] + kernel[7])
	                      +      (kernel[1] + kernel[3] + kernel[6] + kernel[8])
	                      +      ((kernel[9]*weights.x) + (kernel[10]*weights.y)
	                      +      (kernel[11]*weights.z) + (kernel[12]*weights.w)) )
	                      /      (5 + weights.x + weights.y + weights.z + weights.w);

	// Compute sharpening magnitude function, x = edge mag, y = laplace operator mag
	float sharpen_val =  0.01 + (curve_height/(curveslope*pow(edge, 3.5) + 0.5))
	                          - (curve_height/(8192*pow((edge*2.2), 4.5) + 0.5));

	// Calculate sharpening diff and scale
	float sharpdiff = (c0_Y - neg_laplace)*(sharpen_val*0.8);

	// Calculate local near min & max, partial cocktail sort (No branching!)
	[unroll] // ps_4_0+ does not unroll without this
	for (int i = 0; i < 2; ++i)
	{
		for (int i1 = 1+i; i1 < 25-i; ++i1)
		{
			float temp   = kernel[i1-1];
			kernel[i1-1] = min(kernel[i1-1], kernel[i1]);
			kernel[i1]   = max(temp, kernel[i1]);
		}

		for (int i2 = 23-i; i2 > i; --i2)
		{
			float temp   = kernel[i2-1];
			kernel[i2-1] = min(kernel[i2-1], kernel[i2]);
			kernel[i2]   = max(temp, kernel[i2]);
		}
	}

	float nmax = max(((kernel[23] + kernel[24])/2), c0_Y);
	float nmin = min(((kernel[0]  + kernel[1])/2),  c0_Y);

	// Calculate tanh scale factor, pos/neg
	float nmax_scale = max((1/((nmax - c0_Y) + L_overshoot)), max_scale_lim);
	float nmin_scale = max((1/((c0_Y - nmin) + D_overshoot)), max_scale_lim);

	// Soft limit sharpening with tanh, lerp to control maximum compression
	sharpdiff = lerp( (tanh((max(sharpdiff, 0))*nmax_scale)/nmax_scale), (max(sharpdiff, 0)), L_comp_ratio )
	          + lerp( (tanh((min(sharpdiff, 0))*nmin_scale)/nmin_scale), (min(sharpdiff, 0)), D_comp_ratio );

	// Set the alpha channel to 1 to prevent strange effects in MPDN
	if (video_level_out == true) { return float4( (tex.Sample(samp, coord) + sharpdiff).rgb, 1 ); }

	return float4( (c0 + sharpdiff), 1 );
}
