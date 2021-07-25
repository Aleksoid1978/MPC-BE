/*
 * (C) 2019-2021 see Authors.txt
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
#include <DirectXMath.h>
#include "FontBitmap.h"
#include "D3D9Font.h"

#define MAX_NUM_VERTICES 400*6

struct Font9Vertex {
	DirectX::XMFLOAT4 pos;
	DWORD color;
	DirectX::XMFLOAT2 tex;
};

inline auto Char2Index(WCHAR ch)
{
	if (ch >= 0x0020 && ch <= 0x007F) {
		return ch - 32;
	}
	if (ch >= 0x00A0 && ch <= 0x00BF) {
		return ch - 64;
	}
	return 0x007F - 32;
}

// CD3D9Font

// Font class constructor
CD3D9Font::CD3D9Font()
{
	UINT idx = 0;
	for (WCHAR ch = 0x0020; ch < 0x007F; ch++) {
		m_Characters[idx++] = ch;
	}
	m_Characters[idx++] = 0x25CA; // U+25CA Lozenge
	for (WCHAR ch = 0x00A0; ch <= 0x00BF; ch++) {
		m_Characters[idx++] = ch;
	}
	ASSERT(idx == std::size(m_Characters));
}

// Font class destructor
CD3D9Font::~CD3D9Font()
{
	InvalidateDeviceObjects();
}

// Initializes device-dependent objects, including the vertex buffer used
// for rendering text and the texture map which stores the font image.
HRESULT CD3D9Font::InitDeviceObjects(IDirect3DDevice9* pd3dDevice)
{
	HRESULT hr = S_OK;

	InvalidateDeviceObjects();

	// Keep a local copy of the device
	m_pd3dDevice = pd3dDevice;
	m_pd3dDevice->AddRef();

	m_pd3dDevice->GetDeviceCaps(&m_D3DCaps);

	// Create vertex buffer for the letters
	const UINT vertexBufferSize = sizeof(Font9Vertex) * MAX_NUM_VERTICES;
	hr = m_pd3dDevice->CreateVertexBuffer(vertexBufferSize, D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &m_pVertexBuffer, nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = CreateStateBlocks();

	return hr;
}

// TODO: need a description
HRESULT CD3D9Font::CreateStateBlocks()
{
	// Create the state blocks for rendering text
	for (UINT i = 0; i < 2; i++) {
		m_pd3dDevice->BeginStateBlock();

		m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); // not used
		m_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 0);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
		m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		m_pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_CLIPPING, TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
		m_pd3dDevice->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE,
									D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
									D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		m_pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		m_pd3dDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		m_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		m_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		m_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		if (i == 0) {
			m_pd3dDevice->EndStateBlock(&m_pStateBlockSaved);
		} else {
			m_pd3dDevice->EndStateBlock(&m_pStateBlockDrawText);
		}
	}

	return S_OK;
}

// Destroys all device-dependent objects
void CD3D9Font::InvalidateDeviceObjects()
{
	SAFE_RELEASE(m_pStateBlockSaved);
	SAFE_RELEASE(m_pStateBlockDrawText);

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pTexture);

	SAFE_RELEASE(m_pd3dDevice);
}

HRESULT CD3D9Font::CreateFontBitmap(const WCHAR* strFontName, const UINT fontHeight, const UINT fontWidth, const UINT fontFlags)
{
	if (!m_pd3dDevice) {
		return E_ABORT;
	}

	if (m_pTexture
			&& fontHeight == m_fontHeight
			&& fontWidth == m_fontWidth
			&& fontFlags == m_fontFlags
			&& m_strFontName.compare(strFontName) == 0) {
		return S_FALSE;
	}

	m_strFontName = strFontName;
	m_fontHeight  = fontHeight;
	m_fontWidth   = fontWidth;
	m_fontFlags   = fontFlags;

	CFontBitmap fontBitmap;

	HRESULT hr = fontBitmap.Initialize(m_strFontName.c_str(), m_fontHeight, m_fontWidth, m_fontFlags, m_Characters, std::size(m_Characters));
	if (FAILED(hr)) {
		return hr;
	}

	m_MaxCharMetric = fontBitmap.GetMaxCharMetric();
	m_uTexWidth     = fontBitmap.GetWidth();
	m_uTexHeight    = fontBitmap.GetHeight();
	if (m_uTexWidth > m_D3DCaps.MaxTextureWidth || m_uTexHeight > m_D3DCaps.MaxTextureHeight) {
		return E_FAIL;
	}
	EXECUTE_ASSERT(S_OK == fontBitmap.GetFloatCoords((FloatRect*)&m_fTexCoords, std::size(m_Characters)));

	SAFE_RELEASE(m_pTexture);

	// Create a new texture for the font
	hr = m_pd3dDevice->CreateTexture(m_uTexWidth, m_uTexHeight, 1,
		D3DUSAGE_DYNAMIC, D3DFMT_A8L8,
		D3DPOOL_DEFAULT, &m_pTexture, nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	// Lock the surface and write the alpha values for the set pixels
	D3DLOCKED_RECT d3dlr;
	hr = m_pTexture->LockRect(0, &d3dlr, nullptr, D3DLOCK_DISCARD);
	if (S_OK == hr) {
		BYTE* pSrc = nullptr;
		UINT uStride = 0;
		hr = fontBitmap.Lock(&pSrc, uStride);
		if (S_OK == hr) {
			BYTE* pDst = (BYTE*)d3dlr.pBits;
			for (UINT y = 0; y < m_uTexHeight; y++) {
				uint32_t* pSrc32 = (uint32_t*)pSrc;
				uint16_t* pDst16 = (uint16_t*)pDst;

				for (UINT x = 0; x < m_uTexWidth; x++) {
					*pDst16++ = A8R8G8B8toA8L8(*pSrc32++);
				}
				pSrc += uStride;
				pDst += d3dlr.Pitch;
			}
			fontBitmap.Unlock();
		}
		m_pTexture->UnlockRect(0);
	}

	return hr;
}

SIZE CD3D9Font::GetMaxCharMetric()
{
	return m_MaxCharMetric;
}

// Get the dimensions of a text string
HRESULT CD3D9Font::GetTextExtent(const WCHAR* strText, SIZE* pSize)
{
	if (nullptr == strText || nullptr == pSize) {
		return E_POINTER;
	}

	float fRowWidth  = 0.0f;
	float fRowHeight = (m_fTexCoords[0].bottom - m_fTexCoords[0].top)*m_uTexHeight;
	float fWidth     = 0.0f;
	float fHeight    = fRowHeight;

	while (*strText) {
		WCHAR c = *strText++;

		if (c == '\n') {
			fRowWidth = 0.0f;
			fHeight  += fRowHeight;
			continue;
		}

		const auto idx = Char2Index(c);

		const float tx1 = m_fTexCoords[idx].left;
		const float tx2 = m_fTexCoords[idx].right;

		fRowWidth += (tx2 - tx1)*m_uTexWidth;

		if (fRowWidth > fWidth) {
			fWidth = fRowWidth;
		}
	}

	pSize->cx = (LONG)fWidth;
	pSize->cy = (LONG)fHeight;

	return S_OK;
}

// Draws 2D text. Note that sx and sy are in pixels
HRESULT CD3D9Font::Draw2DText(float sx, float sy, const D3DCOLOR color, const WCHAR* strText, const UINT flags)
{
	if (m_pd3dDevice == nullptr) {
		return E_ABORT;
	}

	// Setup renderstate
	m_pStateBlockSaved->Capture();
	m_pStateBlockDrawText->Apply();

	m_pd3dDevice->SetTexture(0, m_pTexture);
	m_pd3dDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
	m_pd3dDevice->SetPixelShader(nullptr);
	m_pd3dDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(Font9Vertex));

	const float fLineHeight = (m_fTexCoords[0].bottom - m_fTexCoords[0].top)*m_uTexHeight;

	// Center the text block in the viewport
	if (flags & D3DFONT_CENTERED_X) {
		D3DVIEWPORT9 vp;
		m_pd3dDevice->GetViewport(&vp);
		const WCHAR* strTextTmp = strText;
		float xFinal = 0.0f;

		while (*strTextTmp) {
			WCHAR c = *strTextTmp++;

			if (c == '\n') {
				break;    // Isn't supported.
			}

			const auto idx = Char2Index(c);

			const float tx1 = m_fTexCoords[idx].left;
			const float tx2 = m_fTexCoords[idx].right;

			const float w = (tx2 - tx1) *  m_uTexWidth;

			xFinal += w;
		}

		sx = (vp.Width - xFinal) / 2.0f;
	}
	if (flags & D3DFONT_CENTERED_Y) {
		D3DVIEWPORT9 vp;
		m_pd3dDevice->GetViewport(&vp);
		sy = (vp.Height - fLineHeight) / 2;
	}

	// Adjust for character spacing
	const float fStartX = sx;

	// Fill vertex buffer
	Font9Vertex* pVertices = nullptr;
	UINT uNumTriangles = 0;
	m_pVertexBuffer->Lock(0, 0, (void**)&pVertices, D3DLOCK_DISCARD);

	while (*strText) {
		const WCHAR c = *strText++;

		if (c == '\n') {
			sx = fStartX;
			sy += fLineHeight;
			continue;
		}

		const auto tex = m_fTexCoords[Char2Index(c)];

		const float w = (tex.right - tex.left) * m_uTexWidth;
		const float h = (tex.bottom - tex.top) * m_uTexHeight;

		if (c != 0x0020 && c != 0x00A0) { // Space and No-Break Space
			const float left   = sx - 0.5f;
			const float right  = sx + w - 0.5f;
			const float top    = sy - 0.5f;
			const float bottom = sy + h - 0.5f;

			*pVertices++ = { {left , bottom, 0.9f, 1.0f}, color, {tex.left,  tex.bottom} };
			*pVertices++ = { {left , top,    0.9f, 1.0f}, color, {tex.left,  tex.top}    };
			*pVertices++ = { {right, bottom, 0.9f, 1.0f}, color, {tex.right, tex.bottom} };
			*pVertices++ = { {right, top,    0.9f, 1.0f}, color, {tex.right, tex.top}    };
			*pVertices++ = { {right, bottom, 0.9f, 1.0f}, color, {tex.right, tex.bottom} };
			*pVertices++ = { {left , top,    0.9f, 1.0f}, color, {tex.left,  tex.top}    };
			uNumTriangles += 2;

			if (uNumTriangles*3 > (MAX_NUM_VERTICES-6)) {
				// Unlock, render, and relock the vertex buffer
				m_pVertexBuffer->Unlock();
				m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, uNumTriangles);
				pVertices = nullptr;
				m_pVertexBuffer->Lock(0, 0, (void**)&pVertices, D3DLOCK_DISCARD);
				uNumTriangles = 0;
			}
		}

		sx += w;
	}

	// Unlock and render the vertex buffer
	m_pVertexBuffer->Unlock();
	if (uNumTriangles > 0) {
		m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, uNumTriangles);
	}

	m_pd3dDevice->SetTexture(0, nullptr);

	// Restore the modified renderstates
	m_pStateBlockSaved->Apply();

	return S_OK;
}
