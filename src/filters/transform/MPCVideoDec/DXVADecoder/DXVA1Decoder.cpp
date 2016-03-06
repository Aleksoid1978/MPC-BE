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

#include "stdafx.h"
#include <moreuuids.h>
#include "DXVA1Decoder.h"
#include "DXVA1DecoderH264.h"
#include "DXVA1DecoderVC1.h"
#include "DXVA1DecoderMPEG2.h"
#include "../MPCVideoDec.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVA1Decoder::CDXVA1Decoder(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, int nPicEntryNumber)
	: CDXVADecoder()
	, m_pAMVideoAccelerator(pAMVideoAccelerator)
	, m_pFilter(pFilter)
	, m_dwBufferIndex(0)
	, m_nMaxWaiting(5)
	, m_nPicEntryNumber(nPicEntryNumber)
	, m_nWaitingPics(0)
	, m_dwNumBuffersInfo(0)
{
	m_pPictureStore = DNew PICTURE_STORE[nPicEntryNumber];

	memset(&m_DXVA1Config, 0, sizeof(m_DXVA1Config));
	memset(&m_DXVA1BufferDesc, 0, sizeof(m_DXVA1BufferDesc));
	m_DXVA1Config.guidConfigBitstreamEncryption	= DXVA_NoEncrypt;
	m_DXVA1Config.guidConfigMBcontrolEncryption	= DXVA_NoEncrypt;
	m_DXVA1Config.guidConfigResidDiffEncryption	= DXVA_NoEncrypt;
	m_DXVA1Config.bConfigBitstreamRaw			= 2;

	memset(&m_DXVA1BufferInfo, 0, sizeof(m_DXVA1BufferInfo));

	memset(&m_DXVA2Config, 0, sizeof(DXVA2_ConfigPictureDecode));
	memset(&m_dxva_context, 0, sizeof(dxva_context));
	m_pFilter->GetAVCtx()->dxva_context = &m_dxva_context;
}

CDXVA1Decoder::~CDXVA1Decoder()
{
	SAFE_DELETE_ARRAY(m_pPictureStore);
}

void CDXVA1Decoder::Flush()
{
	DbgLog((LOG_TRACE, 3, L"CDXVA1Decoder::Flush()"));

	if (m_pPictureStore) {
		for (int i = 0; i < m_nPicEntryNumber; i++) {
			m_pPictureStore[i].bRefPicture		= false;
			m_pPictureStore[i].bInUse			= false;
			m_pPictureStore[i].bDisplayed		= false;
			m_pPictureStore[i].nCodecSpecific	= -1;
			m_pPictureStore[i].dwDisplayCount	= 0;
			m_pPictureStore[i].rtStart			= INVALID_TIME;
			m_pPictureStore[i].rtStop			= INVALID_TIME;
		}
	}

	m_bFlushed			= true;
	m_nWaitingPics		= 0;
	m_nSurfaceIndex		= -1;
	m_dwDisplayCount	= 1;

	__super::Flush();
}

void CDXVA1Decoder::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	memcpy(pDXVABuffer, pBuffer, nSize);
}

HRESULT CDXVA1Decoder::ConfigureDXVA1()
{
	HRESULT						hr = S_FALSE;
	DXVA_ConfigPictureDecode	ConfigRequested;

	if (m_pAMVideoAccelerator) {
		memset(&ConfigRequested, 0, sizeof(ConfigRequested));
		ConfigRequested.guidConfigBitstreamEncryption	= DXVA_NoEncrypt;
		ConfigRequested.guidConfigMBcontrolEncryption	= DXVA_NoEncrypt;
		ConfigRequested.guidConfigResidDiffEncryption	= DXVA_NoEncrypt;
		ConfigRequested.bConfigBitstreamRaw				= 2;

		writeDXVA_QueryOrReplyFunc(&ConfigRequested.dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_PROBE_QUERY, DXVA_PICTURE_DECODING_FUNCTION);
		hr = m_pAMVideoAccelerator->Execute(ConfigRequested.dwFunction, &ConfigRequested, sizeof(DXVA_ConfigPictureDecode), &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), 0, NULL);

		// Copy to DXVA2 structure (simplify code based on accelerator config)
		m_DXVA2Config.ConfigBitstreamRaw			= m_DXVA1Config.bConfigBitstreamRaw;
		m_DXVA2Config.ConfigIntraResidUnsigned		= m_DXVA1Config.bConfigIntraResidUnsigned;
		m_DXVA2Config.ConfigResidDiffAccelerator	= m_DXVA1Config.bConfigResidDiffAccelerator;

		m_dxva_context.cfg       = &m_DXVA2Config;
		m_dxva_context.longslice = false;

		if (SUCCEEDED(hr)) {
			writeDXVA_QueryOrReplyFunc(&m_DXVA1Config.dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_LOCK_QUERY, DXVA_PICTURE_DECODING_FUNCTION);
			hr = m_pAMVideoAccelerator->Execute(m_DXVA1Config.dwFunction, &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), &ConfigRequested, sizeof(DXVA_ConfigPictureDecode), 0, NULL);

			AMVAUncompDataInfo	DataInfo;
			DWORD				dwNum = COMP_BUFFER_COUNT;
			DataInfo.dwUncompWidth	= m_pFilter->PictWidthAligned();
			DataInfo.dwUncompHeight	= m_pFilter->PictHeightAligned();
			memcpy(&DataInfo.ddUncompPixelFormat, m_pFilter->GetDXVA1PixelFormat(), sizeof(DDPIXELFORMAT));
			hr = m_pAMVideoAccelerator->GetCompBufferInfo(m_pFilter->GetDXVADecoderGuid(), &DataInfo, &dwNum, m_ComBufferInfo);
		}
	}
	return hr;
}

CDXVA1Decoder* CDXVA1Decoder::CreateDecoderDXVA1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, int nPicEntryNumber)
{
	CDXVA1Decoder* pDecoder = NULL;

	if ((*guidDecoder == DXVA2_ModeH264_E) || (*guidDecoder == DXVA2_ModeH264_F) || (*guidDecoder == DXVA_Intel_H264_ClearVideo)) {
		pDecoder = DNew CDXVA1DecoderH264(pFilter, pAMVideoAccelerator, nPicEntryNumber);
	} else if (*guidDecoder == DXVA2_ModeVC1_D || *guidDecoder == DXVA_ModeVC1_D2010) {
		pDecoder = DNew CDXVA1DecoderVC1(pFilter, pAMVideoAccelerator, nPicEntryNumber);
	} else if (*guidDecoder == DXVA2_ModeMPEG2_VLD) {
		pDecoder = DNew CDXVA1DecoderMPEG2(pFilter, pAMVideoAccelerator, nPicEntryNumber);
	} else {
		ASSERT(FALSE); // Unknown decoder !!
	}

	return pDecoder;
}

// === DXVA functions

HRESULT CDXVA1Decoder::AddExecuteBuffer(DWORD dwTypeIndex, UINT nSize, void* pBuffer, UINT* pRealSize)
{
	HRESULT	hr			= E_INVALIDARG;
	DWORD	dwNumMBs	= 0;
	BYTE*	pDXVABuffer;
	LONG	lStride;

	hr = m_pAMVideoAccelerator->GetBuffer(dwTypeIndex, m_dwBufferIndex, FALSE, (void**)&pDXVABuffer, &lStride);
	ASSERT(SUCCEEDED(hr));

	if (SUCCEEDED(hr)) {
		if (dwTypeIndex == DXVA_BITSTREAM_DATA_BUFFER) {
			CopyBitstream(pDXVABuffer, (BYTE*)pBuffer, nSize);
		} else {
			memcpy(pDXVABuffer, (BYTE*)pBuffer, nSize);
		}
		m_DXVA1BufferInfo[m_dwNumBuffersInfo].dwTypeIndex		= dwTypeIndex;
		m_DXVA1BufferInfo[m_dwNumBuffersInfo].dwBufferIndex		= m_dwBufferIndex;
		m_DXVA1BufferInfo[m_dwNumBuffersInfo].dwDataSize		= nSize;

		m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwTypeIndex		= dwTypeIndex;
		m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwBufferIndex		= m_dwBufferIndex;
		m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwDataSize		= nSize;
		m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwNumMBsInBuffer	= dwNumMBs;

		m_dwNumBuffersInfo++;
	}

	if (pRealSize) {
		*pRealSize = nSize;
	}

	return hr;
}

HRESULT CDXVA1Decoder::Execute()
{
	HRESULT hr = E_INVALIDARG;

	DWORD dwFunction;

	// writeDXVA_QueryOrReplyFunc(&dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_LOCK_QUERY, DXVA_PICTURE_DECODING_FUNCTION);
	// hr = m_pAMVideoAccelerator->Execute (dwFunction, &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), NULL, 0, m_dwNumBuffersInfo, m_DXVA1BufferInfo);

	DWORD dwResult;
	dwFunction = 0x01000000;
	hr = m_pAMVideoAccelerator->Execute(dwFunction, m_DXVA1BufferDesc, sizeof(DXVA_BufferDescription)*m_dwNumBuffersInfo,&dwResult, sizeof(dwResult), m_dwNumBuffersInfo, m_DXVA1BufferInfo);
	ASSERT(SUCCEEDED(hr));

	for (DWORD i = 0; i < m_dwNumBuffersInfo; i++) {
		HRESULT hr2 = m_pAMVideoAccelerator->ReleaseBuffer(m_DXVA1BufferInfo[i].dwTypeIndex, m_DXVA1BufferInfo[i].dwBufferIndex);
		ASSERT(SUCCEEDED(hr2));
	}

	m_dwNumBuffersInfo = 0;

	return hr;
}

HRESULT CDXVA1Decoder::FindFreeDXVA1Buffer(DWORD& dwBufferIndex)
{
	HRESULT	hr		= E_INVALIDARG;
	int		nTry	= 0;

	dwBufferIndex = 0; //(dwBufferIndex + 1) % m_ComBufferInfo[DXVA_PICTURE_DECODE_BUFFER].dwNumCompBuffers;
	DO_DXVA_PENDING_LOOP (m_pAMVideoAccelerator->QueryRenderStatus((DWORD)-1, dwBufferIndex, 0));

	return hr;
}

HRESULT CDXVA1Decoder::BeginFrame(int nSurfaceIndex)
{
	HRESULT	hr		= E_INVALIDARG;
	int		nTry	= 0;

	for (int i = 0; i < 20; i++) {
		AMVABeginFrameInfo BeginFrameInfo;

		BeginFrameInfo.dwDestSurfaceIndex	= nSurfaceIndex;
		BeginFrameInfo.dwSizeInputData		= sizeof(nSurfaceIndex);
		BeginFrameInfo.pInputData			= &nSurfaceIndex;
		BeginFrameInfo.dwSizeOutputData		= 0;
		BeginFrameInfo.pOutputData			= NULL;

		DO_DXVA_PENDING_LOOP (m_pAMVideoAccelerator->BeginFrame(&BeginFrameInfo));

		if (SUCCEEDED(hr)) {
			hr = FindFreeDXVA1Buffer(m_dwBufferIndex);
		}

		// For slow accelerator wait a little...
		if (SUCCEEDED(hr)) {
			break;
		}

		Sleep(1);
	}

	return hr;
}

HRESULT CDXVA1Decoder::EndFrame(int nSurfaceIndex)
{
	HRESULT	hr		= E_INVALIDARG;
	DWORD	dwDummy	= nSurfaceIndex;

	AMVAEndFrameInfo EndFrameInfo;
	EndFrameInfo.dwSizeMiscData	= sizeof(dwDummy);
	EndFrameInfo.pMiscData		= &dwDummy;

	hr = m_pAMVideoAccelerator->EndFrame(&EndFrameInfo);
	ASSERT(SUCCEEDED(hr));

	return hr;
}

// === Picture store functions
bool CDXVA1Decoder::AddToStore(int nSurfaceIndex, bool bRefPicture,
							   REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool bIsField,
							   int nCodecSpecific)
{
	if (bIsField && m_nSurfaceIndex == -1) {
		m_nSurfaceIndex = nSurfaceIndex;
		m_pPictureStore[nSurfaceIndex].rtStart			= rtStart;
		m_pPictureStore[nSurfaceIndex].rtStop			= rtStop;
		m_pPictureStore[nSurfaceIndex].nCodecSpecific	= nCodecSpecific;
		return false;
	} else {
		ASSERT(nSurfaceIndex < m_nPicEntryNumber);

		m_pPictureStore[nSurfaceIndex].bRefPicture		= bRefPicture;
		m_pPictureStore[nSurfaceIndex].bInUse			= true;
		m_pPictureStore[nSurfaceIndex].bDisplayed		= false;

		if (!bIsField) {
			m_pPictureStore[nSurfaceIndex].rtStart			= rtStart;
			m_pPictureStore[nSurfaceIndex].rtStop			= rtStop;
			m_pPictureStore[nSurfaceIndex].nCodecSpecific	= nCodecSpecific;
		}

		m_nSurfaceIndex	= -1;
		m_nWaitingPics++;
		return true;
	}
}

void CDXVA1Decoder::UpdateStore(int nSurfaceIndex, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	ASSERT((nSurfaceIndex < m_nPicEntryNumber) && m_pPictureStore[nSurfaceIndex].bInUse && !m_pPictureStore[nSurfaceIndex].bDisplayed);

	m_pPictureStore[nSurfaceIndex].rtStart	= rtStart;
	m_pPictureStore[nSurfaceIndex].rtStop	= rtStop;
}

void CDXVA1Decoder::RemoveRefFrame(int nSurfaceIndex)
{
	ASSERT(nSurfaceIndex < m_nPicEntryNumber);

	m_pPictureStore[nSurfaceIndex].bRefPicture = false;
	if (m_pPictureStore[nSurfaceIndex].bDisplayed) {
		FreePictureSlot(nSurfaceIndex);
	}
}

HRESULT CDXVA1Decoder::DisplayNextFrame()
{
	HRESULT	hr			= S_FALSE;
	int		nPicIndex	= FindOldestFrame();

	if (nPicIndex != -1) {
		if (m_pPictureStore[nPicIndex].rtStart >= 0) {
			// For DXVA1, query a media sample at the last time (only one in the allocator)
			CComPtr<IMediaSample> pSampleToDeliver;
			hr = m_pFilter->GetOutputPin()->GetDeliveryBuffer(&pSampleToDeliver, NULL, NULL, 0);
			if (SUCCEEDED(hr)) {
				m_pFilter->SetTypeSpecificFlags(pSampleToDeliver);
				pSampleToDeliver->SetTime(&m_pPictureStore[nPicIndex].rtStart, &m_pPictureStore[nPicIndex].rtStop);
				pSampleToDeliver->SetMediaTime(NULL, NULL);

				hr = m_pAMVideoAccelerator->DisplayFrame(nPicIndex, pSampleToDeliver);
			}

#if defined(_DEBUG) && 0
			static REFERENCE_TIME rtLast = 0;
			TRACE("Deliver : %10I64d - %10I64d, (Dur = %10I64d), {Delta = %10I64d}, Ind = %02d\n",
				   m_pPictureStore[nPicIndex].rtStart,
				   m_pPictureStore[nPicIndex].rtStop,
				   m_pPictureStore[nPicIndex].rtStop - m_pPictureStore[nPicIndex].rtStart,
				   m_pPictureStore[nPicIndex].rtStart - rtLast,
				   nPicIndex);
			rtLast = m_pPictureStore[nPicIndex].rtStart;
#endif
		}

		m_pPictureStore[nPicIndex].bDisplayed = true;
		if (!m_pPictureStore[nPicIndex].bRefPicture) {
			FreePictureSlot(nPicIndex);
		}
	}

	return hr;
}

HRESULT CDXVA1Decoder::GetFreeSurfaceIndex(int& nSurfaceIndex)
{
	int		nPos			= -1;
	DWORD	dwMinDisplay	= MAXDWORD;
	for (int i = 0; i < m_nPicEntryNumber; i++) {
		if (!m_pPictureStore[i].bInUse && m_pPictureStore[i].dwDisplayCount < dwMinDisplay) {
			dwMinDisplay = m_pPictureStore[i].dwDisplayCount;
			nPos  = i;
		}
	}

	if (nPos != -1) {
		nSurfaceIndex = nPos;
		return S_OK;
	}

	ASSERT(FALSE);
	Flush();

	return E_UNEXPECTED;
}

int CDXVA1Decoder::FindOldestFrame()
{
	REFERENCE_TIME	rtMin	= _I64_MAX;
	int				nPos	= -1;

	// TODO : find better solution...
	if (m_nWaitingPics > m_nMaxWaiting) {
		for (int i = 0; i < m_nPicEntryNumber; i++) {
			if (!m_pPictureStore[i].bDisplayed && m_pPictureStore[i].bInUse && (m_pPictureStore[i].rtStart < rtMin)) {
				rtMin	= m_pPictureStore[i].rtStart;
				nPos	= i;
			}
		}
	}

	return nPos;
}

void CDXVA1Decoder::FreePictureSlot(int nSurfaceIndex)
{
	if (nSurfaceIndex >= 0 && nSurfaceIndex < m_nPicEntryNumber) {
		m_pPictureStore[nSurfaceIndex].dwDisplayCount	= m_dwDisplayCount++;
		m_pPictureStore[nSurfaceIndex].bInUse			= false;
		m_pPictureStore[nSurfaceIndex].bDisplayed		= false;
		m_pPictureStore[nSurfaceIndex].nCodecSpecific	= -1;
		m_pPictureStore[nSurfaceIndex].rtStart			= INVALID_TIME;
		m_pPictureStore[nSurfaceIndex].rtStop			= INVALID_TIME;

		m_nWaitingPics--;
	}
}
