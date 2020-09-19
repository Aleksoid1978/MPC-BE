/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2020 see Authors.txt
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
#include "MainFrm.h"
#include "../../DSUtil/DXVAState.h"
#include "../../DSUtil/FileHandle.h"
#include "WicUtils.h"
#include "PlayerToolBar.h"

#include <cmath>

// CPlayerToolBar

typedef HRESULT (__stdcall * SetWindowThemeFunct)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

IMPLEMENT_DYNAMIC(CPlayerToolBar, CToolBar)

CPlayerToolBar::CPlayerToolBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_hDXVAIcon(nullptr)
	, m_nDXVAIconWidth(0)
	, m_nDXVAIconHeight(0)
{
}

CPlayerToolBar::~CPlayerToolBar()
{
	if (m_hDXVAIcon) {
		DestroyIcon(m_hDXVAIcon);
	}
}

struct HLS {
	double H, L, S;

	HLS(const RGBQUAD& rgb) {
		double R = rgb.rgbRed / 255.0;
		double G = rgb.rgbGreen / 255.0;
		double B = rgb.rgbBlue / 255.0;
		double max = std::max({ R, G, B });
		double min = std::min({ R, G, B });

		L = (max + min) / 2.0;

		if (max == min) {
			S = H = 0.0;
		} else {
			double d = max - min;

			S = (L < 0.5) ? (d / (max + min)) : (d / (2.0 - max - min));

			if (R == max) {
				H = (G - B) / d;
			} else if (G == max) {
				H = 2.0 + (B - R) / d;
			} else { // if (B == max)
				H = 4.0 + (R - G) / d;
			}
			H /= 6.0;
			if (H < 0.0) {
				H += 1.0;
			}
		}
	}

	RGBQUAD toRGBQUAD() {
		RGBQUAD rgb;
		rgb.rgbReserved = 255;

		if (S == 0.0) {
			rgb.rgbRed = rgb.rgbGreen = rgb.rgbBlue = BYTE(L * 255);
		} else {
			auto hue2rgb = [](double p, double q, double h) {
				if (h < 0.0) {
					h += 1.0;
				} else if (h > 1.0) {
					h -= 1.0;
				}

				if (h < 1.0 / 6.0) {
					return p + (q - p) * 6.0 * h;
				} else if (h < 0.5) {
					return q;
				} else if (h < 2.0 / 3.0) {
					return p + (q - p) * (2.0 / 3.0 - h) * 6.0;
				}
				return p;
			};

			double q = (L < 0.5) ? (L * (1 + S)) : (L + S - L * S);
			double p = 2 * L - q;

			rgb.rgbRed = BYTE(hue2rgb(p, q, H + 1.0 / 3.0) * 255);
			rgb.rgbGreen = BYTE(hue2rgb(p, q, H) * 255);
			rgb.rgbBlue = BYTE(hue2rgb(p, q, H - 1.0 / 3.0) * 255);
		}

		return rgb;
	}
};

void CPlayerToolBar::SwitchTheme()
{
	CAppSettings& s = AfxGetAppSettings();

	CToolBarCtrl& tb = GetToolBarCtrl();
	m_nButtonHeight = 16;

	if (m_imgListActive.GetSafeHandle()) {
		m_imgListActive.DeleteImageList();
	}

	if (m_imgListDisabled.GetSafeHandle()) {
		m_imgListDisabled.DeleteImageList();
	}

	if (m_nUseDarkTheme != (int)s.bUseDarkTheme) {
		VERIFY(LoadToolBar(IDB_PLAYERTOOLBAR));

		ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);

		tb.SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
		tb.DeleteButton(1);
		tb.DeleteButton(tb.GetButtonCount()-2);

		SetMute(s.fMute);

		UINT styles[] = {
			TBBS_CHECKGROUP, TBBS_CHECKGROUP,
			TBBS_SEPARATOR,
			TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON,
			TBBS_SEPARATOR,
			TBBS_BUTTON,
			TBBS_BUTTON,
			TBBS_SEPARATOR,
			TBBS_SEPARATOR,
			TBBS_CHECKBOX,
		};

		for (int i = 0; i < _countof(styles); i++) {
			SetButtonStyle(i, styles[i] | TBBS_DISABLED);
		}

		m_nUseDarkTheme = (int)s.bUseDarkTheme;
	}

	if (s.bUseDarkTheme) {
		if (HMODULE h = LoadLibraryW(L"uxtheme.dll")) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");

			if (f) {
				f(m_hWnd, L" ", L" ");
			}

			FreeLibrary(h);
		}

		COLORSCHEME cs = { sizeof(COLORSCHEME) };
		cs.clrBtnHighlight = 0x0046413c;
		cs.clrBtnShadow    = 0x0037322d;

		tb.SetColorScheme(&cs);
		tb.SetIndent(5);
	} else {
		if (HMODULE h = LoadLibraryW(L"uxtheme.dll")) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");

			if (f) {
				f(m_hWnd, L"Explorer", nullptr);
			}

			FreeLibrary(h);
		}

		COLORSCHEME cs = { sizeof(COLORSCHEME) };
		cs.clrBtnHighlight = GetSysColor(COLOR_BTNFACE);
		cs.clrBtnShadow    = GetSysColor(COLOR_BTNSHADOW);

		tb.SetColorScheme(&cs);
		tb.SetIndent(0);
	}

	const int dpiScalePercent = m_pMainFrame->GetDPIScalePercent();
	static const int imageDpiScalePercent[] = {
		350, 300, 250, 225, 200, 175, 150, 125
	};
	static const int toolbarImageResId[] = {
		IDB_PLAYERTOOLBAR_PNG_350,
		IDB_PLAYERTOOLBAR_PNG_300,
		IDB_PLAYERTOOLBAR_PNG_250,
		IDB_PLAYERTOOLBAR_PNG_225,
		IDB_PLAYERTOOLBAR_PNG_200,
		IDB_PLAYERTOOLBAR_PNG_175,
		IDB_PLAYERTOOLBAR_PNG_150,
		IDB_PLAYERTOOLBAR_PNG_125
	};

	int imageDpiScalePercentIndex = -1;
	for (size_t i = 0; i < std::size(imageDpiScalePercent); i++) {
		if (dpiScalePercent >= imageDpiScalePercent[i]) {
			imageDpiScalePercentIndex = i;
			break;
		}
	}

	int resid = IDB_PLAYERTOOLBAR_PNG;
	if (imageDpiScalePercentIndex != -1) {
		resid = toolbarImageResId[imageDpiScalePercentIndex];
	}

	// load toolbar image
	CComPtr<IWICBitmapSource> pBitmapSource;
	HBITMAP hBitmap = nullptr;
	UINT width, height;

	// don't use premultiplied alpha here
	HRESULT hr = WicLoadImage(&pBitmapSource, false, (::GetProgramDir()+L"toolbar.png").GetString());

	if (FAILED(hr)) {
		BYTE* data;
		UINT size;
		hr = LoadResourceFile(resid, &data, size) ? S_OK : E_FAIL;
		if (SUCCEEDED(hr)) {
			hr = WicLoadImage(&pBitmapSource, false, data, size);
		}
	}

	if (SUCCEEDED(hr)) {
		hr = pBitmapSource->GetSize(&width, &height);
	}
	if (SUCCEEDED(hr) && width == height * 15) {
		hr = WicCreateHBitmap(hBitmap, pBitmapSource);
	}
	pBitmapSource.Release();

	if (SUCCEEDED(hr)) {
		CBitmap bmp;
		bmp.Attach(hBitmap);

		SetSizes({ (LONG)height + 7, (LONG)height + 6 }, SIZE{ (LONG)height, (LONG)height });

		VERIFY(m_imgListActive.Create(height, height, ILC_COLOR32 | ILC_MASK, 1, 0));
		VERIFY(m_imgListActive.Add(&bmp, nullptr) != -1);

		m_nButtonHeight = height;

		if (32 == 32) {
			const UINT bitmapsize = width * height * 4;

			BYTE* bmpBuffer = (BYTE*)GlobalAlloc(GMEM_FIXED, bitmapsize);
			if (bmpBuffer) {
				DWORD dwValue = bmp.GetBitmapBits(bitmapsize, bmpBuffer);
				if (dwValue) {
					auto adjustBrightness = [](BYTE c, double p) {
						int cAdjusted;
						if (c == 0 && p > 1.0) {
							cAdjusted = std::lround((p - 1.0) * 255);
						} else {
							cAdjusted = std::lround(c * p);
						}

						return BYTE(std::min(cAdjusted, 255));
					};

					BYTE* bits = bmpBuffer;
					for (UINT y = 0; y < height; y++, bits += width*4) {
						RGBQUAD* p = reinterpret_cast<RGBQUAD*>(bits);
						for (UINT x = 0; x < width; x++) {
							HLS hls(p[x]);
							hls.S = 0.0; // Make the color gray

							RGBQUAD rgb = hls.toRGBQUAD();
							p[x].rgbRed = BYTE(adjustBrightness(rgb.rgbRed, 0.55) * p[x].rgbReserved / 255);
							p[x].rgbGreen = BYTE(adjustBrightness(rgb.rgbGreen, 0.55) * p[x].rgbReserved / 255);
							p[x].rgbBlue = BYTE(adjustBrightness(rgb.rgbBlue, 0.55) * p[x].rgbReserved / 255);
						}
					}

					CBitmap bmpDisabled;
					bmpDisabled.CreateBitmap(width, height, 1, 32, bmpBuffer);

					VERIFY(m_imgListDisabled.Create(height, height, ILC_COLOR32 | ILC_MASK, 1, 0));
					VERIFY(m_imgListDisabled.Add(&bmpDisabled, nullptr) != 1);
				}

				GlobalFree((HGLOBAL)bmpBuffer);
			}
		}

		DeleteObject(hBitmap);
		hBitmap = nullptr;
	}

	// load GPU/DXVA indicator
	if (s.bUseDarkTheme) {
		if (m_hDXVAIcon) {
			DestroyIcon(m_hDXVAIcon);
			m_hDXVAIcon = nullptr;
		}
		m_nDXVAIconWidth = m_nDXVAIconHeight = 0;

		const struct {
			int resid;
			int height;
		} gpuImageResId[] = {
			{IDB_DXVA_INDICATOR_350, 56 },
			{IDB_DXVA_INDICATOR_300, 48 },
			{IDB_DXVA_INDICATOR_250, 40 },
			{IDB_DXVA_INDICATOR_225, 36 },
			{IDB_DXVA_INDICATOR_200, 32 },
			{IDB_DXVA_INDICATOR_175, 28 },
			{IDB_DXVA_INDICATOR_150, 24 },
			{IDB_DXVA_INDICATOR_125, 20 }
		};

		resid = IDB_DXVA_INDICATOR;
		if (imageDpiScalePercentIndex != -1) {
			resid = gpuImageResId[imageDpiScalePercentIndex].resid;
		}

		// don't use premultiplied alpha here
		HRESULT hr = WicLoadImage(&pBitmapSource, false, (::GetProgramDir()+L"gpu.png").GetString());

		if (SUCCEEDED(hr)) {
			hr = pBitmapSource->GetSize(&width, &height);
			if (SUCCEEDED(hr) && (LONG)height >= m_nButtonHeight) {
				hr = E_ABORT;
				pBitmapSource.Release();
			}
		}

		if (FAILED(hr)) {
			for (int i = 0; i < _countof(gpuImageResId); i++) {
				const int gpuresid = gpuImageResId[i].resid;
				if (gpuresid < resid && (LONG)height < m_nButtonHeight) {
					resid = gpuresid;
					break;
				}
			}

			BYTE* data;
			UINT size;
			hr = LoadResourceFile(resid, &data, size) ? S_OK : E_FAIL;
			if (SUCCEEDED(hr)) {
				hr = WicLoadImage(&pBitmapSource, false, data, size);
			}
			if (SUCCEEDED(hr)) {
				hr = pBitmapSource->GetSize(&width, &height);
			}
		}

		if (SUCCEEDED(hr)) {
			hr = WicCreateHBitmap(hBitmap, pBitmapSource);
		}

		if (SUCCEEDED(hr)) {
			BOOL ret = TRUE;

			CBitmap *bitmap = DNew CBitmap();
			ret = bitmap->Attach(hBitmap);

			CImageList *pButtonDXVA = DNew CImageList();
			ret = pButtonDXVA->Create(width, height, ILC_COLOR32 | ILC_MASK, 1, 0);
			ret = pButtonDXVA->Add(bitmap, nullptr);

			m_hDXVAIcon = pButtonDXVA->ExtractIconW(0);

			delete pButtonDXVA;
			delete bitmap;

			m_nDXVAIconWidth  = width;
			m_nDXVAIconHeight = height;

			DeleteObject(hBitmap);
		}
	}

	if (!s.bUseDarkTheme && !m_imgListActive.GetSafeHandle()) {
		static COLORMAP cmActive[] = {
			0x00c0c0c0, 0x00ff00ff //background = transparency mask
		};

		static COLORMAP cmDisabled[] = {
			0x00000000, 0x00A0A0A0, //button_face -> black to gray
			0x00c0c0c0, 0x00ff00ff  //background = transparency mask
		};

		auto CreateImageList = [](CImageList& imageList, LPCOLORMAP colorMap, const int mapSize) {
			CBitmap bm;
			VERIFY(bm.LoadMappedBitmap(IDB_PLAYERTOOLBAR, CMB_MASKED, colorMap, mapSize));
			BITMAP bmInfo;
			VERIFY(bm.GetBitmap(&bmInfo));
			VERIFY(imageList.Create(bmInfo.bmHeight, bmInfo.bmHeight, bmInfo.bmBitsPixel | ILC_MASK, 1, 0));
			VERIFY(imageList.Add(&bm, RGB(255, 0, 255)) != -1);
		};

		CreateImageList(m_imgListActive, cmActive, std::size(cmActive));
		CreateImageList(m_imgListDisabled, cmDisabled, std::size(cmDisabled));
	}

	tb.SetImageList(&m_imgListActive);
	tb.SetDisabledImageList(&m_imgListDisabled);

	if (::IsWindow(m_volctrl.GetSafeHwnd())) {
		m_volctrl.Invalidate();
	}

	OAFilterState fs = m_pMainFrame->GetMediaState();

	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;

	if (fs == State_Running) {
		bi.iImage = 1;
	} else {
		bi.iImage = 0;
	}

	tb.SetButtonInfo(ID_PLAY_PLAY, &bi);
}

BOOL CPlayerToolBar::Create(CWnd* pParentWnd)
{
	VERIFY(__super::CreateEx(pParentWnd,
		   TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_AUTOSIZE | TBSTYLE_CUSTOMERASE,
		   WS_CHILD | WS_VISIBLE | CBRS_ALIGN_BOTTOM | CBRS_TOOLTIPS));

	if (m_BackGroundbm.FileExists(L"background")) {
		m_BackGroundbm.LoadExternalGradient(L"background");
	}

	m_volctrl.Create(this);
	m_volctrl.SetRange(0, 100);

	m_nUseDarkTheme = 2; // needed for the first call SwitchTheme()
	m_bMute = false;

	SwitchTheme();
	SetColor();

	return TRUE;
}

BOOL CPlayerToolBar::PreCreateWindow(CREATESTRUCT& cs)
{
	VERIFY(__super::PreCreateWindow(cs));

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;

	return TRUE;
}

void CPlayerToolBar::SetMute(bool fMute)
{
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	bi.iImage = fMute ? 13 : 12;

	GetToolBarCtrl().SetButtonInfo(ID_VOLUME_MUTE, &bi);

	AfxGetAppSettings().fMute = fMute;
}

bool CPlayerToolBar::IsMuted()
{
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;

	GetToolBarCtrl().GetButtonInfo(ID_VOLUME_MUTE, &bi);

	return (bi.iImage == 13);
}

int CPlayerToolBar::GetVolume()
{
	int volume = m_volctrl.GetPos(), type = 0;

	if (volume < 1 && !m_bMute) {
		if (!IsMuted()) {
			type++;
		}
		m_bMute = true;
	} else if (IsMuted() && volume > 0 && m_bMute) {
		type++;
		m_bMute = false;
	}

	/*
	if (type) {
		OnVolumeMute(0);
		SendMessageW(WM_COMMAND, ID_VOLUME_MUTE);
	}
	*/

	if (IsMuted() || volume < 1) {
		volume = -10000;
	} else {
		volume = std::min((int)(4000 * log10(volume / 100.0f)), 0);
	}

	return volume;
}

int CPlayerToolBar::GetMinWidth()
{
	return m_nButtonHeight * 12 + 155 + m_nWidthIncrease;
}

void CPlayerToolBar::SetVolume(int volume)
{
	m_volctrl.SetPosInternal(volume);
}

void CPlayerToolBar::ScaleToolbar()
{
	if (AfxGetAppSettings().bUseDarkTheme) {
		m_volctrl.m_nUseDarkTheme = 1;

		m_nUseDarkTheme = 2;
		SwitchTheme();

		OnInitialUpdate();
	}
}

void CPlayerToolBar::SetColor()
{
	const auto& s = AfxGetAppSettings();
	if (s.bUseDarkTheme) {
		int R, G, B;

		if (m_BackGroundbm.IsExtGradiendLoading()) {
			ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, m_crBackground.R, m_crBackground.G, m_crBackground.B);
		} else {
			ThemeRGB(50, 55, 60, R, G, B);
			tvBackground[0] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
			ThemeRGB(20, 25, 30, R, G, B);
			tvBackground[1] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
		}

		m_penFrHot.DeleteObject();
		m_penFrHot.CreatePen(PS_SOLID, 0, 0x00E9E9E9);
	}
}

BEGIN_MESSAGE_MAP(CPlayerToolBar, CToolBar)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
	ON_WM_SIZE()
	ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	ON_COMMAND_EX(ID_VOLUME_MUTE, OnVolumeMute)
	ON_UPDATE_COMMAND_UI(ID_VOLUME_MUTE, OnUpdateVolumeMute)

	ON_COMMAND_EX(ID_PLAY_PAUSE, OnPause)
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlay)
	ON_COMMAND_EX(ID_PLAY_STOP, OnStop)
	ON_COMMAND_EX(ID_FILE_CLOSEMEDIA, OnStop)

	ON_COMMAND_EX(ID_VOLUME_UP, OnVolumeUp)
	ON_COMMAND_EX(ID_VOLUME_DOWN, OnVolumeDown)

	ON_WM_NCPAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFFFFFF, OnToolTipNotify)
	ON_WM_MOUSEMOVE()

	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_AUDIO, OnUpdateAudio)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_SUBTITLES, OnUpdateSubtitle)
END_MESSAGE_MAP()

// CPlayerToolBar message handlers

void CPlayerToolBar::OnUpdateAudio(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bAudioEnable);
}

void CPlayerToolBar::OnUpdateSubtitle(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bSubtitleEnable);
}

void CPlayerToolBar::OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTBCUSTOMDRAW pTBCD = reinterpret_cast<LPNMTBCUSTOMDRAW>(pNMHDR);
	LRESULT lr = CDRF_DODEFAULT;

	const auto& s = AfxGetAppSettings();
	const auto bGPU = (m_pMainFrame->GetMediaState() != -1) && DXVAState::GetState() && m_hDXVAIcon;

	static const int sep[] = {2, 7, 10, 11};

	if (s.bUseDarkTheme) {
		GRADIENT_RECT gr = { 0, 1 };

		switch(pTBCD->nmcd.dwDrawStage)
		{
		case CDDS_PREERASE:
			m_volctrl.Invalidate();
			lr = CDRF_SKIPDEFAULT;
			break;
		case CDDS_PREPAINT: {
				CDC dc;
				dc.Attach(pTBCD->nmcd.hdc);

				CRect r;
				GetClientRect(&r);

				if (m_BackGroundbm.IsExtGradiendLoading()) {
					m_BackGroundbm.PaintExternalGradient(&dc, r, 21, s.nThemeBrightness, m_crBackground.R, m_crBackground.G, m_crBackground.B);
				} else {
					tvBackground[0].x = r.left; tvBackground[0].y = r.top;
					tvBackground[1].x = r.right; tvBackground[1].y = r.bottom;
					dc.GradientFill(tvBackground, 2, &gr, 1, GRADIENT_FILL_RECT_V);
				}

				dc.Detach();
			}
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			lr |= TBCDRF_NOETCHEDEFFECT;
			lr |= TBCDRF_NOBACKGROUND;
			lr |= TBCDRF_NOEDGES;
			lr |= TBCDRF_NOOFFSET;

			lr |= CDRF_NOTIFYPOSTPAINT;
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPOSTPAINT:
			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);

			CRect r(pTBCD->nmcd.rc);

			if (CDIS_HOT == pTBCD->nmcd.uItemState || CDIS_CHECKED + CDIS_HOT == pTBCD->nmcd.uItemState) {
				const int gWidth  = 8;
				const int gHeight = 8;

				CDC memdc;
				memdc.CreateCompatibleDC(&dc);

				CBitmap *bmOld, bmGlassLike;
				bmGlassLike.CreateCompatibleBitmap(&dc, gWidth, gHeight);
				bmOld = memdc.SelectObject(&bmGlassLike);

				TRIVERTEX tv[2] = {
					{0, 0, 255 * 256, 255 * 256, 255 * 256, 255 * 256},
					{gWidth, gHeight, 0, 0, 0, 0},
				};
				memdc.GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);

				BLENDFUNCTION bf;
				bf.AlphaFormat         = AC_SRC_ALPHA;
				bf.BlendFlags          = 0;
				bf.BlendOp             = AC_SRC_OVER;
				bf.SourceConstantAlpha = 90;

				CBrush* brushSaved = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
				CPen* penSaved = dc.SelectObject(&m_penFrHot);

				dc.RoundRect(r.left + 1, r.top + 1, r.right - 2, r.bottom - 1, 6, 4);
				AlphaBlend(dc.m_hDC, r.left + 2, r.top + 2, r.Width() - 4, 0.7 * r.Height() - 2, memdc, 0, 0, gWidth, gHeight, bf);

				dc.SelectObject(&penSaved);
				dc.SelectObject(&brushSaved);

				DeleteObject(memdc.SelectObject(bmOld));
				memdc.DeleteDC();
			}

			for (size_t j = 0; j < std::size(sep); j++) {
				GetItemRect(sep[j], &r);

				if (m_BackGroundbm.IsExtGradiendLoading()) {
					m_BackGroundbm.PaintExternalGradient(&dc, r, 21, s.nThemeBrightness, m_crBackground.R, m_crBackground.G, m_crBackground.B);
				} else {
					tvBackground[0].x = r.left; tvBackground[0].y = r.top;
					tvBackground[1].x = r.right; tvBackground[1].y = r.bottom;
					dc.GradientFill(tvBackground, 2, &gr, 1, GRADIENT_FILL_RECT_V);
				}
			}

			if (bGPU) {
				CRect rSub;
				GetItemRect(10, &rSub);

				CRect rVolume;
				GetItemRect(12, &rVolume);

				if (rSub.right < rVolume.left - m_nDXVAIconWidth) {
					DrawIconEx(dc.m_hDC, rVolume.left - 8 - m_nDXVAIconWidth, r.CenterPoint().y - (m_nDXVAIconHeight / 2 + 1), m_hDXVAIcon, 0, 0, 0, nullptr, DI_NORMAL);
				}
			}

			dc.Detach();

			lr |= CDRF_SKIPDEFAULT;
			break;
		}
	} else {
		switch(pTBCD->nmcd.dwDrawStage)
		{
		case CDDS_PREERASE:
			m_volctrl.Invalidate();
			lr = CDRF_SKIPDEFAULT;
			break;
		case CDDS_PREPAINT: {
				CDC dc;
				dc.Attach(pTBCD->nmcd.hdc);
				CRect r;
				GetClientRect(&r);
				dc.FillSolidRect(r, GetSysColor(COLOR_BTNFACE));
				dc.Detach();
			}
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			lr |= CDRF_NOTIFYPOSTPAINT;
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPOSTPAINT:
			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);
			CRect r;

			for (size_t j = 0; j < std::size(sep); j++) {
				GetItemRect(sep[j], &r);
				dc.FillSolidRect(r, GetSysColor(COLOR_BTNFACE));
			}

			dc.Detach();

			lr |= CDRF_SKIPDEFAULT;
			break;
		}
	}

	*pResult = lr;
}

void CPlayerToolBar::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	OnInitialUpdate();
}

void CPlayerToolBar::OnInitialUpdate()
{
	if (!::IsWindow(m_volctrl.m_hWnd)) {
		return;
	}

	m_nWidthIncrease = 0;

	CRect r, br, vr2;

	GetClientRect(&r);
	br = GetBorders();

	const int offset_right = 6;
	int offset = 60;
	if (AfxGetAppSettings().bUseDarkTheme) {
		if (r.Height() > 30) {
			offset = (60 + offset_right) * r.Height() / 30 - offset_right;
			m_nWidthIncrease = offset - 60;
		}
		vr2.SetRect(r.right + br.right - offset, r.top, r.right + br.right + offset_right, r.bottom);
	} else {
		vr2.SetRect(r.right + br.right - offset, r.bottom - 25, r.right + br.right + offset_right, r.bottom);
	}

	if (m_nUseDarkTheme != (int)AfxGetAppSettings().bUseDarkTheme) {
		SwitchTheme();
	}

	m_volctrl.MoveWindow(vr2);

	SetButtonInfo(7, GetItemID(7), TBBS_SEPARATOR | TBBS_DISABLED, 48);
	CRect r10, r12;
	GetItemRect(10, &r10);
	GetItemRect(12, &r12);
	SetButtonInfo(11, GetItemID(11), TBBS_SEPARATOR | TBBS_DISABLED, vr2.left - r10.right - r12.Width());
}

BOOL CPlayerToolBar::OnVolumeMute(UINT nID)
{
	SetMute(!IsMuted());

	if (::IsWindow(m_volctrl.GetSafeHwnd())) {
		m_volctrl.Invalidate();
	}

	return FALSE;
}

void CPlayerToolBar::OnUpdateVolumeMute(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(IsMuted());
}

BOOL CPlayerToolBar::OnVolumeUp(UINT nID)
{
	m_volctrl.IncreaseVolume();

	return FALSE;
}

BOOL CPlayerToolBar::OnVolumeDown(UINT nID)
{
	m_volctrl.DecreaseVolume();

	return FALSE;
}

void CPlayerToolBar::OnNcPaint()
{
	CRect	wr, cr;
	CDC*	pDC = CDC::FromHandle(::GetWindowDC(m_hWnd));

	GetClientRect(&cr);
	ClientToScreen(&cr);
	GetWindowRect(&wr);

	cr.OffsetRect(-wr.left, -wr.top);
	wr.OffsetRect(-wr.left, -wr.top);

	pDC->ExcludeClipRect(&cr);

	if (!AfxGetAppSettings().bUseDarkTheme) {
		pDC->FillSolidRect(wr, GetSysColor(COLOR_BTNFACE));
	}

	::ReleaseDC(m_hWnd, pDC->m_hDC);
}

void CPlayerToolBar::OnMouseMove(UINT nFlags, CPoint point)
{
	int i = getHitButtonIdx(point);

	if (i == -1 || (GetButtonStyle(i) & (TBBS_SEPARATOR | TBBS_DISABLED))) {
	} else {
		if (i > 10 || i < 2 || i == 6 || (i < 10 && (m_pMainFrame->IsSomethingLoaded() || m_pMainFrame->IsD3DFullScreenMode()))) {
			::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		}
	}

	__super::OnMouseMove(nFlags, point);
}

BOOL CPlayerToolBar::OnPlay(UINT nID)
{
	OAFilterState fs	= m_pMainFrame->GetMediaState();
	CToolBarCtrl& tb	= GetToolBarCtrl();

	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;

	tb.GetButtonInfo(ID_PLAY_PLAY, &bi);

	if (!bi.iImage) {
		bi.iImage = 1;
	} else {
		bi.iImage = (fs == State_Paused || fs == State_Stopped) ? 1 : 0;
	}

	tb.SetButtonInfo(ID_PLAY_PLAY, &bi);

	return FALSE;
}

BOOL CPlayerToolBar::OnStop(UINT nID)
{
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	bi.iImage = 0;

	GetToolBarCtrl().SetButtonInfo(ID_PLAY_PLAY, &bi);

	return FALSE;
}

BOOL CPlayerToolBar::OnPause(UINT nID)
{
	OAFilterState fs = m_pMainFrame->GetMediaState();

	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	bi.iImage = (fs == State_Paused) ? 1 : 0;

	GetToolBarCtrl().SetButtonInfo(ID_PLAY_PLAY, &bi);

	return FALSE;
}

void CPlayerToolBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	OAFilterState fs = m_pMainFrame->GetMediaState();
	int i = getHitButtonIdx(point);

	if (i == 0 && fs != -1) {
		m_pMainFrame->PostMessageW(WM_COMMAND, ID_PLAY_PLAYPAUSE);
	} else if (i == -1 || (GetButtonStyle(i) & (TBBS_SEPARATOR | TBBS_DISABLED))) {
		if (!m_pMainFrame->m_bFullScreen && !m_pMainFrame->IsZoomed()) {
			MapWindowPoints(m_pMainFrame, &point, 1);
			m_pMainFrame->PostMessageW(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		}
	} else {
		if (i > 10 || (i < 10 && (m_pMainFrame->IsSomethingLoaded() || m_pMainFrame->IsD3DFullScreenMode()))) {
			::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		}

		__super::OnLButtonDown(nFlags, point);
	}
}

int CPlayerToolBar::getHitButtonIdx(CPoint point)
{
	int hit = -1;
	CRect r;

	for (int i = 0, j = GetToolBarCtrl().GetButtonCount(); i < j; i++) {
		GetItemRect(i, r);

		if (r.PtInRect(point)) {
			hit = i;
			break;
		}
	}

	return hit;
}

BOOL CPlayerToolBar::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTW* pTTT = (TOOLTIPTEXTW*)pNMHDR;
	static CString m_strTipText;

	switch (pNMHDR->idFrom) {
	case ID_PLAY_PLAY:
		m_strTipText = (m_pMainFrame->GetMediaState() == State_Running) ? ResStr(IDS_AG_PAUSE) : ResStr(IDS_AG_PLAY);
		break;
	case ID_PLAY_STOP:
		m_strTipText = ResStr(IDS_AG_STOP) + L" | " + ResStr(IDS_AG_CLOSE);
		break;
	case ID_PLAY_FRAMESTEP:
		m_strTipText = ResStr(IDS_AG_STEP) + L" | " + ResStr(IDS_AG_JUMP_TO);
		break;
	case ID_VOLUME_MUTE:
		{
			TBBUTTONINFO bi;
			bi.cbSize = sizeof(bi);
			bi.dwMask = TBIF_IMAGE;

			GetToolBarCtrl().GetButtonInfo(ID_VOLUME_MUTE, &bi);

			if (bi.iImage == 12) {
				m_strTipText = ResStr(ID_VOLUME_MUTE);
			}
			else if (bi.iImage == 13) {
				m_strTipText = ResStr(ID_VOLUME_MUTE_OFF);
			}
			else if (bi.iImage == 14) {
				m_strTipText = ResStr(ID_VOLUME_MUTE_DISABLED);
			}

			int i = m_strTipText.Find('\n'); // TODO: remove it
			if (i > 0) {
				m_strTipText = m_strTipText.Left(i);
			}
		}
		break;
	case ID_FILE_OPENQUICK:
		m_strTipText = ResStr(IDS_MPLAYERC_0) + L" | " + ResStr(IDS_RECENT_FILES);
		break;
	case ID_NAVIGATE_SKIPFORWARD:
		m_strTipText = ResStr(IDS_AG_NEXT);
		break;
	case ID_NAVIGATE_SKIPBACK:
		m_strTipText = ResStr(IDS_AG_PREVIOUS);
		break;
	case ID_NAVIGATE_SUBTITLES:
		m_strTipText = ResStr(IDS_AG_SUBTITLELANG) + L" | " + ResStr(IDS_AG_OPTIONS);
		break;
	case ID_NAVIGATE_AUDIO:
		m_strTipText = ResStr(IDS_AG_AUDIOLANG) + L" | " + ResStr(IDS_AG_OPTIONS);
		break;
	default:
		return FALSE;
	}

	pTTT->lpszText = m_strTipText.GetBuffer();
	*pResult = 0;

	return TRUE;
}

void CPlayerToolBar::OnRButtonDown(UINT nFlags, CPoint point)
{
	int Idx = getHitButtonIdx(point);

	if (Idx == 1) {
		m_pMainFrame->PostMessageW(WM_COMMAND, ID_FILE_CLOSEPLAYLIST);
	} else if (Idx == 5) {
		m_pMainFrame->OnMenuNavJumpTo();
	} else if (Idx == 6) {
		m_pMainFrame->OnMenuRecentFiles();
	} else if (Idx == 8) {
		m_pMainFrame->OnMenuNavAudioOptions();
	} else if (Idx == 9) {
		m_pMainFrame->OnMenuNavSubtitleOptions();
	}

	__super::OnRButtonDown(nFlags, point);
}
