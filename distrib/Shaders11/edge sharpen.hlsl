// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 pxy;
	float  width;
	float  height;
	uint   counter;
	float  clock;
};

#define NbPixel     1
#define Edge_threshold  0.2
#define Sharpen_val0    2.0
#define Sharpen_val1    0.125

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	// size of NbPixel pixels
	float dx = NbPixel / width;
	float dy = NbPixel / height;
	float4 Res = 0;

	// Edge detection using Prewitt operator
	// Get neighbor points
	// [ 1, 2, 3 ]
	// [ 4, 0, 5 ]
	// [ 6, 7, 8 ]
	float4 c0 = tex.Sample(samp, coord);
	float4 c1 = tex.Sample(samp, coord + float2(-dx, -dy));
	float4 c2 = tex.Sample(samp, coord + float2(  0, -dy));
	float4 c3 = tex.Sample(samp, coord + float2( dx, -dy));
	float4 c4 = tex.Sample(samp, coord + float2(-dx,   0));
	float4 c5 = tex.Sample(samp, coord + float2( dx,   0));
	float4 c6 = tex.Sample(samp, coord + float2(-dx,  dy));
	float4 c7 = tex.Sample(samp, coord + float2(  0,  dy));
	float4 c8 = tex.Sample(samp, coord + float2( dx,  dy));

	// Computation of the 3 derived vectors (hor, vert, diag1, diag2)
	float4 delta1 = (c6 + c4 + c1 - c3 - c5 - c8);
	float4 delta2 = (c4 + c1 + c2 - c5 - c8 - c7);
	float4 delta3 = (c1 + c2 + c3 - c8 - c7 - c6);
	float4 delta4 = (c2 + c3 + c5 - c7 - c6 - c4);

	// Computation of the Prewitt operator
	float value = length(abs(delta1) + abs(delta2) + abs(delta3) + abs(delta4)) / 6;

	// If we have an edge (vector length > Edge_threshold) => apply sharpen filter
	if (value > Edge_threshold) {
		Res = c0 * Sharpen_val0 - (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8) * Sharpen_val1;
		// Display edges in red...
		//Res = float4( 1.0, 0.0, 0.0, 0.0 );

		return Res;
	} else {
		return c0;
	}
}
