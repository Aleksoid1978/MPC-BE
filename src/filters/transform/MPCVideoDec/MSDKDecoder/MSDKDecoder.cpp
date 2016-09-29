/*
*      Copyright (C) 2010-2016 Hendrik Leppkes
*      http://www.1f0.de
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*  Adaptation for MPC-BE (C) 2016 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
*/

#include "stdafx.h"

#pragma warning(disable: 4005 4244)
extern "C" {
  #include <ffmpeg/libavcodec/avcodec.h>
  #include <ffmpeg/libavutil/imgutils.h>
  #include <ffmpeg/libavutil/intreadwrite.h>
}
#pragma warning(default: 4005 4244)

#include "MSDKDecoder.h"
#include <moreuuids.h>
#include <IMediaSample3D.h>
#include "../../../../DSUtil/D3D9Helper.h"
#include "../../../../DSUtil/DSUtil.h"
#include "../../../../DSUtil/SysVersion.h"
#include "ByteParser.h"

#include "../MPCVideoDec.h"
#include "../pixconv_sse2_templates.h"
#include <intrin.h>

////////////////////////////////////////////////////////////////////////////////
// Bitstream buffering
////////////////////////////////////////////////////////////////////////////////

class CBitstreamBuffer {
public:
  CBitstreamBuffer(GrowableArray<BYTE> * arrStorage) : m_pStorage(arrStorage) {}

  ~CBitstreamBuffer() {
    if (m_pBuffer) {
      ASSERT(m_nConsumed <= m_nBufferSize);
      if (m_nConsumed < m_nBufferSize)
        m_pStorage->Append(m_pBuffer + m_nConsumed, m_nBufferSize - m_nConsumed);

      if (m_bBufferTemporary)
        av_freep(&m_pBuffer);
    }
    else {
      ASSERT(m_nConsumed <= m_pStorage->GetCount());
      if (m_nConsumed < m_pStorage->GetCount()) {
        BYTE *p = m_pStorage->Ptr();
        memmove(p, p + m_nConsumed, m_pStorage->GetCount() - m_nConsumed);
        m_pStorage->SetSize(m_pStorage->GetCount() - m_nConsumed);
      }
      else {
        m_pStorage->Clear();
      }
    }
  }

  void SetBuffer(BYTE * buffer, size_t size, bool temporary) {
    if (m_pStorage->GetCount() > 0) {
      m_pStorage->Append(buffer, size);

      if (temporary)
        av_free(buffer);
    }
    else {
      m_pBuffer = buffer;
      m_nBufferSize = size;
      m_bBufferTemporary = temporary;
    }
  }

  void Consume(size_t count) {
    m_nConsumed += min(count, GetBufferSize());
  }

  void Clear() {
    m_nConsumed += GetBufferSize();
  }

  BYTE * GetBuffer() {
    if (m_pBuffer) {
      return m_pBuffer + m_nConsumed;
    }
    else {
      return m_pStorage->Ptr() + m_nConsumed;
    }
  }

  size_t GetBufferSize() {
    if (m_nBufferSize) {
      return m_nBufferSize - m_nConsumed;
    }
    else {
      return m_pStorage->GetCount() - m_nConsumed;
    }
  }

  void EnsureWriteable() {
    if (m_pBuffer && !m_bBufferTemporary) {
      m_pStorage->Append(m_pBuffer, m_nBufferSize);
      m_pBuffer = nullptr;
    }
  }

private:
  GrowableArray<BYTE> * m_pStorage = nullptr;

  BOOL m_bBufferTemporary = FALSE;
  BYTE * m_pBuffer = nullptr;
  size_t m_nBufferSize = 0;
  size_t m_nConsumed = 0;
};

CMSDKDecoder::CMSDKDecoder(CMPCVideoDecFilter* pFilter)
  : m_pFilter(pFilter)
{
  m_iOutputMode = m_iNewOutputMode = m_pFilter->m_iMvcOutputMode;
  m_bSwapLR = m_pFilter->m_iMvcSwapLR;

  int info[4] = { 0 };
  __cpuid(info, 0);
  if (info[0] >= 1) {
    __cpuid(info, 0x00000001);
    m_bSSE2 = (info[3] & (1 << 26)) != 0;
  }
}

CMSDKDecoder::~CMSDKDecoder()
{
  DestroyDecoder(true);
}

static UINT GetIntelAdapterIdD3D9()
{
  CComPtr<IDirect3D9> pD3D9 = D3D9Helper::Direct3DCreate9();
  if (pD3D9) {
    D3DADAPTER_IDENTIFIER9 adIdentifier;
    for (UINT adp = 0, num_adp = pD3D9->GetAdapterCount(); adp < num_adp; ++adp) {
      if (SUCCEEDED(pD3D9->GetAdapterIdentifier(adp, 0, &adIdentifier))
          && adIdentifier.VendorId == 0x8086) {
        return adp;
      }
    }
  }

  return UINT_MAX;
}

HRESULT CMSDKDecoder::Init()
{
  const mfxIMPL impls[] = { MFX_IMPL_AUTO_ANY, MFX_IMPL_SOFTWARE };
  for (int i = 0; i < _countof(impls); i++) {
    mfxIMPL impl = impls[i];
    mfxVersion version = { 8, 1 };

    mfxStatus sts = MFXInit(impl, &version, &m_mfxSession);
    if (sts != MFX_ERR_NONE) {
      DLog(L"CMSDKDecoder::Init(): MSDK not available");
      return E_NOINTERFACE;
    }

    // query actual API version
    MFXQueryVersion(m_mfxSession, &version);
    // query implementation
    MFXQueryIMPL(m_mfxSession, &impl);

    const bool bHwAcceleration = impl != MFX_IMPL_SOFTWARE;
    const bool bUseD3D9Alloc   = bHwAcceleration && ((impl & MFX_IMPL_VIA_D3D9) == MFX_IMPL_VIA_D3D9);
    const bool bUseD3D11Alloc  = bHwAcceleration && ((impl & MFX_IMPL_VIA_D3D11) == MFX_IMPL_VIA_D3D11);

    DLog(L"CMSDKDecoder::Init(): MSDK Initialized, version %d.%d, impl 0x%04x, using %s decoding", version.Major, version.Minor, impl, bHwAcceleration ? L"hardware" : L"software");

    if (bUseD3D11Alloc) {
      if (!IsWin8orLater()) {
        DLog(L"CMSDKDecoder::Init(): hardware decoding via D3D11 supported only in Windows 8 and higher");
        if (m_mfxSession) {
          MFXClose(m_mfxSession);
          m_mfxSession = nullptr;
        }
        continue;
      }
    } else if (bUseD3D9Alloc) {
      const UINT d3d9IntelAdapter = GetIntelAdapterIdD3D9();
      if (d3d9IntelAdapter == UINT_MAX) {
        DLog(L"CMSDKDecoder::Init(): hardware decoding via D3D9 supported only if iGPU connected to the screen");
        if (m_mfxSession) {
          MFXClose(m_mfxSession);
          m_mfxSession = nullptr;
        }
        continue;
      }
    }

    break;
  }

  return S_OK;
}

void CMSDKDecoder::DestroyDecoder(bool bFull)
{
  if (m_bDecodeReady) {
    MFXVideoDECODE_Close(m_mfxSession);
    m_bDecodeReady = FALSE;
  }

  {
    CAutoLock lock(&m_BufferCritSec);
    for (int i = 0; i < ASYNC_QUEUE_SIZE; i++) {
      ReleaseBuffer(&m_pOutputQueue[i]->surface);
    }
    memset(m_pOutputQueue, 0, sizeof(m_pOutputQueue));

    for (auto it = m_BufferQueue.begin(); it != m_BufferQueue.end(); it++) {
      if (!(*it)->queued) {
        av_freep(&(*it)->surface.Data.Y);
        delete (*it);
      }
    }
    m_BufferQueue.clear();
  }

  // delete MVC sequence buffers
  SAFE_DELETE(m_mfxExtMVCSeq.View);
  SAFE_DELETE(m_mfxExtMVCSeq.ViewId);
  SAFE_DELETE(m_mfxExtMVCSeq.OP);

  SAFE_DELETE(m_pAnnexBConverter);

  av_frame_free(&m_pFrame);

  if (bFull) {
    if (m_mfxSession) {
      MFXClose(m_mfxSession);
      m_mfxSession = nullptr;
    }
  }
}

HRESULT CMSDKDecoder::InitDecoder(const CMediaType *pmt)
{
  if (*pmt->FormatType() != FORMAT_MPEG2Video)
    return E_UNEXPECTED;
  if (FAILED(Init()))
    return E_UNEXPECTED;

  DestroyDecoder(false);

  // Init and reset video param arrays
  memset(&m_mfxVideoParams, 0, sizeof(m_mfxVideoParams));
  m_mfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;

  memset(&m_mfxExtMVCSeq, 0, sizeof(m_mfxExtMVCSeq));
  m_mfxExtMVCSeq.Header.BufferId = MFX_EXTBUFF_MVC_SEQ_DESC;
  m_mfxExtMVCSeq.Header.BufferSz = sizeof(m_mfxExtMVCSeq);
  m_mfxExtParam[0] = (mfxExtBuffer *)&m_mfxExtMVCSeq;

  // Attach ext params to VideoParams
  m_mfxVideoParams.ExtParam = m_mfxExtParam;
  m_mfxVideoParams.NumExtParam = 1;

  MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)pmt->Format();

  if (*pmt->Subtype() == MEDIASUBTYPE_MVC1) {
    m_pAnnexBConverter = new CAnnexBConverter();
    m_pAnnexBConverter->SetNALUSize(2);

    // Decode sequence header from the media type
    if (mp2vi->cbSequenceHeader) {
      HRESULT hr = Decode((const BYTE *)mp2vi->dwSequenceHeader, mp2vi->cbSequenceHeader, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
      if (FAILED(hr))
        return hr;
    }

    m_pAnnexBConverter->SetNALUSize(mp2vi->dwFlags);
  }
  else if (*pmt->Subtype() == MEDIASUBTYPE_AMVC) {
    // Decode sequence header from the media type
    if (mp2vi->cbSequenceHeader) {
      HRESULT hr = Decode((const BYTE *)mp2vi->dwSequenceHeader, mp2vi->cbSequenceHeader, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
      if (FAILED(hr))
        return hr;
    }
  }

  return S_OK;
}

HRESULT CMSDKDecoder::ParseSEI(const BYTE *buffer, size_t size, mfxU64 timestamp)
{
  CByteParser seiParser(buffer, size);
  while (seiParser.RemainingBits() > 16 && seiParser.BitRead(16, true)) {
    int type = 0;
    unsigned size = 0;

    do {
      if (seiParser.RemainingBits() < 8)
        return E_FAIL;
      type += seiParser.BitRead(8, true);
    } while (seiParser.BitRead(8) == 0xFF);

    do {
      if (seiParser.RemainingBits() < 8)
        return E_FAIL;
      size += seiParser.BitRead(8, true);
    } while (seiParser.BitRead(8) == 0xFF);

    if (size > seiParser.Remaining()) {
      DLog(L"CMSDKDecoder::ParseSEI(): SEI type %d size %d truncated, available: %d", type, size, seiParser.Remaining());
      return E_FAIL;
    }

    switch (type) {
    case 5:
      ParseUnregUserDataSEI(buffer + seiParser.Pos(), size, timestamp);
      break;
    case 37:
      ParseMVCNestedSEI(buffer + seiParser.Pos(), size, timestamp);
      break;
    }

    seiParser.BitSkip(size * 8);
  }

  return S_OK;
}

HRESULT CMSDKDecoder::ParseMVCNestedSEI(const BYTE *buffer, size_t size, mfxU64 timestamp)
{
  CByteParser seiParser(buffer, size);

  // Parse the MVC Scalable Nesting SEI first
  int op_flag = seiParser.BitRead(1);
  if (!op_flag) {
    int all_views_in_au = seiParser.BitRead(1);
    if (!all_views_in_au) {
      int num_views_min1 = seiParser.UExpGolombRead();
      for (int i = 0; i <= num_views_min1; i++) {
        seiParser.BitRead(10); // sei_view_id[i]
      }
    }
  }
  else {
    int num_views_min1 = seiParser.UExpGolombRead();
    for (int i = 0; i <= num_views_min1; i++) {
      seiParser.BitRead(10); // sei_op_view_id[i]
    }
    seiParser.BitRead(3); // sei_op_temporal_id
  }
  seiParser.BitByteAlign();

  // Parse nested SEI
  ParseSEI(buffer + seiParser.Pos(), seiParser.Remaining(), timestamp);

  return S_OK;
}

static const uint8_t uuid_iso_iec_11578[16] = {
  0x17, 0xee, 0x8c, 0x60, 0xf8, 0x4d, 0x11, 0xd9, 0x8c, 0xd6, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

HRESULT CMSDKDecoder::ParseUnregUserDataSEI(const BYTE *buffer, size_t size, mfxU64 timestamp)
{
  if (size < 20)
    return E_FAIL;

  if (memcmp(buffer, uuid_iso_iec_11578, 16) != 0) {
    DLog(L"CMSDKDecoder::ParseUnregUserDataSEI(): Unknown User Data GUID");
    return S_FALSE;
  }

  uint32_t type = AV_RB32(buffer + 16);

  // Offset metadata
  if (type == 0x4F464D44) {
    return ParseOffsetMetadata(buffer + 20, size - 20, timestamp);
  }

  return S_FALSE;
}

HRESULT CMSDKDecoder::ParseOffsetMetadata(const BYTE *buffer, size_t size, mfxU64 timestamp)
{
  if (size < 10)
    return E_FAIL;

  // Skip PTS part, its not used. Start parsing at first marker bit after the PTS
  CByteParser offset(buffer + 6, size - 6);
  offset.BitSkip(2); // Skip marker and re served

  unsigned int nOffsets = offset.BitRead(6);
  unsigned int nFrames = offset.BitRead(8);
  DLog(L"CMSDKDecoder::ParseOffsetMetadata(): offset_metadata with %d offsets and %d frames for time %I64u", nOffsets, nFrames, timestamp);

  if (nOffsets > 32) {
    DLog(L"CMSDKDecoder::ParseOffsetMetadata(): > 32 offsets is not supported");
    return E_FAIL;
  }

  offset.BitSkip(16); // Skip marker and reserved
  if (nOffsets * nFrames > (size - 10)) {
    DLog(L"CMSDKDecoder::ParseOffsetMetadata(): not enough data for all offsets (need %d, have %d)", nOffsets * nFrames, size - 4);
    return E_FAIL;
  }

  MVCGOP GOP;

  for (unsigned int o = 0; o < nOffsets; o++) {
    for (unsigned int f = 0; f < nFrames; f++) {
      if (o == 0) {
        MediaSideData3DOffset off = { (int)nOffsets };
        GOP.offsets.push_back(off);
      }

      int direction_flag = offset.BitRead(1);
      int value = offset.BitRead(7);

      if (direction_flag)
        value = -value;

      GOP.offsets[f].offset[o] = value;
    }
  }

  m_GOPs.push_back(GOP);

  return S_OK;
}

void CMSDKDecoder::AddFrameToGOP(mfxU64 timestamp)
{
  if (m_GOPs.size() > 0)
    m_GOPs.back().timestamps.push_back(timestamp);
}

BOOL CMSDKDecoder::RemoveFrameFromGOP(MVCGOP * pGOP, mfxU64 timestamp)
{
  if (pGOP->timestamps.empty() || pGOP->offsets.empty())
    return FALSE;

  auto e = std::find(pGOP->timestamps.begin(), pGOP->timestamps.end(), timestamp);
  if (e != pGOP->timestamps.end()) {
    pGOP->timestamps.erase(e);
    return TRUE;
  }

  return FALSE;
}

void CMSDKDecoder::GetOffsetSideData(IMediaSample* pSample, mfxU64 timestamp)
{
  MediaSideData3DOffset offset = { 255 };

  // Go over all stored GOPs and find an entry for our timestamp
  // In general it should be found in the first GOP, unless we lost frames in between or something else went wrong.
  for (auto it = m_GOPs.begin(); it != m_GOPs.end(); it++) {
    if (RemoveFrameFromGOP(&(*it), timestamp)) {
      offset = it->offsets.front();
      it->offsets.pop_front();

      // Erase previous GOPs when we start accessing a new one
      if (it != m_GOPs.begin()) {
#ifdef DEBUG
        // Check that all to-be-erased GOPs are empty
        for (auto itd = m_GOPs.begin(); itd < it; itd++) {
          if (!itd->offsets.empty()) {
            DLog(L"CMSDKDecoder::GetOffsetSideData(): Switched to next GOP at %I64u with %Iu entries remaining", timestamp, itd->offsets.size());
          }
        }
#endif
        m_GOPs.erase(m_GOPs.begin(), it);
      }
      break;
    }
  }

  if (offset.offset_count == 255) {
    DLog(L"CMSDKDecoder::GetOffsetSideData(): No offset for frame at %I64u", timestamp);
    offset = m_PrevOffset;
  }

  m_PrevOffset = offset;

  // Only set the offset when data is present
  if (offset.offset_count > 0) {
    CComPtr<IMediaSideData> pMediaSideData;
    if (SUCCEEDED(pSample->QueryInterface(&pMediaSideData))) {
      pMediaSideData->SetSideData(IID_MediaSideData3DOffset, (const BYTE*)&offset, sizeof(offset));
    }
  }
}

void CMSDKDecoder::SetTypeSpecificFlags(IMediaSample* pSample)
{
  m_pFilter->m_bInterlaced = TRUE;
  if (CComQIPtr<IMediaSample2> pMS2 = pSample) {
    AM_SAMPLE2_PROPERTIES props;
    if (SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))) {
      props.dwTypeSpecificFlags &= ~0x7f;

      switch (m_pFilter->m_nDeinterlacing) {
        case AUTO :
        case PROGRESSIVE :
          props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_WEAVE;
          break;
        case TOPFIELD :
          props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_FIELD1FIRST;
          break;
      }

      pMS2->SetProperties(sizeof(props), (BYTE*)&props);
    }
  }
}

HRESULT CMSDKDecoder::HandleOutput(MVCBuffer * pOutputBuffer)
{
  int nCur = m_nOutputQueuePosition, nNext = (m_nOutputQueuePosition + 1) % ASYNC_QUEUE_SIZE;
  if (m_pOutputQueue[nCur] && m_pOutputQueue[nNext]) {
    DeliverOutput(m_pOutputQueue[nCur], m_pOutputQueue[nNext]);

    m_pOutputQueue[nCur] = nullptr;
    m_pOutputQueue[nNext] = nullptr;
  }
  else if (m_pOutputQueue[nCur]) {
    DLog(L"CMSDKDecoder::HandleOutput(): Dropping unpaired frame");

    ReleaseBuffer(&m_pOutputQueue[nCur]->surface);
    m_pOutputQueue[nCur]->sync = nullptr;
    m_pOutputQueue[nCur] = nullptr;
  }

  m_pOutputQueue[nCur] = pOutputBuffer;
  m_nOutputQueuePosition = nNext;

  return S_OK;
}

HRESULT CMSDKDecoder::DeliverOutput(MVCBuffer * pBaseView, MVCBuffer * pExtraView)
{
  mfxStatus sts = MFX_ERR_NONE;

  ASSERT(pBaseView->surface.Info.FrameId.ViewId == 0 && pExtraView->surface.Info.FrameId.ViewId > 0);
  ASSERT(pBaseView->surface.Data.FrameOrder == pExtraView->surface.Data.FrameOrder);

  // Sync base view
  do {
    sts = MFXVideoCORE_SyncOperation(m_mfxSession, pBaseView->sync, 1000);
  } while (sts == MFX_WRN_IN_EXECUTION);
  pBaseView->sync = nullptr;

  // Sync extra view
  do {
    sts = MFXVideoCORE_SyncOperation(m_mfxSession, pExtraView->sync, 1000);
  } while (sts == MFX_WRN_IN_EXECUTION);
  pExtraView->sync = nullptr;

  if (!(pBaseView->surface.Data.DataFlag & MFX_FRAMEDATA_ORIGINAL_TIMESTAMP))
    pBaseView->surface.Data.TimeStamp = MFX_TIMESTAMP_UNKNOWN;

  REFERENCE_TIME rtStart = INVALID_TIME;
  REFERENCE_TIME rtStop  = INVALID_TIME;
  if (pBaseView->surface.Data.TimeStamp != MFX_TIMESTAMP_UNKNOWN) {
    rtStart = pBaseView->surface.Data.TimeStamp;
    rtStart -= TIMESTAMP_OFFSET;
  }

  m_pFilter->UpdateFrameTime(rtStart, rtStop);

  const int width  = pBaseView->surface.Info.CropW;
  const int height = pBaseView->surface.Info.CropH;

  auto allocateFrame = [&](bool bMain) {
    if (bMain) {
      m_pFrame->data[0]   = pBaseView->surface.Data.Y;
      m_pFrame->data[1]   = pBaseView->surface.Data.UV;
    } else {
      m_pFrame->data[0]   = pExtraView->surface.Data.Y;
      m_pFrame->data[1]   = pExtraView->surface.Data.UV;
    }
    m_pFrame->linesize[0] = pBaseView->surface.Data.PitchLow;
    m_pFrame->linesize[1] = pBaseView->surface.Data.PitchLow;
  };

  CComPtr<IMediaSample> pOut;
  BYTE* pDataOut = nullptr;

  HRESULT hr = S_OK;

  if (m_iNewOutputMode != m_iOutputMode) {
    av_frame_free(&m_pFrame);
    m_iOutputMode = m_iNewOutputMode;
  }

  if (m_pFrame && (m_pFrame->width != width || m_pFrame->height != height)) {
    av_frame_free(&m_pFrame);
  }

  if (!m_pFrame) {
    m_pFrame = av_frame_alloc();
    if (!m_pFrame) {
      hr = E_POINTER;
      goto error;
    }

    m_pFrame->format      = AV_PIX_FMT_NV12;
    m_pFrame->width       = width;
    m_pFrame->height      = height;
    m_pFrame->colorspace  = AVCOL_SPC_UNSPECIFIED;
    m_pFrame->color_range = AVCOL_RANGE_UNSPECIFIED;
  }

  if (rtStart >= 0
      && SUCCEEDED(hr = m_pFilter->GetDeliveryBuffer(width, height, &pOut, m_pFilter->GetFrameDuration()))
      && SUCCEEDED(hr = pOut->GetPointer(&pDataOut))) {
    bool bMediaSample3DSupport = false;
    if (m_iOutputMode == MVC_OUTPUT_Auto) {
      // Write the second view into IMediaSample3D, if available
      CComPtr<IMediaSample3D> pSample3D;
      if (SUCCEEDED(hr = pOut->QueryInterface(&pSample3D))) {
        BYTE *pDataOut3D = nullptr;
        if (SUCCEEDED(pSample3D->Enable3D()) && SUCCEEDED(pSample3D->GetPointer3D(&pDataOut3D))) {
          bMediaSample3DSupport = true;
          allocateFrame(false);

          m_pFilter->m_FormatConverter.Converting(pDataOut3D, m_pFrame);
        }
      }
    }

    if (m_iOutputMode == MVC_OUTPUT_TopBottom
        || (m_iOutputMode == MVC_OUTPUT_Auto && !bMediaSample3DSupport)) {
      if (!m_pFrame->data[0] && av_frame_get_buffer(m_pFrame, 64) < 0) {
        hr = E_POINTER;
        goto error;
      }

      const unsigned half_lines = height / 2;
      const unsigned half_chromalines = half_lines / 2;
      const size_t linesize = pBaseView->surface.Data.PitchLow;
      unsigned line;

      // luminance
      uint8_t* dstBase = m_pFrame->data[0];
      uint8_t* dstExtra = dstBase + (half_lines)* linesize;;

      bool swapLR = !!m_pFilter->m_MVC_Base_View_R_flag;
      if (m_bSwapLR) {
        swapLR != swapLR;
      }
      uint8_t* srcBase = swapLR ? pExtraView->surface.Data.Y : pBaseView->surface.Data.Y;
      uint8_t* srcExtra = (swapLR ? pBaseView->surface.Data.Y : pExtraView->surface.Data.Y) + linesize;

      if (m_bSSE2 && ((size_t)dstBase % 16u) == 0 && ((size_t)dstExtra % 16u) == 0) {
        for (line = 0; line < half_lines; line++) {
          PIXCONV_MEMCPY_ALIGNED_TWO(dstBase, srcBase, dstExtra, srcExtra, (ptrdiff_t)linesize)
      
          dstBase += linesize;
          srcBase += linesize * 2;

          dstExtra += linesize;
          srcExtra += linesize * 2;
        }
      } else {
        for (line = 0; line < half_lines; line++) {
          memcpy(dstBase, srcBase, linesize);
          dstBase += linesize;
          srcBase += linesize * 2;

          memcpy(dstExtra, srcExtra, linesize);
          dstExtra += linesize;
          srcExtra += linesize * 2;
        }
      }

      // color
      dstBase  = m_pFrame->data[1];
      dstExtra = dstBase + (half_chromalines) * linesize;
      srcBase  = swapLR ? pExtraView->surface.Data.UV : pBaseView->surface.Data.UV;
      srcExtra = (swapLR ? pBaseView->surface.Data.UV : pExtraView->surface.Data.UV) + linesize;

      if (m_bSSE2 && ((size_t)dstBase % 16u) == 0 && ((size_t)dstExtra % 16u) == 0) {
        for (line = 0; line < half_chromalines; line++) {
          PIXCONV_MEMCPY_ALIGNED_TWO(dstBase, srcBase, dstExtra, srcExtra, (ptrdiff_t)linesize);

          dstBase += linesize;
          srcBase += linesize * 2;

          dstExtra += linesize;
          srcExtra += linesize * 2;
        }
      } else {
        for (line = 0; line < half_chromalines; line++) {
          memcpy(dstBase, srcBase, linesize);
          dstBase += linesize;
          srcBase += linesize * 2;

          memcpy(dstExtra, srcExtra, linesize);
          dstExtra += linesize;
          srcExtra += linesize * 2;
        }
      }
    } else {
      allocateFrame(true);
    }

    m_pFilter->m_FormatConverter.Converting(pDataOut, m_pFrame);

    pOut->SetTime(&rtStart, &rtStop);
    pOut->SetMediaTime(NULL, NULL);

    GetOffsetSideData(pOut, pBaseView->surface.Data.TimeStamp);

    SetTypeSpecificFlags(pOut);

    hr = m_pFilter->m_pOutput->Deliver(pOut);
  }

error:
  {
    MVCBuffer * pStoredBuffer = FindBuffer(&pBaseView->surface);
    if (pStoredBuffer) {
      ReleaseBuffer(&pBaseView->surface);
    } else {
      av_free(pBaseView->surface.Data.Y);
      SAFE_DELETE(pBaseView);
    }

    pStoredBuffer = FindBuffer(&pExtraView->surface);
    if (pStoredBuffer) {
      ReleaseBuffer(&pExtraView->surface);
    } else {
      av_free(pExtraView->surface.Data.Y);
      SAFE_DELETE(pExtraView);
    }
  }

  return hr;
}

void CMSDKDecoder::Flush()
{
  m_buff.Clear();

  if (m_mfxSession) {
    if (m_bDecodeReady)
      MFXVideoDECODE_Reset(m_mfxSession, &m_mfxVideoParams);
    // TODO: decode sequence data

    for (int i = 0; i < ASYNC_QUEUE_SIZE; i++) {
      ReleaseBuffer(&m_pOutputQueue[i]->surface);
    }
    memset(m_pOutputQueue, 0, sizeof(m_pOutputQueue));
  }

  m_GOPs.clear();
  memset(&m_PrevOffset, 0, sizeof(m_PrevOffset));
}

HRESULT CMSDKDecoder::EndOfStream()
{
  if (!m_bDecodeReady)
    return S_FALSE;

  // Flush frames out of the decoder
  Decode(nullptr, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

  // Process all remaining frames in the queue
  for (int i = 0; i < ASYNC_QUEUE_SIZE; i++) {
    int nCur = (m_nOutputQueuePosition + i) % ASYNC_QUEUE_SIZE, nNext = (m_nOutputQueuePosition + i + 1) % ASYNC_QUEUE_SIZE;
    if (m_pOutputQueue[nCur] && m_pOutputQueue[nNext]) {
      DeliverOutput(m_pOutputQueue[nCur], m_pOutputQueue[nNext]);

      m_pOutputQueue[nCur] = nullptr;
      m_pOutputQueue[nNext] = nullptr;

      i++;
    }
    else if (m_pOutputQueue[nCur]) {
      DLog(L"CMSDKDecoder::EndOfStream(): Dropping unpaired frame");

      ReleaseBuffer(&m_pOutputQueue[nCur]->surface);
      m_pOutputQueue[nCur] = nullptr;
    }
  }
  m_nOutputQueuePosition = 0;

  return S_OK;
}

HRESULT CMSDKDecoder::Decode(const BYTE *buffer, int buflen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
  if (!m_mfxSession)
    return E_UNEXPECTED;

  HRESULT hr = S_OK;
  CBitstreamBuffer bsBuffer(&m_buff);
  mfxStatus sts = MFX_ERR_NONE;
  mfxBitstream bs = { 0 };
  BOOL bFlush = (buffer == nullptr);

  if (rtStart >= -TIMESTAMP_OFFSET && rtStart != AV_NOPTS_VALUE)
    bs.TimeStamp = rtStart + TIMESTAMP_OFFSET;
  else
    bs.TimeStamp = MFX_TIMESTAMP_UNKNOWN;

  bs.DecodeTimeStamp = MFX_TIMESTAMP_UNKNOWN;

  if (!bFlush) {
    if (m_pAnnexBConverter) {
      BYTE *pOutBuffer = nullptr;
      int pOutSize = 0;
      hr = m_pAnnexBConverter->Convert(&pOutBuffer, &pOutSize, buffer, buflen);
      if (FAILED(hr))
        return hr;

      bsBuffer.SetBuffer(pOutBuffer, pOutSize, true);
    }
    else {
      bsBuffer.SetBuffer((BYTE *)buffer, buflen, false);
    }

#if (FALSE)
    DLog(L"CMSDKDecoder::Decode(): Frame %I64u, size %u", bs.TimeStamp, bsBuffer.GetBufferSize());
#endif

    // Check the buffer for SEI NALU, and some unwanted NALUs that need filtering
    // MSDK's SEI reading functionality is slightly buggy
    CH264Nalu nalu;
    nalu.SetBuffer(bsBuffer.GetBuffer(), bsBuffer.GetBufferSize(), 0);
    BOOL bNeedFilter = FALSE;
    while (nalu.ReadNext()) {
      if (nalu.GetType() == NALU_TYPE_SEI) {
        ParseSEI(nalu.GetDataBuffer() + 1, nalu.GetDataLength() - 1, bs.TimeStamp);
      }
      else if (nalu.GetType() == NALU_TYPE_EOSEQ) {
        bsBuffer.EnsureWriteable();
        // This is rather ugly, and relies on the bitstream being AnnexB, so simply overwriting the EOS NAL with zero works.
        // In the future a more elaborate bitstream filter might be advised
        memset(bsBuffer.GetBuffer() + nalu.GetNALPos(), 0, 4);
      }
    }

    bs.Data = bsBuffer.GetBuffer();
    bs.DataLength = bsBuffer.GetBufferSize();
    bs.MaxLength = bs.DataLength;

    AddFrameToGOP(bs.TimeStamp);
  }

  if (!m_bDecodeReady) {
    sts = MFXVideoDECODE_DecodeHeader(m_mfxSession, &bs, &m_mfxVideoParams);
    if (sts == MFX_ERR_NOT_ENOUGH_BUFFER) {
      hr = AllocateMVCExtBuffers();
      if (FAILED(hr))
        return hr;

      sts = MFXVideoDECODE_DecodeHeader(m_mfxSession, &bs, &m_mfxVideoParams);
    }

    if (sts == MFX_ERR_NONE) {
      m_mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
      m_mfxVideoParams.AsyncDepth = ASYNC_DEPTH;

      sts = MFXVideoDECODE_Init(m_mfxSession, &m_mfxVideoParams);
      if (sts != MFX_ERR_NONE) {
        DLog(L"CMSDKDecoder::Decode(): Error initializing the MSDK decoder (%d)", sts);
        return E_FAIL;
      }

      if (m_mfxExtMVCSeq.NumView != 2) {
        DLog(L"CMSDKDecoder::Decode(): Only MVC with two views is supported");
        return E_FAIL;
      }

      DLog(L"CMSDKDecoder::Decode(): Initialized MVC with View Ids %d, %d", m_mfxExtMVCSeq.View[0].ViewId, m_mfxExtMVCSeq.View[1].ViewId);

      m_bDecodeReady = TRUE;
    }
  }

  if (!m_bDecodeReady)
    return S_FALSE;

  mfxSyncPoint sync = nullptr;

  // Loop over the decoder to ensure all data is being consumed
  while (1) {
    MVCBuffer *pInputBuffer = GetBuffer();
    mfxFrameSurface1 *outsurf = nullptr;
    sts = MFXVideoDECODE_DecodeFrameAsync(m_mfxSession, bFlush ? nullptr : &bs, &pInputBuffer->surface, &outsurf, &sync);

    if (sts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM) {
      DLog(L"CMSDKDecoder::Decode(): Incompatible video parameters detected, flushing decoder");
      bsBuffer.Clear();
      bFlush = TRUE;
      m_bDecodeReady = FALSE;
      continue;
    }

    if (sync) {
      MVCBuffer * pOutputBuffer = FindBuffer(outsurf);
      pOutputBuffer->queued = 1;
      pOutputBuffer->sync = sync;
      HandleOutput(pOutputBuffer);
      continue;
    }

    if (sts != MFX_ERR_MORE_SURFACE && sts < 0)
      break;
  }

  if (!bs.DataOffset && !sync && !bFlush) {
    DLog(L"CMSDKDecoder::Decode(): Decoder did not consume any data, discarding");
    bs.DataOffset = bsBuffer.GetBufferSize();
  }

  bsBuffer.Consume(bs.DataOffset);

  if (sts != MFX_ERR_MORE_DATA && sts < 0) {
    DLog(L"CMSDKDecoder::Decode(): Error from Decode call (%d)", sts);
    return S_FALSE;
  }

  return S_OK;
}

HRESULT CMSDKDecoder::AllocateMVCExtBuffers()
{
  mfxU32 i;
  SAFE_DELETE(m_mfxExtMVCSeq.View);
  SAFE_DELETE(m_mfxExtMVCSeq.ViewId);
  SAFE_DELETE(m_mfxExtMVCSeq.OP);

  m_mfxExtMVCSeq.View = new mfxMVCViewDependency[m_mfxExtMVCSeq.NumView];
  CheckPointer(m_mfxExtMVCSeq.View, E_OUTOFMEMORY);
  for (i = 0; i < m_mfxExtMVCSeq.NumView; ++i)
  {
    memset(&m_mfxExtMVCSeq.View[i], 0, sizeof(m_mfxExtMVCSeq.View[i]));
  }
  m_mfxExtMVCSeq.NumViewAlloc = m_mfxExtMVCSeq.NumView;

  m_mfxExtMVCSeq.ViewId = new mfxU16[m_mfxExtMVCSeq.NumViewId];
  CheckPointer(m_mfxExtMVCSeq.ViewId, E_OUTOFMEMORY);
  for (i = 0; i < m_mfxExtMVCSeq.NumViewId; ++i)
  {
    memset(&m_mfxExtMVCSeq.ViewId[i], 0, sizeof(m_mfxExtMVCSeq.ViewId[i]));
  }
  m_mfxExtMVCSeq.NumViewIdAlloc = m_mfxExtMVCSeq.NumViewId;

  m_mfxExtMVCSeq.OP = new mfxMVCOperationPoint[m_mfxExtMVCSeq.NumOP];
  CheckPointer(m_mfxExtMVCSeq.OP, E_OUTOFMEMORY);
  for (i = 0; i < m_mfxExtMVCSeq.NumOP; ++i)
  {
    memset(&m_mfxExtMVCSeq.OP[i], 0, sizeof(m_mfxExtMVCSeq.OP[i]));
  }
  m_mfxExtMVCSeq.NumOPAlloc = m_mfxExtMVCSeq.NumOP;

  return S_OK;
}

MVCBuffer * CMSDKDecoder::GetBuffer()
{
  CAutoLock lock(&m_BufferCritSec);
  MVCBuffer *pBuffer = nullptr;
  for (auto it = m_BufferQueue.begin(); it != m_BufferQueue.end(); it++) {
    if (!(*it)->surface.Data.Locked && !(*it)->queued) {
      pBuffer = *it;
      break;
    }
  }

  if (!pBuffer) {
    pBuffer = new MVCBuffer();

    pBuffer->surface.Info = m_mfxVideoParams.mfx.FrameInfo;
    pBuffer->surface.Info.FourCC = MFX_FOURCC_NV12;

    pBuffer->surface.Data.PitchLow = FFALIGN(m_mfxVideoParams.mfx.FrameInfo.Width, 64);
    pBuffer->surface.Data.Y  = (mfxU8 *)av_malloc(pBuffer->surface.Data.PitchLow * FFALIGN(m_mfxVideoParams.mfx.FrameInfo.Height, 64) * 3 / 2);
    pBuffer->surface.Data.UV = pBuffer->surface.Data.Y + (pBuffer->surface.Data.PitchLow * FFALIGN(m_mfxVideoParams.mfx.FrameInfo.Height, 64));

    m_BufferQueue.push_back(pBuffer);
    DLog(L"CMSDKDecoder::GetBuffer(): Allocated new MSDK MVC buffer (%d total)", m_BufferQueue.size());
  }

  return pBuffer;
}

MVCBuffer * CMSDKDecoder::FindBuffer(mfxFrameSurface1 * pSurface)
{
  CAutoLock lock(&m_BufferCritSec);
  bool bFound = false;
  for (auto it = m_BufferQueue.begin(); it != m_BufferQueue.end(); it++) {
    if (&(*it)->surface == pSurface) {
      return *it;
    }
  }

  return nullptr;
}

void CMSDKDecoder::ReleaseBuffer(mfxFrameSurface1 * pSurface)
{
  if (!pSurface)
    return;

  CAutoLock lock(&m_BufferCritSec);
  MVCBuffer * pBuffer = FindBuffer(pSurface);

  if (pBuffer) {
    pBuffer->queued = 0;
    pBuffer->sync = nullptr;
  }
}

void CMSDKDecoder::SetOutputMode(int mode, bool swaplr)
{
  m_iNewOutputMode = mode;
}
