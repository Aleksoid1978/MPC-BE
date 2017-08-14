/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include "PlayerToolBar.h"
#include <IPinHook.h>

// CPlayerToolBar

typedef HRESULT (__stdcall * SetWindowThemeFunct)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

IMPLEMENT_DYNAMIC(CPlayerToolBar, CToolBar)

CPlayerToolBar::CPlayerToolBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_bDisableImgListRemap(false)
	, m_pButtonsImages(NULL)
	, m_hDXVAIcon(NULL)
	, m_nDXVAIconWidth(0)
	, m_nDXVAIconHeight(0)
{
}

CPlayerToolBar::~CPlayerToolBar()
{
	if (m_pButtonsImages) {
		delete m_pButtonsImages;
	}

	if (m_hDXVAIcon) {
		DestroyIcon(m_hDXVAIcon);
	}
}

void CPlayerToolBar::SwitchTheme()
{
	CAppSettings& s = AfxGetAppSettings();

	CToolBarCtrl& tb = GetToolBarCtrl();
	m_nButtonHeight = 16;

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
		if (HMODULE h = LoadLibrary(L"uxtheme.dll")) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");

			if (f) {
				f(m_hWnd, L" ", L" ");
			}

			FreeLibrary(h);
		}

		SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 0);// 0 Remap Active

		COLORSCHEME cs;
		cs.dwSize		= sizeof(COLORSCHEME);
		cs.clrBtnHighlight	= 0x0046413c;
		cs.clrBtnShadow		= 0x0037322d;

		tb.SetColorScheme(&cs);
		tb.SetIndent(5);
	} else {
		if (HMODULE h = LoadLibrary(L"uxtheme.dll")) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");

			if (f) {
				f(m_hWnd, L"Explorer", NULL);
			}

			FreeLibrary(h);
		}

		SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 2);// 2 Undo  Active

		COLORSCHEME cs;
		cs.dwSize		= sizeof(COLORSCHEME);
		cs.clrBtnHighlight	= GetSysColor(COLOR_BTNFACE);
		cs.clrBtnShadow		= GetSysColor(COLOR_BTNSHADOW);

		tb.SetColorScheme(&cs);
		tb.SetIndent(0);
	}

	const int dpiScalePercent = m_pMainFrame->GetDPIScalePercent();
	const int imageDpiScalePercent[] = {
		350, 300, 250, 225, 200, 175, 150, 125
	};
	const int toolbarImageResId[] = {
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
	for (int i = 0; i < _countof(imageDpiScalePercent); i++) {
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
	HBITMAP hBmp = NULL;
	bool fp = CMPCPngImage::FileExists(CString(L"toolbar"));
	if (s.bUseDarkTheme && !fp) {
		/*
		int col = s.clrFaceABGR;
		int r, g, b, R, G, B;
		r = col & 0xFF;
		g = (col >> 8) & 0xFF;
		b = col >> 16;
		*/
		hBmp = CMPCPngImage::LoadExternalImage(L"toolbar", resid, IMG_TYPE::PNG, s.nThemeBrightness, s.nThemeRed, s.nThemeGreen, s.nThemeBlue);
	} else if (fp) {
		hBmp = CMPCPngImage::LoadExternalImage(L"toolbar", 0, IMG_TYPE::UNDEF);
	}

	BITMAP bitmapBmp;
	if (NULL != hBmp) {
		::GetObject(hBmp, sizeof(bitmapBmp), &bitmapBmp);

		if (fp && bitmapBmp.bmWidth != bitmapBmp.bmHeight * 15) {
			if (s.bUseDarkTheme) {
				hBmp = CMPCPngImage::LoadExternalImage(L"", resid, IMG_TYPE::PNG, s.nThemeBrightness, s.nThemeRed, s.nThemeGreen, s.nThemeBlue);
				::GetObject(hBmp, sizeof(bitmapBmp), &bitmapBmp);
			} else {
				DeleteObject(hBmp);
				hBmp = NULL;
			}
		}
	}

	if (NULL != hBmp) {
		CBitmap *bmp = DNew CBitmap();
		bmp->Attach(hBmp);

		SetSizes(CSize(bitmapBmp.bmHeight + 7, bitmapBmp.bmHeight + 6), CSize(bitmapBmp.bmHeight, bitmapBmp.bmHeight));

		SAFE_DELETE(m_pButtonsImages);

		m_pButtonsImages = DNew CImageList();

		if (32 == bitmapBmp.bmBitsPixel) {
			m_pButtonsImages->Create(bitmapBmp.bmHeight, bitmapBmp.bmHeight, ILC_COLOR32 | ILC_MASK, 1, 0);
			m_pButtonsImages->Add(bmp, static_cast<CBitmap*>(0));
		} else {
			m_pButtonsImages->Create(bitmapBmp.bmHeight, bitmapBmp.bmHeight, ILC_COLOR24 | ILC_MASK, 1, 0);
			m_pButtonsImages->Add(bmp, RGB(255, 0, 255));
		}

		m_nButtonHeight = bitmapBmp.bmHeight;
		tb.SetImageList(m_pButtonsImages);
		m_bDisableImgListRemap = true;

		delete bmp;
		DeleteObject(hBmp);
	}

	// load GPU/DXVA indicator
	if (s.bUseDarkTheme) {
		if (m_hDXVAIcon) {
			DestroyIcon(m_hDXVAIcon);
			m_hDXVAIcon = NULL;
		}
		m_nDXVAIconWidth = m_nDXVAIconHeight = 0;

		const int gpuImageResId[] = {
			IDB_DXVA_INDICATOR_350,
			IDB_DXVA_INDICATOR_300,
			IDB_DXVA_INDICATOR_250,
			IDB_DXVA_INDICATOR_225,
			IDB_DXVA_INDICATOR_200,
			IDB_DXVA_INDICATOR_175,
			IDB_DXVA_INDICATOR_150,
			IDB_DXVA_INDICATOR_125
		};

		resid = IDB_DXVA_INDICATOR;
		if (imageDpiScalePercentIndex != -1) {
			resid = gpuImageResId[imageDpiScalePercentIndex];
		}

		hBmp = NULL;
		fp = CMPCPngImage::FileExists(CString(L"gpu"));
		BITMAP bm = { 0 };
		if (fp) {
			hBmp = CMPCPngImage::LoadExternalImage(L"gpu", 0, IMG_TYPE::UNDEF);
			if (hBmp) {
				::GetObject(hBmp, sizeof(bm), &bm);
			}
		}
		if (!hBmp || bm.bmHeight >= m_nButtonHeight) {
			hBmp = CMPCPngImage::LoadExternalImage(L"", resid, IMG_TYPE::PNG);
			if (hBmp) {
				::GetObject(hBmp, sizeof(bm), &bm);
				if (bm.bmHeight >= m_nButtonHeight) {
					DeleteObject(hBmp);
					hBmp = NULL;

					for (int i = 0; i < _countof(gpuImageResId); i++) {
						const int gpuresid = gpuImageResId[i];
						if (gpuresid < resid) {
							hBmp = CMPCPngImage::LoadExternalImage(L"", gpuresid, IMG_TYPE::PNG);
							if (hBmp) {
								::GetObject(hBmp, sizeof(bm), &bm);
								if (bm.bmHeight < m_nButtonHeight) {
									break;
								}

								DeleteObject(hBmp);
								hBmp = NULL;
							}
						}
					}

					if (!hBmp) {
						hBmp = CMPCPngImage::LoadExternalImage(L"", IDB_DXVA_INDICATOR, IMG_TYPE::PNG);
					}
				}
			}
		}
		if (hBmp) {
			::GetObject(hBmp, sizeof(bm), &bm);

			CBitmap *bmp = DNew CBitmap();
			bmp->Attach(hBmp);

			CImageList *pButtonDXVA = DNew CImageList();
			pButtonDXVA->Create(bm.bmWidth, bm.bmHeight, ILC_COLOR32 | ILC_MASK, 1, 0);
			pButtonDXVA->Add(bmp, static_cast<CBitmap*>(NULL));

			m_hDXVAIcon = pButtonDXVA->ExtractIcon(0);

			delete pButtonDXVA;
			delete bmp;

			m_nDXVAIconWidth  = bm.bmWidth;
			m_nDXVAIconHeight = bm.bmHeight;

			DeleteObject(hBmp);
		}
	}

	if (s.bUseDarkTheme) {
		if (!m_bDisableImgListRemap) {
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 0);// 0 Remap Active
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 1);// 1 Remap Disabled
		}
	} else {
		m_bDisableImgListRemap = true;

		if (NULL == fp) {
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 2);// 2 Undo  Active

			if (!m_bDisableImgListRemap) {
				SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 3);// 3 Undo  Disabled
			}
		}
	}

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

	if (m_BackGroundbm.FileExists(CString(L"background"))) {
		m_BackGroundbm.LoadExternalGradient(L"background");
	}

	m_volctrl.Create(this);
	m_volctrl.SetRange(0, 100);

	m_nUseDarkTheme = 2; // needed for the first call SwitchTheme()
	m_bMute = false;

	SwitchTheme();

	return TRUE;
}

BOOL CPlayerToolBar::PreCreateWindow(CREATESTRUCT& cs)
{
	VERIFY(__super::PreCreateWindow(cs));

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;

	return TRUE;
}

void CPlayerToolBar::CreateRemappedImgList(UINT bmID, int nRemapState, CImageList& reImgList)
{
	CAppSettings& s = AfxGetAppSettings();
	CBitmap bm;

	COLORMAP cmActive[] = {
		0x00000000, s.clrFaceABGR,
		0x00808080, s.clrOutlineABGR,
		0x00c0c0c0, 0x00ff00ff//background = transparency mask
	};

	COLORMAP cmDisabled[] = {
		0x00000000, 0x00ff00ff,//button_face -> transparency mask
		0x00808080, s.clrOutlineABGR,
		0x00c0c0c0, 0x00ff00ff//background = transparency mask
	};

	COLORMAP cmUndoActive[] = {
		0x00c0c0c0, 0x00ff00ff//background = transparency mask
	};

	COLORMAP cmUndoDisabled[] = {
		0x00000000, 0x00A0A0A0,//button_face -> black to gray
		0x00c0c0c0, 0x00ff00ff//background = transparency mask
	};

	switch (nRemapState) {
		default:
		case 0:
			bm.LoadMappedBitmap(bmID, CMB_MASKED, cmActive, 3);
			break;
		case 1:
			bm.LoadMappedBitmap(bmID, CMB_MASKED, cmDisabled, 3);
			break;
		case 2:
			bm.LoadMappedBitmap(bmID, CMB_MASKED, cmUndoActive, 1);
			break;
		case 3:
			bm.LoadMappedBitmap(bmID, CMB_MASKED, cmUndoDisabled, 2);
			break;
	}

	BITMAP bmInfo;
	VERIFY(bm.GetBitmap(&bmInfo));
	VERIFY(reImgList.Create(bmInfo.bmHeight, bmInfo.bmHeight, bmInfo.bmBitsPixel | ILC_MASK, 1, 0));
	VERIFY(reImgList.Add(&bm, 0x00ff00ff) != -1);
}

void CPlayerToolBar::SwitchRemmapedImgList(UINT bmID, int nRemapState)
{
	CToolBarCtrl& tb = GetToolBarCtrl();

	if (nRemapState == 0 || nRemapState == 2) {
		if (m_reImgListActive.GetSafeHandle()) {
			m_reImgListActive.DeleteImageList();
		}

		CreateRemappedImgList(bmID, nRemapState, m_reImgListActive);
		ASSERT(m_reImgListActive.GetSafeHandle());
		tb.SetImageList(&m_reImgListActive);
	} else {
		if (m_reImgListDisabled.GetSafeHandle()) {
			m_reImgListDisabled.DeleteImageList();
		}

		CreateRemappedImgList(bmID, nRemapState, m_reImgListDisabled);
		ASSERT(m_reImgListDisabled.GetSafeHandle());
		tb.SetDisabledImageList(&m_reImgListDisabled);
	}
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
		SendMessage(WM_COMMAND, ID_VOLUME_MUTE);
	}
	*/

	if (IsMuted() || volume < 1) {
		volume = -10000;
	} else {
		volume = min((int)(4000 * log10(volume / 100.0f)), 0);
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
	//UpdateButonState(ID_NAVIGATE_SUBTITLES, FALSE);

	LPNMTBCUSTOMDRAW pTBCD	= reinterpret_cast<LPNMTBCUSTOMDRAW>(pNMHDR);
	LRESULT lr				= CDRF_DODEFAULT;

	CAppSettings& s	= AfxGetAppSettings();
	bool bGPU		= (m_pMainFrame->GetMediaState() != -1) && DXVAState::GetState();

	int R, G, B, R2, G2, B2;

	GRADIENT_RECT gr = {0, 1};

	int sep[] = {2, 7, 10, 11};

	if (s.bUseDarkTheme) {

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
					ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
					m_BackGroundbm.PaintExternalGradient(&dc, r, 21, s.nThemeBrightness, R, G, B);
				} else {
					ThemeRGB(50, 55, 60, R, G, B);
					ThemeRGB(20, 25, 30, R2, G2, B2);
					TRIVERTEX tv[2] = {
						{r.left, r.top, R * 256, G * 256, B * 256, 255 * 256},
						{r.right, r.bottom, R2 * 256, G2 * 256, B2 * 256, 255 * 256},
					};
					dc.GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);
				}

				dc.Detach();
			}
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			lr = CDRF_DODEFAULT;

			lr |= TBCDRF_NOETCHEDEFFECT;
			lr |= TBCDRF_NOBACKGROUND;
			lr |= TBCDRF_NOEDGES;
			lr |= TBCDRF_NOOFFSET;

			lr |= CDRF_NOTIFYPOSTPAINT;
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPOSTPAINT:
			lr = CDRF_DODEFAULT;

			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);
			CRect r;
			CopyRect(&r,&pTBCD->nmcd.rc);

			CRect rGlassLike(0, 0, 8, 8);
			int nW = rGlassLike.Width(), nH = rGlassLike.Height();
			CDC memdc;
			memdc.CreateCompatibleDC(&dc);
			CBitmap *bmOld, bmGlassLike;
			bmGlassLike.CreateCompatibleBitmap(&dc, nW, nH);
			bmOld = memdc.SelectObject(&bmGlassLike);

			TRIVERTEX tv[2] = {
				{0, 0, 255 * 256, 255 * 256, 255 * 256, 255 * 256},
				{nW, nH, 0, 0, 0, 0},
			};
			memdc.GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);

			BLENDFUNCTION bf;
			bf.AlphaFormat			= AC_SRC_ALPHA;
			bf.BlendFlags			= 0;
			bf.BlendOp				= AC_SRC_OVER;
			bf.SourceConstantAlpha	= 90;

			CPen penFrHot(PS_SOLID, 0, 0x00e9e9e9);//clr_resFace
			CPen *penSaved		= dc.SelectObject(&penFrHot);
			CBrush *brushSaved	= (CBrush*)dc.SelectStockObject(NULL_BRUSH);

			//CDIS_SELECTED,CDIS_GRAYED,CDIS_DISABLED,CDIS_CHECKED,CDIS_FOCUS,CDIS_DEFAULT,CDIS_HOT,CDIS_MARKED,CDIS_INDETERMINATE

			if (CDIS_HOT == pTBCD->nmcd.uItemState || CDIS_CHECKED + CDIS_HOT == pTBCD->nmcd.uItemState) {
				dc.SelectObject(&penFrHot);
				dc.RoundRect(r.left + 1, r.top + 1, r.right - 2, r.bottom - 1, 6, 4);
				AlphaBlend(dc.m_hDC, r.left + 2, r.top + 2, r.Width() - 4, 0.7 * r.Height() - 2, memdc, 0, 0, nW, nH, bf);
			}
			/*
			if (CDIS_CHECKED == pTBCD->nmcd.uItemState) {
				CPen penFrChecked(PS_SOLID,0,0x00808080);//clr_resDark
				dc.SelectObject(&penFrChecked);
				dc.RoundRect(r.left + 1, r.top + 1, r.right - 2, r.bottom - 1, 6, 4);
			}
			*/
			for (int j = 0; j < _countof(sep); j++) {
				GetItemRect(sep[j], &r);

				if (m_BackGroundbm.IsExtGradiendLoading()) {
					ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
					m_BackGroundbm.PaintExternalGradient(&dc, r, 21, s.nThemeBrightness, R, G, B);
				} else {
					ThemeRGB(50, 55, 60, R, G, B);
					ThemeRGB(20, 25, 30, R2, G2, B2);
					TRIVERTEX tv[2] = {
						{r.left, r.top, R * 256, G * 256, B * 256, 255 * 256},
						{r.right, r.bottom, R2 * 256, G2 * 256, B2 * 256, 255 * 256},
					};
					dc.GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);
				}
			}
			CRect r10; //SUB
			GetItemRect(10, &r10);

			CRect r12; //MUTE
			GetItemRect(12, &r12);

			if (bGPU && m_hDXVAIcon) {
				if (r10.right < r12.left - m_nDXVAIconWidth) {
					DrawIconEx(dc.m_hDC, r12.left - 8 - m_nDXVAIconWidth, r.CenterPoint().y - (m_nDXVAIconHeight / 2 + 1), m_hDXVAIcon, 0, 0, 0, NULL, DI_NORMAL);
				}
			}

			dc.SelectObject(&penSaved);
			dc.SelectObject(&brushSaved);
			dc.Detach();
			DeleteObject(memdc.SelectObject(bmOld));
			memdc.DeleteDC();
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
			lr = CDRF_DODEFAULT;

			lr |= CDRF_NOTIFYPOSTPAINT;
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPOSTPAINT:
			lr = CDRF_DODEFAULT;

			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);
			CRect r;

			for (int j = 0; j < _countof(sep); j++) {
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
		m_pMainFrame->PostMessage(WM_COMMAND, ID_PLAY_PLAYPAUSE);
	} else if (i == -1 || (GetButtonStyle(i) & (TBBS_SEPARATOR | TBBS_DISABLED))) {
		if (!m_pMainFrame->m_bFullScreen) {
			MapWindowPoints(m_pMainFrame, &point, 1);
			m_pMainFrame->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
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
	TOOLTIPTEXT* pTTT	= (TOOLTIPTEXT*)pNMHDR;
	static CString m_strTipText;

	OAFilterState fs = m_pMainFrame->GetMediaState();

	::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);

	if (pNMHDR->idFrom == ID_PLAY_PLAY) {
		(fs == State_Running) ? m_strTipText = ResStr(IDS_AG_PAUSE) : m_strTipText = ResStr(IDS_AG_PLAY);

	} else if (pNMHDR->idFrom == ID_PLAY_STOP) {
		m_strTipText = ResStr(IDS_AG_STOP) + L" | " + ResStr(IDS_AG_CLOSE);

	} else if (pNMHDR->idFrom == ID_PLAY_FRAMESTEP) {
		m_strTipText = ResStr(IDS_AG_STEP) + L" | " + ResStr(IDS_AG_JUMP_TO);

	} else if (pNMHDR->idFrom == ID_VOLUME_MUTE) {

		TBBUTTONINFO bi;
		bi.cbSize = sizeof(bi);
		bi.dwMask = TBIF_IMAGE;

		GetToolBarCtrl().GetButtonInfo(ID_VOLUME_MUTE, &bi);

		if (bi.iImage == 12) {
			m_strTipText = ResStr(ID_VOLUME_MUTE);
		} else if (bi.iImage == 13) {
			m_strTipText = ResStr(ID_VOLUME_MUTE_ON);
		} else if (bi.iImage == 14) {
			m_strTipText = ResStr(ID_VOLUME_MUTE_DISABLED);
		}

		int i = m_strTipText.Find('\n'); // TODO: remove it

		if (i > 0) {
			m_strTipText = m_strTipText.Left(i);
		}

	} else if (pNMHDR->idFrom == ID_FILE_OPENQUICK) {
		m_strTipText = ResStr(IDS_MPLAYERC_0) + L" | " + ResStr(IDS_RECENT_FILES);

	} else if (pNMHDR->idFrom == ID_NAVIGATE_SKIPFORWARD) {
		m_strTipText = ResStr(IDS_AG_NEXT);

	} else if (pNMHDR->idFrom == ID_NAVIGATE_SKIPBACK) {
		m_strTipText = ResStr(IDS_AG_PREVIOUS);

	} else if (pNMHDR->idFrom == ID_NAVIGATE_SUBTITLES) {
		m_strTipText = ResStr(IDS_AG_SUBTITLELANG) + L" | " + ResStr(IDS_AG_OPTIONS);

	} else if (pNMHDR->idFrom == ID_NAVIGATE_AUDIO) {
		m_strTipText = ResStr(IDS_AG_AUDIOLANG) + L" | " + ResStr(IDS_AG_OPTIONS);
	}

	pTTT->lpszText = m_strTipText.GetBuffer();
	*pResult = 0;

	return TRUE;
}

void CPlayerToolBar::OnRButtonDown(UINT nFlags, CPoint point)
{
	int Idx = getHitButtonIdx(point);

	if (Idx == 1) {
		m_pMainFrame->PostMessage(WM_COMMAND, ID_FILE_CLOSEPLAYLIST);
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
