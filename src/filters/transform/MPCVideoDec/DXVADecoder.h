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
#include <dxva2api.h>
#include <videoacc.h>
#include "../../../DSUtil/DSUtil.h"

#pragma warning(push)
#pragma warning(disable: 4005)
#include <stdint.h>
#pragma warning(pop)

#define CHECK_HR_FALSE(x)	hr = ##x; if (FAILED(hr)) { DbgLog((LOG_TRACE, 3, L"DXVA Error : 0x%08x, %s : %i", hr, CString(__FILE__), __LINE__)); return S_FALSE; }
#define CHECK_HR_FRAME(x)	hr = ##x; if (FAILED(hr)) { DbgLog((LOG_TRACE, 3, L"DXVA Error : 0x%08x, %s : %i", hr, CString(__FILE__), __LINE__)); CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex)); return S_FALSE; }

class CMPCVideoDecFilter;
struct AVFrame;

class CDXVADecoder
{
	#define MAX_COM_BUFFER			6				// Max uncompressed buffer for an Execute command (DXVA1)
	#define COMP_BUFFER_COUNT		18
	#define NO_REF_FRAME			0xFFFF

	enum DXVA_ENGINE {
		ENGINE_DXVA1,
		ENGINE_DXVA2
	};
	struct PICTURE_STORE {
		bool						bRefPicture;	// True if reference picture
		bool						bInUse;			// Slot in use
		bool						bDisplayed;		// True if picture have been presented
		CComPtr<IMediaSample>		pSample;		// Only for DXVA2 !
		REFERENCE_TIME				rtStart;
		REFERENCE_TIME				rtStop;
		int							nCodecSpecific;
		DWORD						dwDisplayCount;

		PICTURE_STORE() {
			bRefPicture		= false;
			bInUse			= false;
			bDisplayed		= false;
			pSample			= NULL;
			rtStart			= INVALID_TIME;
			rtStop			= INVALID_TIME;
			nCodecSpecific	= -1;
			dwDisplayCount	= 0;
		}
	};

public :
	enum DXVAMode {
		H264_VLD,
		HEVC_VLD,
		VC1_VLD,
		MPEG2_VLD
	};

	virtual						~CDXVADecoder();
	DXVAMode					GetMode() const { return m_nMode; }
	DXVA_ENGINE					GetEngine() const { return m_nEngine; }
	void						AllocExecuteParams(int nSize);
	void						SetDirectXVideoDec(IDirectXVideoDecoder* pDirectXVideoDec) { m_pDirectXVideoDec = pDirectXVideoDec; }

	virtual void				Flush();
	virtual HRESULT				CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize, UINT nDXVASize = UINT_MAX) PURE;
	virtual HRESULT				DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop) PURE;

	HRESULT						ConfigureDXVA1();

	static CDXVADecoder*		CreateDecoderDXVA1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, int nPicEntryNumber);
	static CDXVADecoder*		CreateDecoderDXVA2(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);

	void						EndOfStream();

	void						FreePictureSlot(int nSurfaceIndex);

	HRESULT						get_buffer_dxva(struct AVFrame *pic);
	static void					release_buffer_dxva(void *opaque, uint8_t *data);

protected :
	CDXVADecoder(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoder(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);

	GUID						m_guidDecoder;

	CMPCVideoDecFilter*			m_pFilter;

	PICTURE_STORE*				m_pPictureStore;		// Store reference picture, and delayed B-frames
	int							m_nPicEntryNumber;		// Total number of picture in store

	int							m_nSurfaceIndex;
	CComPtr<IMediaSample>		m_pSampleToDeliver;

	struct SurfaceWrapper {
		void*					opaque;
		int						nSurfaceIndex;
		CComPtr<IMediaSample>	pSample;
	};

	// === DXVA functions
	HRESULT						AddExecuteBuffer(DWORD CompressedBufferType, UINT nSize = 0, void* pBuffer = NULL, UINT* pRealSize = NULL);
	HRESULT						GetDeliveryBuffer(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, IMediaSample** ppSampleToDeliver);
	HRESULT						Execute();
	DWORD						GetDXVA1CompressedType(DWORD dwDXVA2CompressedType);
	HRESULT						FindFreeDXVA1Buffer(DWORD dwTypeIndex, DWORD& dwBufferIndex);
	HRESULT						BeginFrame(int nSurfaceIndex, IMediaSample* pSampleToDeliver);
	HRESULT						EndFrame(int nSurfaceIndex);
	HRESULT						QueryStatus(PVOID LPDXVAStatus, UINT nSize);
	BYTE						GetConfigIntraResidUnsigned();
	BYTE						GetConfigResidDiffAccelerator();
	DXVA_ConfigPictureDecode*	GetDXVA1Config() { return &m_DXVA1Config; }
	DXVA2_ConfigPictureDecode*	GetDXVA2Config() { return &m_DXVA2Config; }

	// === Picture store functions
	bool						AddToStore(int nSurfaceIndex, IMediaSample* pSample);
	HRESULT						DisplayNextFrame();
	virtual HRESULT				GetFreeSurfaceIndex(int& nSurfaceIndex, IMediaSample** ppSampleToDeliver, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual int					FindOldestFrame();

private :
	DXVAMode						m_nMode;
	DXVA_ENGINE						m_nEngine;

	// === DXVA1 variables
	CComQIPtr<IAMVideoAccelerator>	m_pAMVideoAccelerator;
	AMVABUFFERINFO					m_DXVA1BufferInfo[MAX_COM_BUFFER];
	DXVA_BufferDescription 			m_DXVA1BufferDesc[MAX_COM_BUFFER];
	DWORD							m_dwNumBuffersInfo;
	DXVA_ConfigPictureDecode		m_DXVA1Config;
	AMVACompBufferInfo				m_ComBufferInfo[COMP_BUFFER_COUNT];
	DWORD							m_dwBufferIndex;
	DWORD							m_dwDisplayCount;

	// === DXVA2 variables
	CComPtr<IDirectXVideoDecoder>	m_pDirectXVideoDec;
	DXVA2_ConfigPictureDecode		m_DXVA2Config;
	DXVA2_DecodeExecuteParams		m_ExecuteParams;

	void							Init(CMPCVideoDecFilter* pFilter, DXVAMode nMode, int nPicEntryNumber);
	void							SetTypeSpecificFlags(PICTURE_STORE* pPicture, IMediaSample* pMS);
};
