/*
 * Copyright (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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

#include <afxinet.h>
#include "DIB.h"

using namespace Gdiplus;

static TCHAR* extimages[] = { L".bmp", L".jpg", L".jpeg", L".png", L".gif" };

static bool OpenImageCheck(CString fn)
{
	CString ext = CPath(fn).GetExtension().MakeLower();
	if (ext.IsEmpty()) {
		return false;
	}

	for (size_t i = 0; i < _countof(extimages); i++) {
		if (ext == extimages[i]) {
			return true;
		}
	}

	return false;
}

static HBITMAP SaveImageDIB(CString out, ULONG quality, bool mode, BYTE* pBuf, size_t pSize)
{
	HBITMAP hB = NULL;

	HGLOBAL hG = ::GlobalAlloc(GMEM_MOVEABLE, pSize);
	BYTE* lpBits = (BYTE*)::GlobalLock(hG);
	memcpy(lpBits, pBuf, pSize);

	IStream *s;
	::CreateStreamOnHGlobal(hG, 1, &s);

	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	Bitmap *bm = new Bitmap(s);

	if (mode) {
		bm->GetHBITMAP(0, &hB);
	} else {
		CString format;
		CString ext = CPath(out).GetExtension().MakeLower();

		if (ext == L".bmp") {
			format = L"image/bmp";
		} else if (ext == L".jpg") {
			format = L"image/jpeg";
		} else if (ext == L".png") {
			format = L"image/png";
		}
		GdiplusConvert(bm, out, format, quality, 0, NULL, 0);
	}

	delete bm;
	GdiplusShutdown(gdiplusToken);

	s->Release();
	::GlobalUnlock(hG);
	::GlobalFree(hG);

	return hB;
}

static HBITMAP OpenImageDIB(CString fn, CString out, ULONG quality, bool mode)
{
	if (OpenImageCheck(fn)) {
		CString ext = CPath(fn).GetExtension().MakeLower();

		HBITMAP hB = NULL;
		FILE *fp;
		TCHAR path_fn[_MAX_PATH];
		int type = 0, sih = sizeof(BITMAPINFOHEADER);

		if (wcsstr(fn, L"://")) {
			HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);
			if (s) {
				f = InternetOpenUrl(s, fn, 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
				if (f) {
					type = 1;

					DWORD len;
					char buf[8192];
					TCHAR path[_MAX_PATH];

					GetTempPath(_MAX_PATH, path);
					GetTempFileName(path, _T("mpc_image"), 0, path_fn);
					fp = _tfopen(path_fn, _T("wb+"));

					for (;;) {
						InternetReadFile(f, buf, sizeof(buf), &len);

						if (!len) {
							break;
						}

						fwrite(buf, len, 1, fp);
					}
					InternetCloseHandle(f);
				}
				InternetCloseHandle(s);
				if (!f) {
					return NULL;
				}
			} else {
				return NULL;
			}
		} else {
			fp = _tfopen(fn, _T("rb"));
			if (!fp) {
				return NULL;
			}
			fseek(fp, 0, SEEK_END);
		}

		DWORD fs = ftell(fp);
		rewind(fp);
		BYTE *data = (BYTE*)malloc(fs);
		fread(data, fs, 1, fp);
		fclose(fp);

		if (type) {
			_tunlink(path_fn);
		}

		BITMAPFILEHEADER bfh;
		if (!mode) {
			bfh.bfType = 0x4d42;
			bfh.bfOffBits = sizeof(bfh) + sih;
			bfh.bfReserved1 = bfh.bfReserved2 = 0;
		}

		hB = SaveImageDIB(out, quality, mode, data, fs);

		free(data);

		return hB;
	}

	return NULL;
}

static HBITMAP OpenImage(CString fn)
{
	return OpenImageDIB(fn, L"", 0, 1);
}
