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

#include "stdafx.h"
#include <algorithm>
#include <lcms2/include/lcms2.h>
#include "DX9Shaders.h"
#include "Dither.h"
#include "DX9RenderingEngine.h"

#define NULL_PTR_ARRAY(a) for (size_t i = 0; i < _countof(a); i++) { a[i] = NULL; }

#pragma pack(push, 1)
template<int texcoords>
struct MYD3DVERTEX {
	float x, y, z, rhw;
	struct {
		float u, v;
	} t[texcoords];
};

template<>
struct MYD3DVERTEX<0> {
	float x, y, z, rhw;
	DWORD Diffuse;
};
#pragma pack(pop)

template<int texcoords>
static void AdjustQuad(MYD3DVERTEX<texcoords>* v, double dx, double dy)
{
	for (int i = 0; i < 4; i++) {
		v[i].x -= 0.5f;
		v[i].y -= 0.5f;

		for (int j = 0; j < max(texcoords - 1, 1); j++) {
			v[i].t[j].u -= (float)(0.5f*dx);
			v[i].t[j].v -= (float)(0.5f*dy);
		}

		if (texcoords > 1) {
			v[i].t[texcoords - 1].u -= 0.5f;
			v[i].t[texcoords - 1].v -= 0.5f;
		}
	}
}

template<int texcoords>
static HRESULT TextureBlt(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<texcoords> v[4], D3DTEXTUREFILTERTYPE filter)
{
	if (!pD3DDev) {
		return E_POINTER;
	}

	DWORD FVF = 0;

	switch (texcoords) {
		case 1: FVF = D3DFVF_TEX1; break;
		case 2: FVF = D3DFVF_TEX2; break;
		case 3: FVF = D3DFVF_TEX3; break;
		case 4: FVF = D3DFVF_TEX4; break;
		case 5: FVF = D3DFVF_TEX5; break;
		case 6: FVF = D3DFVF_TEX6; break;
		case 7: FVF = D3DFVF_TEX7; break;
		case 8: FVF = D3DFVF_TEX8; break;
		default:
			return E_FAIL;
	}

	HRESULT hr;

	hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

	for (int i = 0; i < texcoords; i++) {
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	//

	hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | FVF);
	// hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));

	MYD3DVERTEX<texcoords> tmp = v[2];
	v[2] = v[3];
	v[3] = tmp;
	hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

	//

	for (int i = 0; i < texcoords; i++) {
		pD3DDev->SetTexture(i, NULL);
	}

	return S_OK;
}


using namespace DSObjects;

CDX9RenderingEngine::CDX9RenderingEngine(HWND hWnd, HRESULT& hr, CString *_pError)
	: CSubPicAllocatorPresenterImpl(hWnd, hr, _pError)
	, m_ScreenSize(0, 0)
	, m_nNbDXSurface(1)
	, m_nCurSurface(0)
	, m_CurrentAdapter(0)
{
	HINSTANCE hDll = GetRenderersData()->GetD3X9Dll();
	m_bD3DX = hDll != NULL;

	if (m_bD3DX) {
		(FARPROC&)m_pD3DXFloat32To16Array = GetProcAddress(hDll, "D3DXFloat32To16Array");
	}
}

void CDX9RenderingEngine::InitRenderingEngine()
{
	// Get the device caps
	ZeroMemory(&m_Caps, sizeof(m_Caps));
	m_pD3DDev->GetDeviceCaps(&m_Caps);

	// Define the shader profile.
	if (m_Caps.PixelShaderVersion >= D3DPS_VERSION(3, 0)) {
		m_ShaderProfile = "ps_3_0";
	} else if (m_Caps.PixelShaderVersion >= D3DPS_VERSION(2,0)) {
		// http://en.wikipedia.org/wiki/High-level_shader_language

		if (m_Caps.PS20Caps.NumTemps >= 22
			&& (m_Caps.PS20Caps.Caps & (D3DPS20CAPS_ARBITRARYSWIZZLE | D3DPS20CAPS_GRADIENTINSTRUCTIONS |
			D3DPS20CAPS_PREDICATION | D3DPS20CAPS_NODEPENDENTREADLIMIT | D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))) {
			m_ShaderProfile = "ps_2_a";
		} else if (m_Caps.PS20Caps.NumTemps >= 32
			&& (m_Caps.PS20Caps.Caps & D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT)) {
			m_ShaderProfile = "ps_2_b";
		} else {
			m_ShaderProfile = "ps_2_0";
		}
	} else {
		m_ShaderProfile = NULL;
	}

	// Initialize the pixel shader compiler
	m_pPSC.Attach(DNew CPixelShaderCompiler(m_pD3DDev, true));
}

void CDX9RenderingEngine::CleanupRenderingEngine()
{
	m_pPSC.Free();

	NULL_PTR_ARRAY(m_pResizerPixelShaders);

	CleanupFinalPass();

	POSITION pos = m_pCustomScreenSpacePixelShaders.GetHeadPosition();
	while (pos) {
		CExternalPixelShader &Shader = m_pCustomScreenSpacePixelShaders.GetNext(pos);
		Shader.m_pPixelShader = NULL;
	}
	pos = m_pCustomPixelShaders.GetHeadPosition();
	while (pos) {
		CExternalPixelShader &Shader = m_pCustomPixelShaders.GetNext(pos);
		Shader.m_pPixelShader = NULL;
	}

	NULL_PTR_ARRAY(m_pFrameTextures)
	NULL_PTR_ARRAY(m_pScreenSpaceTextures);

#if ENABLE_2PASS_RESIZE
	m_pResizeTexture = NULL;
#endif
}

HRESULT CDX9RenderingEngine::CreateVideoSurfaces()
{
	HRESULT hr;
	CRenderersSettings& settings = GetRenderersSettings();

	// Free previously allocated video surfaces
	FreeVideoSurfaces();

	// Free previously allocated temporary video textures, because the native video size might have been changed!
	NULL_PTR_ARRAY(m_pFrameTextures);

	if (settings.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE2D) {
		if (FAILED(hr = m_pD3DDev->CreateTexture(
			m_nativeVideoSize.cx, m_nativeVideoSize.cy, 1,
			D3DUSAGE_RENDERTARGET, m_SurfaceType,
			D3DPOOL_DEFAULT, &m_pVideoTexture[0], NULL))) {
			return hr;
		}

		hr = m_pVideoTexture[0]->GetSurfaceLevel(0, &m_pVideoSurface[0]);
		m_pVideoTexture[0] = NULL;

		if (FAILED(hr)) {
			return hr;
		}

		m_RenderingPath = RENDERING_PATH_STRETCHRECT;
	}
	else if (settings.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D) {
		for (int i = 0; i < m_nNbDXSurface; i++) {
			if (FAILED(hr = m_pD3DDev->CreateTexture(
								m_nativeVideoSize.cx, m_nativeVideoSize.cy, 1,
								D3DUSAGE_RENDERTARGET, m_SurfaceType,
								D3DPOOL_DEFAULT, &m_pVideoTexture[i], NULL))) {
				return hr;
			}

			if (FAILED(hr = m_pVideoTexture[i]->GetSurfaceLevel(0, &m_pVideoSurface[i]))) {
				return hr;
			}
		}

		m_RenderingPath = RENDERING_PATH_DRAW;
	}
	else {
		if (FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
							m_nativeVideoSize.cx, m_nativeVideoSize.cy,
							m_SurfaceType,
							D3DPOOL_DEFAULT, &m_pVideoSurface[m_nCurSurface], NULL))) {
			return hr;
		}

		m_RenderingPath = RENDERING_PATH_STRETCHRECT;
	}

	hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	return S_OK;
}

void CDX9RenderingEngine::FreeVideoSurfaces()
{
	for (int i = 0; i < m_nNbDXSurface; i++) {
		m_pVideoTexture[i] = NULL;
		m_pVideoSurface[i] = NULL;
	}
}

HRESULT CDX9RenderingEngine::RenderVideo(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect)
{
	if (destRect.IsRectEmpty()) {
		return S_OK;
	}

	if (m_RenderingPath == RENDERING_PATH_DRAW) {
		return RenderVideoDrawPath(pRenderTarget, srcRect, destRect);
	} else {
		return RenderVideoStretchRectPath(pRenderTarget, srcRect, destRect);
	}
}

HRESULT CDX9RenderingEngine::RenderVideoDrawPath(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect)
{
	HRESULT hr;

	// Return if the video texture is not initialized
	if (m_pVideoTexture[m_nCurSurface] == 0) {
		return S_OK;
	}

	CRenderersSettings& settings = GetRenderersSettings();

	// Initialize the processing pipeline
	bool bCustomPixelShaders;
	bool bCustomScreenSpacePixelShaders;
	bool bFinalPass;

	int screenSpacePassCount = 0;
	DWORD iDX9Resizer = settings.iDX9Resizer;

	if (m_bD3DX) {
		// Final pass. Must be initialized first!
		hr = InitFinalPass();
		if (SUCCEEDED(hr)) {
			bFinalPass = m_bFinalPass;
		} else {
			bFinalPass = false;
		}

		if (bFinalPass) {
			screenSpacePassCount++;
		}

		// Resizers
		hr = E_FAIL;
		switch (iDX9Resizer) {
		case RESIZER_NEAREST:
		case RESIZER_BILINEAR:
			hr = S_OK;
			break;
		case RESIZER_SHADER_SMOOTHERSTEP: hr = InitShaderResizer(shader_smootherstep); break;
#if ENABLE_2PASS_RESIZE
		case RESIZER_SHADER_BICUBIC06: hr = InitShaderResizer(shader_bicubic06_x); break;
		case RESIZER_SHADER_BICUBIC08: hr = InitShaderResizer(shader_bicubic08_x); break;
		case RESIZER_SHADER_BICUBIC10: hr = InitShaderResizer(shader_bicubic10_x); break;
		case RESIZER_SHADER_BSPLINE4:  hr = InitShaderResizer(shader_bspline4_x);  break;
		case RESIZER_SHADER_MITCHELL4: hr = InitShaderResizer(shader_mitchell4_x); break;
		case RESIZER_SHADER_CATMULL4:  hr = InitShaderResizer(shader_catmull4_x);  break;
		case RESIZER_SHADER_LANCZOS2:  hr = InitShaderResizer(shader_lanczos2_x);  break;
#else
		case RESIZER_SHADER_BICUBIC06: hr = InitShaderResizer(shader_bicubic06); break;
		case RESIZER_SHADER_BICUBIC08: hr = InitShaderResizer(shader_bicubic08); break;
		case RESIZER_SHADER_BICUBIC10: hr = InitShaderResizer(shader_bicubic10); break;
		case RESIZER_SHADER_BSPLINE4:  hr = InitShaderResizer(shader_bspline4);  break;
		case RESIZER_SHADER_MITCHELL4: hr = InitShaderResizer(shader_mitchell4); break;
		case RESIZER_SHADER_CATMULL4:  hr = InitShaderResizer(shader_catmull4);  break;
#endif
		}

		if (FAILED(hr)) {
			iDX9Resizer = RESIZER_BILINEAR;
		}

		screenSpacePassCount++; // currently all resizers are 1-pass
#if ENABLE_2PASS_RESIZE
		if (iDX9Resizer >= RESIZER_SHADER_BICUBIC06 && iDX9Resizer != RESIZER_SHADER_SMOOTHERSTEP && srcRect.Size() != destRect.Size()) {
			screenSpacePassCount++;
		}
#endif

		// Custom screen space pixel shaders
		bCustomScreenSpacePixelShaders = !m_pCustomScreenSpacePixelShaders.IsEmpty();

		if (bCustomScreenSpacePixelShaders) {
			screenSpacePassCount += (int)m_pCustomScreenSpacePixelShaders.GetCount();
		}

		// Custom pixel shaders
		bCustomPixelShaders = !m_pCustomPixelShaders.IsEmpty();

		hr = InitVideoTextures(m_pCustomPixelShaders.GetCount());
		if (FAILED(hr)) {
			bCustomPixelShaders = false;
		}
	} else {
		bCustomPixelShaders = false;
		if (iDX9Resizer != RESIZER_NEAREST) {
			iDX9Resizer = RESIZER_BILINEAR;
		}
		bCustomScreenSpacePixelShaders = false;
		bFinalPass = false;
	}

	hr = InitScreenSpaceTextures(screenSpacePassCount);
	if (FAILED(hr)) {
		bCustomScreenSpacePixelShaders = false;
		bFinalPass = false;
	}

	// Apply the custom pixel shaders if there are any. Result: pVideoTexture
	CComPtr<IDirect3DTexture9> pVideoTexture = m_pVideoTexture[m_nCurSurface];

	if (bCustomPixelShaders) {
		static __int64 counter = 0;
		static long start = clock();

		long stop = clock();
		long diff = stop - start;

		if (diff >= 10*60*CLOCKS_PER_SEC) {
			start = stop;    // reset after 10 min (ps float has its limits in both range and accuracy)
		}

#if 1
		D3DSURFACE_DESC desc;
		m_pVideoTexture[m_nCurSurface]->GetLevelDesc(0, &desc);

		float fConstData[][4] = {
			{(float)desc.Width, (float)desc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
			{1.0f / desc.Width, 1.0f / desc.Height, 0, 0},
		};
#else
		CSize VideoSize = GetVisibleVideoSize();

		float fConstData[][4] = {
			{(float)VideoSize.cx, (float)VideoSize.cy, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
			{1.0f / VideoSize.cx, 1.0f / VideoSize.cy, 0, 0},
		};
#endif

		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));

		int src = 1;
		int dest = 0;
		bool first = true;

		POSITION pos = m_pCustomPixelShaders.GetHeadPosition();
		while (pos) {
			CComPtr<IDirect3DSurface9> pTemporarySurface;
			hr = m_pFrameTextures[dest]->GetSurfaceLevel(0, &pTemporarySurface);
			hr = m_pD3DDev->SetRenderTarget(0, pTemporarySurface);

			CExternalPixelShader &Shader = m_pCustomPixelShaders.GetNext(pos);
			if (!Shader.m_pPixelShader) {
				Shader.Compile(m_pPSC);
			}
			hr = m_pD3DDev->SetPixelShader(Shader.m_pPixelShader);

			if (first) {
				TextureCopy(m_pVideoTexture[m_nCurSurface]);
				first = false;
			} else {
				TextureCopy(m_pFrameTextures[src]);
			}

			std::swap(src, dest);
		}

		pVideoTexture = m_pFrameTextures[src];
	}

	// Resize the frame
	Vector dst[4];
	Transform(destRect, dst);

	if (bCustomScreenSpacePixelShaders || bFinalPass) {
		CComPtr<IDirect3DSurface9> pTemporarySurface;
		hr = m_pScreenSpaceTextures[0]->GetSurfaceLevel(0, &pTemporarySurface);

		if (SUCCEEDED(hr)) {
			hr = m_pD3DDev->SetRenderTarget(0, pTemporarySurface);
			hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
		}
	} else {
		hr = m_pD3DDev->SetRenderTarget(0, pRenderTarget);
	}

	if (srcRect.Size() != destRect.Size()) {
		switch (iDX9Resizer) {
		case RESIZER_NEAREST:  hr = TextureResize(pVideoTexture, dst, srcRect, D3DTEXF_POINT);  break;
		case RESIZER_BILINEAR: hr = TextureResize(pVideoTexture, dst, srcRect, D3DTEXF_LINEAR); break;
		case RESIZER_SHADER_SMOOTHERSTEP: hr = TextureResizeShader(pVideoTexture, dst, srcRect, shader_smootherstep); break;
#if ENABLE_2PASS_RESIZE
		case RESIZER_SHADER_BICUBIC06: hr = TextureResizeShader2pass(pVideoTexture, dst, srcRect, shader_bicubic06_x); break;
		case RESIZER_SHADER_BICUBIC08: hr = TextureResizeShader2pass(pVideoTexture, dst, srcRect, shader_bicubic08_x); break;
		case RESIZER_SHADER_BICUBIC10: hr = TextureResizeShader2pass(pVideoTexture, dst, srcRect, shader_bicubic10_x); break;
		case RESIZER_SHADER_BSPLINE4:  hr = TextureResizeShader2pass(pVideoTexture, dst, srcRect, shader_bspline4_x);  break;
		case RESIZER_SHADER_MITCHELL4: hr = TextureResizeShader2pass(pVideoTexture, dst, srcRect, shader_mitchell4_x); break;
		case RESIZER_SHADER_CATMULL4:  hr = TextureResizeShader2pass(pVideoTexture, dst, srcRect, shader_catmull4_x);  break;
		case RESIZER_SHADER_LANCZOS2:  hr = TextureResizeShader2pass(pVideoTexture, dst, srcRect, shader_lanczos2_x);  break;
#else
		case RESIZER_SHADER_BICUBIC06: hr = TextureResizeShader(pVideoTexture, dst, srcRect, shader_bicubic06); break;
		case RESIZER_SHADER_BICUBIC08: hr = TextureResizeShader(pVideoTexture, dst, srcRect, shader_bicubic08); break;
		case RESIZER_SHADER_BICUBIC10: hr = TextureResizeShader(pVideoTexture, dst, srcRect, shader_bicubic10); break;
		case RESIZER_SHADER_BSPLINE4:  hr = TextureResizeShader(pVideoTexture, dst, srcRect, shader_bspline4);  break;
		case RESIZER_SHADER_MITCHELL4: hr = TextureResizeShader(pVideoTexture, dst, srcRect, shader_mitchell4); break;
		case RESIZER_SHADER_CATMULL4:  hr = TextureResizeShader(pVideoTexture, dst, srcRect, shader_catmull4);  break;
#endif
		}
	} else {
		hr = TextureResize(pVideoTexture, dst, srcRect, D3DTEXF_POINT);
	}


	int src = 0;
	int dest = 1;
	// Apply the custom screen size pixel shaders
	if (bCustomScreenSpacePixelShaders) {
		static __int64 counter = 555;
		static long start = clock() + 333;

		long stop = clock() + 333;
		long diff = stop - start;

		if (diff >= 10*60*CLOCKS_PER_SEC) {
			start = stop;    // reset after 10 min (ps float has its limits in both range and accuracy)
		}

		float fConstData[][4] = {
			{(float)m_ScreenSpaceTexWidth, (float)m_ScreenSpaceTexHeight, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
			{1.0f / m_ScreenSpaceTexWidth, 1.0f / m_ScreenSpaceTexHeight, 0, 0},
		};

		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));

		POSITION pos = m_pCustomScreenSpacePixelShaders.GetHeadPosition();
		while (pos) {
			CExternalPixelShader &Shader = m_pCustomScreenSpacePixelShaders.GetNext(pos);
			if (!Shader.m_pPixelShader) {
				Shader.Compile(m_pPSC);
			}

			if (pos || bFinalPass) {
				CComPtr<IDirect3DSurface9> pTemporarySurface;
				hr = m_pScreenSpaceTextures[dest]->GetSurfaceLevel(0, &pTemporarySurface);
				if (SUCCEEDED(hr)) {
					hr = m_pD3DDev->SetRenderTarget(0, pTemporarySurface);
				}
			} else {
				hr = m_pD3DDev->SetRenderTarget(0, pRenderTarget);
			}

			hr = m_pD3DDev->SetPixelShader(Shader.m_pPixelShader);
			TextureCopy(m_pScreenSpaceTextures[src]);
			std::swap(src, dest);
		}
	}

	// Final pass
	if (bFinalPass) {
		hr = m_pD3DDev->SetRenderTarget(0, pRenderTarget);

		hr = FinalPass(m_pScreenSpaceTextures[src]);
	}

	hr = m_pD3DDev->SetPixelShader(NULL);

	return hr;
}

HRESULT CDX9RenderingEngine::RenderVideoStretchRectPath(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect)
{
	HRESULT hr;

	// Return if the render target or the video surface is not initialized
	if (pRenderTarget == 0 || m_pVideoSurface[m_nCurSurface] == 0) {
		return S_OK;
	}

	D3DTEXTUREFILTERTYPE filter = GetRenderersSettings().iDX9Resizer == RESIZER_NEAREST ? D3DTEXF_POINT : D3DTEXF_LINEAR;

	CRect rSrcVid(srcRect);
	CRect rDstVid(destRect);

	ClipToSurface(pRenderTarget, rSrcVid, rDstVid); // grrr
	// IMPORTANT: rSrcVid has to be aligned on mod2 for yuy2->rgb conversion with StretchRect!!!
	rSrcVid.left &= ~1;
	rSrcVid.right &= ~1;
	rSrcVid.top &= ~1;
	rSrcVid.bottom &= ~1;
	hr = m_pD3DDev->StretchRect(m_pVideoSurface[m_nCurSurface], rSrcVid, pRenderTarget, rDstVid, filter);

	return hr;
}

HRESULT CDX9RenderingEngine::InitVideoTextures(size_t count)
{
	HRESULT hr = S_OK;

	if (count > _countof(m_pFrameTextures)) {
		count = _countof(m_pFrameTextures);
	}

	for (size_t i = 0; i < count; i++) {
		if (m_pFrameTextures[i] == NULL) {
			hr = m_pD3DDev->CreateTexture(
					 m_nativeVideoSize.cx, m_nativeVideoSize.cy, 1, D3DUSAGE_RENDERTARGET, m_SurfaceType,
					 D3DPOOL_DEFAULT, &m_pFrameTextures[i], NULL);

			if (FAILED(hr)) {
				// Free all textures
				NULL_PTR_ARRAY(m_pFrameTextures);

				return hr;
			}
		}
	}

	// Free unnecessary textures
	for (size_t i = count; i < _countof(m_pFrameTextures); i++) {
		m_pFrameTextures[i] = NULL;
	}

	return hr;
}

HRESULT CDX9RenderingEngine::InitScreenSpaceTextures(size_t count)
{
	HRESULT hr = S_OK;
	if (count > _countof(m_pScreenSpaceTextures)) {
		count = _countof(m_pScreenSpaceTextures);
	}

	m_ScreenSpaceTexWidth = min(m_ScreenSize.cx, (int)m_Caps.MaxTextureWidth);
	m_ScreenSpaceTexHeight = min(m_ScreenSize.cy, (int)m_Caps.MaxTextureHeight);

	for (size_t i = 0; i < count; i++) {
		if (m_pScreenSpaceTextures[i] == NULL) {
			hr = m_pD3DDev->CreateTexture(
					m_ScreenSpaceTexWidth, m_ScreenSpaceTexHeight, 1, D3DUSAGE_RENDERTARGET, m_SurfaceType,
					D3DPOOL_DEFAULT, &m_pScreenSpaceTextures[i], NULL);

			if (FAILED(hr)) {
				// Free all textures
				NULL_PTR_ARRAY(m_pScreenSpaceTextures);

				return hr;
			}
		}
	}

	// Free unnecessary textures
	for (size_t i = count; i < _countof(m_pScreenSpaceTextures); i++) {
		m_pScreenSpaceTextures[i] = NULL;
	}

	return hr;
}

HRESULT CDX9RenderingEngine::InitShaderResizer(int iShader)
{
	if (iShader < 0 || iShader >= shader_count) {
		return E_INVALIDARG;
	}

	if (m_pResizerPixelShaders[iShader]) {
		return S_OK;
	}

	if (!m_ShaderProfile) {
		return E_FAIL;
	}

	bool twopass = false;
	LPCSTR pSrcData = NULL;
	D3DXMACRO ShaderMacros[3] = {
		{ "Ml", m_Caps.PixelShaderVersion >= D3DPS_VERSION(3, 0) ? "1" : "0" },
		{ NULL, NULL },
		{ NULL, NULL }
	};

	switch (iShader) {
	case shader_smootherstep:
		pSrcData = shader_resizer_smootherstep;
		break;
#if ENABLE_2PASS_RESIZE
	case shader_bicubic06_y:
		iShader--;
	case shader_bicubic06_x:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-0.6" };
		twopass = true;
		break;
	case shader_bicubic08_y:
		iShader--;
	case shader_bicubic08_x:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-0.8" };
		twopass = true;
		break;
	case shader_bicubic10_y:
		iShader--;
	case shader_bicubic10_x:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-1.0" };
		twopass = true;
		break;
	case shader_bspline4_y:
		iShader--;
	case shader_bspline4_x:
		pSrcData = shader_resizer_bspline4;
		twopass = true;
		break;
	case shader_mitchell4_y:
		iShader--;
	case shader_mitchell4_x:
		pSrcData = shader_resizer_mitchell4;
		twopass = true;
		break;
	case shader_catmull4_y:
		iShader--;
	case shader_catmull4_x:
		pSrcData = shader_resizer_catmull4;
		twopass = true;
		break;
	case shader_lanczos2_y:
		iShader--;
	case shader_lanczos2_x:
		pSrcData = shader_resizer_lanczos2;
		twopass = true;
		break;
#else
	case shader_bicubic06:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-0.6" };
		break;
	case shader_bicubic08:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-0.8" };
		break;
	case shader_bicubic10:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-1.0" };
		break;
	case shader_bspline4:
		pSrcData = shader_resizer_bspline4;
		break;
	case shader_mitchell4:
		pSrcData = shader_resizer_mitchell4;
		break;
	case shader_catmull4:
		pSrcData = shader_resizer_catmull4;
		break;
#endif
	}

	if (!pSrcData) {
		return E_FAIL;
	}

	HRESULT hr = S_OK;
	CString ErrorMessage;

	if (twopass) {
#if ENABLE_2PASS_RESIZE
		hr = m_pPSC->CompileShader(pSrcData, "main_x", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShaders[iShader], &ErrorMessage);
		if (hr == S_OK) {
			hr = m_pPSC->CompileShader(pSrcData, "main_y", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShaders[iShader + 1], &ErrorMessage);
		}

		if (hr == S_OK && !m_pResizerPixelShaders[shader_downscaling_x]) {
			pSrcData = shader_resizer_downscaling_2pass;

			hr = m_pPSC->CompileShader(pSrcData, "main_x", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShaders[shader_downscaling_x], &ErrorMessage);
			if (hr == S_OK) {
				hr = m_pPSC->CompileShader(pSrcData, "main_y", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShaders[shader_downscaling_y], &ErrorMessage);
			}
		}
#endif
	} else {
		hr = m_pPSC->CompileShader(pSrcData, "main", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShaders[iShader], &ErrorMessage);

		if (hr == S_OK && m_Caps.PixelShaderVersion >= D3DPS_VERSION(3, 0) && !m_pResizerPixelShaders[shader_downscaling]) {
			pSrcData = shader_resizer_downscaling;
			hr = m_pPSC->CompileShader(pSrcData, "main", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShaders[shader_downscaling], &ErrorMessage);
		}
	}

	if (FAILED(hr)) {
		DbgLog((LOG_TRACE, 3, L"CDX9RenderingEngine::InitShaderResizer() : shader compilation failed\n%s", ErrorMessage.GetString()));
		ASSERT(0);
		return hr;
	}

	return S_OK;
}

HRESULT CDX9RenderingEngine::TextureResize(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &srcRect, D3DTEXTUREFILTERTYPE filter)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	float dx2 = 1.0f/w;
	float dy2 = 1.0f/h;

	MYD3DVERTEX<1> v[] = {
		{dst[0].x, dst[0].y, dst[0].z, 1.0f/dst[0].z, {srcRect.left * dx2,  srcRect.top * dy2} },
		{dst[1].x, dst[1].y, dst[1].z, 1.0f/dst[1].z, {srcRect.right * dx2, srcRect.top * dy2} },
		{dst[2].x, dst[2].y, dst[2].z, 1.0f/dst[2].z, {srcRect.left * dx2,  srcRect.bottom * dy2} },
		{dst[3].x, dst[3].y, dst[3].z, 1.0f/dst[3].z, {srcRect.right * dx2, srcRect.bottom * dy2} },
	};
	AdjustQuad(v, 0, 0);

	hr = m_pD3DDev->SetTexture(0, pTexture);
	hr = m_pD3DDev->SetPixelShader(NULL);
	hr = TextureBlt(m_pD3DDev, v, filter);

	return hr;
}

HRESULT CDX9RenderingEngine::TextureResizeShader(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &srcRect, int iShader)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	const float w = sqrt(pow(dst[1].x - dst[0].x, 2) + pow(dst[1].y - dst[0].y, 2) + pow(dst[1].z - dst[0].z, 2));
	const float h = sqrt(pow(dst[2].x - dst[0].x, 2) + pow(dst[2].y - dst[0].y, 2) + pow(dst[2].z - dst[0].z, 2));
	const float rx = srcRect.Width() / w;
	const float ry = srcRect.Height() / h;

	// make const to give compiler a chance of optimising, also float faster than double and converted to float to sent to PS anyway
	const float dx = 1.0f/(float)desc.Width;
	const float dy = 1.0f/(float)desc.Height;
	const float tx0 = (float)srcRect.left - 0.5f;
	const float tx1 = (float)srcRect.right - 0.5f;
	const float ty0 = (float)srcRect.top - 0.5f;
	const float ty1 = (float)srcRect.bottom - 0.5f;

	MYD3DVERTEX<1> v[] = {
		{dst[0].x - 0.5f, dst[0].y -0.5f, dst[0].z, 1.0f/dst[0].z, { tx0, ty0 } },
		{dst[1].x - 0.5f, dst[1].y -0.5f, dst[1].z, 1.0f/dst[1].z, { tx1, ty0 } },
		{dst[2].x - 0.5f, dst[2].y -0.5f, dst[2].z, 1.0f/dst[2].z, { tx0, ty1 } },
		{dst[3].x - 0.5f, dst[3].y -0.5f, dst[3].z, 1.0f/dst[3].z, { tx1, ty1 } },
	};

	hr = m_pD3DDev->SetTexture(0, pTexture);

	if (m_pResizerPixelShaders[shader_downscaling] && rx > 2.0f && ry > 2.0f) {
		float fConstData[][4] = {{dx, dy, 0, 0}, {rx, 0, 0, 0}, {ry, 0, 0, 0}};
		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShaders[shader_downscaling]);
	}
	else {
		float fConstData[][4] = { { dx, dy, 0, 0 }, { dx*0.5f, dy*0.5f, 0, 0 }, { dx, 0, 0, 0 }, { 0, dy, 0, 0 } };
		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShaders[iShader]);
	}
	hr = TextureBlt(m_pD3DDev, v, D3DTEXF_POINT);

	return hr;
}

#if ENABLE_2PASS_RESIZE
// The 2 pass sampler is incorrect in that it only does bilinear resampling in the y direction.
HRESULT CDX9RenderingEngine::TextureResizeShader2pass(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &srcRect, int iShader1)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	UINT texWidth = min((UINT)m_ScreenSize.cx, m_Caps.MaxTextureWidth);
	UINT TexHeight = min((UINT)m_nativeVideoSize.cy, m_Caps.MaxTextureHeight);
	if (!m_pResizeTexture) {
		hr = m_pD3DDev->CreateTexture(
					texWidth, TexHeight, 1, D3DUSAGE_RENDERTARGET,
					m_SurfaceType == D3DFMT_A32B32G32R32F ? D3DFMT_A32B32G32R32F : D3DFMT_A16B16G16R16F, // use only float textures here
					D3DPOOL_DEFAULT, &m_pResizeTexture, NULL);
		if (FAILED(hr)) {
			m_pResizeTexture = NULL;
			return TextureResize(pTexture, dst, srcRect, D3DTEXF_LINEAR);
		}
	}

	float w2 = sqrt(pow(dst[1].x - dst[0].x, 2) + pow(dst[1].y - dst[0].y, 2) + pow(dst[1].z - dst[0].z, 2));
	float h2 = sqrt(pow(dst[2].x - dst[0].x, 2) + pow(dst[2].y - dst[0].y, 2) + pow(dst[2].z - dst[0].z, 2));

	float rx = srcRect.Width() / w2;
	float ry = srcRect.Height() / h2;

	const float dx0 = 1.0f/(float)desc.Width;
	const float dy0 = 1.0f/(float)desc.Height;

	float w1 = min(w2, (float)texWidth);
	float h1 = (float)min(srcRect.Height(), (int)TexHeight);

	if (FAILED(m_pResizeTexture->GetLevelDesc(0, &desc))) {
		return TextureResize(pTexture, dst, srcRect, D3DTEXF_LINEAR);
	}

	const float dx1 = 1.0f/(float)desc.Width;
	const float dy1 = 1.0f/(float)desc.Height;

	const float tx0 = (float)srcRect.left - 0.5f;
	const float tx1 = (float)srcRect.right - 0.5f;
	const float ty0 = (float)srcRect.top - 0.5f;
	const float ty1 = (float)srcRect.bottom - 0.5f;

	w1 -= 0.5f;
	h1 -= 0.5f;

	MYD3DVERTEX<1> vx[] = {
		{ -0.5f, -0.5f, 0.5f, 2.0f, { tx0, ty0 } },
		{    w1, -0.5f, 0.5f, 2.0f, { tx1, ty0 } },
		{ -0.5f,    h1, 0.5f, 2.0f, { tx0, ty1 } },
		{    w1,    h1, 0.5f, 2.0f, { tx1, ty1 } },
	};

	MYD3DVERTEX<1> vy[] = {
		{dst[0].x - 0.5f, dst[0].y - 0.5f, dst[0].z, 1.0/dst[0].z, { -0.5f, -0.5f } },
		{dst[1].x - 0.5f, dst[1].y - 0.5f, dst[1].z, 1.0/dst[1].z, {    w1, -0.5f } },
		{dst[2].x - 0.5f, dst[2].y - 0.5f, dst[2].z, 1.0/dst[2].z, { -0.5f,    h1 } },
		{dst[3].x - 0.5f, dst[3].y - 0.5f, dst[3].z, 1.0/dst[3].z, {    w1,    h1 } },
	};

	// remember current RenderTarget
	CComPtr<IDirect3DSurface9> pRenderTarget;
	hr = m_pD3DDev->GetRenderTarget(0, &pRenderTarget);
	// set temp RenderTarget
	CComPtr<IDirect3DSurface9> pTempRenderTarget;
	hr = m_pResizeTexture->GetSurfaceLevel(0, &pTempRenderTarget);
	hr = m_pD3DDev->SetRenderTarget(0, pTempRenderTarget);

	// resize width
	hr = m_pD3DDev->SetTexture(0, pTexture);
	if (rx > 2.0f) {
		float fConstData[][4] = {{dx0, dy0, 0, 0}, {rx, 0, 0, 0}, {ry, 0, 0, 0}};
		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShaders[shader_downscaling_x]);
	}
	else {
		float fConstData[][4] = {{dx0, dy0, 0, 0}};
		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShaders[iShader1]);
	}
	hr = TextureBlt(m_pD3DDev, vx, D3DTEXF_POINT);

	// restore current RenderTarget
	hr = m_pD3DDev->SetRenderTarget(0, pRenderTarget);

	// resize height
	hr = m_pD3DDev->SetTexture(0, m_pResizeTexture);
	if (ry > 2.0f) {
		float fConstData[][4] = {{dx1, dy1, 0, 0}, {rx, 0, 0, 0}, {ry, 0, 0, 0}};
		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShaders[shader_downscaling_y]);
	}
	else {
		float fConstData[][4] = {{dx1, dy1, 0, 0}};
		hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShaders[iShader1 + 1]);
	}
	hr = TextureBlt(m_pD3DDev, vy, D3DTEXF_POINT);

	return hr;
}
#endif

HRESULT CDX9RenderingEngine::InitFinalPass()
{
	HRESULT hr;

	CRenderersSettings& settings = GetRenderersSettings();
	CRenderersData* data = GetRenderersData();

	// Check whether the final pass must be initialized
	bool bColorManagement = settings.m_AdvRendSets.iVMR9ColorManagementEnable;
	VideoSystem inputVideoSystem = static_cast<VideoSystem>(settings.m_AdvRendSets.iVMR9ColorManagementInput);
	AmbientLight ambientLight = static_cast<AmbientLight>(settings.m_AdvRendSets.iVMR9ColorManagementAmbientLight);
	ColorRenderingIntent renderingIntent = static_cast<ColorRenderingIntent>(settings.m_AdvRendSets.iVMR9ColorManagementIntent);

	bool bInitRequired = false;

	if (m_bColorManagement != bColorManagement) {
		bInitRequired = true;
	}

	if (m_bColorManagement && bColorManagement) {
		if ((m_InputVideoSystem != inputVideoSystem) ||
				(m_RenderingIntent != renderingIntent) ||
				(m_AmbientLight != ambientLight)) {
			bInitRequired = true;
		}
	}

	if (!m_bFinalPass) {
		bInitRequired = true;
	}

	if (!bInitRequired) {
		return S_OK;
	}

	// Check whether the final pass is supported by the hardware
	m_bFinalPass = data->m_bFP16Support;
	if (!m_bFinalPass) {
		return S_OK;
	}

	// Update the settings
	m_bColorManagement = bColorManagement;
	m_InputVideoSystem = inputVideoSystem;
	m_AmbientLight = ambientLight;
	m_RenderingIntent = renderingIntent;

	// Check whether the final pass is required
	m_bFinalPass = bColorManagement || m_SurfaceType == D3DFMT_A16B16G16R16F || m_SurfaceType == D3DFMT_A32B32G32R32F || m_SurfaceType == D3DFMT_A2R10G10B10 && m_DisplayType != D3DFMT_A2R10G10B10;

	if (!m_bFinalPass) {
		return S_OK;
	}

	// Initial cleanup
	m_pLut3DTexture = NULL;
	m_pFinalPixelShader = NULL;

	if (!m_pDitherTexture) {
		// Create the dither texture
		hr = m_pD3DDev->CreateTexture(DITHER_MATRIX_SIZE, DITHER_MATRIX_SIZE,
									  1,
									  D3DUSAGE_DYNAMIC,
									  D3DFMT_A16B16G16R16F,
									  D3DPOOL_DEFAULT,
									  &m_pDitherTexture,
									  NULL);

		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}

		D3DLOCKED_RECT lockedRect;
		hr = m_pDitherTexture->LockRect(0, &lockedRect, NULL, D3DLOCK_DISCARD);
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}

		char* outputRowIterator = static_cast<char*>(lockedRect.pBits);
		for (int y = 0; y < DITHER_MATRIX_SIZE; y++) {
			unsigned short* outputIterator = reinterpret_cast<unsigned short*>(outputRowIterator);
			for (int x = 0; x < DITHER_MATRIX_SIZE; x++) {
				for (int i = 0; i < 4; i++) {
					*outputIterator++ = DITHER_MATRIX[y][x];
				}
			}

			outputRowIterator += lockedRect.Pitch;
		}

		hr = m_pDitherTexture->UnlockRect(0);
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}
	}

	// Initialize the color management if necessary
	if (bColorManagement) {
		// Get the ICC profile path
		TCHAR* iccProfilePath = 0;
		HDC hDC = GetDC(m_hWnd);

		if (hDC != NULL) {
			DWORD icmProfilePathSize = 0;
			GetICMProfile(hDC, &icmProfilePathSize, NULL);
			iccProfilePath = DNew TCHAR[icmProfilePathSize];
			if (!GetICMProfile(hDC, &icmProfilePathSize, iccProfilePath)) {
				delete[] iccProfilePath;
				iccProfilePath = 0;
			}

			ReleaseDC(m_hWnd, hDC);
		}

		// Create the 3D LUT texture
		m_Lut3DSize = 64; // 64x64x64 LUT is enough for high-quality color management
		m_Lut3DEntryCount = m_Lut3DSize * m_Lut3DSize * m_Lut3DSize;

		hr = m_pD3DDev->CreateVolumeTexture(m_Lut3DSize, m_Lut3DSize, m_Lut3DSize,
											1,
											D3DUSAGE_DYNAMIC,
											D3DFMT_A16B16G16R16F,
											D3DPOOL_DEFAULT,
											&m_pLut3DTexture,
											NULL);

		if (FAILED(hr)) {
			delete[] iccProfilePath;
			CleanupFinalPass();
			return hr;
		}

		float* lut3DFloat32 = DNew float[m_Lut3DEntryCount * 3];
		hr = CreateIccProfileLut(iccProfilePath, lut3DFloat32);
		delete[] iccProfilePath;
		if (FAILED(hr)) {
			delete[] lut3DFloat32;
			CleanupFinalPass();
			return hr;
		}

		D3DXFLOAT16* lut3DFloat16 = DNew D3DXFLOAT16[m_Lut3DEntryCount * 3];
		m_pD3DXFloat32To16Array(lut3DFloat16, lut3DFloat32, m_Lut3DEntryCount * 3);
		delete[] lut3DFloat32;

		const float oneFloat32 = 1.0f;
		D3DXFLOAT16 oneFloat16;
		m_pD3DXFloat32To16Array(&oneFloat16, &oneFloat32, 1);

		D3DLOCKED_BOX lockedBox;
		hr = m_pLut3DTexture->LockBox(0, &lockedBox, NULL, D3DLOCK_DISCARD);
		if (FAILED(hr)) {
			delete[] lut3DFloat16;
			CleanupFinalPass();
			return hr;
		}

		D3DXFLOAT16* lut3DFloat16Iterator = lut3DFloat16;
		char* outputSliceIterator = static_cast<char*>(lockedBox.pBits);
		for (int b = 0; b < m_Lut3DSize; b++) {
			char* outputRowIterator = outputSliceIterator;

			for (int g = 0; g < m_Lut3DSize; g++) {
				D3DXFLOAT16* outputIterator = reinterpret_cast<D3DXFLOAT16*>(outputRowIterator);

				for (int r = 0; r < m_Lut3DSize; r++) {
					// R, G, B
					for (int i = 0; i < 3; i++) {
						*outputIterator++ = *lut3DFloat16Iterator++;
					}

					// A
					*outputIterator++ = oneFloat16;
				}

				outputRowIterator += lockedBox.RowPitch;
			}

			outputSliceIterator += lockedBox.SlicePitch;
		}

		hr = m_pLut3DTexture->UnlockBox(0);
		delete[] lut3DFloat16;
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}
	}

	// Compile the final pixel shader
	LPCSTR pSrcData = shader_final;
	D3DXMACRO ShaderMacros[5] = { { NULL, NULL }, };
	size_t i = 0;

	ShaderMacros[i++] = { "Ml", m_Caps.PixelShaderVersion >= D3DPS_VERSION(3, 0) ? "1" : "0" };
	ShaderMacros[i++] = { "QUANTIZATION", m_DisplayType == D3DFMT_A2R10G10B10 ? "1023.0" : "255.0"}; // 10-bit or 8-bit
	ShaderMacros[i++] = { "LUT3D_ENABLED", bColorManagement ? "1": "0" };

	if (bColorManagement) {
		static char lut3DSizeStr[12];
		sprintf(lut3DSizeStr, "%d", m_Lut3DSize);
		ShaderMacros[i++] = { "LUT3D_SIZE", lut3DSizeStr };
	}

	CString ErrorMessage;
	hr = m_pPSC->CompileShader(pSrcData, "main", m_ShaderProfile, 0, ShaderMacros, &m_pFinalPixelShader, &ErrorMessage);
	if (FAILED(hr)) {
		DbgLog((LOG_TRACE, 3, L"CDX9RenderingEngine::InitFinalPass() : shader compilation failed\n%s", ErrorMessage.GetString()));
		ASSERT(0);
		CleanupFinalPass();
		return hr;
	}

	return S_OK;
}

void CDX9RenderingEngine::CleanupFinalPass()
{
	m_bFinalPass = false;
	m_pDitherTexture = NULL;
	m_pLut3DTexture = NULL;
	m_pFinalPixelShader = NULL;
}

HRESULT CDX9RenderingEngine::CreateIccProfileLut(TCHAR* profilePath, float* lut3D)
{
	// Get the input video system
	VideoSystem videoSystem;

	if (m_InputVideoSystem == VIDEO_SYSTEM_UNKNOWN) {
		static const int ntscSizes[][2] = {{720, 480}, {720, 486}, {704, 480}};
		static const int palSizes[][2] = {{720, 576}, {704, 576}};

		videoSystem = VIDEO_SYSTEM_HDTV; // default

		for (int i = 0; i < _countof(ntscSizes); i++) {
			if (m_nativeVideoSize.cx == ntscSizes[i][0] && m_nativeVideoSize.cy == ntscSizes[i][1]) {
				videoSystem = VIDEO_SYSTEM_SDTV_NTSC;
			}
		}

		for (int i = 0; i < _countof(palSizes); i++) {
			if (m_nativeVideoSize.cx == palSizes[i][0] && m_nativeVideoSize.cy == palSizes[i][1]) {
				videoSystem = VIDEO_SYSTEM_SDTV_PAL;
			}
		}
	} else {
		videoSystem = m_InputVideoSystem;
	}

	// Get the gamma
	double gamma;

	switch (m_AmbientLight) {
		case AMBIENT_LIGHT_BRIGHT:
			gamma = 2.2;
			break;

		case AMBIENT_LIGHT_DIM:
			gamma = 2.35;
			break;

		case AMBIENT_LIGHT_DARK:
			gamma = 2.4;
			break;

		default:
			return E_FAIL;
	}

	// Get the rendering intent
	cmsUInt32Number intent;

	switch (m_RenderingIntent) {
		case COLOR_RENDERING_INTENT_PERCEPTUAL:
			intent = INTENT_PERCEPTUAL;
			break;

		case COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
			intent = INTENT_RELATIVE_COLORIMETRIC;
			break;

		case COLOR_RENDERING_INTENT_SATURATION:
			intent = INTENT_SATURATION;
			break;

		case COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
			intent = INTENT_ABSOLUTE_COLORIMETRIC;
			break;

		default:
			return E_FAIL;
	}

	// Set the input white point. It's D65 in all cases.
	cmsCIExyY whitePoint;

	whitePoint.x = 0.312713;
	whitePoint.y = 0.329016;
	whitePoint.Y = 1.0;

	// Set the input primaries
	cmsCIExyYTRIPLE primaries;

	switch (videoSystem) {
		case VIDEO_SYSTEM_HDTV:
			// Rec. 709
			primaries.Red.x   = 0.64;
			primaries.Red.y   = 0.33;
			primaries.Green.x = 0.30;
			primaries.Green.y = 0.60;
			primaries.Blue.x  = 0.15;
			primaries.Blue.y  = 0.06;
			break;

		case VIDEO_SYSTEM_SDTV_NTSC:
			// SMPTE-C
			primaries.Red.x   = 0.630;
			primaries.Red.y   = 0.340;
			primaries.Green.x = 0.310;
			primaries.Green.y = 0.595;
			primaries.Blue.x  = 0.155;
			primaries.Blue.y  = 0.070;
			break;

		case VIDEO_SYSTEM_SDTV_PAL:
			// PAL/SECAM
			primaries.Red.x   = 0.64;
			primaries.Red.y   = 0.33;
			primaries.Green.x = 0.29;
			primaries.Green.y = 0.60;
			primaries.Blue.x  = 0.15;
			primaries.Blue.y  = 0.06;
			break;

		default:
			return E_FAIL;
	}

	primaries.Red.Y   = 1.0;
	primaries.Green.Y = 1.0;
	primaries.Blue.Y  = 1.0;

	// Set the input gamma, which is the gamma of a reference studio display we want to simulate
	// For more information, see the paper at http://www.poynton.com/notes/PU-PR-IS/Poynton-PU-PR-IS.pdf
	cmsToneCurve* transferFunction = cmsBuildGamma(0, gamma);

	cmsToneCurve* transferFunctionRGB[3];
	for (int i = 0; i < 3; i++) {
		transferFunctionRGB[i] = transferFunction;
	}

	// Create the input profile
	cmsHPROFILE hInputProfile = cmsCreateRGBProfile(&whitePoint, &primaries, transferFunctionRGB);
	cmsFreeToneCurve(transferFunction);

	if (hInputProfile == NULL) {
		return E_FAIL;
	}

	// Open the output profile
	cmsHPROFILE hOutputProfile;
	FILE* outputProfileStream;

	if (profilePath != 0) {
		if (_wfopen_s(&outputProfileStream, T2W(profilePath), L"rb") != 0) {
			cmsCloseProfile(hInputProfile);
			return E_FAIL;
		}

		hOutputProfile = cmsOpenProfileFromStream(outputProfileStream, "r");
	} else {
		hOutputProfile = cmsCreate_sRGBProfile();
	}

	if (hOutputProfile == NULL) {
		if (profilePath != 0) {
			fclose(outputProfileStream);
		}

		cmsCloseProfile(hInputProfile);
		return E_FAIL;
	}

	// Create the transform
	cmsHTRANSFORM hTransform = cmsCreateTransform(hInputProfile, TYPE_RGB_16, hOutputProfile, TYPE_RGB_16, intent, cmsFLAGS_HIGHRESPRECALC);

	cmsCloseProfile(hOutputProfile);

	if (profilePath != 0) {
		fclose(outputProfileStream);
	}

	cmsCloseProfile(hInputProfile);

	if (hTransform == NULL) {
		return E_FAIL;
	}

	// Create the 3D LUT input
	unsigned short* lut3DOutput = DNew unsigned short[m_Lut3DEntryCount * 3];
	unsigned short* lut3DInput  = DNew unsigned short[m_Lut3DEntryCount * 3];

	unsigned short* lut3DInputIterator = lut3DInput;

	for (int b = 0; b < m_Lut3DSize; b++) {
		for (int g = 0; g < m_Lut3DSize; g++) {
			for (int r = 0; r < m_Lut3DSize; r++) {
				*lut3DInputIterator++ = r * 65535 / (m_Lut3DSize - 1);
				*lut3DInputIterator++ = g * 65535 / (m_Lut3DSize - 1);
				*lut3DInputIterator++ = b * 65535 / (m_Lut3DSize - 1);
			}
		}
	}

	// Do the transform
	cmsDoTransform(hTransform, lut3DInput, lut3DOutput, m_Lut3DEntryCount);

	// Convert the output to floating point
	for (int i = 0; i < m_Lut3DEntryCount * 3; i++) {
		lut3D[i] = static_cast<float>(lut3DOutput[i]) * (1.0f / 65535.0f);
	}

	// Cleanup
	delete[] lut3DOutput;
	delete[] lut3DInput;
	cmsDeleteTransform(hTransform);

	return S_OK;
}

HRESULT CDX9RenderingEngine::FinalPass(IDirect3DTexture9* pTexture)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	MYD3DVERTEX<1> v[] = {
		{0, 0, 0.5f, 2.0f, 0, 0},
		{w, 0, 0.5f, 2.0f, 1, 0},
		{0, h, 0.5f, 2.0f, 0, 1},
		{w, h, 0.5f, 2.0f, 1, 1},
	};

	for (int i = 0; i < _countof(v); i++) {
		v[i].x -= 0.5f;
		v[i].y -= 0.5f;
	}

	hr = m_pD3DDev->SetPixelShader(m_pFinalPixelShader);

	// Set sampler: image
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	hr = m_pD3DDev->SetTexture(0, pTexture);

	// Set sampler: ditherMap
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

	hr = m_pD3DDev->SetTexture(1, m_pDitherTexture);

	if (m_bColorManagement) {
		hr = m_pD3DDev->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		hr = m_pD3DDev->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		hr = m_pD3DDev->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = m_pD3DDev->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = m_pD3DDev->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		hr = m_pD3DDev->SetSamplerState(2, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

		hr = m_pD3DDev->SetTexture(2, m_pLut3DTexture);
	}

	// Set constants
	float fConstData[][4] = {{(float)w / DITHER_MATRIX_SIZE, (float)h / DITHER_MATRIX_SIZE, 0, 0}};
	hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));

	hr = TextureBlt(m_pD3DDev, v, D3DTEXF_POINT);

	hr = m_pD3DDev->SetTexture(1, NULL);

	if (m_bColorManagement) {
		hr = m_pD3DDev->SetTexture(2, NULL);
	}

	return hr;
}

HRESULT CDX9RenderingEngine::TextureCopy(IDirect3DTexture9* pTexture)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float w = (float)desc.Width - 0.5f;
	float h = (float)desc.Height - 0.5f;

	MYD3DVERTEX<1> v[] = {
		{-0.5f, -0.5f, 0.5f, 2.0f, 0, 0},
		{    w, -0.5f, 0.5f, 2.0f, 1, 0},
		{-0.5f,     h, 0.5f, 2.0f, 0, 1},
		{    w,     h, 0.5f, 2.0f, 1, 1},
	};

	hr = m_pD3DDev->SetTexture(0, pTexture);

	return TextureBlt(m_pD3DDev, v, D3DTEXF_POINT);
}

bool CDX9RenderingEngine::ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d)
{
	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
	if (FAILED(pSurface->GetDesc(&d3dsd))) {
		return false;
	}

	int w = d3dsd.Width, h = d3dsd.Height;
	int sw = s.Width(), sh = s.Height();
	int dw = d.Width(), dh = d.Height();

	if (d.left >= w || d.right < 0 || d.top >= h || d.bottom < 0
			|| sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) {
		s.SetRectEmpty();
		d.SetRectEmpty();
		return true;
	}

	if (d.right > w) {
		s.right -= (d.right-w)*sw/dw;
		d.right = w;
	}
	if (d.bottom > h) {
		s.bottom -= (d.bottom-h)*sh/dh;
		d.bottom = h;
	}
	if (d.left < 0) {
		s.left += (0-d.left)*sw/dw;
		d.left = 0;
	}
	if (d.top < 0) {
		s.top += (0-d.top)*sh/dh;
		d.top = 0;
	}

	return true;
}

HRESULT CDX9RenderingEngine::DrawRect(DWORD _Color, DWORD _Alpha, const CRect &_Rect)
{
	if (!m_pD3DDev) {
		return E_POINTER;
	}

	DWORD Color = D3DCOLOR_ARGB(_Alpha, GetRValue(_Color), GetGValue(_Color), GetBValue(_Color));
	MYD3DVERTEX<0> v[] = {
		{float(_Rect.left), float(_Rect.top), 0.5f, 2.0f, Color},
		{float(_Rect.right), float(_Rect.top), 0.5f, 2.0f, Color},
		{float(_Rect.left), float(_Rect.bottom), 0.5f, 2.0f, Color},
		{float(_Rect.right), float(_Rect.bottom), 0.5f, 2.0f, Color},
	};

	for (int i = 0; i < _countof(v); i++) {
		v[i].x -= 0.5f;
		v[i].y -= 0.5f;
	}

	HRESULT hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	//D3DRS_COLORVERTEX
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);


	hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

	hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX0 | D3DFVF_DIFFUSE);
	// hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));

	MYD3DVERTEX<0> tmp = v[2];
	v[2] = v[3];
	v[3] = tmp;
	hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

	return S_OK;
}

HRESULT CDX9RenderingEngine::AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture)
{
	if (!pSrc || !pDst) {
		return E_POINTER;
	}

	CRect src(*pSrc), dst(*pDst);

	HRESULT hr;

	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
	if (FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/) {
		return E_FAIL;
	}

	float w = (float)d3dsd.Width;
	float h = (float)d3dsd.Height;

	struct {
		float x, y, z, rhw;
		float tu, tv;
	}
	pVertices[] = {
		{(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / w, (float)src.top / h},
		{(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / w, (float)src.top / h},
		{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w, (float)src.bottom / h},
		{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h},
	};

	for (int i = 0; i < _countof(pVertices); i++) {
		pVertices[i].x -= 0.5f;
		pVertices[i].y -= 0.5f;
	}

	hr = m_pD3DDev->SetTexture(0, pTexture);

	// GetRenderState fails for devices created with D3DCREATE_PUREDEVICE
	// so we need to provide default values in case GetRenderState fails
	DWORD abe, sb, db;
	if (FAILED(m_pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe)))
		abe = FALSE;
	if (FAILED(m_pD3DDev->GetRenderState(D3DRS_SRCBLEND, &sb)))
		sb = D3DBLEND_ONE;
	if (FAILED(m_pD3DDev->GetRenderState(D3DRS_DESTBLEND, &db)))
		db = D3DBLEND_ZERO;

	hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
	hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

	hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	/*//

	D3DCAPS9 d3dcaps9;
	hr = m_pD3DDev->GetDeviceCaps(&d3dcaps9);
	if (d3dcaps9.AlphaCmpCaps & D3DPCMPCAPS_LESS)
	{
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)0x000000FE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DPCMPCAPS_LESS);
	}

	*///

	hr = m_pD3DDev->SetPixelShader(NULL);

	hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
	hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

	//

	m_pD3DDev->SetTexture(0, NULL);

	m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
	m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, sb);
	m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, db);

	return S_OK;
}

HRESULT CDX9RenderingEngine::SetCustomPixelShader(LPCSTR pSrcData, LPCSTR pTarget, bool bScreenSpace)
{
	CAtlList<CExternalPixelShader> *pPixelShaders;
	if (bScreenSpace) {
		pPixelShaders = &m_pCustomScreenSpacePixelShaders;
	} else {
		pPixelShaders = &m_pCustomPixelShaders;
	}

	if (!pSrcData && !pTarget) {
		pPixelShaders->RemoveAll();
		m_pD3DDev->SetPixelShader(NULL);
		return S_OK;
	}

	if (!pSrcData || !pTarget) {
		return E_INVALIDARG;
	}

	CExternalPixelShader Shader;
	Shader.m_SourceData = pSrcData;
	Shader.m_SourceTarget = m_ShaderProfile;

	if (Shader.m_SourceTarget.Compare(pTarget) < 0) {
		// shader is not supported by hardware
		return E_FAIL;
	}

	CComPtr<IDirect3DPixelShader9> pPixelShader;

	HRESULT hr = Shader.Compile(m_pPSC);
	if (FAILED(hr)) {
		return hr;
	}

	pPixelShaders->AddTail(Shader);

	Paint(false);

	return S_OK;
}
