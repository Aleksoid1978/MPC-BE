/*
* (C) 2017 see Authors.txt
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

#include "stdafx.h"
#include <math.h>
#include <ppl.h>
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

static filter_t BOX      = { box_filter,      0.5 };
static filter_t BILINEAR = { bilinear_filter, 1.0 };
static filter_t HAMMING  = { hamming_filter,  1.0 };
static filter_t BICUBIC  = { bicubic_filter,  2.0 };
static filter_t LANCZOS  = { lanczos_filter,  3.0 };

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
	filterscale = scale = (double)inSize / outSize;
	if (filterscale < 1.0) {
		filterscale = 1.0;
	}

	// determine support size (length of resampling filter)
	support = filterp->support * filterscale;

	// maximum number of coeffs
	kmax = (int)ceil(support) * 2 + 1;

	// check for overflow
	if (outSize > INT_MAX / (kmax * (int)sizeof(double))) {
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

void CResampleARGB::ResampleHorizontal(BYTE* dest, int destW, int H, const BYTE* const src, int srcW)
{
	concurrency::parallel_for(0, H, [&](int yy) {
		const BYTE* lineIn = src + yy * srcW * 4;
		BYTE* const lineOut = dest + yy * destW * 4;

		int ss0, ss1, ss2, ss3;
		for (int xx = 0; xx < destW; xx++) {
			const INT32* k = &m_kkHor[xx * m_kmaxHor];
			const int xmin = m_xboundsHor[xx * 2 + 0];
			const int xmax = m_xboundsHor[xx * 2 + 1];
			ss0 = ss1 = ss2 = ss3 = 1 << (PRECISION_BITS -1);

			for (int x = 0; x < xmax; x++) {
				ss0 += lineIn[(x + xmin)*4 + 0] * k[x];
				ss1 += lineIn[(x + xmin)*4 + 1] * k[x];
				ss2 += lineIn[(x + xmin)*4 + 2] * k[x];
				ss3 += lineIn[(x + xmin)*4 + 3] * k[x];
			}

			((UINT32*)lineOut)[xx] = MAKE_UINT32(clip8(ss0), clip8(ss1), clip8(ss2), clip8(ss3));
		}
	});
}

void CResampleARGB::ResampleVertical(BYTE* dest, int W, int destH, const BYTE* const src, int srcH)
{
	concurrency::parallel_for(0, destH, [&](int yy) {
		BYTE* const lineOut = dest + yy * W * 4;
		const INT32* k = &m_kkVer[yy * m_kmaxVer];
		const int ymin = m_xboundsVer[yy * 2 + 0];
		const int ymax = m_xboundsVer[yy * 2 + 1];
		int ss0, ss1, ss2, ss3;
		for (int xx = 0; xx < W; xx++) {
			ss0 = ss1 = ss2 = ss3 = 1 << (PRECISION_BITS -1);
			for (int y = 0; y < ymax; y++) {
				const BYTE* lineIn = src + (y + ymin) * W * 4;
				ss0 += lineIn[xx*4 + 0] * k[y];
				ss1 += lineIn[xx*4 + 1] * k[y];
				ss2 += lineIn[xx*4 + 2] * k[y];
				ss3 += lineIn[xx*4 + 3] * k[y];
			}

			((UINT32*)lineOut)[xx] = MAKE_UINT32(clip8(ss0), clip8(ss1), clip8(ss2), clip8(ss3));
		}
	});
}

void CResampleARGB::FreeData()
{
	free(m_pTemp);
	m_pTemp = nullptr;

	free(m_xboundsHor);
	m_xboundsHor = nullptr;
	free(m_kkHor);
	m_kkHor  = nullptr;

	free(m_xboundsVer);
	m_xboundsVer = nullptr;
	free(m_kkVer);
	m_kkVer = nullptr;
}

HRESULT CResampleARGB::Init()
{
	FreeData();

	if (m_srcW <= 0 || m_srcH <= 0 || m_destW <= 0 || m_destH <= 0) {
		return E_INVALIDARG;
	}

	switch (m_filter) {
	case FILTER_BOX:      m_pFilter = &BOX;      break;
	case FILTER_BILINEAR: m_pFilter = &BILINEAR; break;
	case FILTER_HAMMING:  m_pFilter = &HAMMING;  break;
	case FILTER_BICUBIC:  m_pFilter = &BICUBIC;  break;
	case FILTER_LANCZOS:  m_pFilter = &LANCZOS;  break;
	default:
		return E_INVALIDARG;
	}

	m_bResampleHor = (m_srcW != m_destW);
	m_bResampleVer = (m_srcH != m_destH);

	if (m_bResampleHor && m_bResampleVer) {
		m_pTemp = (BYTE*)malloc(m_destW * m_srcH * 4);
		if (!m_pTemp) {
			FreeData();
			return E_OUTOFMEMORY;
		}
	}

	if (m_bResampleHor) {
		double *prekk;
		m_kmaxHor = precompute_coeffs(m_srcW, m_destW, m_pFilter, &m_xboundsHor, &prekk);
		if (!m_kmaxHor) {
			FreeData();
			return E_OUTOFMEMORY;
		}
		m_kmaxHor = normalize_coeffs_8bpc(m_destW, m_kmaxHor, prekk, &m_kkHor);
		free(prekk);
		if (!m_kmaxHor) {
			FreeData();
			return E_OUTOFMEMORY;
		}
	}

	if (m_bResampleVer) {
		double *prekk;
		m_kmaxVer = precompute_coeffs(m_srcH, m_destH, m_pFilter, &m_xboundsVer, &prekk);
		if (!m_kmaxVer) {
			FreeData();
			return E_OUTOFMEMORY;
		}
		m_kmaxVer = normalize_coeffs_8bpc(m_destH, m_kmaxVer, prekk, &m_kkVer);
		free(prekk);
		if (!m_kmaxVer) {
			FreeData();
			return E_OUTOFMEMORY;
		}
	}

	m_actual = true;

	return S_OK;
}

CResampleARGB::~CResampleARGB()
{
	FreeData();
}

HRESULT CResampleARGB::SetParameters(const int destW, const int destH, const int srcW, const int srcH, const int filter)
{
	if (m_actual && m_srcW == srcW && m_srcH == srcH && m_destW == destW && m_destH == destH && m_filter == filter) {
		return S_OK;
	}

	m_actual = false;
	m_srcW   = srcW;
	m_srcH   = srcH;
	m_destW  = destW;
	m_destH  = destH;
	m_filter = filter;

	return Init();
}

HRESULT CResampleARGB::Process(BYTE* const dest, const BYTE* const src)
{
	if (!m_actual) {
		return E_ABORT;
	}

	if (m_srcW == m_destW && m_srcH == m_destH) {
		memcpy((BYTE*)dest, src, m_srcW * m_srcH * 4);

		return S_OK;
	}

	// two-pass resize, first pass
	if (m_bResampleHor) {
		ResampleHorizontal(m_pTemp ? m_pTemp : dest, m_destW, m_srcH, src, m_srcW);
	}

	// second pass
	if (m_bResampleVer) {
		// can be the original image or horizontally resampled one
		ResampleVertical(dest, m_destW, m_destH, m_pTemp ? m_pTemp : src, m_srcH);
	}

	return S_OK;
}
