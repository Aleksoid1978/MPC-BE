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

#include "AllocatorCommon.h"
#include "RenderersSettings.h"
#include <d3d9.h>
#include <dx/d3dx9.h>
#include <dxva2api.h>
#include "../SubPic/SubPicAllocatorPresenterImpl.h"

enum {
	shader_smootherstep,
#if ENABLE_2PASS_RESIZE
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
#else
	shader_bspline4,
	shader_mitchell4,
	shader_catmull4,
	shader_bicubic06,
	shader_bicubic08,
	shader_bicubic10,
#endif
	shader_downscaling,
	shader_count
};

namespace DSObjects
{

	class CDX9RenderingEngine
		: public CSubPicAllocatorPresenterImpl
	{
	protected:
		enum RenderingPath {
			RENDERING_PATH_STRETCHRECT,
			RENDERING_PATH_DRAW,
		};

		static const int MAX_VIDEO_SURFACES = 60;

		// Variables initialized/managed by the allocator-presenter!
		CComPtr<IDirect3D9>			m_pD3D;
		CComPtr<IDirect3D9Ex>		m_pD3DEx;
		CComPtr<IDirect3DDevice9>	m_pD3DDev;
		CComPtr<IDirect3DDevice9Ex>	m_pD3DDevEx;
		UINT						m_CurrentAdapter;
		D3DCAPS9					m_Caps;
		LPCSTR						m_ShaderProfile;
		D3DFORMAT					m_BackbufferFmt;
		D3DFORMAT					m_DisplayFmt;
		CSize						m_ScreenSize;
		int							m_nNbDXSurface;					// Total number of DX Surfaces
		int							m_nCurSurface;					// Surface currently displayed
		bool						m_bIsEVR;
		DWORD						m_D3D9VendorId;

#if DXVAVP
		CComPtr<IDirectXVideoProcessorService> m_pDXVAVPS;
		CComPtr<IDirectXVideoProcessor> m_pDXVAVPD;

		DXVA2_VideoDesc          m_VideoDesc;
		DXVA2_VideoProcessorCaps m_VPCaps;

		DXVA2_Fixed32 m_ProcAmpValues[4];
		DXVA2_Fixed32 m_NFilterValues[6];
		DXVA2_Fixed32 m_DFilterValues[6];
#endif

		// Variables initialized/managed by this class but can be accessed by the allocator-presenter
		bool						m_bD3DX;
		RenderingPath				m_RenderingPath;
		D3DFORMAT					m_VideoBufferFmt;
		D3DFORMAT					m_SurfaceFmt;
		CComPtr<IDirect3DTexture9>	m_pVideoTexture[MAX_VIDEO_SURFACES];
		CComPtr<IDirect3DSurface9>	m_pVideoSurface[MAX_VIDEO_SURFACES];

		bool						m_bColorManagement;

		int							m_nDX9Resizer;

		CDX9RenderingEngine(HWND hWnd, HRESULT& hr, CString *_pError);
		~CDX9RenderingEngine();

		void InitRenderingEngine();
		void CleanupRenderingEngine();

		HRESULT CreateVideoSurfaces();
		void FreeVideoSurfaces();

		HRESULT RenderVideo(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);

		HRESULT DrawRect(DWORD _Color, DWORD _Alpha, const CRect &_Rect);
		HRESULT AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture);

		HRESULT SetCustomPixelShader(LPCSTR pSrcData, LPCSTR pTarget, bool bScreenSpace);


	private:
		class CExternalPixelShader
		{
		public:
			CComPtr<IDirect3DPixelShader9> m_pPixelShader;
			CStringA m_SourceData;
			CStringA m_SourceTarget;
			HRESULT Compile(CPixelShaderCompiler *pCompiler) {
				HRESULT hr = pCompiler->CompileShader(m_SourceData, "main", m_SourceTarget, 0, NULL, &m_pPixelShader);
				if (FAILED(hr)) {
					return hr;
				}

				return S_OK;
			}
		};

		// D3DX functions
		typedef D3DXFLOAT16* (WINAPI* D3DXFloat32To16ArrayPtr)(
			D3DXFLOAT16 *pOut,
			CONST FLOAT *pIn,
			UINT		 n);


		CAutoPtr<CPixelShaderCompiler>	 m_pPSC;

		// Settings
		VideoSystem						m_InputVideoSystem;
		AmbientLight					m_AmbientLight;
		ColorRenderingIntent			m_RenderingIntent;

		// Custom pixel shaders
		CAtlList<CExternalPixelShader>	m_pCustomPixelShaders;
		CComPtr<IDirect3DTexture9>		m_pFrameTextures[2];

		// Screen space pipeline
		int								m_ScreenSpaceTexWidth;
		int								m_ScreenSpaceTexHeight;
		CComPtr<IDirect3DTexture9>		m_pScreenSpaceTextures[2];

		// Resizers
		CComPtr<IDirect3DPixelShader9>	m_pResizerPixelShaders[shader_count];
#if ENABLE_2PASS_RESIZE
		CComPtr<IDirect3DTexture9>		m_pResizeTexture;
#endif

		// Final pass
		bool							m_bFinalPass;
		int								m_Lut3DSize;
		int								m_Lut3DEntryCount;
		CComPtr<IDirect3DVolumeTexture9> m_pLut3DTexture;
		CComPtr<IDirect3DTexture9>		m_pDitherTexture;
		CComPtr<IDirect3DPixelShader9>	m_pFinalPixelShader;

		// Custom screen space pixel shaders
		CAtlList<CExternalPixelShader>	m_pCustomScreenSpacePixelShaders;

		// D3DX function pointers
		D3DXFloat32To16ArrayPtr			m_pD3DXFloat32To16Array;


		// Video rendering paths
		HRESULT RenderVideoDrawPath(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);
		HRESULT RenderVideoStretchRectPath(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);

#if DXVAVP
		HMODULE m_hDxva2Lib = NULL;
		BOOL InitializeDXVA2VP(int width, int height);
		BOOL CreateDXVA2VPDevice(REFGUID guid);
		HRESULT RenderVideoDXVA(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect);
#endif

		// Custom pixel shaders
		HRESULT InitVideoTextures(size_t count);

		// Custom Screen space shaders
		HRESULT InitScreenSpaceTextures(size_t count);

		// Resizers
		HRESULT InitShaderResizer(int iShader);
		HRESULT TextureResize(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &srcRect, D3DTEXTUREFILTERTYPE filter);
		HRESULT TextureResizeShader(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &srcRect, int iShader);
#if ENABLE_2PASS_RESIZE
		HRESULT TextureResizeShader2pass(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &srcRect, int iShader1);
#endif

		// Final pass
		HRESULT InitFinalPass();
		void CleanupFinalPass();
		HRESULT CreateIccProfileLut(TCHAR* profilePath, float* lut3D);
		HRESULT FinalPass(IDirect3DTexture9* pTexture);

		HRESULT TextureCopy(IDirect3DTexture9* pTexture);
		bool ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d);
	};

}
