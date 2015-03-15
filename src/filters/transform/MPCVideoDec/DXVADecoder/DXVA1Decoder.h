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

#pragma once

#include <dxva.h>
#include <videoacc.h>
#include "DXVADecoder.h"

#define CHECK_HR_FRAME_DXVA1(x)	hr = ##x; if (FAILED(hr)) { DbgLog((LOG_TRACE, 3, L"DXVA Error : 0x%08x, %s : %i", hr, CString(__FILE__), __LINE__)); CHECK_HR_FALSE (EndFrame(nSurfaceIndex)); return S_FALSE; }

class CMPCVideoDecFilter;

class CDXVA1Decoder : public CDXVADecoder
{
	#define MAX_COM_BUFFER			6				// Max uncompressed buffer for an Execute command (DXVA1)
	#define COMP_BUFFER_COUNT		18
	#define NO_REF_FRAME			0xFFFF

	struct PICTURE_STORE {
		bool						bRefPicture		= false;	// True if reference picture
		bool						bInUse			= false;	// Slot in use
		bool						bDisplayed		= false;	// True if picture have been presented
		REFERENCE_TIME				rtStart			= INVALID_TIME;
		REFERENCE_TIME				rtStop			= INVALID_TIME;
		int							nCodecSpecific	= -1;
		DWORD						dwDisplayCount	= 0;
	};

public :
	static CDXVA1Decoder*			CreateDecoderDXVA1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, int nPicEntryNumber);
	virtual							~CDXVA1Decoder();

	virtual void					Flush();
	virtual void					CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);

	HRESULT							ConfigureDXVA1();

protected :
	CDXVA1Decoder(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, int nPicEntryNumber);

	CMPCVideoDecFilter*				m_pFilter;

	bool							m_bFlushed;
	int								m_nMaxWaiting;

	PICTURE_STORE*					m_pPictureStore;		// Store reference picture, and delayed B-frames
	int								m_nPicEntryNumber;		// Total number of picture in store
	int								m_nWaitingPics;			// Number of picture not yet displayed

	int								m_nSurfaceIndex;

	DXVA_ConfigPictureDecode		m_DXVA1Config;
	DXVA2_ConfigPictureDecode		m_DXVA2Config;

	// === DXVA functions
	HRESULT							BeginFrame(int nSurfaceIndex);
	HRESULT							EndFrame(int nSurfaceIndex);
	HRESULT							AddExecuteBuffer(DWORD dwTypeIndex, UINT nSize = 0, void* pBuffer = NULL, UINT* pRealSize = NULL);
	HRESULT							Execute();
	HRESULT							FindFreeDXVA1Buffer(DWORD& dwBufferIndex);

	// === Picture store functions
	bool							AddToStore(int nSurfaceIndex, bool bRefPicture, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool bIsField, int nCodecSpecific);
	void							UpdateStore(int nSurfaceIndex, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	void							RemoveRefFrame(int nSurfaceIndex);
	HRESULT							DisplayNextFrame();
	virtual HRESULT					GetFreeSurfaceIndex(int& nSurfaceIndex);
	virtual int						FindOldestFrame();
	void							FreePictureSlot(int nSurfaceIndex);

private :
	CComQIPtr<IAMVideoAccelerator>	m_pAMVideoAccelerator;
	AMVABUFFERINFO					m_DXVA1BufferInfo[MAX_COM_BUFFER];
	DXVA_BufferDescription 			m_DXVA1BufferDesc[MAX_COM_BUFFER];
	DWORD							m_dwNumBuffersInfo;
	AMVACompBufferInfo				m_ComBufferInfo[COMP_BUFFER_COUNT];
	DWORD							m_dwBufferIndex;
	DWORD							m_dwDisplayCount;
};
