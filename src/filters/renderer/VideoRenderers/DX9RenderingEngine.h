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

#pragma once

#include "AllocatorCommon.h"
#include "RenderersSettings.h"
#include <d3d9.h>
#include <dxva2api.h>
#include "../SubPic/SubPicAllocatorPresenterImpl.h"

namespace DSObjects
{
	enum {
		shader_bspline4_x,
		shader_bspline4_y,
		shader_mitchell4_x,
		shader_mitchell4_y,
		shader_catmull4_x,
		shader_catmull4_y,
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
		shader_downscaling_x,
		shader_downscaling_y,
		shader_count
	};

	class CDX9RenderingEngine
		: public CSubPicAllocatorPresenterImpl
	{
	protected:
		enum RenderingPath {
			RENDERING_PATH_STRETCHRECT,
			RENDERING_PATH_DRAW,
		};

		static const int MAX_VIDEO_SURFACES = 30;

		// Variables initialized/managed by the allocator-presenter!
		CComPtr<IDirect3D9Ex>		m_pD3DEx;
		CComPtr<IDirect3DDevice9Ex>	m_pD3DDevEx;

		D3DFORMAT					m_BackbufferFmt;
		D3DFORMAT					m_DisplayFmt;
		CSize						m_ScreenSize;
		unsigned					m_nNbDXSurface; // Total number of DX Surfaces
		unsigned					m_nCurSurface;  // Surface currently displayed
		DWORD						m_D3D9VendorId;
		bool						m_bFP16Support;

		// Variables initialized/managed by this class but can be accessed by the allocator-presenter
		D3DFORMAT					m_VideoBufferFmt;
		D3DFORMAT					m_SurfaceFmt;

		CComPtr<IDirect3DTexture9>	m_pVideoTextures[MAX_VIDEO_SURFACES];
		CComPtr<IDirect3DSurface9>	m_pVideoSurfaces[MAX_VIDEO_SURFACES];

		bool	m_bColorManagement;
		DXVA2_ExtendedFormat m_inputExtFormat;
		const wchar_t* m_wsResizer;
		const wchar_t* m_wsResizer2;


		CDX9RenderingEngine(HWND hWnd, HRESULT& hr, CString *_pError);
		~CDX9RenderingEngine();

		void InitRenderingEngine();
		void CleanupRenderingEngine();

		HRESULT CreateVideoSurfaces();
		void FreeVideoSurfaces();

		HRESULT RenderVideo(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);
		HRESULT Stereo3DTransform(IDirect3DSurface9* pRenderTarget, const CRect& destRect);

		HRESULT DrawRect(DWORD _Color, DWORD _Alpha, const CRect &_Rect);
		HRESULT AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture);

		HRESULT ClearCustomPixelShaders(int target);
		HRESULT AddCustomPixelShader(int target, LPCSTR sourceCode, LPCSTR profile);

	private:
		D3DCAPS9					m_Caps;
		LPCSTR						m_ShaderProfile; // for shader compiler

#if DXVAVP
		CComPtr<IDirectXVideoProcessorService> m_pDXVAVPS;
		CComPtr<IDirectXVideoProcessor> m_pDXVAVPD;

		DXVA2_VideoDesc          m_VideoDesc;
		DXVA2_VideoProcessorCaps m_VPCaps;

		DXVA2_Fixed32 m_ProcAmpValues[4];
		DXVA2_Fixed32 m_NFilterValues[6];
		DXVA2_Fixed32 m_DFilterValues[6];
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
		CComPtr<IDirect3DPixelShader9>	m_pYCgCoCorrectionPixelShader;
		CAtlList<CExternalPixelShader>	m_pCustomPixelShaders;

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
		CAtlList<CExternalPixelShader>	m_pCustomScreenSpacePixelShaders;
		CComPtr<IDirect3DPixelShader9>	m_pConvertToInterlacePixelShader;

		// Video rendering paths
		HRESULT RenderVideoDrawPath(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);

#if DXVAVP
		HMODULE m_hDxva2Lib = NULL;
		BOOL InitializeDXVA2VP(int width, int height);
		BOOL CreateDXVA2VPDevice(REFGUID guid);
		HRESULT TextureResizeDXVA(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect);
#endif

		// init processing textures
		HRESULT InitVideoTextures();
		HRESULT InitScreenSpaceTextures(unsigned count);

		// Resizers
		HRESULT InitShaderResizer(int resizer);
		HRESULT TextureResize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, D3DTEXTUREFILTERTYPE filter);
		HRESULT TextureResizeShader(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, int iShader);
		HRESULT TextureResizeShader2pass(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, int iShader1);
		HRESULT ApplyResize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, int resizer, int y);
		HRESULT Resize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect);

		// Final pass
		HRESULT InitFinalPass();
		void CleanupFinalPass();
		HRESULT CreateIccProfileLut(TCHAR* profilePath, float* lut3D);
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
