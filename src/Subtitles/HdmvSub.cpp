/*
 * (C) 2006-2018 see Authors.txt
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
#include "HdmvSub.h"
#include "../DSUtil/GolombBuffer.h"

#if (0) // Set to 1 to activate HDMV subtitles debug log
	#define TRACE_HDMVSUB DLog
#else
	#define TRACE_HDMVSUB __noop
#endif

CHdmvSub::CHdmvSub()
	: CBaseSub(ST_HDMV)
{
}

CHdmvSub::~CHdmvSub()
{
	Reset();

	SAFE_DELETE_ARRAY(m_pSegBuffer);
	SAFE_DELETE_ARRAY(m_DefaultCLUT.Palette);

	for (int i = 0; i < _countof(m_CLUT); i++) {
		SAFE_DELETE_ARRAY(m_CLUT[i].Palette);
	}
}

POSITION CHdmvSub::GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld)
{
	if (CleanOld) {
		CHdmvSub::CleanOld(rt);
	}

	POSITION pos = m_pObjects.GetHeadPosition();
	while (pos) {
		const CompositionObject* pObject = m_pObjects.GetAt(pos);
		if (pObject->m_rtStop <= rt || (g_bForcedSubtitle && !pObject->m_forced_on_flag)) {
			m_pObjects.GetNext(pos);
		} else {
			break;
		}
	}

	return pos;
}

POSITION CHdmvSub::GetNext(POSITION pos)
{
	m_pObjects.GetNext(pos);
	return pos;
}

REFERENCE_TIME CHdmvSub::GetStart(POSITION nPos)
{
	CompositionObject* pObject = m_pObjects.GetAt(nPos);
	return pObject ? pObject->m_rtStart : INVALID_TIME;
};

REFERENCE_TIME CHdmvSub::GetStop(POSITION nPos)
{
	CompositionObject* pObject = m_pObjects.GetAt(nPos);
	return pObject ? pObject->m_rtStop : INVALID_TIME;
};

HRESULT CHdmvSub::ParseSample(BYTE* pData, long nLen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	CheckPointer(pData, E_POINTER);

	CGolombBuffer SampleBuffer(pData, nLen);

	while (!SampleBuffer.IsEOF()) {
		if (m_nCurSegment == NO_SEGMENT) {
			HDMV_SEGMENT_TYPE nSegType  = (HDMV_SEGMENT_TYPE)SampleBuffer.ReadByte();
			USHORT            nUnitSize = SampleBuffer.ReadShort();
			nLen -=3;

			switch (nSegType) {
				case PALETTE :
				case OBJECT :
				case PRESENTATION_SEG :
				case END_OF_DISPLAY :
					m_nCurSegment = nSegType;
					AllocSegment(nUnitSize);
					break;

				case WINDOW_DEF :
				case INTERACTIVE_SEG :
				case HDMV_SUB1 :
				case HDMV_SUB2 :
					// Ignored stuff...
					SampleBuffer.SkipBytes(nUnitSize);
					break;
				default :
					return VFW_E_SAMPLE_REJECTED;
			}
		}

		if (m_nCurSegment != NO_SEGMENT) {
			if (m_nSegBufferPos < m_nSegSize) {
				int nSize = std::min(m_nSegSize - m_nSegBufferPos, (int)nLen);
				SampleBuffer.ReadBuffer(m_pSegBuffer + m_nSegBufferPos, nSize);
				m_nSegBufferPos += nSize;
			}

			if (m_nSegBufferPos >= m_nSegSize) {
				CGolombBuffer SegmentBuffer(m_pSegBuffer, m_nSegSize);

				switch (m_nCurSegment) {
					case PALETTE :
						TRACE_HDMVSUB(L"CHdmvSub::ParseSample() : PALETTE");
						ParsePalette(&SegmentBuffer, m_nSegSize);
						break;
					case OBJECT :
						TRACE_HDMVSUB(L"CHdmvSub::ParseSample() : OBJECT");
						ParseObject(&SegmentBuffer, m_nSegSize);
						break;
					case PRESENTATION_SEG :
						TRACE_HDMVSUB(L"CHdmvSub::ParseSample() : PRESENTATION_SEG - %I64d [%s], size = %d", rtStart, ReftimeToString(rtStart), m_nSegSize);
						if (rtStart == INVALID_TIME) {
							break;
						}
						// Update the timestamp for the previous segment
						UpdateTimeStamp(rtStart);

						// Parse the new presentation segment
						ParsePresentationSegment(&SegmentBuffer, rtStart);
						break;
					case WINDOW_DEF :
						//TRACE_HDMVSUB(L"CHdmvSub::ParseSample() : WINDOW_DEF - %I64d [%s]", rtStart, ReftimeToString(rtStart));
						break;
					case END_OF_DISPLAY :
						TRACE_HDMVSUB(L"CHdmvSub::ParseSample() : END_OF_DISPLAY - %I64d [%s]", rtStart, ReftimeToString(rtStart));
						// Enqueue the current presentation segment
						EnqueuePresentationSegment();
						break;
					default :
						TRACE_HDMVSUB(L"CHdmvSub::ParseSample() : UNKNOWN Seg %d - %I64d [%s]", m_nCurSegment, rtStart, ReftimeToString(rtStart));
				}

				m_nCurSegment = NO_SEGMENT;
			}
		}
	}

	return S_OK;
}

static void SetPalette(CompositionObject* pObject, const int nNbEntry, HDMV_PALETTE* pPalette, const CString yuvMatrix, const SHORT nVideoWidth, ColorConvert::convertType convertType)
{
	pObject->SetPalette(nNbEntry, pPalette, yuvMatrix == L"709" ? true : yuvMatrix == L"601" ? false : nVideoWidth > 720, convertType);
}

HRESULT CHdmvSub::Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox)
{
	bbox.left   = LONG_MAX;
	bbox.top    = LONG_MAX;
	bbox.right  = 0;
	bbox.bottom = 0;

	HRESULT hr = E_FAIL;

	POSITION pos = m_pObjects.GetHeadPosition();
	while (pos) {
		CompositionObject* pObject = m_pObjects.GetNext(pos);

		if (pObject && rt >= pObject->m_rtStart && rt < pObject->m_rtStop) {
			if (pObject->GetRLEDataSize() && pObject->m_width > 0 && pObject->m_height > 0 &&
					m_VideoDescriptor.nVideoWidth >= (pObject->m_horizontal_position + pObject->m_width) &&
					m_VideoDescriptor.nVideoHeight >= (pObject->m_vertical_position + pObject->m_height)) {

				if (!pObject->HavePalette() && m_DefaultCLUT.Palette) {
					SetPalette(pObject, m_DefaultCLUT.pSize, m_DefaultCLUT.Palette, yuvMatrix, m_VideoDescriptor.nVideoWidth, convertType);
				}

				if (!pObject->HavePalette()) {
					TRACE_HDMVSUB(L"CHdmvSub::Render() : The palette is missing - cancel rendering");
					continue;
				}

				bbox.left   = std::min((LONG)pObject->m_horizontal_position, bbox.left);
				bbox.top    = std::min((LONG)pObject->m_vertical_position, bbox.top);
				bbox.right  = std::max((LONG)pObject->m_horizontal_position + pObject->m_width, bbox.right);
				bbox.bottom = std::max((LONG)pObject->m_vertical_position + pObject->m_height, bbox.bottom);

				bbox.left = bbox.left > 0 ? bbox.left : 0;
				bbox.top  = bbox.top > 0 ? bbox.top : 0;
				if (m_VideoDescriptor.nVideoWidth != spd.w) {
					bbox.left  = MulDiv(bbox.left, spd.w, m_VideoDescriptor.nVideoWidth);
					bbox.right = MulDiv(bbox.right, spd.w, m_VideoDescriptor.nVideoWidth);
				}

				if (m_VideoDescriptor.nVideoHeight != spd.h) {
					bbox.top    = MulDiv(bbox.top, spd.h, m_VideoDescriptor.nVideoHeight);
					bbox.bottom = MulDiv(bbox.bottom, spd.h, m_VideoDescriptor.nVideoHeight);
				}

				TRACE_HDMVSUB(L"CHdmvSub::Render() : size - %ld, ObjRes - %dx%d, SPDRes - %dx%d, %I64d [%s]",
							  pObject->GetRLEDataSize(),
							  pObject->m_width, pObject->m_height, spd.w, spd.h,
							  rt, ReftimeToString(rt));

				InitSpd(spd, m_VideoDescriptor.nVideoWidth, m_VideoDescriptor.nVideoHeight);
				pObject->RenderHdmv(spd, m_bResizedRender ? &m_spd : nullptr);

				hr = S_OK;
			}
		}
	}

	FinalizeRender(spd);

	return hr;
}

HRESULT CHdmvSub::GetTextureSize(POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
	CompositionObject* pObject = m_pObjects.GetAt (pos);

	if (pObject) {
		MaxTextureSize.cx = VideoSize.cx = m_VideoDescriptor.nVideoWidth;
		MaxTextureSize.cy = VideoSize.cy = m_VideoDescriptor.nVideoHeight;

		VideoTopLeft.x = 0;
		VideoTopLeft.y = 0;

		return S_OK;
	}

	ASSERT(FALSE);
	return E_INVALIDARG;
}

void CHdmvSub::Reset()
{
	CompositionObject* pObject;
	while (m_pObjects.GetCount() > 0) {
		pObject = m_pObjects.RemoveHead();
		if (pObject) {
			delete pObject;
		}
	}

	SAFE_DELETE(m_pCurrentWindow);

	for (int i = 0; i< MAX_WINDOWS; i++) {
		m_ParsedObjects[i].m_version_number	= 0;
		m_ParsedObjects[i].m_width			= 0;
		m_ParsedObjects[i].m_height			= 0;
	}
}

void CHdmvSub::CleanOld(REFERENCE_TIME rt)
{
	CompositionObject* pObject_old;

	while (!m_pObjects.IsEmpty()) {
		pObject_old = m_pObjects.GetHead();
		if (pObject_old->m_rtStop < rt) {
			TRACE_HDMVSUB(L"CCHdmvSub::CleanOld() : remove object, size - %d, %I64d -> %I64d [%s -> %s], %I64d [%s]", pObject_old->GetRLEDataSize(),
						  pObject_old->m_rtStart, pObject_old->m_rtStop,
						  ReftimeToString(pObject_old->m_rtStart), ReftimeToString(pObject_old->m_rtStop),
						  rt, ReftimeToString(rt));
			m_pObjects.RemoveHeadNoReturn();
			delete pObject_old;
		} else {
			break;
		}
	}
}

void CHdmvSub::UpdateTimeStamp(REFERENCE_TIME rtStop)
{
	if (!m_pObjects.IsEmpty()) {
		POSITION pos = m_pObjects.GetTailPosition();
		while (pos) {
			auto& pObject = m_pObjects.GetPrev(pos);
			if (pObject->m_rtStop == UNKNOWN_TIME) {
				pObject->m_rtStop = rtStop;

				TRACE_HDMVSUB(L"CHdmvSub::UpdateTimeStamp() : Update Presentation Segment TimeStamp : compositionNumber - %d, %I64d -> %I64d [%s -> %s]", pObject->m_compositionNumber,
							  pObject->m_rtStart, pObject->m_rtStop,
							  ReftimeToString(pObject->m_rtStart), ReftimeToString(pObject->m_rtStop));
			} else {
				break;
			}
		}
	}
}

void CHdmvSub::ParsePresentationSegment(CGolombBuffer* pGBuffer, REFERENCE_TIME rtTime)
{
	ParseVideoDescriptor(pGBuffer, &m_VideoDescriptor);

	COMPOSITION_DESCRIPTOR CompositionDescriptor;
	ParseCompositionDescriptor(pGBuffer, &CompositionDescriptor);

	bool palette_update_flag = !!(pGBuffer->ReadByte() & 0x80);
	UNREFERENCED_PARAMETER(palette_update_flag);
	BYTE palette_id_ref = pGBuffer->ReadByte();
	BYTE nObjectNumber  = pGBuffer->ReadByte();

	TRACE_HDMVSUB(L"CHdmvSub::ParsePresentationSegment() : Size - %d, nObjectNumber - %d, palette_id_ref - %d, compositionNumber - %d",
				  pGBuffer->GetSize(), nObjectNumber, palette_id_ref, CompositionDescriptor.nNumber);

	if (!m_pObjects.IsEmpty()) {
		auto& pObject = m_pObjects.GetTail();
		if (!pObject->HavePalette() && m_CLUT[palette_id_ref].Palette) {
			SetPalette(pObject, m_CLUT[palette_id_ref].pSize, m_CLUT[palette_id_ref].Palette, yuvMatrix, m_VideoDescriptor.nVideoWidth, convertType);
		}
	}

	if (!m_pCurrentWindow) {
		m_pCurrentWindow = DNew HDMV_WindowDefinition();
	} else {
		m_pCurrentWindow->Reset();
	}

	if (nObjectNumber > 0) {
		m_pCurrentWindow->m_nObjectNumber     = nObjectNumber;
		m_pCurrentWindow->m_palette_id_ref    = (SHORT)palette_id_ref;
		m_pCurrentWindow->m_compositionNumber = CompositionDescriptor.nNumber;

		for (int i = 0; i < nObjectNumber; i++) {
			m_pCurrentWindow->Objects[i]                      = DNew CompositionObject();
			m_pCurrentWindow->Objects[i]->m_rtStart           = rtTime;
			m_pCurrentWindow->Objects[i]->m_rtStop            = UNKNOWN_TIME;
			m_pCurrentWindow->Objects[i]->m_compositionNumber = CompositionDescriptor.nNumber;

			ParseCompositionObject(pGBuffer, m_pCurrentWindow->Objects[i]);
		}
	}
}

void CHdmvSub::EnqueuePresentationSegment()
{
	if (m_pCurrentWindow && m_pCurrentWindow->m_nObjectNumber && m_pCurrentWindow->m_compositionNumber != -1) {
		for (int i = 0; i < m_pCurrentWindow->m_nObjectNumber; i++) {
			if (m_pCurrentWindow->Objects[i]) {
				CompositionObject* pObject     = m_pCurrentWindow->Objects[i];
				CompositionObject& pObjectData = m_ParsedObjects[pObject->m_object_id_ref];
				if (pObjectData.m_width) {
					pObject->m_width  = pObjectData.m_width;
					pObject->m_height = pObjectData.m_height;

					if (pObjectData.GetRLEData()) {
						pObject->SetRLEData(pObjectData.GetRLEData(), pObjectData.GetRLEDataSize(), pObjectData.GetRLEDataSize());
					}

					TRACE_HDMVSUB(L"CHdmvSub::EnqueuePresentationSegment() : Enqueue Presentation Segment : m_object_id_ref - %d, m_window_id_ref - %d, compositionNumber - %d, %I64d [%s]",
								  pObject->m_object_id_ref, pObject->m_window_id_ref, pObject->m_compositionNumber,
								  pObject->m_rtStart, ReftimeToString(pObject->m_rtStart));

					m_pObjects.AddTail(m_pCurrentWindow->Objects[i]);
				} else {
					delete m_pCurrentWindow->Objects[i];
				}
			}
			m_pCurrentWindow->Objects[i] = nullptr;
		}
	}
}

void CHdmvSub::ParseCompositionObject(CGolombBuffer* pGBuffer, CompositionObject* pCompositionObject)
{
	pCompositionObject->m_object_id_ref       = pGBuffer->ReadShort();
	pCompositionObject->m_window_id_ref       = (SHORT)pGBuffer->ReadByte();
	BYTE bTemp                                = pGBuffer->ReadByte();
	pCompositionObject->m_object_cropped_flag = !!(bTemp & 0x80);
	pCompositionObject->m_forced_on_flag      = !!(bTemp & 0x40);
	pCompositionObject->m_horizontal_position = pGBuffer->ReadShort();
	pCompositionObject->m_vertical_position   = pGBuffer->ReadShort();

	if (pCompositionObject->m_object_cropped_flag) {
		pCompositionObject->m_cropping_horizontal_position = pGBuffer->ReadShort();
		pCompositionObject->m_cropping_vertical_position   = pGBuffer->ReadShort();
		pCompositionObject->m_cropping_width               = pGBuffer->ReadShort();
		pCompositionObject->m_cropping_height              = pGBuffer->ReadShort();
	}

	TRACE_HDMVSUB(L"CHdmvSub::ParseCompositionObject() : m_object_id_ref - %d, m_window_id_ref - %d, pos = %d:%d",
				  pCompositionObject->m_object_id_ref, pCompositionObject->m_window_id_ref,
				  pCompositionObject->m_horizontal_position, pCompositionObject->m_vertical_position);
}

void CHdmvSub::ParsePalette(CGolombBuffer* pGBuffer, USHORT nSize)
{
	BYTE palette_id             = pGBuffer->ReadByte();
	BYTE palette_version_number = pGBuffer->ReadByte();
	UNREFERENCED_PARAMETER(palette_version_number);

	ASSERT((nSize - 2) % sizeof(HDMV_PALETTE) == 0);
	int nNbEntry = (nSize - 2) / sizeof(HDMV_PALETTE);
	HDMV_PALETTE* pPalette = (HDMV_PALETTE*)pGBuffer->GetBufferPos();

	SAFE_DELETE_ARRAY(m_CLUT[palette_id].Palette);
	m_CLUT[palette_id].pSize   = nNbEntry;
	m_CLUT[palette_id].Palette = DNew HDMV_PALETTE[nNbEntry];
	memcpy(m_CLUT[palette_id].Palette, pPalette, nNbEntry * sizeof(HDMV_PALETTE));

	if (m_DefaultCLUT.Palette == nullptr || m_DefaultCLUT.pSize != nNbEntry) {
		SAFE_DELETE_ARRAY(m_DefaultCLUT.Palette);
		m_DefaultCLUT.Palette = DNew HDMV_PALETTE[nNbEntry];
		m_DefaultCLUT.pSize   = nNbEntry;
	}
	memcpy(m_DefaultCLUT.Palette, pPalette, nNbEntry * sizeof(HDMV_PALETTE));

	if (m_pCurrentWindow && m_pCurrentWindow->m_palette_id_ref == palette_id && m_pCurrentWindow->m_nObjectNumber) {
		for (int i = 0; i < m_pCurrentWindow->m_nObjectNumber; i++) {
			SetPalette(m_pCurrentWindow->Objects[i], nNbEntry, pPalette, yuvMatrix, m_VideoDescriptor.nVideoWidth, convertType);
		}
	}
}

void CHdmvSub::ParseObject(CGolombBuffer* pGBuffer, USHORT nUnitSize)
{
	SHORT object_id = pGBuffer->ReadShort();
	if (object_id > _countof(m_ParsedObjects)) {
		TRACE_HDMVSUB(L"CHdmvSub::ParseObject() : FAILED, object_id - %d", object_id);
		return;
	}

	CompositionObject &pObject = m_ParsedObjects[object_id];

	pObject.m_version_number = pGBuffer->ReadByte();
	BYTE m_sequence_desc     = pGBuffer->ReadByte();

	if (m_sequence_desc & 0x80) {
		DWORD object_data_length = (DWORD)pGBuffer->BitRead(24);

		pObject.m_width  = pGBuffer->ReadShort();
		pObject.m_height = pGBuffer->ReadShort();

		pObject.SetRLEData(pGBuffer->GetBufferPos(), nUnitSize - 11, object_data_length - 4);

		TRACE_HDMVSUB(L"CHdmvSub::ParseObject() : NewObject : size - %ld, version - %d, total obj - %d, size - %dx%d",
					  object_data_length, pObject.m_version_number, m_pObjects.GetCount(),
					  pObject.m_width, pObject.m_height);
	} else {
		pObject.AppendRLEData(pGBuffer->GetBufferPos(), nUnitSize - 4);
	}
}

void CHdmvSub::ParseVideoDescriptor(CGolombBuffer* pGBuffer, VIDEO_DESCRIPTOR* pVideoDescriptor)
{
	pVideoDescriptor->nVideoWidth  = pGBuffer->ReadShort();
	pVideoDescriptor->nVideoHeight = pGBuffer->ReadShort();
	pVideoDescriptor->bFrameRate   = pGBuffer->ReadByte();
}

void CHdmvSub::ParseCompositionDescriptor(CGolombBuffer* pGBuffer, COMPOSITION_DESCRIPTOR* pCompositionDescriptor)
{
	pCompositionDescriptor->nNumber = pGBuffer->ReadShort();
	pCompositionDescriptor->bState  = pGBuffer->ReadByte();
}

void CHdmvSub::AllocSegment(int nSize)
{
	if (nSize > m_nTotalSegBuffer) {
		SAFE_DELETE_ARRAY(m_pSegBuffer);
		m_pSegBuffer      = DNew BYTE[nSize];
		m_nTotalSegBuffer = nSize;
	}
	m_nSegBufferPos = 0;
	m_nSegSize      = nSize;
}
