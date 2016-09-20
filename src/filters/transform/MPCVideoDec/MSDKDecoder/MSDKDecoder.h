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

#pragma once

#include <IMediaSideData.h>

#include "./include/mfxvideo.h"
#include "./include/mfxmvc.h"
#include "growarray.h"

#include "AnnexBConverter.h"

#include <atlcoll.h>
#include <deque>
#include <vector>

#define ASYNC_DEPTH 8
#define ASYNC_QUEUE_SIZE (ASYNC_DEPTH + 2)

// 1s timestamp offset to avoid negative timestamps
#define TIMESTAMP_OFFSET 10000000i64

typedef struct _MVCBuffer {
  mfxFrameSurface1 surface = { 0 };
  bool queued = false;
  mfxSyncPoint sync = nullptr;
} MVCBuffer;

typedef struct _MVCGOP {
  std::deque<mfxU64> timestamps;
  std::deque<MediaSideData3DOffset> offsets;
} MVCGOP;

class CMPCVideoDecFilter;
struct AVFrame;

class CMSDKDecoder
{
public:
  CMSDKDecoder(CMPCVideoDecFilter* pFilter);
  virtual ~CMSDKDecoder();

  HRESULT Init();

  HRESULT InitDecoder(const CMediaType *pmt);
  HRESULT Decode(const BYTE *buffer, int buflen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);

  void Flush();
  HRESULT EndOfStream();

private:
  void DestroyDecoder(bool bFull);
  HRESULT AllocateMVCExtBuffers();

  MVCBuffer * GetBuffer();
  MVCBuffer * FindBuffer(mfxFrameSurface1 * pSurface);
  void ReleaseBuffer(mfxFrameSurface1 * pSurface);

  HRESULT HandleOutput(MVCBuffer * pOutputBuffer);
  HRESULT DeliverOutput(MVCBuffer * pBaseView, MVCBuffer * pExtraView);

  HRESULT ParseSEI(const BYTE *buffer, size_t size, mfxU64 timestamp);
  HRESULT ParseMVCNestedSEI(const BYTE *buffer, size_t size, mfxU64 timestamp);
  HRESULT ParseUnregUserDataSEI(const BYTE *buffer, size_t size, mfxU64 timestamp);
  HRESULT ParseOffsetMetadata(const BYTE *buffer, size_t size, mfxU64 timestamp);

  void AddFrameToGOP(mfxU64 timestamp);
  BOOL RemoveFrameFromGOP(MVCGOP * pGOP, mfxU64 timestamp);
  void GetOffsetSideData(IMediaSample* pSample, mfxU64 timestamp);

  void SetTypeSpecificFlags(IMediaSample* pSample);

private:
  mfxSession m_mfxSession = nullptr;

  BOOL                 m_bDecodeReady   = FALSE;
  mfxVideoParam        m_mfxVideoParams = { 0 };

  mfxExtBuffer        *m_mfxExtParam[1] = { 0 };
  mfxExtMVCSeqDesc     m_mfxExtMVCSeq   = { 0 };

  CCritSec m_BufferCritSec;
  std::vector<MVCBuffer *> m_BufferQueue;

  GrowableArray<BYTE>  m_buff;
  CAnnexBConverter    *m_pAnnexBConverter = nullptr;

  MVCBuffer           *m_pOutputQueue[ASYNC_QUEUE_SIZE] = { 0 };
  int                  m_nOutputQueuePosition = 0;

  std::deque<MVCGOP>    m_GOPs;
  MediaSideData3DOffset m_PrevOffset;

  CMPCVideoDecFilter   *m_pFilter;
  AVFrame              *m_pFrame = nullptr;
};
