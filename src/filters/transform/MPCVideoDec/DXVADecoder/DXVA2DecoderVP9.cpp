/*
 * (C) 2015 see Authors.txt
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
#include "DXVA2DecoderVP9.h"
#include "../MPCVideoDec.h"

CDXVA2DecoderVP9::CDXVA2DecoderVP9(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVA2Decoder(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config, 4)
{
	memset(&m_DXVA_VP9_Picture_Context, 0, sizeof(m_DXVA_VP9_Picture_Context));
	m_dxva_context.dxva_decoder_context = &m_DXVA_VP9_Picture_Context;
}

HRESULT CDXVA2DecoderVP9::CopyBitstream(BYTE* pDXVABuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
{
	DXVA_VP9_Picture_Context *ctx_pic	= &m_DXVA_VP9_Picture_Context;
	const UINT padding					= 128 - ((ctx_pic->slice.SliceBytesInBuffer) & 127);
	nSize								= ctx_pic->slice.SliceBytesInBuffer + padding;

	if (nSize > nDXVASize) {
		nSize = 0;
		return E_FAIL;
	}

    memcpy(pDXVABuffer, ctx_pic->bitstream, ctx_pic->slice.SliceBytesInBuffer);
    if (padding > 0) {
        memset(pDXVABuffer + ctx_pic->slice.SliceBytesInBuffer, 0, padding);
        ctx_pic->slice.SliceBytesInBuffer += padding;
    }

	return S_OK;
}

HRESULT CDXVA2DecoderVP9::ProcessDXVAFrame(IMediaSample* pSample)
{
	HRESULT hr = S_OK;

	if (m_DXVA_VP9_Picture_Context.slice.SliceBytesInBuffer) {
		DXVA_VP9_Picture_Context *ctx_pic = &m_DXVA_VP9_Picture_Context;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(pSample));
		// Send picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PicParams_VP9), &ctx_pic->pp));
		// Send bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType));
		// Send slice control
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_VPx_Short), &ctx_pic->slice));
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame());

		ZeroMemory(&m_DXVA_VP9_Picture_Context, sizeof(m_DXVA_VP9_Picture_Context));
	}

	return hr;
}
