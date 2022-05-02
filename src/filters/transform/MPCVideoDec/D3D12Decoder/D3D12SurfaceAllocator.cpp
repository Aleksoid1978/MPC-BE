/*
 *      Copyright (C) 2021      Benoit Plourde
 *
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
 */
#include "stdafx.h"
#if USE_D3D12
#include "D3D12SurfaceAllocator.h"
#include "D3D12Decoder.h"
#include "DSUtil/DSUtil.h"

extern "C"
{
#include "libavutil/hwcontext.h"
}

CD3D12MediaSample::CD3D12MediaSample(CD3D12SurfaceAllocator* pAllocator, AVFrame* pFrame, HRESULT* phr)
  : CMediaSampleSideData(NAME("CD3D12MediaSample"), (CD3D12SurfaceAllocator*)pAllocator, phr, nullptr, 0)
  , m_pFrame(pFrame)
{
  ASSERT(m_pFrame && m_pFrame->format == AV_PIX_FMT_D3D12_VLD);
  pAllocator->AddRef();
}

CD3D12MediaSample::~CD3D12MediaSample()
{
  av_frame_free(&m_pFrame);
  SAFE_RELEASE(m_pAllocator);
}

// Note: CMediaSample does not derive from CUnknown, so we cannot use the
//		DECLARE_IUNKNOWN macro that is used by most of the filter classes.

STDMETHODIMP CD3D12MediaSample::QueryInterface(REFIID riid, __deref_out void** ppv)
{
  CheckPointer(ppv, E_POINTER);
  ValidateReadWritePtr(ppv, sizeof(PVOID));

  if (riid == __uuidof(IMediaSampleD3D12)) {
    return GetInterface((IMediaSampleD3D12*)this, ppv);
  }
  else {
    return __super::QueryInterface(riid, ppv);
  }
}

STDMETHODIMP_(ULONG) CD3D12MediaSample::AddRef()
{
  return __super::AddRef();
}

STDMETHODIMP_(ULONG) CD3D12MediaSample::Release()
{
  // Return a temporary variable for thread safety.
  ULONG cRef = __super::Release();
  return cRef;
}

STDMETHODIMP CD3D12MediaSample::GetD3D12Texture(ID3D12Resource** ppResource, int* iTextureIndex)
{
  CheckPointer(ppResource, E_POINTER);
  if (m_pFrame) {
    *ppResource = (ID3D12Resource*)m_pFrame->data[2];
    *iTextureIndex = (int)m_pFrame->data[1];
    (*ppResource)->AddRef();
    return S_OK;
  }
  return E_FAIL;
}

static void bufref_release_sample(void* opaque, uint8_t* data)
{
  CD3D12MediaSample* pSample = (CD3D12MediaSample*)opaque;
  pSample->Release();
}



STDMETHODIMP CD3D12MediaSample::GetAVFrameBuffer(AVFrame* pFrame)
{
  CheckPointer(pFrame, E_POINTER);

  // reference bufs
  for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
    // copy existing refs
    if (m_pFrame->buf[i]) {
      pFrame->buf[i] = av_buffer_ref(m_pFrame->buf[i]);
      if (pFrame->buf[i] == 0)
        return E_OUTOFMEMORY;
    }
    else {
      // and add a ref to this sample
      pFrame->buf[i] = av_buffer_create((uint8_t*)this, 1, bufref_release_sample, this, 0);

      if (pFrame->buf[i] == 0)
        return E_OUTOFMEMORY;

      AddRef();
      break;
    }
  }
  // ref the hwframes ctx
  //maybe we will need to remove this
  //pFrame->hw_frames_ctx = av_buffer_ref(m_pFrame->hw_frames_ctx);

  // copy data into the new frame
  /*in lavfilters*/
  /*pFrame->data[0] = m_pFrame->data[0];
  pFrame->data[1] = (uint8_t*)this;
  pFrame->data[2] = m_pFrame->data[2];
  pFrame->data[3] = m_pFrame->data[3];*/
  pFrame->data[0] = m_pFrame->data[0];
  pFrame->data[1] = m_pFrame->data[1];
  pFrame->data[2] = m_pFrame->data[2];
  pFrame->data[3] = (uint8_t*)this;

  pFrame->format = AV_PIX_FMT_D3D12_VLD;

  return S_OK;
}

void CD3D12MediaSample::SetD3D12Texture(ID3D12Resource* tex)
{
  m_pFrame->data[2] = (uint8_t*)tex;
}

int CD3D12MediaSample::GetTextureIndex()
{
  return (int)m_pFrame->data[3];
}

CD3D12SurfaceAllocator::CD3D12SurfaceAllocator(CD3D12Decoder* pDec, HRESULT* phr)
  : CBaseAllocator(NAME("CD3D12SurfaceAllocator"), nullptr, phr)
  , m_pDec(pDec)
{
}

CD3D12SurfaceAllocator::~CD3D12SurfaceAllocator()
{
}

HRESULT CD3D12SurfaceAllocator::Alloc(void)
{

  DbgLog((LOG_TRACE, 10, L"CD3D12SurfaceAllocator::Alloc()"));
  HRESULT hr = S_OK;

  CAutoLock cObjectLock(this);

  if (m_pDec == nullptr)
    return E_FAIL;

  hr = __super::Alloc();
  if (FAILED(hr))
    return hr;

  Free();

  long maxindex = m_pDec->GetBufferCount();

  // create samples
  //this might ref some frame twice
  for (int i = 0; i < m_lCount; i++) {
    AVFrame* pFrame = av_frame_alloc();

    pFrame->format = AV_PIX_FMT_D3D12_VLD;

    int ret = 0;// av_hwframe_get_buffer(m_pFramesCtx, pFrame, 0);
    if (ret < 0) {
      av_frame_free(&pFrame);
      Free();
      return E_FAIL;
    }

    pFrame->data[1] = (uint8_t*)i;// index in the array
    CD3D12MediaSample* pSample = new CD3D12MediaSample(this, pFrame, &hr);
    if (pSample == nullptr || FAILED(hr)) {
      delete pSample;
      Free();
      return E_FAIL;
    }
    m_pSampleList.Add(pSample);
    //m_lFree.Add(pSample);
  }

  m_lAllocated = m_lCount;
  m_bChanged = FALSE;

  return S_OK;
}

void CD3D12SurfaceAllocator::Free(void)
{
  CAutoLock lock(this);
  CMediaSample* pSample = nullptr;

  do {
    pSample = m_pSampleList.RemoveHead();
    //pSample = m_lFree.RemoveHead();
    if (pSample) {
      delete pSample;
    }
  } while (pSample);

  m_lAllocated = 0;
  //av_buffer_unref(&m_pFramesCtx);
}

STDMETHODIMP CD3D12SurfaceAllocator::ReleaseBuffer(IMediaSample* pSample)
{
  //CD3D12MediaSample *pD3D11Sample = dynamic_cast<CD3D12MediaSample *>(pSample);
  CheckPointer(pSample, E_POINTER);
  ValidateReadPtr(pSample, sizeof(IMediaSample));

  BOOL bRelease = FALSE;
  {
    CAutoLock cal(this);

    /* Put back on the free list */
    m_pSampleList.Add((CD3D12MediaSample*)pSample);
    //m_lFree.Add((CMediaSample*)pSample);
    if (m_lWaiting != 0)
    {
      NotifySample();
    }

    // if there is a pending Decommit, then we need to complete it by
    // calling Free() when the last buffer is placed on the free list

    //LONG l1 = m_lFree.GetCount();
    LONG l1 = m_pSampleList.GetCount();
    if (m_bDecommitInProgress && (l1 == m_lAllocated))
    {
      Free();
      m_bDecommitInProgress = FALSE;
      bRelease = TRUE;
    }
  }

  if (m_pNotify)
  {

    ASSERT(m_fEnableReleaseCallback);

    //
    // Note that this is not synchronized with setting up a notification
    // method.
    //
    m_pNotify->NotifyRelease();
  }

  /* For each buffer there is one AddRef, made in GetBuffer and released
     here. This may cause the allocator and all samples to be deleted */

  if (bRelease)
  {
    Release();
  }
  return NOERROR;
}

STDMETHODIMP_(void) CD3D12SurfaceAllocator::ForceDecommit()
{
  {
    CAutoLock lock(this);

    if (m_bCommitted || !m_bDecommitInProgress)
      return;

    // actually free all the samples that are already back
    Free();

    // finish decommit, so that Alloc works again
    m_bDecommitInProgress = FALSE;
  }

  if (m_pNotify)
  {
    m_pNotify->NotifyRelease();
  }

  // alloc holds one reference, we need free that here
  Release();
}


STDMETHODIMP
CD3D12SurfaceAllocator::SetProperties(__in ALLOCATOR_PROPERTIES* pRequest, __out ALLOCATOR_PROPERTIES* pActual)
{
  CheckPointer(pRequest, E_POINTER);
  CheckPointer(pActual, E_POINTER);
  ValidateReadWritePtr(pActual, sizeof(ALLOCATOR_PROPERTIES));
  CAutoLock cObjectLock(this);

  ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

  ASSERT(pRequest->cbBuffer > 0);

  /*  Check the alignment requested */
  if (pRequest->cbAlign != 1)
  {
    DbgLog((LOG_ERROR, 2, TEXT("Alignment requested was 0x%x, not 1"), pRequest->cbAlign));
    return VFW_E_BADALIGN;
  }

  /* Can't do this if already committed, there is an argument that says we
     should not reject the SetProperties call if there are buffers still
     active. However this is called by the source filter, which is the same
     person who is holding the samples. Therefore it is not unreasonable
     for them to free all their samples before changing the requirements */

  if (m_bCommitted)
  {
    return VFW_E_ALREADY_COMMITTED;
  }

  /* Must be no outstanding buffers */

  //if (m_lAllocated != m_lFree.GetCount())
  if (m_lAllocated != m_pSampleList.GetCount())
  {
    return VFW_E_BUFFERS_OUTSTANDING;
  }

  /* There isn't any real need to check the parameters as they
     will just be rejected when the user finally calls Commit */

  pActual->cbBuffer = m_lSize = pRequest->cbBuffer;
  pActual->cBuffers = m_lCount = pRequest->cBuffers;
  pActual->cbAlign = m_lAlignment = pRequest->cbAlign;
  pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

  m_bChanged = TRUE;
  return NOERROR;
}

STDMETHODIMP
CD3D12SurfaceAllocator::GetProperties(__out ALLOCATOR_PROPERTIES* pActual)
{
  CheckPointer(pActual, E_POINTER);
  ValidateReadWritePtr(pActual, sizeof(ALLOCATOR_PROPERTIES));

  CAutoLock cObjectLock(this);
  pActual->cbBuffer = m_lSize;
  pActual->cBuffers = m_lCount;
  pActual->cbAlign = m_lAlignment;
  pActual->cbPrefix = m_lPrefix;
  return NOERROR;
}

// get container for a sample. Blocking, synchronous call to get the
// next free buffer (as represented by an IMediaSample interface).
// on return, the time etc properties will be invalid, but the buffer
// pointer and size will be correct.

HRESULT CD3D12SurfaceAllocator::GetBuffer(__deref_out IMediaSample** ppBuffer, __in_opt REFERENCE_TIME* pStartTime,
  __in_opt REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
  UNREFERENCED_PARAMETER(pStartTime);
  UNREFERENCED_PARAMETER(pEndTime);
  UNREFERENCED_PARAMETER(dwFlags);
  CMediaSample* pSample;

  *ppBuffer = NULL;
  for (;;)
  {
    { // scope for lock
      CAutoLock cObjectLock(this);

      /* Check we are committed */
      if (!m_bCommitted)
      {
        return VFW_E_NOT_COMMITTED;
      }
      //pSample = (CMediaSample*)m_lFree.RemoveHead();
      pSample = m_pSampleList.GetFreeSample();
      if (pSample == NULL)
      {
        SetWaiting();
      }
    }

    /* If we didn't get a sample then wait for the list to signal */

    if (pSample)
    {
      break;
    }
    if (dwFlags & AM_GBF_NOWAIT)
    {
      return VFW_E_TIMEOUT;
    }
    ASSERT(m_hSem != NULL);
    WaitForSingleObject(m_hSem, INFINITE);
  }

  /* Addref the buffer up to one. On release
     back to zero instead of being deleted, it will requeue itself by
     calling the ReleaseBuffer member function. NOTE the owner of a
     media sample must always be derived from CD3D12SurfaceAllocator */

  ASSERT(pSample->m_cRef == 0);
  pSample->m_cRef = 1;
  *ppBuffer = pSample;
  return NOERROR;
}

STDMETHODIMP
CD3D12SurfaceAllocator::Commit()
{
  /* Check we are not decommitted */
  CAutoLock cObjectLock(this);

  // cannot need to alloc or re-alloc if we are committed
  if (m_bCommitted)
  {
    return NOERROR;
  }

  // is there a pending decommit ? if so, just cancel it
  if (m_bDecommitInProgress)
  {
    m_bDecommitInProgress = FALSE;
    m_bCommitted = TRUE;

    // don't call Alloc at this point. He cannot allow SetProperties
    // between Decommit and the last free, so the buffer size cannot have
    // changed. And because some of the buffers are not free yet, he
    // cannot re-alloc anyway.
    return NOERROR;
  }

  DbgLog((LOG_MEMORY, 1, TEXT("Allocating: %ldx%ld"), m_lCount, m_lSize));

  // actually need to allocate the samples
  HRESULT hr = Alloc();
  if (FAILED(hr))
  {
    m_bCommitted = FALSE;
    return hr;
  }

  /* Allow GetBuffer calls */
  m_bCommitted = TRUE;

  AddRef();
  return NOERROR;
}

STDMETHODIMP
CD3D12SurfaceAllocator::Decommit()
{
  BOOL bRelease = FALSE;
  {
    /* Check we are not already decommitted */
    CAutoLock cObjectLock(this);
    if (m_bCommitted == FALSE)
    {
      if (m_bDecommitInProgress == FALSE)
      {
        return NOERROR;
      }
    }

    /* No more GetBuffer calls will succeed */
    m_bCommitted = FALSE;

    // are any buffers outstanding?
    if (m_lFree.GetCount() < m_lAllocated)
    {
      // please complete the decommit when last buffer is freed
      m_bDecommitInProgress = TRUE;
    }
    else
    {
      m_bDecommitInProgress = FALSE;

      // need to complete the decommit here as there are no
      // outstanding buffers

      Free();
      bRelease = TRUE;
    }

    // Tell anyone waiting that they can go now so we can
    // reject their call
#pragma warning(push)
#ifndef _PREFAST_
#pragma warning(disable : 4068)
#endif
#pragma prefast(                \
    suppress                    \
    : __WARNING_DEREF_NULL_PTR, \
      "Suppress warning related to Free() invalidating 'this' which is no applicable to CBaseAllocator::Free()")
    NotifySample();

#pragma warning(pop)
  }

  if (bRelease)
  {
    Release();
  }
  return NOERROR;
}
#endif