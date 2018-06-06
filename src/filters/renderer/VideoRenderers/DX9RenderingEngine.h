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

#pragma once

#include "AllocatorCommon.h"
#include "PixelShaderCompiler.h"
#include "RenderersSettings.h"
#include <d3d9.h>
#include <dxva2api.h>
#if DXVAHDVP
#include <dxvahd.h>
#endif
#include "../SubPic/SubPicAllocatorPresenterImpl.h"

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
		: public CSubPicAllocatorPresenterImpl
	{
	protected:
		static const int MAX_VIDEO_SURFACES = RS_EVRBUFFERS_MAX;

		// Variables initialized/managed by the allocator-presenter!
		CComPtr<IDirect3D9Ex>		m_pD3DEx;
		CComPtr<IDirect3DDevice9Ex>	m_pD3DDevEx;

		D3DFORMAT					m_BackbufferFmt;
		D3DFORMAT					m_DisplayFmt;
		CSize						m_ScreenSize;
		unsigned					m_nSurfaces;   // Total number of DX Surfaces
		UINT32						m_iCurSurface; // Surface currently displayed
		DWORD						m_D3D9VendorId;
		bool						m_bFP16Support;

		// Variables initialized/managed by this class but can be accessed by the allocator-presenter
		D3DFORMAT					m_VideoBufferFmt;
		D3DFORMAT					m_SurfaceFmt;

		CComPtr<IDirect3DTexture9>	m_pVideoTextures[MAX_VIDEO_SURFACES];
		CComPtr<IDirect3DSurface9>	m_pVideoSurfaces[MAX_VIDEO_SURFACES];

		bool						m_bColorManagement;
		bool						m_bDither;
		DXVA2_ExtendedFormat		m_inputExtFormat;
		const wchar_t*				m_wsResizer;
		const wchar_t*				m_wsResizer2;
		CString						m_strMixerOutputFmt;


		CDX9RenderingEngine(HWND hWnd, HRESULT& hr, CString *_pError);
		~CDX9RenderingEngine();

		void InitRenderingEngine();
		void CleanupRenderingEngine();

		HRESULT CreateVideoSurfaces();
		void FreeVideoSurfaces();
		void FreeScreenTextures();

		HRESULT RenderVideo(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);
		HRESULT Stereo3DTransform(IDirect3DSurface9* pRenderTarget, const CRect& destRect);

		HRESULT DrawRect(DWORD _Color, DWORD _Alpha, const CRect &_Rect);
		HRESULT AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture);

		HRESULT ClearCustomPixelShaders(int target);
		HRESULT AddCustomPixelShader(int target, LPCSTR sourceCode, LPCSTR profile);

		HRESULT InitCorrectionPass(const AM_MEDIA_TYPE& mt);

	private:
		D3DCAPS9					m_Caps;
		LPCSTR						m_ShaderProfile; // for shader compiler

#if DXVA2VP || DXVAHDVP
		HMODULE m_hDxva2Lib = nullptr;
#endif
#if DXVA2VP
		CComPtr<IDirectXVideoProcessorService> m_pDXVA2_VPService;
		CComPtr<IDirectXVideoProcessor> m_pDXVA2_VP;

		DXVA2_VideoDesc          m_VideoDesc;
		DXVA2_VideoProcessorCaps m_VPCaps;

		DXVA2_Fixed32 m_ProcAmpValues[4];
		DXVA2_Fixed32 m_NFilterValues[6];
		DXVA2_Fixed32 m_DFilterValues[6];
#endif
#if DXVAHDVP
		CComPtr<IDXVAHD_Device>         m_pDXVAHD_Device;
		CComPtr<IDXVAHD_VideoProcessor> m_pDXVAHD_VP;
#endif

		CComPtr<IDirect3DTexture9>	m_pFrameTextures[2];
		CComPtr<IDirect3DTexture9>	m_pRotateTexture;
		CComPtr<IDirect3DTexture9>	m_pResizeTexture;
		CComPtr<IDirect3DTexture9>	m_pScreenSpaceTextures[2];
		unsigned					m_iScreenTex;

		int							m_ScreenSpaceTexWidth;
		int							m_ScreenSpaceTexHeight;

		int							m_iRotation; // total rotation angle clockwise of frame (0, 90, 180 or 270 deg.)
		bool						m_bFlip; // horizontal flip. for vertical flip use together with a rotation of 180 deg.

		CAutoPtr<CPixelShaderCompiler>	m_pPSC;

		// Settings
		VideoSystem						m_InputVideoSystem;
		AmbientLight					m_AmbientLight;
		ColorRenderingIntent			m_RenderingIntent;

		// Custom pixel shaders
		CComPtr<IDirect3DPixelShader9>	m_pPSCorrection;
		std::list<CExternalPixelShader>	m_pCustomPixelShaders;

		// Resizers
		CComPtr<IDirect3DPixelShader9>	m_pResizerPixelShaders[shader_count];

		// Final pass
		bool							m_bFinalPass;
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
		HRESULT Resize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect);

		// Final pass
		HRESULT InitFinalPass();
		void CleanupFinalPass();
		HRESULT CreateIccProfileLut(wchar_t* profilePath, float* lut3D);
		HRESULT FinalPass(IDirect3DTexture9* pTexture);

		HRESULT TextureCopy(IDirect3DTexture9* pTexture);
		bool ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d);

	public:
		// ISubPicAllocatorPresenter3
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
