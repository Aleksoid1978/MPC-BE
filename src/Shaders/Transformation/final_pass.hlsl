
#ifndef QUANTIZATION
	// 255 or 1023 (Maximum quantized integer value)
	#define QUANTIZATION 255
#endif
#ifndef LUT3D_SIZE
	#define LUT3D_SIZE 64
#endif

sampler image : register(s0);

#if DITHER_ENABLED
sampler ditherMatrix : register(s1);
float2 ditherMatrixCoordScale : register(c0);
#endif

#if LUT3D_ENABLED
sampler lut3D : register(s2);
// 3D LUT texture coordinate scale and offset required for correct linear interpolation
static const float LUT3D_SCALE = (LUT3D_SIZE - 1.0f) / LUT3D_SIZE;
static const float LUT3D_OFFSET = 1.0f / (2.0f * LUT3D_SIZE);
#endif

float4 main(float2 imageCoord : TEXCOORD0) : COLOR
{
	float4 pixel = tex2D(image, imageCoord);

#if LUT3D_ENABLED
	pixel = tex3D(lut3D, pixel.rgb * LUT3D_SCALE + LUT3D_OFFSET);
#endif

#if DITHER_ENABLED
	float4 ditherValue = tex2D(ditherMatrix, imageCoord * ditherMatrixCoordScale);
	pixel = floor(pixel * QUANTIZATION + ditherValue) / QUANTIZATION;
#endif

	return pixel;
};
