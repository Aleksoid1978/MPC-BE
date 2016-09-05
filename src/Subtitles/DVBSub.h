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

#pragma once

#include "BaseSub.h"

class CGolombBuffer;

class CDVBSub : public CBaseSub
{
public:
	CDVBSub();
	~CDVBSub();

	virtual HRESULT			ParseSample(BYTE* pData, long nLen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void			Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox);
	virtual HRESULT			GetTextureSize (POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft);
	virtual POSITION		GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld = false);
	virtual POSITION		GetNext(POSITION pos);
	virtual REFERENCE_TIME	GetStart(POSITION nPos);
	virtual REFERENCE_TIME	GetStop(POSITION nPos);
	virtual void			Reset();
	virtual	void			CleanOld(REFERENCE_TIME rt);
	virtual HRESULT			EndOfStream();

	// EN 300-743, table 2
	enum DVB_SEGMENT_TYPE {
		NO_SEGMENT			= 0xFFFF,
		PAGE				= 0x10,
		REGION				= 0x11,
		CLUT				= 0x12,
		OBJECT				= 0x13,
		DISPLAY				= 0x14,
		END_OF_DISPLAY		= 0x80
	};

	const CString GetSegmentType(const DVB_SEGMENT_TYPE SegmentType) {
		switch (SegmentType) {
			case NO_SEGMENT:
				return L"NO_SEGMENT";
			case PAGE:
				return L"PAGE";
			case REGION:
				return L"REGION";
			case CLUT:
				return L"CLUT";
			case OBJECT:
				return L"OBJECT";
			case DISPLAY:
				return L"DISPLAY";
			case END_OF_DISPLAY:
				return L"END_OF_DISPLAY";
			default:
				return L"UNKNOWN_SEGMENT";
		}
	}

	// EN 300-743, table 6
	enum DVB_OBJECT_TYPE {
		OT_BASIC_BITMAP			= 0x00,
		OT_BASIC_CHAR			= 0x01,
		OT_COMPOSITE_STRING		= 0x02
	};

	enum DVB_PAGE_STATE {
		DPS_NORMAL				= 0x00,
		DPS_ACQUISITION			= 0x01,
		DPS_MODE_CHANGE			= 0x02,
		DPS_RESERVED			= 0x03
	};

	struct DVB_CLUT {
		BYTE			id				= 0;
		BYTE			version_number	= 0;
		BYTE			size			= 0;
		HDMV_PALETTE	palette[256];

		DVB_CLUT() {
			memset(palette, 0, sizeof(palette));
		}
	};

	struct DVB_DISPLAY {
		BYTE	version_number				= 0;
		BYTE	display_window_flag			= 0;
		SHORT	width						= 720;
		SHORT	height						= 576;
		SHORT	horizontal_position_minimun	= 0;
		SHORT	horizontal_position_maximum	= 0;
		SHORT	vertical_position_minimun	= 0;
		SHORT	vertical_position_maximum	= 0;
	};

	struct DVB_OBJECT {
		SHORT	object_id					= 0xFF;
		BYTE	object_type					= 0;
		BYTE	object_provider_flag		= 0;
		SHORT	object_horizontal_position	= 0;
		SHORT	object_vertical_position	= 0;
		BYTE	foreground_pixel_code		= 0;
		BYTE	background_pixel_code		= 0;
	};

	struct DVB_REGION_POS {
		BYTE	id			= 0;
		WORD	horizAddr	= 0;
		WORD	vertAddr	= 0;
	};

	struct DVB_REGION {
		BYTE	id						= 0;
		BYTE	version_number			= 0;
		BYTE	fill_flag				= 0;
		WORD	width					= 0;
		WORD	height					= 0;
		BYTE	level_of_compatibility	= 0;
		BYTE	depth					= 0;
		BYTE	CLUT_id					= 0;
		BYTE	_8_bit_pixel_code		= 0;
		BYTE	_4_bit_pixel_code		= 0;
		BYTE	_2_bit_pixel_code		= 0;
		CAtlList<DVB_OBJECT> objects;

		DVB_REGION() {}

		DVB_REGION(const CDVBSub::DVB_REGION& region)
			: id(region.id)
			, version_number(region.version_number)
			, fill_flag(region.fill_flag)
			, width(region.width)
			, height(region.height)
			, level_of_compatibility(region.level_of_compatibility)
			, depth(region.depth)
			, CLUT_id(region.CLUT_id)
			, _8_bit_pixel_code(region._8_bit_pixel_code)
			, _4_bit_pixel_code(region._4_bit_pixel_code)
			, _2_bit_pixel_code(region._2_bit_pixel_code) {
			objects.AddHeadList(&region.objects);
		}
	};

	class DVB_PAGE
	{
	public :
		REFERENCE_TIME					rtStart				= 0;
		REFERENCE_TIME					rtStop				= 0;
		BYTE							pageTimeOut			= 0;
		BYTE							pageVersionNumber	= 0;
		BYTE							pageState			= 0;
		bool							rendered			= 0;
		CAtlList<DVB_REGION_POS>		regionsPos;
		CAtlList<DVB_REGION*>			regions;
		CAtlList<CompositionObject*>	objects;
		CAtlList<DVB_CLUT*>				CLUTs;

		~DVB_PAGE() {
			while (!regions.IsEmpty()) {
				delete regions.RemoveHead();
			}
			while (!objects.IsEmpty()) {
				delete objects.RemoveHead();
			}
			while (!CLUTs.IsEmpty()) {
				delete CLUTs.RemoveHead();
			}
		}
	};

private:
	int					m_nBufferSize		= 0;
	int					m_nBufferReadPos	= 0;
	int					m_nBufferWritePos	= 0;
	BYTE*				m_pBuffer			= NULL;
	CAtlList<DVB_PAGE*>	m_pages;
	CAutoPtr<DVB_PAGE>	m_pCurrentPage;
	DVB_DISPLAY			m_Display;

	HRESULT				AddToBuffer(BYTE* pData, int nSize);
	DVB_PAGE*			FindPage(REFERENCE_TIME rt);
	DVB_REGION*			FindRegion(DVB_PAGE* pPage, BYTE bRegionId);
	DVB_CLUT*			FindClut(DVB_PAGE* pPage, BYTE bClutId);
	CompositionObject*	FindObject(DVB_PAGE* pPage, SHORT sObjectId);

	HRESULT				ParsePage(CGolombBuffer& gb, WORD wSegLength, CAutoPtr<DVB_PAGE>& pPage);
	HRESULT				ParseDisplay(CGolombBuffer& gb, WORD wSegLength);
	HRESULT				ParseRegion(CGolombBuffer& gb, WORD wSegLength);
	HRESULT				ParseClut(CGolombBuffer& gb, WORD wSegLength);
	HRESULT				ParseObject(CGolombBuffer& gb, WORD wSegLength);

	HRESULT				EnqueuePage(REFERENCE_TIME rtStop);
	HRESULT				UpdateTimeStamp(REFERENCE_TIME rtStop);
};
