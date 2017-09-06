#pragma once

#define IMAGING_TRANSFORM_BOX      1
#define IMAGING_TRANSFORM_BILINEAR 2
#define IMAGING_TRANSFORM_HAMMING  3
#define IMAGING_TRANSFORM_BICUBIC  4
#define IMAGING_TRANSFORM_LANCZOS  5

HRESULT ResampleARGB(BYTE* const dest, const int destW, const int destH, const BYTE* const src, const int srcW, const int srcH, const int filter);
