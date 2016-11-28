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
#include <moreuuids.h>
#include <mpc_defines.h>
#include "DXVA2DecoderH264.h"
#include "../MPCVideoDec.h"
#include "../FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVA2DecoderH264::CDXVA2DecoderH264(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVA2Decoder(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config, 4)
{
	if (m_pFilter->GetPCIVendor() == PCIV_Intel && m_guidDecoder == DXVA_Intel_H264_ClearVideo) {
		m_dxva_context.workaround	= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
	} else if (IsATIUVD(m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice())) {
		m_dxva_context.workaround	= FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;
	}

	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	m_dxva_context.dxva_decoder_context = &m_DXVA_Context;
}

HRESULT CDXVA2DecoderH264::CopyBitstream(BYTE* pDXVABuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
{
	DXVA_H264_Picture_Context *ctx_pic	= &m_DXVA_Context.ctx_pic[m_ctx_pic_num];
	DXVA_Slice_H264_Short *slice		= NULL;
	BYTE* current						= pDXVABuffer;
	UINT MBCount						= FFGetMBCount(m_pFilter->GetAVCtx());

	static const BYTE start_code[]		= { 0, 0, 1 };
	static const UINT start_code_size	= sizeof(start_code);

	for (unsigned i = 0; i < ctx_pic->slice_count; i++) {
		slice							= m_dxva_context.longslice ? (DXVA_Slice_H264_Short*)&ctx_pic->slice_long[i] : &ctx_pic->slice_short[i];
		UINT position					= slice->BSNALunitDataLocation;
		UINT size						= slice->SliceBytesInBuffer;

		slice->BSNALunitDataLocation	= current - pDXVABuffer;
		slice->SliceBytesInBuffer		= start_code_size + size;

		if (m_dxva_context.longslice) {
			DXVA_Slice_H264_Long *slice_long = (DXVA_Slice_H264_Long*)slice;
			if (i < ctx_pic->slice_count - 1) {
				slice_long->NumMbsForSlice = slice_long[1].first_mb_in_slice - slice_long[0].first_mb_in_slice;
			} else {
				slice_long->NumMbsForSlice = MBCount - slice_long->first_mb_in_slice;
			}
		}

		if ((current - pDXVABuffer) + (start_code_size + size) > nDXVASize) {
			nSize = 0;
			return E_FAIL;
		}

		memcpy(current, start_code, start_code_size);
		current += start_code_size;

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

		slice->SliceBytesInBuffer += padding;
	}

	return S_OK;
}

HRESULT CDXVA2DecoderH264::ProcessDXVAFrame(IMediaSample* pSample)
{
	HRESULT hr = S_OK;

	for (UINT i = 0; i < m_DXVA_Context.ctx_pic_count; i++) {
		DXVA_H264_Picture_Context* ctx_pic = &m_DXVA_Context.ctx_pic[i];

		if (!ctx_pic->slice_count) {
			continue;
		}

		m_ctx_pic_num = i;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(pSample));
		// Send picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PicParams_H264), &ctx_pic->pp));
		// Send quantization matrix
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), &ctx_pic->qm));
		// Send bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType));
		// Send slice control
		if (m_dxva_context.longslice) {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Long) * ctx_pic->slice_count, ctx_pic->slice_long));
		} else {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short) * ctx_pic->slice_count, ctx_pic->slice_short));
		}
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame());
	}

	return hr;
}
