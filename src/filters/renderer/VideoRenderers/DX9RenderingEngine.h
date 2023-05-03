/*
 * (C) 2006-2023 see Authors.txt
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

#define DXVA2VP 1
#define DXVAHDVP 1

#include "AllocatorCommon.h"
#include "PixelShaderCompiler.h"
#include <d3d9.h>
#include <dxva2api.h>
#if DXVAHDVP
#include <dxvahd.h>
#endif
#include "AllocatorPresenterImpl.h"

namespace DSObjects
{
	enum {
		shader_bspline_x,
		shader_bspline_y,
		shader_mitchell_x,
		shader_mitchell_y,
		shader_catmull_x,
		shader_catmull_y,
		shader_bicubic06_x,
		shader_bicubic06_y,
		shader_bicubic08_x,
		shader_bicubic08_y,
		shader_bicubic10_x,
		shader_bicubic10_y,
		shader_lanczos2_x,
		shader_lanczos2_y,
		shader_lanczos3_x,
		shader_lanczos3_y,
		shader_downscaler_simple_x,
		shader_downscaler_simple_y,
		shader_downscaler_box_x,
		shader_downscaler_box_y,
		shader_downscaler_bilinear_x,
		shader_downscaler_bilinear_y,
		shader_downscaler_hamming_x,
		shader_downscaler_hamming_y,
		shader_downscaler_bicubic_x,
		shader_downscaler_bicubic_y,
		shader_downscaler_lanczos_x,
		shader_downscaler_lanczos_y,
		shader_count
	};

	class CDX9RenderingEngine
		: public CAllocatorPresenterImpl
	{
	protected:
		static const int MAX_VIDEO_SURFACES = RS_EVRBUFFERS_MAX;

		// Variables initialized/managed by the allocator-presenter!
		CComPtr<IDirect3D9Ex>		m_pD3D9Ex;
		CComPtr<IDirect3DDevice9Ex>	m_pDevice9Ex;
		CComPtr<IDirect3DDevice9Ex>	m_pDevice9ExRefresh;

		ExtraRendererSettings m_ExtraSets;

		bool m_bDeviceResetRequested = false;
		bool m_bPendingResetDevice   = false;
		bool m_bNeedResetDevice      = false;

		D3DFORMAT					m_BackbufferFmt = D3DFMT_X8R8G8B8;
		D3DFORMAT					m_DisplayFmt    = D3DFMT_X8R8G8B8;
		CSize						m_ScreenSize;
		unsigned					m_nSurfaces    = 4; // Total number of DX Surfaces
		UINT32						m_iCurSurface  = 0; // Surface currently displayed
		DWORD						m_D3D9VendorId = 0;
		bool						m_bFP16Support = true; // don't disable hardware features before initializing a renderer

		// Variables initialized/managed by this class but can be accessed by the allocator-presenter
		D3DFORMAT					m_VideoBufferFmt = D3DFMT_X8R8G8B8;
		D3DFORMAT					m_SurfaceFmt     = D3DFMT_X8R8G8B8;

		CComPtr<IDirect3DTexture9>	m_pVideoTextures[MAX_VIDEO_SURFACES];
		CComPtr<IDirect3DSurface9>	m_pVideoSurfaces[MAX_VIDEO_SURFACES];

		bool						m_bColorManagement = false;
		bool						m_bDither          = false;
		DXVA2_ExtendedFormat		m_inputExtFormat = {};
		CString						m_strMixerOutputFmt;
		const wchar_t*				m_wsResizer    = nullptr;
		const wchar_t*				m_wsResizer2   = nullptr;
		const wchar_t*				m_wsCorrection = nullptr;
		CString						m_strFinalPass;

		HMODULE m_hDxva2Lib = nullptr;

		CDX9RenderingEngine(HWND hWnd, HRESULT& hr, CString *_pError);
		~CDX9RenderingEngine();

		void InitRenderingEngine();
		void CleanupRenderingEngine();

		HRESULT CreateVideoSurfaces();
		void FreeVideoSurfaces();
		void FreeScreenTextures();

		HRESULT RenderVideo(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);
		HRESULT Stereo3DTransform(IDirect3DSurface9* pRenderTarget, const CRect& destRect);

		HRESULT DrawRect(const D3DCOLOR _Color, const CRect &_Rect);
		HRESULT AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture);

		HRESULT ClearCustomPixelShaders(int target);
		HRESULT AddCustomPixelShader(int target, LPCSTR sourceCode, LPCSTR profile);

		HRESULT InitCorrectionPass(const AM_MEDIA_TYPE& mt);

		D3DCAPS9					m_Caps = {};
		LPCSTR						m_ShaderProfile = nullptr; // for shader compiler

#if DXVA2VP
		CComPtr<IDirectXVideoProcessorService> m_pDXVA2_VPService;
		CComPtr<IDirectXVideoProcessor> m_pDXVA2_VP;

		DXVA2_VideoDesc          m_VideoDesc = {};
		DXVA2_VideoProcessorCaps m_VPCaps    = {};

		DXVA2_Fixed32 m_ProcAmpValues[4] = {};
		DXVA2_Fixed32 m_NFilterValues[6] = {};
		DXVA2_Fixed32 m_DFilterValues[6] = {};
#endif
#if DXVAHDVP
		CComPtr<IDXVAHD_Device>         m_pDXVAHD_Device;
		CComPtr<IDXVAHD_VideoProcessor> m_pDXVAHD_VP;
#endif

		CComPtr<IDirect3DTexture9>	m_pFrameTextures[2];
		CComPtr<IDirect3DTexture9>	m_pRotateTexture;
		CComPtr<IDirect3DTexture9>	m_pResizeTexture;
		CComPtr<IDirect3DTexture9>	m_pScreenSpaceTextures[2];
		unsigned					m_iScreenTex = 0;

		int							m_ScreenSpaceTexWidth  = 0;
		int							m_ScreenSpaceTexHeight = 0;

		int							m_iRotation = 0; // total rotation angle clockwise of frame (0, 90, 180 or 270 deg.)
		bool						m_bFlip = false; // horizontal flip. for vertical flip use together with a rotation of 180 deg.

		std::unique_ptr<CPixelShaderCompiler> m_pPSC;

		// Settings
		VideoSystem						m_InputVideoSystem = VIDEO_SYSTEM_UNKNOWN;
		AmbientLight					m_AmbientLight     = AMBIENT_LIGHT_BRIGHT;
		ColorRenderingIntent			m_RenderingIntent  = COLOR_RENDERING_INTENT_PERCEPTUAL;

		// Custom pixel shaders
		CComPtr<IDirect3DPixelShader9>	m_pPSCorrection;
		std::list<CExternalPixelShader>	m_pCustomPixelShaders;

		// Resizers
		CComPtr<IDirect3DPixelShader9>	m_pResizerPixelShaders[shader_count];

		// Final pass
		bool							m_bFinalPass = false;
		const unsigned					m_Lut3DSize = 64; // 64x64x64 LUT is enough for high-quality color management
		const unsigned					m_Lut3DEntryCount = 64 * 64 * 64;
		CComPtr<IDirect3DVolumeTexture9> m_pLut3DTexture;
		CComPtr<IDirect3DTexture9>		m_pDitherTexture;
		CComPtr<IDirect3DPixelShader9>	m_pFinalPixelShader;

		// Custom screen space pixel shaders
		std::list<CExternalPixelShader>	m_pCustomScreenSpacePixelShaders;
		CComPtr<IDirect3DPixelShader9>	m_pConvertToInterlacePixelShader;

#if DXVA2VP
		BOOL InitializeDXVA2VP(int width, int height);
		BOOL CreateDXVA2VPDevice(REFGUID guid);
		HRESULT TextureResizeDXVA2(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect);
#endif
#if DXVAHDVP
		BOOL InitializeDXVAHDVP(int width, int height);
		HRESULT TextureResizeDXVAHD(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect);
#endif

		// init processing textures
		HRESULT InitVideoTextures();
		HRESULT InitScreenSpaceTextures(unsigned count);

		// Resizers
		HRESULT InitShaderResizer(int resizer);
		HRESULT TextureResize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, D3DTEXTUREFILTERTYPE filter);
		HRESULT TextureResizeShader(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, int iShader);
		HRESULT ApplyResize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, int resizer, int y);
	protected:
		HRESULT Resize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect);

		// Final pass
		HRESULT InitFinalPass();
		void UpdateFinalPassStr();
		void CleanupFinalPass();
		HRESULT CreateIccProfileLut(wchar_t* profilePath, float* lut3D);
		HRESULT FinalPass(IDirect3DTexture9* pTexture);

		HRESULT TextureCopy(IDirect3DTexture9* pTexture);
		bool ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d);

	public:
		// IAllocatorPresenter
		STDMETHODIMP_(SIZE) GetVideoSize() override;
		STDMETHODIMP_(SIZE) GetVideoSizeAR() override;
		STDMETHODIMP SetRotation(int rotation) override;
		STDMETHODIMP_(int) GetRotation() override;
		STDMETHODIMP SetFlip(bool flip) override;
		STDMETHODIMP_(bool) GetFlip() override;

		// ISubRenderOptions
		STDMETHODIMP GetInt(LPCSTR field, int* value) override;
		STDMETHODIMP GetString(LPCSTR field, LPWSTR* value, int* chars) override;
	};

}
