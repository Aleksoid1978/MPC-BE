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

#include "stdafx.h"
#include "CompositionObject.h"
#include "ColorConvert.h"
#include "../DSUtil/GolombBuffer.h"
#include <d3d9types.h>

CompositionObject::CompositionObject()
{
	memsetd(m_Colors, 0, sizeof(m_Colors));
}

CompositionObject::~CompositionObject()
{
	SAFE_DELETE_ARRAY(m_pRLEData);
}

void CompositionObject::SetPalette(int nNbEntry, HDMV_PALETTE* pPalette, bool bRec709, ColorConvert::convertType type/* = ColorConvert::convertType::DEFAULT*/, bool bIsRGB/* = false*/)
{
	m_nColorNumber = nNbEntry;
	for (int i = 0; i < nNbEntry; i++) {
		if (bIsRGB) {
			m_Colors[pPalette[i].entry_id] = D3DCOLOR_ARGB(pPalette[i].T, pPalette[i].Y, pPalette[i].Cr, pPalette[i].Cb);
		} else {
			m_Colors[pPalette[i].entry_id] = ColorConvert::YCrCbToRGB(pPalette[i].T, pPalette[i].Y, pPalette[i].Cr, pPalette[i].Cb, bRec709, type);
		}
	}
}

void CompositionObject::SetRLEData(const BYTE* pBuffer, int nSize, int nTotalSize)
{
	SAFE_DELETE_ARRAY(m_pRLEData);

	m_pRLEData		= DNew BYTE[nTotalSize];
	m_nRLEDataSize	= nTotalSize;
	m_nRLEPos		= std::min(nSize, nTotalSize);

	memcpy(m_pRLEData, pBuffer, std::min(nSize, nTotalSize));
}

void CompositionObject::AppendRLEData(const BYTE* pBuffer, int nSize)
{
	//ASSERT (m_nRLEPos+nSize <= m_nRLEDataSize);
	if (m_nRLEPos + nSize <= m_nRLEDataSize) {
		memcpy(m_pRLEData + m_nRLEPos, pBuffer, nSize);
		m_nRLEPos += nSize;
	}
}

void CompositionObject::RenderHdmv(SubPicDesc& spd, SubPicDesc* spdResized)
{
	if (!m_pRLEData || !m_nColorNumber) {
		return;
	}

	CGolombBuffer	GBuffer (m_pRLEData, m_nRLEDataSize);
	BYTE			bTemp;
	BYTE			bSwitch;

	BYTE			nPaletteIndex = 0;
	SHORT			nCount;
	SHORT			nX	= m_horizontal_position;
	SHORT			nY	= m_vertical_position;

	while ((nY < (m_vertical_position + m_height)) && !GBuffer.IsEOF()) {
		bTemp = GBuffer.ReadByte();

		nPaletteIndex = bTemp;
		nCount		  = 1;

		if (bTemp == 0x00) {
			bSwitch	= GBuffer.ReadByte();
			nCount	= bSwitch & 0x3f;
			if (bSwitch & 0x40) {
				nCount = (nCount << 8) + GBuffer.ReadByte();
			}
			nPaletteIndex = bSwitch & 0x80 ? GBuffer.ReadByte() : 0;
		}

		if (nCount > 0) {
			if (nPaletteIndex != 0xFF) {	// Fully transparent (section 9.14.4.2.2.1.1)
				FillSolidRect(spdResized ? *spdResized : spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
			}
			nX += nCount;
		} else {
			nY++;
			nX = m_horizontal_position;
		}
	}
}

void CompositionObject::RenderDvb(SubPicDesc& spd, SHORT nX, SHORT nY, SubPicDesc* spdResized)
{
	if (!m_pRLEData) {
		return;
	}

	CGolombBuffer	gb(m_pRLEData, m_nRLEDataSize);
	SHORT			sTopFieldLength;
	SHORT			sBottomFieldLength;

	sTopFieldLength		= gb.ReadShort();
	sBottomFieldLength	= gb.ReadShort();

	DvbRenderField(spdResized ? *spdResized : spd, gb, nX, nY,   sTopFieldLength);
	DvbRenderField(spdResized ? *spdResized : spd, gb, nX, nY+1, sBottomFieldLength);
}

void CompositionObject::DvbRenderField(SubPicDesc& spd, CGolombBuffer& gb, SHORT nXStart, SHORT nYStart, SHORT nLength)
{
	//FillSolidRect (spd, 0,  0, 300, 10, 0xFFFF0000);	// Red opaque
	//FillSolidRect (spd, 0, 10, 300, 10, 0xCC00FF00);	// Green 80%
	//FillSolidRect (spd, 0, 20, 300, 10, 0x100000FF);	// Blue 60%
	//return;
	SHORT	nX		= nXStart;
	SHORT	nY		= nYStart;
	int		nEnd	= std::min(gb.GetPos()+nLength, gb.GetSize());
	while (gb.GetPos() < nEnd) {
		BYTE bType = gb.ReadByte();
		switch (bType) {
			case 0x10 :
				Dvb2PixelsCodeString(spd, gb, nX, nY);
				break;
			case 0x11 :
				Dvb4PixelsCodeString(spd, gb, nX, nY);
				break;
			case 0x12 :
				Dvb8PixelsCodeString(spd, gb, nX, nY);
				break;
			case 0x20 :
				gb.SkipBytes (2);
				break;
			case 0x21 :
				gb.SkipBytes (4);
				break;
			case 0x22 :
				gb.SkipBytes (16);
				break;
			case 0xF0 :
				nX  = nXStart;
				nY += 2;
				break;
			default :
				DLog(L"DvbRenderField(): Unknown DVBSUB segment 0x%02x, offset %d", bType, gb.GetPos()-1);
				break;
		}
	}
}

void CompositionObject::Dvb2PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY)
{
	BYTE	bTemp;
	BYTE	nPaletteIndex = 0;
	SHORT	nCount;
	bool	bQuit = false;

	while (!bQuit && !gb.IsEOF()) {
		nCount			= 0;
		nPaletteIndex	= 0;
		bTemp			= (BYTE)gb.BitRead(2);
		if (bTemp != 0) {
			nPaletteIndex = bTemp;
			nCount		  = 1;
		} else {
			if (gb.BitRead(1) == 1) {							// switch_1
				nCount		  = 3 + (SHORT)gb.BitRead(3);		// run_length_3-9
				nPaletteIndex = (BYTE)gb.BitRead(2);
			} else {
				if (gb.BitRead(1) == 0) {						// switch_2
					switch (gb.BitRead(2)) {					// switch_3
						case 0 :
							bQuit		  = true;
							break;
						case 1 :
							nCount		  = 2;
							break;
						case 2 :										// if (switch_3 == '10')
							nCount		  = 12 + (SHORT)gb.BitRead(4);	// run_length_12-27
							nPaletteIndex = (BYTE)gb.BitRead(2);		// 4-bit_pixel-code
							break;
						case 3 :
							nCount		  = 29 + gb.ReadByte();			// run_length_29-284
							nPaletteIndex = (BYTE)gb.BitRead(2);		// 4-bit_pixel-code
							break;
					}
				} else {
					nCount = 1;
				}
			}
		}

		if (nX+nCount > m_width) {
			ASSERT (FALSE);
			break;
		}

		if (nCount>0) {
			FillSolidRect(spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
			nX += nCount;
		}
	}

	gb.BitByteAlign();
}

void CompositionObject::Dvb4PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY)
{
	BYTE	bTemp;
	BYTE	nPaletteIndex = 0;
	SHORT	nCount;
	bool	bQuit = false;

	while (!bQuit && !gb.IsEOF()) {
		nCount			= 0;
		nPaletteIndex	= 0;
		bTemp			= (BYTE)gb.BitRead(4);
		if (bTemp != 0) {
			nPaletteIndex = bTemp;
			nCount		  = 1;
		} else {
			if (gb.BitRead(1) == 0) {							// switch_1
				nCount = (SHORT)gb.BitRead(3);					// run_length_3-9
				if (nCount != 0) {
					nCount += 2;
				} else {
					bQuit = true;
				}
			} else {
				if (gb.BitRead(1) == 0) {						// switch_2
					nCount		  = 4 + (SHORT)gb.BitRead(2);	// run_length_4-7
					nPaletteIndex = (BYTE)gb.BitRead(4);		// 4-bit_pixel-code
				} else {
					switch (gb.BitRead(2)) {					// switch_3
						case 0 :
							nCount		  = 1;
							break;
						case 1 :
							nCount		  = 2;
							break;
						case 2 :										// if (switch_3 == '10')
							nCount		  = 9 + (SHORT)gb.BitRead(4);	// run_length_9-24
							nPaletteIndex = (BYTE)gb.BitRead(4);		// 4-bit_pixel-code
							break;
						case 3 :
							nCount		  = 25 + gb.ReadByte();			// run_length_25-280
							nPaletteIndex = (BYTE)gb.BitRead(4);		// 4-bit_pixel-code
							break;
					}
				}
			}
		}

#if 0
		if (nX+nCount > m_width) {
			ASSERT (FALSE);
			break;
		}
#endif

		if (nCount>0) {
			FillSolidRect(spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
			nX += nCount;
		}
	}

	gb.BitByteAlign();
}

void CompositionObject::Dvb8PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY)
{
	BYTE	bTemp;
	BYTE	nPaletteIndex = 0;
	SHORT	nCount;
	bool	bQuit = false;

	while (!bQuit && !gb.IsEOF()) {
		nCount			= 0;
		nPaletteIndex	= 0;
		bTemp			= gb.ReadByte();
		if (bTemp != 0) {
			nPaletteIndex = bTemp;
			nCount		  = 1;
		} else {
			if (gb.BitRead(1) == 0) {							// switch_1
				nCount = (SHORT)gb.BitRead(7);					// run_length_1-127
				if (nCount == 0) {
					bQuit = true;
				}
			} else {
				nCount			= (SHORT)gb.BitRead(7);			// run_length_3-127
				nPaletteIndex	= gb.ReadByte();
			}
		}

		if (nX+nCount > m_width) {
			ASSERT (FALSE);
			break;
		}

		if (nCount>0) {
			FillSolidRect(spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
			nX += nCount;
		}
	}

	gb.BitByteAlign();
}

// from ffmpeg
const BYTE ff_log2_tab[256] = {
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

void CompositionObject::RenderXSUB(SubPicDesc& spd)
{
	if (!m_pRLEData) {
		return;
	}

	CGolombBuffer gb(m_pRLEData, m_nRLEDataSize);
	BYTE nPaletteIndex = 0;
	int  nCount;
	int  nX = m_horizontal_position;
	int  nY = m_vertical_position;

	for (SHORT y = 0; y < m_height; y++) {
		if (gb.IsEOF()) {
			break;
		}
		if (y == (m_height + 1) / 2) {
			// interlaced: do odd lines
			nY = m_vertical_position + 1;
		}
		nX = m_horizontal_position;
		while (nX < (m_horizontal_position + m_width)) {
			int log2		= ff_log2_tab[gb.BitRead(8, true)];
			nCount			= gb.BitRead(14 - 4 * (log2 >> 1));
			nCount			= std::min(nCount, m_width - (nX - m_horizontal_position));
			nPaletteIndex	= gb.BitRead(2);
			// count 0 - means till end of row
			if (!nCount) {
				nCount = m_width - (nX - m_horizontal_position);
			}
			FillSolidRect(spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
			nX += nCount;
		}
		// interlaced, skip every second line
		nY += 2;

		gb.BitByteAlign();
	}
}
