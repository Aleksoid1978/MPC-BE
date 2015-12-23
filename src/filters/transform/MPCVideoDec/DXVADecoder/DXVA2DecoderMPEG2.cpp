/*
 * (C) 2006-2015 see Authors.txt
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
#include "DXVA2DecoderMPEG2.h"
#include "../MPCVideoDec.h"
#include "../FfmpegContext.h"

CDXVA2DecoderMPEG2::CDXVA2DecoderMPEG2(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVA2Decoder(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config, 4)
{
	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	m_dxva_context.dxva_decoder_context = &m_DXVA_Context;
}

HRESULT CDXVA2DecoderMPEG2::CopyBitstream(BYTE* pDXVABuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
{
	DXVA_MPEG2_Picture_Context *ctx_pic	= &m_DXVA_Context.ctx_pic[m_nFieldNum];
	BYTE* current						= pDXVABuffer;
	UINT MBCount						= FFGetMBCount(m_pFilter->GetAVCtx());

	for (unsigned i = 0; i < ctx_pic->slice_count; i++) {
		DXVA_SliceInfo *slice	= &ctx_pic->slice[i];
		UINT position			= slice->dwSliceDataLocation;
		UINT size				= slice->dwSliceBitsInBuffer / 8;

		slice->dwSliceDataLocation = current - pDXVABuffer;

		if (i < ctx_pic->slice_count - 1) {
			slice->wNumberMBsInSlice =
				slice[1].wNumberMBsInSlice - slice[0].wNumberMBsInSlice;
		} else {
			slice->wNumberMBsInSlice =
				MBCount - slice[0].wNumberMBsInSlice;
		}

		if ((current - pDXVABuffer) + size > nDXVASize) {
			nSize = 0;
			return E_FAIL;
		}

		memcpy(current, &ctx_pic->bitstream[position], size);
		current += size;
	}

	nSize = current - pDXVABuffer;

	return S_OK;
}

HRESULT CDXVA2DecoderMPEG2::ProcessDXVAFrame(IMediaSample* pSample)
{
	HRESULT hr = S_OK;

	for (UINT i = 0; i < m_DXVA_Context.frame_count; i++) {
		DXVA_MPEG2_Picture_Context* ctx_pic = &m_DXVA_Context.ctx_pic[i];

		if (!ctx_pic->slice_count) {
			continue;
		}

		m_nFieldNum = i;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(pSample));
		// Send picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PictureParameters), &ctx_pic->pp));
		// Send quantization matrix
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_QmatrixData), &ctx_pic->qm));
		// Send bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType));
		// Send slice control
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_SliceInfo) * ctx_pic->slice_count, ctx_pic->slice));
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame());
	}

	return hr;
}
