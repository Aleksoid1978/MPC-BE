/*
 * (C) 2003-2006 Gabest
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

#include <atlstr.h>
#include <D3Dcompiler.h>
#include <dx/d3dx9.h>

class CPixelShaderCompiler
{
	typedef HRESULT(WINAPI* D3DCompilePtr)(
		__in_bcount(SrcDataSize) LPCVOID pSrcData,
		__in SIZE_T SrcDataSize,
		__in_opt LPCSTR pSourceName,
		__in_xcount_opt(pDefines->Name != NULL) CONST D3D_SHADER_MACRO* pDefines,
		__in_opt ID3DInclude* pInclude,
		__in LPCSTR pEntrypoint,
		__in LPCSTR pTarget,
		__in UINT Flags1,
		__in UINT Flags2,
		__out ID3DBlob** ppCode,
		__out_opt ID3DBlob** ppErrorMsgs
	);
	typedef HRESULT(WINAPI* D3DDisassemblePtr)(
		__in_bcount(SrcDataSize) LPCVOID pSrcData,
		__in SIZE_T SrcDataSize,
		__in UINT Flags,
		__in_opt LPCSTR szComments,
		__out ID3DBlob** ppDisassembly
	);
	// warning: the constructor function initializes these pointers as a sorted array
	D3DCompilePtr m_fnD3DCompile;
	D3DDisassemblePtr m_fnD3DDisassemble;

	CComPtr<IDirect3DDevice9> m_pD3DDev;

public:
	CPixelShaderCompiler(IDirect3DDevice9* pD3DDev, bool fStaySilent = false);
	virtual ~CPixelShaderCompiler();

	HRESULT CompileShader(
		LPCSTR pSrcData,
		LPCSTR pFunctionName,
		LPCSTR pProfile,
		DWORD Flags,
		const D3D_SHADER_MACRO* pDefines,
		IDirect3DPixelShader9** ppPixelShader,
		CString* errmsg = NULL,
		CString* disasm = NULL);
};
