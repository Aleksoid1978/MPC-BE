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
	DXVA_VC1_Picture_Context *ctx_pic	= &m_DXVA_Context.ctx_pic[m_nFieldNum];
	DXVA_SliceInfo *slice				= &ctx_pic->slice;
	BYTE* current						= pDXVABuffer;

    static const uint8_t start_code[]	= { 0, 0, 1, 0x0d };
    const unsigned start_code_size		= m_pFilter->GetCodec() == AV_CODEC_ID_VC1 ? sizeof(start_code) : 0;
    const unsigned slice_size			= slice->dwSliceBitsInBuffer / 8;
    const unsigned padding				= 128 - ((start_code_size + slice_size) & 127);
	nSize								= start_code_size + slice_size + padding;

	if (nSize > nDXVASize) {
		nSize = 0;
		return E_FAIL;
	}

	if (start_code_size > 0) {
		memcpy(pDXVABuffer, start_code, start_code_size);
		if (m_nFieldNum == 1) {
			pDXVABuffer[3] = 0x0c;
		}
	}
	memcpy(pDXVABuffer + start_code_size, ctx_pic->bitstream + slice->dwSliceDataLocation, slice_size);
	if (padding > 0) {
		memset(pDXVABuffer + start_code_size + slice_size, 0, padding);
	}
	slice->dwSliceBitsInBuffer = 8 * nSize;

	return S_OK;
}

HRESULT	CDXVA2DecoderVC1::ProcessDXVAFrame(IMediaSample* pSample)
{
	HRESULT	hr = S_OK;

	for (UINT i = 0; i < m_DXVA_Context.frame_count; i++) {
		DXVA_VC1_Picture_Context *ctx_pic = &m_DXVA_Context.ctx_pic[i];

		m_nFieldNum = i;

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
