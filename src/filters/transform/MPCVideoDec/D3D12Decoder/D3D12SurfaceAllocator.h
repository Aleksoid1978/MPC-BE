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
#if USE_D3D12
#pragma once

#include <d3d12.h>

#include "../DXVADecoder/MediaSampleSideData.h"
#include "ID3D12VideoMemoryConfiguration.h"
#include <queue>

class CD3D12Decoder;
struct AVFrame;
struct AVBufferRef;

class CD3D12MediaSample
  : public CMediaSampleSideData
  , public IMediaSampleD3D12
{
  friend class CD3D12SurfaceAllocator;

public:
  CD3D12MediaSample(CD3D12SurfaceAllocator* pAllocator, AVFrame* pFrame, HRESULT* phr);
  virtual ~CD3D12MediaSample();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, __deref_out void** ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IMediaSample
  STDMETHODIMP GetPointer(BYTE** ppBuffer) { return E_NOTIMPL; }

  // IMediaSampleD3D12
  HRESULT STDMETHODCALLTYPE GetD3D12Texture(ID3D12Resource** ppTexture, int* iTextureIndex);

  // LAV Interface
  STDMETHODIMP GetAVFrameBuffer(AVFrame* pFrame);

  void SetD3D12Texture(ID3D12Resource* tex);


  int GetTextureIndex();
private:
  AVFrame* m_pFrame = nullptr;
};

class CD3D12SurfaceAllocator : public CBaseAllocator
{
  friend class CD3D12SampleList;
  //its a modified version of baseclasse sample list
  //We need one that use send back sample in back of the queue and not the one that just got freed
  class CD3D12SampleList
  {
  public:
    CD3D12SampleList()
    {
    };
    ~CD3D12SampleList() {};

    void Add(CD3D12MediaSample* pSample)
    {
      ASSERT(pSample != NULL);
      m_pFreeList.push(pSample);
    }
    int GetCount()
    {
      return m_pFreeList.size();
    }

    CD3D12MediaSample* RemoveHead()
    {
      if (m_pFreeList.size() == 0)
        return nullptr;
      CD3D12MediaSample* sample = m_pFreeList.front();
      m_pFreeList.pop();
      return sample;
    }

    CD3D12MediaSample* GetFreeSample()
    {
      if (m_pFreeList.size() == 0)
        ASSERT(0);
      CD3D12MediaSample* freesample = m_pFreeList.front();
      m_pFreeList.pop();
      return freesample;
    }
  private:
    std::queue<CD3D12MediaSample*> m_pFreeList;
    std::string m_presentedindex;
  };

public:
  CD3D12SurfaceAllocator(CD3D12Decoder* pDec, HRESULT* phr);
  virtual ~CD3D12SurfaceAllocator();

  STDMETHODIMP SetProperties(__in ALLOCATOR_PROPERTIES* pRequest, __out ALLOCATOR_PROPERTIES* pActual);

  STDMETHODIMP GetProperties(__out ALLOCATOR_PROPERTIES* pProps);

  STDMETHODIMP Commit();

  STDMETHODIMP Decommit();

  STDMETHODIMP GetBuffer(__deref_out IMediaSample** ppBuffer, __in_opt REFERENCE_TIME* pStartTime,
    __in_opt REFERENCE_TIME* pEndTime, DWORD dwFlags);


  STDMETHODIMP ReleaseBuffer(IMediaSample* pSample);

  STDMETHODIMP_(BOOL) DecommitInProgress()
  {
    CAutoLock cal(this);
    return m_bDecommitInProgress;
  }
  STDMETHODIMP_(BOOL) IsCommited()
  {
    CAutoLock cal(this);
    return m_bCommitted;
  }

  STDMETHODIMP_(void) ForceDecommit();

  STDMETHODIMP_(void) DecoderDestruct()
  {
    CAutoLock cal(this);
    m_pDec = nullptr;
  }

protected:
  CD3D12SampleList m_pSampleList;
  virtual void Free(void);
  virtual HRESULT Alloc(void);

private:
  CD3D12Decoder* m_pDec = nullptr;

  friend class CD3D12MediaSample;
};
#endif