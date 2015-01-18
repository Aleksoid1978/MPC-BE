/*
 * (C) 2006-2014 see Authors.txt
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

#include "Rasterizer.h"

struct HDMV_PALETTE {
	BYTE entry_id	= 0;
	BYTE Y			= 0;
	BYTE Cr			= 0;
	BYTE Cb			= 0;
	BYTE T			= 0; // HDMV rule : 0 transparent, 255 opaque (compatible DirectX)
};

class CGolombBuffer;

class CompositionObject : Rasterizer
{
public :
	SHORT			m_object_id_ref					= 0;
	SHORT			m_window_id_ref					= 0;
	bool			m_object_cropped_flag			= false;
	bool			m_forced_on_flag				= false;
	BYTE			m_version_number				= 0;

	SHORT			m_horizontal_position			= 0;
	SHORT			m_vertical_position				= 0;
	SHORT			m_width							= 0;
	SHORT			m_height						= 0;

	SHORT			m_cropping_horizontal_position	= 0;
	SHORT			m_cropping_vertical_position	= 0;
	SHORT			m_cropping_width				= 0;
	SHORT			m_cropping_height				= 0;

	SHORT			m_compositionNumber				= -1;

	REFERENCE_TIME	m_rtStart						= INVALID_TIME;
	REFERENCE_TIME	m_rtStop						= INVALID_TIME;

	CompositionObject();
	~CompositionObject();

	void				SetRLEData(const BYTE* pBuffer, int nSize, int nTotalSize);
	void				AppendRLEData(const BYTE* pBuffer, int nSize);
	int					GetRLEDataSize() { return m_nRLEDataSize; };
	const BYTE*			GetRLEData() { return m_pRLEData; };
	bool				IsRLEComplete() { return m_nRLEPos >= m_nRLEDataSize; };

	void				RenderHdmv(SubPicDesc& spd, SubPicDesc* spdResized);
	void				RenderDvb(SubPicDesc& spd, SHORT nX, SHORT nY, SubPicDesc* spdResized);
	void				RenderXSUB(SubPicDesc& spd);

	void				SetPalette(int nNbEntry, HDMV_PALETTE* pPalette, bool bIsHD, bool bIsRGB = false);
	const bool			HavePalette() { return m_nColorNumber > 0; };

	CompositionObject* Copy() {
		CompositionObject* pCompositionObject = DNew CompositionObject(*this);
		if (m_pRLEData) {
			pCompositionObject->m_pRLEData = NULL;
			pCompositionObject->SetRLEData(m_pRLEData, m_nRLEDataSize, m_nRLEDataSize);
		}

		return pCompositionObject;
	}

private :
	BYTE*	m_pRLEData		= NULL;
	int		m_nRLEDataSize	= 0;
	int		m_nRLEPos		= 0;
	int		m_nColorNumber	= 0;
	DWORD	m_Colors[256];

	void	DvbRenderField(SubPicDesc& spd, CGolombBuffer& gb, SHORT nXStart, SHORT nYStart, SHORT nLength);
	void	Dvb2PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
	void	Dvb4PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
	void	Dvb8PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
};
