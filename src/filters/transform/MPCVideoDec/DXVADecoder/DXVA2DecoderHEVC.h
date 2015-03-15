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

#pragma once

#include "DXVA2Decoder.h"
#include <ffmpeg/libavcodec/dxva_hevc.h>

class CDXVA2DecoderHEVC : public CDXVA2Decoder
{
	DXVA_HEVC_Picture_Context	m_DXVA_Picture_Context;

public:
	CDXVA2DecoderHEVC(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config);

	virtual HRESULT				CopyBitstream(BYTE* pDXVABuffer, UINT& nSize, UINT nDXVASize = UINT_MAX);
	virtual HRESULT				ProcessDXVAFrame(IMediaSample* pSample);
};
