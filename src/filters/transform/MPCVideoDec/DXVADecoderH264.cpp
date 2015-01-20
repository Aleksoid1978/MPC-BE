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
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"
#include "DXVADecoderH264.h"
#include "MPCVideoDec.h"
#include "DXVAAllocator.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVADecoderH264::CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, guidDecoder, nMode, nPicEntryNumber, pDXVA2Config)
{
	m_bUseLongSlice = (GetDXVA2Config()->ConfigBitstreamRaw != 2);
	Init();
}

CDXVADecoderH264::~CDXVADecoderH264()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264::Destroy()"));
}

void CDXVADecoderH264::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264::Init()"));

	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	m_DXVA_Context.longslice = m_bUseLongSlice;

	Reserved16Bits = 3;
	if (m_pFilter->GetPCIVendor() == PCIV_Intel && m_guidDecoder == DXVA_Intel_H264_ClearVideo) {
		Reserved16Bits				= 0x534c;
		m_DXVA_Context.workaround	= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
	} else if (IsATIUVD(m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice())) {
		Reserved16Bits				= 0;
		m_DXVA_Context.workaround	= FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;
	}

	switch (GetMode()) {
		case H264_VLD :
			AllocExecuteParams(4);
			break;
		default :
			ASSERT(FALSE);
	}

	FFH264SetDxvaParams(m_pFilter->GetAVCtx(), &m_DXVA_Context);

	Flush();
}

void CDXVADecoderH264::Flush()
{
	StatusReportFeedbackNumber = 1;

	__super::Flush();
}

HRESULT CDXVADecoderH264::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
{
	DXVA_H264_Picture_Context *ctx_pic	= &m_DXVA_Context.ctx_pic[m_nFieldNum];
	DXVA_Slice_H264_Short *slice		= NULL;
	BYTE* current						= pDXVABuffer;
	UINT MBCount						= FFGetMBCount(m_pFilter->GetAVCtx());

	for (unsigned i = 0; i < ctx_pic->slice_count; i++) {
		static const BYTE start_code[]		= { 0, 0, 1 };
		static const UINT start_code_size	= sizeof(start_code);

		slice = m_bUseLongSlice ? (DXVA_Slice_H264_Short*)&ctx_pic->slice_long[i] : &ctx_pic->slice_short[i];

		UINT position					= slice->BSNALunitDataLocation;
		UINT size						= slice->SliceBytesInBuffer;

		slice->BSNALunitDataLocation	= current - pDXVABuffer;
		slice->SliceBytesInBuffer		= start_code_size + size;

		if (m_bUseLongSlice) {
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

HRESULT CDXVADecoderH264::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT	hr			= S_FALSE;
	int		got_picture	= 0;

	CHECK_HR_FALSE (FFDecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(),
								  pDataIn, nSize, rtStart, rtStop,
								  &got_picture));

	if (m_nSurfaceIndex == -1 || !m_DXVA_Context.ctx_pic[0].slice_count) {
		return S_FALSE;
	}

	for (UINT i = 0; i < _countof(m_DXVA_Context.ctx_pic); i++) {
		DXVA_H264_Picture_Context* ctx_pic = &m_DXVA_Context.ctx_pic[i];

		if (!ctx_pic->slice_count) {
			continue;
		}

		m_nFieldNum = i;

		ctx_pic->pp.Reserved16Bits				= Reserved16Bits;
		ctx_pic->pp.StatusReportFeedbackNumber	= StatusReportFeedbackNumber++;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));
		// Add picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PicParams_H264), &ctx_pic->pp));
		// Add quantization matrix
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), &ctx_pic->qm));
		// Add bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType));
		// Add slice control
		if (m_bUseLongSlice) {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Long) * ctx_pic->slice_count, ctx_pic->slice_long));
		} else {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short) * ctx_pic->slice_count, ctx_pic->slice_short));
		}
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));
	}

	if (got_picture) {
		AddToStore(m_nSurfaceIndex, m_pSampleToDeliver);
		hr = DisplayNextFrame();
	}

	return hr;
}
