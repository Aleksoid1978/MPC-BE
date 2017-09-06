#include "stdafx.h"
#include <math.h>
#include "ResampleARGB.h"

// based on https://github.com/uploadcare/pillow-simd/blob/3.4.x/libImaging/Resample.c
// and use same fixes from https://github.com/uploadcare/pillow-simd/blob/4.3-demo/libImaging/Resample.c

#define MAKE_UINT32(u0, u1, u2, u3) (u0 | (u1<<8) | (u2<<16) | (u3<<24))

struct filter_t {
	double (*pfilter)(double x);
	double support;
};

static inline double box_filter(double x)
{
	if (x >= -0.5 && x < 0.5)
		return 1.0;
	return 0.0;
}

static inline double bilinear_filter(double x)
{
	if (x < 0.0)
		x = -x;
	if (x < 1.0)
		return 1.0-x;
	return 0.0;
}

static inline double hamming_filter(double x)
{
	if (x < 0.0)
		x = -x;
	if (x == 0.0)
		return 1.0;
	if (x >= 1.0)
		return 0.0;
	x = x * M_PI;
	return sin(x) / x * (0.54f + 0.46f * cos(x));
}

static inline double bicubic_filter(double x)
{
	// https://en.wikipedia.org/wiki/Bicubic_interpolation#Bicubic_convolution_algorithm
#define a -0.5
	if (x < 0.0)
		x = -x;
	if (x < 1.0)
		return ((a + 2.0) * x - (a + 3.0)) * x*x + 1;
	if (x < 2.0)
		return (((x - 5) * x + 8) * x - 4) * a;
	return 0.0;
#undef a
}

static inline double sinc_filter(double x)
{
	if (x == 0.0)
		return 1.0;
	x = x * M_PI;
	return sin(x) / x;
}

static inline double lanczos_filter(double x)
{
	// truncated sinc
	if (-3.0 <= x && x < 3.0)
		return sinc_filter(x) * sinc_filter(x/3);
	return 0.0;
}

static struct filter_t BOX      = { box_filter,      0.5 };
static struct filter_t BILINEAR = { bilinear_filter, 1.0 };
static struct filter_t HAMMING  = { hamming_filter,  1.0 };
static struct filter_t BICUBIC  = { bicubic_filter,  2.0 };
static struct filter_t LANCZOS  = { lanczos_filter,  3.0 };

// 8 bits for result. Filter can have negative areas.
// In one cases the sum of the coefficients will be negative,
// in the other it will be more than 1.0. That is why we need
// two extra bits for overflow and int type.
#define PRECISION_BITS (32 - 8 - 2)

UINT8 _lookups[512] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

UINT8 *lookups = &_lookups[128];

static inline UINT8 clip8(int in)
{
	return lookups[in >> PRECISION_BITS];
}

int precompute_coeffs(int inSize, int outSize, filter_t* filterp, int **xboundsp, double **kkp)
{
	double support, scale, filterscale;
	double center, ww, ss;
	int xx, x, kmax, xmin, xmax;
	int *xbounds;
	double *kk, *k;

	// prepare for horizontal stretch
	filterscale = scale = (double) inSize / outSize;
	if (filterscale < 1.0) {
		filterscale = 1.0;
	}

	// determine support size (length of resampling filter)
	support = filterp->support * filterscale;

	// maximum number of coeffs
	kmax = (int) ceil(support) * 2 + 1;

	// check for overflow
	if (outSize > INT_MAX / (kmax * sizeof(double))) {
		return 0;
	}

	// coefficient buffer
	// malloc check ok, overflow checked above
	kk = (double*)malloc(outSize * kmax * sizeof(double));
	if (!kk) {
		return 0;
	}

	// malloc check ok, kmax*sizeof(double) > 2*sizeof(int)
	xbounds = (int*)malloc(outSize * 2 * sizeof(int));
	if (!xbounds) {
		free(kk);
		return 0;
	}

	for (xx = 0; xx < outSize; xx++) {
		center = (xx + 0.5) * scale;
		ww = 0.0;
		ss = 1.0 / filterscale;
		xmin = (int)floor(center - support);
		if (xmin < 0)
			xmin = 0;
		xmax = (int)ceil(center + support);
		if (xmax > inSize)
			xmax = inSize;
		xmax -= xmin;
		k = &kk[xx * kmax];
		for (x = 0; x < xmax; x++) {
			double w = filterp->pfilter((x + xmin - center + 0.5) * ss);
			k[x] = w;
			ww += w;

			// We can skip extreme coefficients if they are zeroes.
			if (w == 0) {
				// Skip from the start.
				if (x == 0) {
					// At next loop `x` will be 0.
					x -= 1;
					// But `w` will not be 0, because it based on `xmin`.
					xmin += 1;
					xmax -= 1;
				} else if (x == xmax - 1) {
					// Truncate the last coefficient for current `xx`.
					xmax -= 1;
				}
			}
		}
		for (x = 0; x < xmax; x++) {
			if (ww != 0.0)
				k[x] /= ww;
		}
		// Remaining values should stay empty if they are used despite of xmax.
		for (; x < kmax; x++) {
			k[x] = 0;
		}
		xbounds[xx * 2 + 0] = xmin;
		xbounds[xx * 2 + 1] = xmax;
	}
	*xboundsp = xbounds;
	*kkp = kk;
	return kmax;
}

int normalize_coeffs_8bpc(int outSize, int kmax, double *prekk, INT32 **kkp)
{
	int x;
	INT32 *kk;

	// malloc check ok, overflow checked in precompute_coeffs
	kk = (INT32*)malloc(outSize * kmax * sizeof(INT32));
	if (!kk) {
		return 0;
	}

	for (x = 0; x < outSize * kmax; x++) {
		if (prekk[x] < 0) {
			kk[x] = (int) (-0.5 + prekk[x] * (1 << PRECISION_BITS));
		} else {
			kk[x] = (int) (0.5 + prekk[x] * (1 << PRECISION_BITS));
		}
	}

	*kkp = kk;
	return kmax;
}

HRESULT ResampleHorizontal(BYTE* dest, int destW, int H, const BYTE* const src, int srcW, filter_t* filterp)
{
	int ss0, ss1, ss2, ss3;
	int xx, yy, x, kmax, xmin, xmax;
	int *xbounds;
	INT32 *k, *kk;
	double *prekk;

	kmax = precompute_coeffs(srcW, destW, filterp, &xbounds, &prekk);
	if (!kmax) {
		return E_OUTOFMEMORY;
	}

	kmax = normalize_coeffs_8bpc(destW, kmax, prekk, &kk);
	free(prekk);
	if ( ! kmax) {
		free(xbounds);
		return E_OUTOFMEMORY;
	}

	for (yy = 0; yy < H; yy++) {
		const BYTE* lineIn = src + yy * srcW * 4;
		BYTE* const lineOut = dest + yy * destW * 4;

		for (xx = 0; xx < destW; xx++) {
			xmin = xbounds[xx * 2 + 0];
			xmax = xbounds[xx * 2 + 1];
			k = &kk[xx * kmax];
			ss0 = ss1 = ss2 = ss3 = 1 << (PRECISION_BITS -1);

			for (x = 0; x < xmax; x++) {
				ss0 += lineIn[(x + xmin)*4 + 0] * k[x];
				ss1 += lineIn[(x + xmin)*4 + 1] * k[x];
				ss2 += lineIn[(x + xmin)*4 + 2] * k[x];
				ss3 += lineIn[(x + xmin)*4 + 3] * k[x];
			}

			((UINT32*)lineOut)[xx] = MAKE_UINT32(clip8(ss0), clip8(ss1), clip8(ss2), clip8(ss3));
		}
	}

	free(kk);
	free(xbounds);
	return S_OK;
}

HRESULT ResampleVertical(BYTE* dest, int W, int destH, const BYTE* const src, int srcH, filter_t* filterp)
{
	int ss0, ss1, ss2, ss3;
	int xx, yy, y, kmax, ymin, ymax;
	int *xbounds;
	INT32 *k, *kk;
	double *prekk;

	kmax = precompute_coeffs(srcH, destH, filterp, &xbounds, &prekk);
	if (!kmax) {
		return E_OUTOFMEMORY;
	}

	kmax = normalize_coeffs_8bpc(destH, kmax, prekk, &kk);
	free(prekk);
	if ( ! kmax) {
		free(xbounds);
		return E_OUTOFMEMORY;
	}

	for (yy = 0; yy < destH; yy++) {
		BYTE* const lineOut = dest + yy * W * 4;
		k = &kk[yy * kmax];
		ymin = xbounds[yy * 2 + 0];
		ymax = xbounds[yy * 2 + 1];
		for (xx = 0; xx < W; xx++) {
			ss0 = ss1 = ss2 = ss3 = 1 << (PRECISION_BITS -1);
			for (y = 0; y < ymax; y++) {
				const BYTE* lineIn = src + (y + ymin) * W * 4;
				ss0 += lineIn[xx*4 + 0] * k[y];
				ss1 += lineIn[xx*4 + 1] * k[y];
				ss2 += lineIn[xx*4 + 2] * k[y];
				ss3 += lineIn[xx*4 + 3] * k[y];
			}

			((UINT32*)lineOut)[xx] = MAKE_UINT32(clip8(ss0), clip8(ss1), clip8(ss2), clip8(ss3));
		}
	}

	free(kk);
	free(xbounds);
	return S_OK;
}

HRESULT ResampleARGB(BYTE* const dest, const int destW, const int destH, const BYTE* const src, const int srcW, const int srcH, const int filter)
{
	if (srcW == destW && srcH == destH) {
		memcpy((BYTE*)dest, src, srcW * srcH * 4);

		return S_OK;
	}

	struct filter_t* filterp;

	// check filter
	switch (filter) {
	case IMAGING_TRANSFORM_BOX:
		filterp = &BOX;
		break;
	case IMAGING_TRANSFORM_BILINEAR:
		filterp = &BILINEAR;
		break;
	case IMAGING_TRANSFORM_HAMMING:
		filterp = &HAMMING;
		break;
	case IMAGING_TRANSFORM_BICUBIC:
		filterp = &BICUBIC;
		break;
	case IMAGING_TRANSFORM_LANCZOS:
		filterp = &LANCZOS;
		break;
	default:
		return E_INVALIDARG;
	}

	BYTE* temp = nullptr;
	int tempW = 0;
	int tempH = 0;

	if (srcW != destW && srcH != destH) { // two-pass mode will be used
		tempW = destW;
		tempH = srcH;

		temp = (BYTE*)malloc(tempW * tempH * 4);
		if (!temp) {
			return E_OUTOFMEMORY;
		}
	}

	// two-pass resize, first pass
	if (srcW != destW) {
		HRESULT hr = ResampleHorizontal(temp ? temp : dest, destW, srcH, src, srcW, filterp);
		if (hr != S_OK) {
			free(temp);
			return hr;
		}
	}

	// second pass
	if (srcH != destH) {
		// imIn can be the original image or horizontally resampled one
		HRESULT hr = ResampleVertical(dest, destW, destH, temp ? temp : src, srcH, filterp);
		// it's safe to call free with empty value if there was no previous step.
		free(temp);
		return hr;
	}

	return S_OK;
}
