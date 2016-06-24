/*
 * (C) 2012-2016 see Authors.txt
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

#include "DIB.h"
#include "../../DSUtil/Filehandle.h"

static LPCTSTR extimages[] = { L".bmp", L".jpg", L".jpeg", L".png", L".gif" };

static const bool OpenImageCheck(CString fn)
{
	CString ext = GetFileExt(fn).MakeLower();
	if (!ext.IsEmpty()) {
		for (size_t i = 0; i < _countof(extimages); i++) {
			if (ext == extimages[i]) {
				return true;
			}
		}
	}

	return false;
}

static HBITMAP OpenImage(CString fn)
{
	HBITMAP hB = NULL;
	if (OpenImageCheck(fn)) {
		FILE *fp = _tfopen(fn, _T("rb"));
		if (!fp) {
			return NULL;
		}
		
		fseek(fp, 0, SEEK_END);
		size_t fs = ftell(fp);
		rewind(fp);

		HGLOBAL hG = ::GlobalAlloc(GMEM_MOVEABLE, fs);
		BYTE* lpBits = (BYTE*)::GlobalLock(hG);
		fread(lpBits, fs, 1, fp);

		fclose(fp);

		IStream* s;
		::CreateStreamOnHGlobal(hG, 1, &s);

		ULONG_PTR gdiplusToken;
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);

		Bitmap *bm = new Bitmap(s);

		bm->GetHBITMAP(0, &hB);

		delete bm;
		Gdiplus::GdiplusShutdown(gdiplusToken);

		s->Release();
		::GlobalUnlock(hG);
		::GlobalFree(hG);
	}

	return hB;
}