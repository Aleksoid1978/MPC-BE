/*
*      Copyright (C) 2017-2019 Hendrik Leppkes
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
*  Adaptation for MPC-BE (C) 2021 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
*/

#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include "./D3D11Decoder/D3D11SurfaceAllocator.h"

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID riid, void** ppFactory);

class CMPCVideoDecFilter;
struct AVCodecContext;
struct AVD3D11VAContext;
struct AVBufferRef;

class CD3D11Decoder
{
public:
	CD3D11Decoder(CMPCVideoDecFilter* pFilter);
	virtual ~CD3D11Decoder();

	HRESULT Init();

	void AdditionaDecoderInit(AVCodecContext* c);
	void PostInitDecoder(AVCodecContext* c);

	HRESULT PostConnect(AVCodecContext* c, IPin* pPin);
	HRESULT DeliverFrame();

	HRESULT InitAllocator(IMemAllocator** ppAlloc);

	GUID* GetDecoderGuid() { return &m_DecoderGUID; }
	DXGI_ADAPTER_DESC* GetAdapterDesc() { return &m_AdapterDesc; }

private:
	friend class CD3D11SurfaceAllocator;
	CMPCVideoDecFilter *m_pFilter = nullptr;

	struct {
		HMODULE d3d11lib;
		PFN_D3D11_CREATE_DEVICE mD3D11CreateDevice;

		HMODULE dxgilib;
		PFN_CREATE_DXGI_FACTORY1 mCreateDXGIFactory1;
	} m_dxLib = {};

	DXGI_ADAPTER_DESC m_AdapterDesc = {};

	CD3D11SurfaceAllocator* m_pAllocator = nullptr;

	AVBufferRef* m_pDevCtx = nullptr;
	AVBufferRef* m_pFramesCtx = nullptr;

	D3D11_VIDEO_DECODER_CONFIG m_DecoderConfig = {};
	ID3D11VideoDecoder* m_pDecoder = nullptr;

	int m_nOutputViews = 0;
	ID3D11VideoDecoderOutputView** m_pOutputViews = nullptr;

	DWORD m_dwSurfaceWidth = 0;
	DWORD m_dwSurfaceHeight = 0;
	DWORD m_dwSurfaceCount = 0;
	DXGI_FORMAT m_SurfaceFormat = DXGI_FORMAT_UNKNOWN;
	GUID m_DecoderGUID = GUID_NULL;

	HRESULT CreateD3D11Device(UINT nDeviceIndex, ID3D11Device** ppDevice, DXGI_ADAPTER_DESC* pDesc);
	HRESULT CreateD3D11Decoder(AVCodecContext* c);
	HRESULT AllocateFramesContext(AVCodecContext* c, int width, int height, enum AVPixelFormat format, int nSurfaces, AVBufferRef** pFramesCtx);

	HRESULT FindVideoServiceConversion(AVCodecContext* c, enum AVCodecID codec, int profile, DXGI_FORMAT surface_format, GUID* input);
	HRESULT FindDecoderConfiguration(AVCodecContext* c, const D3D11_VIDEO_DECODER_DESC* desc, D3D11_VIDEO_DECODER_CONFIG* pConfig);

	HRESULT ReInitD3D11Decoder(AVCodecContext* c);

	void DestroyDecoder(const bool bFull);

	void FillHWContext(AVD3D11VAContext* ctx);

	static enum AVPixelFormat get_d3d11_format(struct AVCodecContext* s, const enum AVPixelFormat* pix_fmts);
	static int get_d3d11_buffer(struct AVCodecContext* c, AVFrame* pic, int flags);
};
