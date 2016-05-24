/*
 * (C) 2014-2016 see Authors.txt.
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
#include <MMReg.h>
#include "AudioHelper.h"

#define limit(a, x, b) if (x < a) { x = a; } else if (x > b) { x = b; }

SampleFormat GetSampleFormat(const WAVEFORMATEX* wfe)
{
	SampleFormat sample_format = SAMPLE_FMT_NONE;

	const WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
	WORD tag = wfe->wFormatTag;

	if (tag == WAVE_FORMAT_PCM || (tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)) {
		switch (wfe->wBitsPerSample) {
		case 8:
			sample_format = SAMPLE_FMT_U8;
			break;
		case 16:
			sample_format = SAMPLE_FMT_S16;
			break;
		case 24:
			sample_format = SAMPLE_FMT_S24;
			break;
		case 32:
			sample_format = SAMPLE_FMT_S32;
			break;
		}
	} else if (tag == WAVE_FORMAT_IEEE_FLOAT || (tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
		switch (wfe->wBitsPerSample) {
		case 32:
			sample_format = SAMPLE_FMT_FLT;
			break;
		case 64:
			sample_format = SAMPLE_FMT_DBL;
			break;
		}
	}

	return sample_format;
}

HRESULT convert_to_int16(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, int16_t* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_uint8_to_int16(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			memcpy(pOut, pIn, allsamples * sizeof(int16_t));
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = *(int16_t*)(pIn + 1); // read the high bits only
				pIn += 3;
			}
			break;
		case SAMPLE_FMT_S32:
			convert_int32_to_int16(pOut, (int32_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_FLT:
			convert_float_to_int16(pOut, (float*)pIn, allsamples);
			break;
		case SAMPLE_FMT_DBL:
			convert_double_to_int16(pOut, (double*)pIn, allsamples);
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				uint8_t* p = (uint8_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_uint8_to_int16(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				int16_t* p = (int16_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = p[nSamples * ch];
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				int32_t* p = (int32_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int32_to_int16(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				float* p = (float*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_float_to_int16(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				double* p = (double*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_double_to_int16(p[nSamples * ch]);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_int24(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, BYTE* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = 0;
				*pOut++ = 0;
				*pOut++ = (*pIn++) ^ 0x80;
			}
			break;
		case SAMPLE_FMT_S16:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = 0;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
			}
			break;
		case SAMPLE_FMT_S24:
			memcpy(pOut, pIn, allsamples * 3);
			break;
		case SAMPLE_FMT_S32:
			for (size_t i = 0; i < allsamples; ++i) {
				pIn++;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
			}
			break;
		case SAMPLE_FMT_FLT:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = (double)(*(float*)pIn);
				limit(-1, d, D32MAX);
				uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
				*pOut++ = (BYTE)(u32 >> 8);
				*pOut++ = (BYTE)(u32 >> 16);
				*pOut++ = (BYTE)(u32 >> 24);
				pIn += sizeof(float);
			}
			break;
		case SAMPLE_FMT_DBL:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = *(double*)pIn;
				limit(-1, d, D32MAX);
				uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
				*pOut++ = (BYTE)(u32 >> 8);
				*pOut++ = (BYTE)(u32 >> 16);
				*pOut++ = (BYTE)(u32 >> 24);
				pIn += sizeof(double);
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = 0;
					*pOut++ = 0;
					*pOut++ = pIn[nSamples * ch + i] ^ 0x80;
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					uint16_t u16 = ((uint16_t*)pIn)[nSamples * ch + i];
					*pOut++ = 0;
					*pOut++ = (BYTE)(u16);
					*pOut++ = (BYTE)(u16 >> 8);
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					uint32_t u32 = ((uint32_t*)pIn)[nSamples * ch + i];
					*pOut++ = (BYTE)(u32 >> 8);
					*pOut++ = (BYTE)(u32 >> 16);
					*pOut++ = (BYTE)(u32 >> 24);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = (double)((float*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
					*pOut++ = (BYTE)(u32 >> 8);
					*pOut++ = (BYTE)(u32 >> 16);
					*pOut++ = (BYTE)(u32 >> 24);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = ((double*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
					*pOut++ = (BYTE)(u32 >> 8);
					*pOut++ = (BYTE)(u32 >> 16);
					*pOut++ = (BYTE)(u32 >> 24);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_int32(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, int32_t* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_uint8_to_int32(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			convert_int16_to_int32(pOut, (int16_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				pOut[i] = (uint32_t)pIn[3 * i]     << 8  |
				          (uint32_t)pIn[3 * i + 1] << 16 |
				          (uint32_t)pIn[3 * i + 2] << 24;
			}
			break;
		case SAMPLE_FMT_S32:
			memcpy(pOut, pIn, nSamples * nChannels * sizeof(int32_t));
			break;
		case SAMPLE_FMT_FLT:
			convert_float_to_int32(pOut, (float*)pIn, allsamples);
			break;
		case SAMPLE_FMT_DBL:
			convert_double_to_int32(pOut, (double*)pIn, allsamples);
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				uint8_t* p = (uint8_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_uint8_to_int32(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				int16_t* p = (int16_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int16_to_int32(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				int32_t* p = (int32_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = p[nSamples * ch];
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				float* p = (float*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_float_to_int32(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				double* p = (double*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_double_to_int32(p[nSamples * ch]);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_float(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, float* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_uint8_to_float(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			convert_int16_to_float(pOut, (int16_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				int32_t i32 = 0;
				BYTE* p = (BYTE*)(&i32);
				p[1] = *(pIn++);
				p[2] = *(pIn++);
				p[3] = *(pIn++);
				i32 >>= 8;
				*pOut++ = (float)((double)i32 / INT24_PEAK);
			}
			break;
		case SAMPLE_FMT_S32:
			convert_int32_to_float(pOut, (int32_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_FLT:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBL:
			convert_double_to_float(pOut, (double*)pIn, allsamples);
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				uint8_t* p = (uint8_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_uint8_to_float(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				int16_t* p = (int16_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int16_to_float(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				int32_t* p = (int32_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int32_to_float(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				float* p = (float*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = p[nSamples * ch];
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				double* p = (double*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = (float)p[nSamples * ch];
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_planar_float(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, float* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (int ch = 0; ch < nChannels; ++ch) {
				uint8_t* p = (uint8_t*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = SAMPLE_uint8_to_float(p[nChannels * i]);
				}
			}
			break;
		case SAMPLE_FMT_S16:
			for (int ch = 0; ch < nChannels; ++ch) {
				int16_t* p = (int16_t*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = SAMPLE_int16_to_float(p[nChannels * i]);
				}
			}
			break;
		case SAMPLE_FMT_S32:
			for (int ch = 0; ch < nChannels; ++ch) {
				int32_t* p = (int32_t*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = SAMPLE_int32_to_float(p[nChannels * i]);
				}
			}
			break;
		case SAMPLE_FMT_FLT:
			for (int ch = 0; ch < nChannels; ++ch) {
				float* p = (float*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = p[nChannels * i];
				}
			}
			break;
		case SAMPLE_FMT_DBL:
			for (int ch = 0; ch < nChannels; ++ch) {
				double* p = (double*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = (float)p[nChannels * i];
				}
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			convert_uint8_to_float(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16P:
			convert_int16_to_float(pOut, (int16_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S32P:
			convert_int32_to_float(pOut, (int32_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_FLTP:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBLP:
			convert_double_to_float(pOut, (double*)pIn, allsamples);
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_float_to(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, float* pIn, BYTE* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_float_to_uint8((uint8_t*)pOut, pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			convert_float_to_int16((int16_t*)pOut, pIn, allsamples);
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = (double)(*pIn++);
				limit(-1, d, D32MAX);
				uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
				*pOut++ = (BYTE)(u32 >> 8);
				*pOut++ = (BYTE)(u32 >> 16);
				*pOut++ = (BYTE)(u32 >> 24);
			}
			break;
		case SAMPLE_FMT_S32:
			convert_float_to_int32((int32_t*)pOut, pIn, allsamples);
			break;
		case SAMPLE_FMT_FLT:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBL:
			convert_float_to_double((double*)pOut, pIn, allsamples);
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}
