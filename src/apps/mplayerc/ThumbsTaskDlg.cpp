/*
 * (C) 2016 see Authors.txt
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

#include <afxwin.h>
#include "MainFrm.h"
#include "../../DSUtil/Filehandle.h"
#include "SmoothImageResampling.h"

#include "ThumbsTaskDlg.h"

enum {
	PROGRESS_E_VIDFMT	= -5,
	PROGRESS_E_VIDSIZE	= -4,
	PROGRESS_E_MEMORY	= -3,
	PROGRESS_E_FAIL		= -2,
	PROGRESS_E_WAIT		= -1,
	PROGRESS_COMPLETED	= INT16_MAX,
};

// CThumbsTaskDlg dialog

void CThumbsTaskDlg::SaveThumbnails(LPCTSTR thumbpath)
{
	m_iProgress = 0;

	if (!thumbpath
			|| !(m_pMainFrm->m_pMS)
			|| !(m_pMainFrm->m_pFS)) {
		m_iProgress = PROGRESS_E_FAIL;
		return;
	}

	// get frame size and aspect ratio
	CSize framesize, dar;
	if (m_pMainFrm->m_pCAP) {
		framesize = m_pMainFrm->m_pCAP->GetVideoSize();
		dar = m_pMainFrm->m_pCAP->GetVideoSizeAR();
	}
	else if (m_pMainFrm->m_pMFVDC) {
		m_pMainFrm->m_pMFVDC->GetNativeVideoSize(&framesize, &dar);
	}
	else if (m_pMainFrm->m_pBV) {
		m_pMainFrm->m_pBV->GetVideoSize(&framesize.cx, &framesize.cy);
		long arx = 0, ary = 0;
		CComQIPtr<IBasicVideo2> pBV2 = m_pMainFrm->m_pBV;
		if (pBV2 && SUCCEEDED(pBV2->GetPreferredAspectRatio(&arx, &ary)) && arx > 0 && ary > 0) {
			dar.SetSize(arx, ary);
		}
	}
	else {
		m_iProgress = PROGRESS_E_VIDSIZE;
		return;
	}

	// with the overlay mixer IBasicVideo2 won't tell the new AR when changed dynamically
	DVD_VideoAttributes VATR;
	if (m_pMainFrm->GetPlaybackMode() == PM_DVD && SUCCEEDED(m_pMainFrm->m_pDVDI->GetCurrentVideoAttributes(&VATR))) {
		dar.SetSize(VATR.ulAspectX, VATR.ulAspectY);
	}

	// get duration
	REFERENCE_TIME duration = 0;
	m_pMainFrm->m_pMS->GetDuration(&duration);
	if (duration <= 0) {
		m_iProgress = PROGRESS_E_FAIL;
		return;
	}

	const CAppSettings& s = AfxGetAppSettings();

	const int infoheight = 70;
	const int margin = 10;

	const int width = clamp(s.iThumbWidth, 256, 2560);
	const int cols  = clamp(s.iThumbCols, 1, 10);
	const int rows  = clamp(s.iThumbRows, 1, 20);

	CSize thumbsize;
	thumbsize.cx = (width - margin) / cols - margin;
	thumbsize.cy = MulDiv(thumbsize.cx, framesize.cy, framesize.cx);

	const int height = infoheight + margin + (thumbsize.cy + margin) * rows;
	const int dibsize = sizeof(BITMAPINFOHEADER) + width * height * 4;

	std::unique_ptr<BYTE[]> dib(new(std::nothrow) BYTE[dibsize]);
	if (!dib) {
		m_iProgress = PROGRESS_E_MEMORY;
		return;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)dib.get();
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize           = sizeof(BITMAPINFOHEADER);
	bih->biWidth          = width;
	bih->biHeight         = height;
	bih->biPlanes         = 1;
	bih->biBitCount       = 32;
	bih->biCompression    = BI_RGB;
	bih->biSizeImage      = DIBSIZE(*bih);
	memsetd(bih + 1, 0xffffff, bih->biSizeImage);

	SubPicDesc spd;
	spd.w       = width;
	spd.h       = height;
	spd.bpp     = 32;
	spd.pitch   = -width * 4;
	spd.vidrect = CRect(0, 0, width, height);
	spd.bits    = (BYTE*)(bih + 1) + (width * 4) * (height - 1);

	{
		BYTE* p = (BYTE*)spd.bits;
		for (int y = 0; y < spd.h; y++, p += spd.pitch) {
			for (int x = 0; x < spd.w; x++) {
				((DWORD*)p)[x] = 0x010101 * (0xe0 + 0x08 * y / spd.h + 0x18 * (spd.w - x) / spd.w);
			}
		}
	}

	std::unique_ptr<BYTE[]> thumb(new(std::nothrow) BYTE[thumbsize.cx * thumbsize.cy * 4]);
	if (!thumb) {
		m_iProgress = PROGRESS_E_MEMORY;
		return;
	}

	m_iProgress = 1; // prepared DIB
	if (m_bAbort) {
		return;
	}

	CCritSec csSubLock;
	RECT bbox;

	for (int i = 1, pics = cols * rows; i <= pics; i++) {
		const REFERENCE_TIME rt = duration * i / (pics + 1);
		const DVD_HMSF_TIMECODE hmsf = RT2HMS_r(rt);

		m_pMainFrm->SeekTo(rt, false);

		m_pMainFrm->m_fFrameSteppingActive = true;
		// Number of steps you need to do more than one for some decoders.
		// TODO - maybe need to find another way to get correct frame ???
		HRESULT hr = m_pMainFrm->m_pFS->Step(2, NULL);
		while (m_pMainFrm->m_fFrameSteppingActive) {
			if (m_bAbort) {
				return;
			}
			Sleep(50);
		}

		const int col = (i - 1) % cols;
		const int row = (i - 1) / cols;

		const CPoint p(margin + col * (thumbsize.cx + margin), infoheight + margin + row * (thumbsize.cy + margin));
		const CRect r(p, thumbsize);

		CRenderedTextSubtitle rts(&csSubLock);
		rts.CreateDefaultStyle(0);
		rts.m_dstScreenSize.SetSize(width, height);
		STSStyle* style = DNew STSStyle();
		style->marginRect.SetRectEmpty();
		rts.AddStyle(L"thumbs", style);

		CString str;
		str.Format(L"{\\an7\\1c&Hffffff&\\4a&Hb0&\\bord1\\shad4\\be1}{\\p1}m %d %d l %d %d %d %d %d %d{\\p}",
				   r.left, r.top, r.right, r.top, r.right, r.bottom, r.left, r.bottom);
		rts.Add(str, true, 0, 1, L"thumbs");
		str.Format(L"{\\an3\\1c&Hffffff&\\3c&H000000&\\alpha&H80&\\fs16\\b1\\bord2\\shad0\\pos(%d,%d)}%02d:%02d:%02d",
				   r.right - 5, r.bottom - 3, hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
		rts.Add(str, true, 1, 2, L"thumbs");

		rts.Render(spd, 0, 25, bbox);

		BYTE* pData = NULL;
		long size = 0;
		if (!m_pMainFrm->GetDIB(&pData, size)) {
			m_iProgress = PROGRESS_E_FAIL;
			return;
		}

		const BITMAPINFO* bi = (BITMAPINFO*)pData;
		if (bi->bmiHeader.biBitCount != 32) {
			m_ErrorMsg.Format(ResStr(IDS_MAINFRM_57), bi->bmiHeader.biBitCount);
			delete [] pData;

			m_iProgress = PROGRESS_E_VIDFMT;
			return;
		}

		Resize_HQ_4ch((const BYTE*)(&bi->bmiHeader + 1), bi->bmiHeader.biWidth, abs(bi->bmiHeader.biHeight),
					  thumb.get(), thumbsize.cx, thumbsize.cy);

		const BYTE* src = thumb.get();
		int srcPitch = thumbsize.cx * 4;
		if (bi->bmiHeader.biHeight >= 0) {
			src += srcPitch * (thumbsize.cy - 1);
			srcPitch = -srcPitch;
		}

		BYTE* dst = (BYTE*)spd.bits + spd.pitch * r.top + r.left * 4;
		for (int y = 0; y < thumbsize.cy; y++, dst += spd.pitch, src += srcPitch) {
			memcpy(dst, src, abs(srcPitch));
		}

		rts.Render(spd, 10000, 25, bbox);

		delete [] pData;

		m_iProgress++; // make one more thumbnail
		if (m_bAbort) {
			return;
		}
	}

	{
		CRenderedTextSubtitle rts(&csSubLock);
		rts.CreateDefaultStyle(0);
		rts.m_dstScreenSize.SetSize(width, height);
		STSStyle* style = DNew STSStyle();
		style->marginRect.SetRect(margin, margin, margin, height - infoheight - margin);
		rts.AddStyle(L"thumbs", style);

		CStringW str;
		str.Format(L"{\\an9\\fs%d\\b1\\bord0\\shad0\\1c&Hffffff&}%s", infoheight - 10, width >= 550 ? L"MPC-BE" : L"MPC");

		rts.Add(str, true, 0, 1, L"thumbs", L"", L"", CRect(0, 0, 0, 0), -1);

		const DVD_HMSF_TIMECODE hmsf = RT2HMS_r(duration);

		CStringW ar;
		if (dar.cx > 0 && dar.cy > 0 && dar.cx != framesize.cx && dar.cy != framesize.cy) {
			ar.Format(L"(%d:%d)", dar.cx, dar.cy);
		}

		CString filename = m_pMainFrm->GetAltFileName(); // YouTube
		CString filesize;

		if (filename.IsEmpty()) {
			const CString filepath = m_pMainFrm->GetCurFileName();
			filename = GetFileOnly(filepath);

			WIN32_FIND_DATA wfd;
			HANDLE hFind = FindFirstFile(filepath, &wfd);
			if (hFind != INVALID_HANDLE_VALUE) {
				FindClose(hFind);

				const __int64 size = (__int64(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
				TCHAR szFileSize[65] = { 0 };
				StrFormatByteSize(size, szFileSize, _countof(szFileSize));
				CString szByteSize;
				szByteSize.Format(L"%I64d", size);
				filesize.Format(ResStr(IDS_MAINFRM_58), szFileSize, FormatNumber(szByteSize));
			}
		}

		str.Format(ResStr(IDS_MAINFRM_59),
				   CompactPath(filename, 90),
				   filesize,
				   framesize.cx, framesize.cy, ar,
				   hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);

		rts.Add(str, true, 0, 1, L"thumbs");
		rts.Render(spd, 0, 25, bbox);
	}

	m_pMainFrm->SaveDIB(thumbpath, dib.get(), dibsize);

	m_bSuccessfully = true;
	m_iProgress = PROGRESS_COMPLETED; // the end
}

IMPLEMENT_DYNAMIC(CThumbsTaskDlg, CTaskDialog)

CThumbsTaskDlg::CThumbsTaskDlg(LPCTSTR filename)
	: CTaskDialog(L"", L"", ResStr(IDS_SAVING_THUMBNAIL),
		TDCBF_CANCEL_BUTTON,
		TDF_CALLBACK_TIMER | TDF_SHOW_PROGRESS_BAR | TDF_POSITION_RELATIVE_TO_WINDOW)
	, m_filename(filename)
	, m_pMainFrm(NULL)
	, m_iProgress(0)
	, m_bAbort(false)
	, m_bSuccessfully(false)
{

	SetDialogWidth(150);
}

CThumbsTaskDlg::~CThumbsTaskDlg()
{
	if (m_Thread.joinable()) {
		m_bAbort = true;
		m_Thread.join();
	}

	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

HRESULT CThumbsTaskDlg::OnInit()
{
	if (m_filename.IsEmpty()) {
		return E_INVALIDARG;
	}

	m_pMainFrm = AfxGetMainFrame();

	const CAppSettings& s = AfxGetAppSettings();
	int n = 1 + clamp(s.iThumbCols, 1, 10) * clamp(s.iThumbRows, 1, 20) + 1;

	SetProgressBarRange(0, n);
	SetProgressBarPosition(0);

	m_Thread = std::thread([this] { SaveThumbnails(m_filename); });

	return S_OK;
}

HRESULT CThumbsTaskDlg::OnTimer(_In_ long lTime)
{
	switch (m_iProgress) {
	// waiting after error
	case PROGRESS_E_WAIT:

		return S_FALSE;
	// errors
	case PROGRESS_E_FAIL:
		SetContent(ResStr(IDS_AG_ERROR));
		break;
	case PROGRESS_E_MEMORY:
		SetContent(ResStr(IDS_MAINFRM_56));
		break;
	case PROGRESS_E_VIDSIZE:
		SetContent(ResStr(IDS_MAINFRM_55));
		break;
	case PROGRESS_E_VIDFMT:
		SetContent(m_ErrorMsg);
		break;
	// operation is completed, close the dialog
	case PROGRESS_COMPLETED:
		ClickCommandControl(IDCANCEL);

		return S_OK;
	}

	if (m_iProgress < PROGRESS_E_WAIT) {
		m_iProgress = PROGRESS_E_WAIT;

		return S_FALSE;
	}

	SetProgressBarPosition(m_iProgress);
	return S_OK;
}
