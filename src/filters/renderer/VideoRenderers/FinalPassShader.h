/*
 * (C) 2016 see Authors.txt
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
