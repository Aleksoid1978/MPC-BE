/*
 * (C) 2006-2017 see Authors.txt
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

#include <d3d9.h>
#include <dxva2api.h>

class CMPCVideoDecFilter;
struct AVFrame;

#define DXVA2_MAX_SURFACES 64

class CDXVA2Decoder
{
	GUID                          m_guidDecoder;
	CMPCVideoDecFilter*           m_pFilter;
	DXVA2_ConfigPictureDecode     m_DXVA2Config;
	CComPtr<IDirectXVideoDecoder> m_pDirectXVideoDec;
	LPDIRECT3DSURFACE9            m_pD3DSurface[DXVA2_MAX_SURFACES];
	UINT                          m_nNumSurfaces;

public :
	CDXVA2Decoder(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config, LPDIRECT3DSURFACE9* ppD3DSurface, UINT numSurfaces);
	virtual                       ~CDXVA2Decoder() {}

	HRESULT                       DeliverFrame();
	void                          FillHWContext();

	int                           get_buffer_dxva(struct AVFrame *pic);
	static void                   release_buffer_dxva(void *opaque, uint8_t *data);
};
