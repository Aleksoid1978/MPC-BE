/*
 * (C) 2006-2016 see Authors.txt
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
#include "DXVA2DecoderVC1.h"
#include "../MPCVideoDec.h"
#include "../FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVA2DecoderVC1::CDXVA2DecoderVC1(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVA2Decoder(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config, 3)
{
	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	m_dxva_context.dxva_decoder_context = &m_DXVA_Context;
}

HRESULT CDXVA2DecoderVC1::CopyBitstream(BYTE* pDXVABuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
{
	DXVA_VC1_Picture_Context *ctx_pic = &m_DXVA_Context.ctx_pic[m_ctx_pic_num];
	DXVA_SliceInfo *slice             = NULL;
	BYTE* current                     = pDXVABuffer;
	UINT MBCount                      = FFGetMBCount(m_pFilter->GetAVCtx());

	static const BYTE start_code[]    = { 0, 0, 1, 0x0d };
	const UINT start_code_size        = m_pFilter->GetCodec() == AV_CODEC_ID_VC1 ? sizeof(start_code) : 0;

	for (unsigned i = 0; i < ctx_pic->slice_count; i++) {
		slice         = &ctx_pic->slice[i];
		UINT position = slice->dwSliceDataLocation;
		UINT size     = slice->dwSliceBitsInBuffer / 8;

		if ((current - pDXVABuffer) + size + start_code_size > nDXVASize) {
			nSize = 0;
			return E_FAIL;
		}
		slice->dwSliceDataLocation = current - pDXVABuffer;

		if (i < ctx_pic->slice_count - 1) {
			slice->wNumberMBsInSlice =
				slice[1].wNumberMBsInSlice - slice[0].wNumberMBsInSlice;
		} else {
			slice->wNumberMBsInSlice =
				MBCount - slice[0].wNumberMBsInSlice;
		}

		if (start_code_size) {
			memcpy(current, start_code, start_code_size);
			if (i == 0 && m_ctx_pic_num == 1) {
				current[3] = 0x0c;
			} else if (i > 0) {
				current[3] = 0x0b;
			}

			current += start_code_size;
			slice->dwSliceBitsInBuffer += start_code_size * 8;
		}

		memcpy(current, &ctx_pic->bitstream[position], size);
		current += size;
	}

	nSize = current - pDXVABuffer;
	if (slice && nSize % 128) {
		UINT padding = 128 - (nSize % 128);

		if (nSize + padding > nDXVASize) {
			nSize = 0;
			return E_FAIL;
		}

		memset(current, 0, padding);
		nSize += padding;

		slice->dwSliceBitsInBuffer += padding * 8;
	}

	return S_OK;
}

HRESULT	CDXVA2DecoderVC1::ProcessDXVAFrame(IMediaSample* pSample)
{
	HRESULT	hr = S_OK;

	for (UINT i = 0; i < m_DXVA_Context.ctx_pic_count; i++) {
		DXVA_VC1_Picture_Context *ctx_pic = &m_DXVA_Context.ctx_pic[i];

		m_ctx_pic_num = i;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(pSample));
		// Send picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PictureParameters), &ctx_pic->pp));
		// Send bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType));
		// Send slice control
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_SliceInfo), &ctx_pic->slice));
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame());
	}

	return hr;
}
