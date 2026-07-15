/*
 * (C) 2026 see Authors.txt
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
#include "DarkTheme.h"
#include "../MainFrm.h" // AfxGetAppSettings()
#include "DSUtil/SysVersion.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>  // BP_CHECKBOX / BP_RADIOBUTTON / CBS_* / RBS_*
#include <commctrl.h> // SetWindowSubclass
#include <shellapi.h> // SHGetStockIconInfo (UAC shield)

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace DarkTheme
{
	namespace {
		// ---- undocumented uxtheme.dll ordinals (Windows 10 1809+) ----
		enum PreferredAppMode { APPMODE_DEFAULT, APPMODE_ALLOWDARK, APPMODE_FORCEDARK, APPMODE_FORCELIGHT, APPMODE_MAX };

		using fnAllowDarkModeForWindow          = bool (WINAPI*)(HWND, bool);
		using fnAllowDarkModeForApp             = bool (WINAPI*)(bool);              // 1809 (ord 135)
		using fnSetPreferredAppMode             = PreferredAppMode (WINAPI*)(PreferredAppMode); // 1903+ (ord 135)
		using fnFlushMenuThemes                 = void (WINAPI*)();
		using fnRefreshImmersiveColorPolicyState = void (WINAPI*)();

		fnAllowDarkModeForWindow           pAllowDarkModeForWindow = nullptr;
		fnAllowDarkModeForApp              pAllowDarkModeForApp    = nullptr;
		fnSetPreferredAppMode              pSetPreferredAppMode    = nullptr;
		fnFlushMenuThemes                  pFlushMenuThemes        = nullptr;
		fnRefreshImmersiveColorPolicyState pRefreshImmersiveColorPolicyState = nullptr;

		bool g_bApiChecked = false;
		bool g_bApiOk      = false;
		bool g_bAppAllowed = false;

		// cached theme brushes (owned by the OS at exit; never handed to MFC to delete)
		COLORREF g_clrFace = CLR_INVALID;
		COLORREF g_clrCtrl = CLR_INVALID;
		HBRUSH   g_hbrFace = nullptr;
		HBRUSH   g_hbrCtrl = nullptr;

		// Snapshot of the colours used by the R/G/B/Brightness sliders. Those sliders control the
		// theme, so painting them with the *live* colour makes the one being dragged change colour
		// under the cursor while the others wait for release. Instead they paint from this snapshot,
		// updated only when a drag ends (CommitThemeColors), so all four move together on release.
		COLORREF g_committedFace   = CLR_INVALID;
		COLORREF g_committedGroove = CLR_INVALID;
		COLORREF g_committedBorder = CLR_INVALID;

		bool Build1903orLater() {
			static const bool b = IsWindowsVersionOrGreaterBuild(HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), 18362);
			return b;
		}

		void LoadApi() {
			if (g_bApiChecked) {
				return;
			}
			g_bApiChecked = true;

			if (!SysVersion::IsWin10v1809orLater()) {
				return;
			}

			HMODULE hUx = GetModuleHandleW(L"uxtheme.dll");
			if (!hUx) {
				hUx = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			}
			if (!hUx) {
				return;
			}

			pAllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUx, MAKEINTRESOURCEA(133));
			if (Build1903orLater()) {
				pSetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUx, MAKEINTRESOURCEA(135));
			} else {
				pAllowDarkModeForApp = (fnAllowDarkModeForApp)GetProcAddress(hUx, MAKEINTRESOURCEA(135));
			}
			pFlushMenuThemes                  = (fnFlushMenuThemes)GetProcAddress(hUx, MAKEINTRESOURCEA(136));
			pRefreshImmersiveColorPolicyState = (fnRefreshImmersiveColorPolicyState)GetProcAddress(hUx, MAKEINTRESOURCEA(104));

			g_bApiOk = pAllowDarkModeForWindow && (pSetPreferredAppMode || pAllowDarkModeForApp);
		}

		void EnsureBrushes() {
			const COLORREF clrFace = ThemeRGB(22, 27, 32);
			const COLORREF clrCtrl = ThemeRGB(10, 14, 18);
			if (clrFace != g_clrFace || !g_hbrFace) {
				if (g_hbrFace) {
					::DeleteObject(g_hbrFace);
				}
				g_hbrFace = ::CreateSolidBrush(clrFace);
				g_clrFace = clrFace;
			}
			if (clrCtrl != g_clrCtrl || !g_hbrCtrl) {
				if (g_hbrCtrl) {
					::DeleteObject(g_hbrCtrl);
				}
				g_hbrCtrl = ::CreateSolidBrush(clrCtrl);
				g_clrCtrl = clrCtrl;
			}
		}

		// Group boxes (BS_GROUPBOX) are not repainted by Windows' native dark mode
		// (their frame and caption stay drawn with the classic black-on-light system
		// colors) and they do not emit NM_CUSTOMDRAW, so we subclass them and paint
		// the frame + caption ourselves.
		const UINT_PTR kGroupBoxSubclassId = 1;

		LRESULT CALLBACK GroupBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_ERASEBKGND: {
					CDC* pDC = CDC::FromHandle(reinterpret_cast<HDC>(wParam));
					CRect rc;
					::GetClientRect(hWnd, &rc);
					pDC->FillSolidRect(rc, FaceColor());
					return 1;
				}
				case WM_PAINT: {
					PAINTSTRUCT ps;
					CDC* pDC = CDC::FromHandle(::BeginPaint(hWnd, &ps));

					CRect rc;
					::GetClientRect(hWnd, &rc);
					pDC->FillSolidRect(rc, FaceColor());

					CString text;
					const int len = ::GetWindowTextLengthW(hWnd);
					if (len > 0) {
						::GetWindowTextW(hWnd, text.GetBuffer(len + 1), len + 1);
						text.ReleaseBuffer();
					}

					HFONT hFont = reinterpret_cast<HFONT>(::SendMessageW(hWnd, WM_GETFONT, 0, 0));
					CFont* pOldFont = hFont ? pDC->SelectObject(CFont::FromHandle(hFont)) : nullptr;

					const int textH = pDC->GetTextExtent(L"Ag").cy;

					CRect rcFrame = rc;
					rcFrame.top += textH / 2;
					CBrush brFrame(ThemeRGB(70, 75, 80));
					pDC->FrameRect(rcFrame, &brFrame);

					if (!text.IsEmpty()) {
						const int x = rc.left + 9;
						const CSize ext = pDC->GetTextExtent(text);
						CRect rcGap(x - 2, rc.top, x + ext.cx + 2, rc.top + textH);
						pDC->FillSolidRect(rcGap, FaceColor()); // break the frame behind the caption

						const bool disabled = (::GetWindowLongW(hWnd, GWL_STYLE) & WS_DISABLED) != 0;
						pDC->SetBkMode(TRANSPARENT);
						pDC->SetTextColor(disabled ? RGB(110, 115, 120) : TextColor());
						CRect rcLabel(x, rc.top, rc.right - 4, rc.top + textH);
						pDC->DrawTextW(text, rcLabel, DT_LEFT | DT_SINGLELINE | DT_TOP);
					}

					if (pOldFont) {
						pDC->SelectObject(pOldFont);
					}

					::EndPaint(hWnd, &ps);
					return 0;
				}
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, GroupBoxSubclassProc, kGroupBoxSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// Push buttons: Windows 11 draws them rounded, optionally with an icon, but the dark
		// BUTTON visual style ("DarkMode_Explorer") is flat and drops the icon, while leaving
		// them un-themed keeps them light. So we owner-draw them: a rounded dark face (with
		// hover/pressed shades from the themed palette), the shared border (an accent for
		// the default button), the native icon + text, and a
		// focus rectangle. The default window proc still runs all the click/keyboard logic; we
		// only take over painting. dwRefData carries the hot (hover) state.
		const UINT_PTR kButtonSubclassId = 9;

		LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR dwHot) {
			switch (msg) {
				case WM_MOUSEMOVE:
					if (!dwHot) {
						TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
						::TrackMouseEvent(&tme);
						SetWindowSubclass(hWnd, ButtonSubclassProc, kButtonSubclassId, 1);
						::InvalidateRect(hWnd, nullptr, FALSE);
					}
					break;
				case WM_MOUSELEAVE:
					SetWindowSubclass(hWnd, ButtonSubclassProc, kButtonSubclassId, 0);
					::InvalidateRect(hWnd, nullptr, FALSE);
					break;
				case WM_ENABLE:
					// The property sheet enables/disables Apply as pages are modified. On that
					// transition the default button proc repaints itself light, bypassing our
					// owner-draw; swallow it and force our own dark repaint instead.
					::InvalidateRect(hWnd, nullptr, FALSE);
					return 0;
				case WM_ERASEBKGND:
					return 1;
				case WM_PAINT: {
					PAINTSTRUCT ps;
					HDC hdc = ::BeginPaint(hWnd, &ps);
					CDC* pDC = CDC::FromHandle(hdc);

					CRect rc;
					::GetClientRect(hWnd, &rc);

					const LONG    style    = ::GetWindowLongW(hWnd, GWL_STYLE);
					const LRESULT bst      = ::SendMessageW(hWnd, BM_GETSTATE, 0, 0);
					const bool    disabled = (style & WS_DISABLED) != 0;
					const bool    pressed  = (bst & BST_PUSHED) != 0;
					const bool    focused  = (bst & BST_FOCUS) != 0;
					const bool    hot      = dwHot != 0 && !disabled;
					const bool    isDef    = (style & BS_TYPEMASK) == BS_DEFPUSHBUTTON;

					pDC->FillSolidRect(rc, FaceColor()); // dialog bg behind the rounded corners

					const COLORREF face = disabled ? ThemeRGB(38, 43, 48)
										: pressed  ? ThemeRGB(36, 41, 46)
										: hot      ? ThemeRGB(62, 69, 76)
												   : ThemeRGB(50, 56, 62);
					const COLORREF border = disabled ? ThemeRGB(60, 65, 70)
										  : isDef    ? RGB(76, 194, 255)
													 : ThemeRGB(84, 90, 96);

					CBrush brFace(face);
					CPen   penBd(PS_SOLID, 1, border);
					HGDIOBJ ob = pDC->SelectObject(brFace);
					HGDIOBJ op = pDC->SelectObject(penBd);
					pDC->RoundRect(rc.left, rc.top, rc.right, rc.bottom, 8, 8);
					pDC->SelectObject(ob);
					pDC->SelectObject(op);

					CString text;
					const int len = ::GetWindowTextLengthW(hWnd);
					if (len > 0) {
						::GetWindowTextW(hWnd, text.GetBuffer(len + 1), len + 1);
						text.ReleaseBuffer();
					}
					HICON hIcon = reinterpret_cast<HICON>(::SendMessageW(hWnd, BM_GETIMAGE, IMAGE_ICON, 0));
					// The UAC elevation shield (BCM_SETSHIELD) is drawn internally by the button and
					// is NOT returned by BM_GETIMAGE, so owner-drawing dropped it. When the button is
					// flagged (MarkUacShield), draw the system shield ourselves.
					if (::GetPropW(hWnd, L"MPC_UAC_SHIELD")) {
						static HICON s_hShield = nullptr; // cached once for the process lifetime
						if (!s_hShield) {
							// Load the large (32px) shield rather than the small (16px) one: we scale
							// it to the DPI-adjusted size below, and downscaling 32->size stays crisp
							// where upscaling a 16px source would blur (visible at 150-200% DPI).
							SHSTOCKICONINFO sii = { sizeof(sii) };
							if (SUCCEEDED(::SHGetStockIconInfo(SIID_SHIELD, SHGSI_ICON, &sii))) {
								s_hShield = sii.hIcon;
							} else {
								s_hShield = ::LoadIconW(nullptr, IDI_SHIELD);
							}
						}
						if (s_hShield) {
							hIcon = s_hShield;
						}
					}

					HFONT hFont = reinterpret_cast<HFONT>(::SendMessageW(hWnd, WM_GETFONT, 0, 0));
					CFont* pOldFont = hFont ? pDC->SelectObject(CFont::FromHandle(hFont)) : nullptr;
					pDC->SetBkMode(TRANSPARENT);
					pDC->SetTextColor(disabled ? RGB(120, 125, 130) : TextColor());

					// Scale the icon (UAC shield) by the DC's DPI so it isn't a tiny 16px glyph on a
					// button that Windows has scaled up at 150-200% display scaling.
					const int icon = hIcon ? ::MulDiv(16, pDC->GetDeviceCaps(LOGPIXELSY), 96) : 0;
					if (!text.IsEmpty() && (style & BS_MULTILINE)) {
						// Multi-line captions (e.g. the "AVI Splitter\nconfiguration" filter buttons)
						// must wrap: a single-line draw collapses the line break and overflows the
						// button. Word-wrap and centre the text block vertically.
						CRect rt(rc.left + 4, rc.top, rc.right - 4, rc.bottom);
						CRect calc = rt;
						pDC->DrawTextW(text, calc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
						const int oy = (rc.Height() - calc.Height()) / 2;
						rt.top = rc.top + (oy > 0 ? oy : 0);
						pDC->DrawTextW(text, rt, DT_CENTER | DT_WORDBREAK);
					} else {
						// Measure the *rendered* width with DT_CALCRECT so the "&" accelerator prefix
						// isn't counted (GetTextExtent includes it, which shifted the caption left).
						int textW = 0;
						if (!text.IsEmpty()) {
							CRect calc(0, 0, 0, 0);
							pDC->DrawTextW(text, calc, DT_SINGLELINE | DT_CALCRECT);
							textW = calc.Width();
						}
						const int gap  = (hIcon && !text.IsEmpty()) ? 4 : 0;
						int x = rc.left + (rc.Width() - (icon + gap + textW)) / 2;
						const int cy = rc.top + rc.Height() / 2;
						if (hIcon) {
							::DrawIconEx(hdc, x, cy - icon / 2, hIcon, icon, icon, 0, nullptr, DI_NORMAL);
							x += icon + gap;
						}
						if (!text.IsEmpty()) {
							CRect rt(x, rc.top, rc.right, rc.bottom);
							pDC->DrawTextW(text, rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
						}
					}
					if (pOldFont) {
						pDC->SelectObject(pOldFont);
					}

					if (focused && !disabled) {
						CRect fr = rc;
						fr.DeflateRect(3, 3);
						pDC->DrawFocusRect(fr);
					}
					::EndPaint(hWnd, &ps);
					return 0;
				}
				case WM_NCDESTROY:
					::RemovePropW(hWnd, L"MPC_UAC_SHIELD");
					RemoveWindowSubclass(hWnd, ButtonSubclassProc, kButtonSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// The native LVS_EX_GRIDLINES are drawn with a light system colour that dark
		// mode does not change. We strip that style (see ThemeControl) and draw our own
		// grid, a shade darker than the control borders, over the default paint.
		void DrawListGridlines(HWND hWnd) {
			if ((GetWindowLongW(hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT) {
				return;
			}
			const int count = static_cast<int>(::SendMessageW(hWnd, LVM_GETITEMCOUNT, 0, 0));
			if (count <= 0) {
				return;
			}

			HDC hdc = ::GetDC(hWnd);
			if (!hdc) {
				return;
			}

			CRect rcClient;
			::GetClientRect(hWnd, &rcClient);

			HWND hHeader = reinterpret_cast<HWND>(::SendMessageW(hWnd, LVM_GETHEADER, 0, 0));
			int headerH = 0;
			if (hHeader) {
				RECT rh;
				::GetWindowRect(hHeader, &rh);
				headerH = rh.bottom - rh.top;
			}

			RECT rcLast{}; rcLast.left = LVIR_BOUNDS;
			::SendMessageW(hWnd, LVM_GETITEMRECT, count - 1, reinterpret_cast<LPARAM>(&rcLast));
			const int gridBottom = (rcLast.bottom < rcClient.bottom) ? rcLast.bottom : rcClient.bottom;

			HPEN pen = ::CreatePen(PS_SOLID, 1, GridlineColor());
			HGDIOBJ oldPen = ::SelectObject(hdc, pen);

			// horizontal line under each visible row
			const int top = static_cast<int>(::SendMessageW(hWnd, LVM_GETTOPINDEX, 0, 0));
			const int per = static_cast<int>(::SendMessageW(hWnd, LVM_GETCOUNTPERPAGE, 0, 0));
			for (int i = top; i <= top + per && i < count; ++i) {
				RECT r{}; r.left = LVIR_BOUNDS;
				::SendMessageW(hWnd, LVM_GETITEMRECT, i, reinterpret_cast<LPARAM>(&r));
				::MoveToEx(hdc, rcClient.left, r.bottom - 1, nullptr);
				::LineTo(hdc, rcClient.right, r.bottom - 1);
			}

			// vertical line at each column's right edge, through the item rows only. Header_GetItemRect
			// is relative to the header window, which is scrolled left when the list is scrolled
			// horizontally; map the header's client origin into the list-view's client so the lines
			// follow the scroll (otherwise they stay at the unscrolled column positions).
			if (hHeader) {
				POINT hdrOrg = { 0, 0 };
				::ClientToScreen(hHeader, &hdrOrg);
				::ScreenToClient(hWnd, &hdrOrg); // hdrOrg.x = -horizontal scroll offset
				const int cols = static_cast<int>(::SendMessageW(hHeader, HDM_GETITEMCOUNT, 0, 0));
				for (int c = 0; c < cols; ++c) {
					RECT hr{};
					if (Header_GetItemRect(hHeader, c, &hr)) {
						const int x = hdrOrg.x + hr.right - 1;
						::MoveToEx(hdc, x, headerH, nullptr);
						::LineTo(hdc, x, gridBottom);
					}
				}
			}

			::SelectObject(hdc, oldPen);
			::DeleteObject(pen);
			::ReleaseDC(hWnd, hdc);
		}

		// A report list-view draws faint column separators (and, past the last item, a light
		// "empty" strip) that the light system theme provides; on the dark background they show as
		// a bright vertical line (e.g. the empty Organize Favorites list) or a light band. Overpaint
		// the area from the last item's bottom down to the client bottom with the dark face colour.
		// For an empty list that is the whole body, which covers the stray column separator.
		void FillListEmptyArea(HWND hWnd) {
			if ((GetWindowLongW(hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT) {
				return;
			}
			CRect rcClient;
			::GetClientRect(hWnd, &rcClient);

			int top = rcClient.top;
			const int count = static_cast<int>(::SendMessageW(hWnd, LVM_GETITEMCOUNT, 0, 0));
			if (count > 0) {
				RECT rcLast{}; rcLast.left = LVIR_BOUNDS;
				::SendMessageW(hWnd, LVM_GETITEMRECT, count - 1, reinterpret_cast<LPARAM>(&rcLast));
				top = rcLast.bottom;
			} else if (HWND hHeader = reinterpret_cast<HWND>(::SendMessageW(hWnd, LVM_GETHEADER, 0, 0))) {
				if (::IsWindowVisible(hHeader)) {
					RECT rh; ::GetClientRect(hHeader, &rh);
					top = rcClient.top + rh.bottom;
				}
			}
			if (top < rcClient.bottom) {
				if (HDC hdc = ::GetDC(hWnd)) {
					RECT rcEmpty = { rcClient.left, top, rcClient.right, rcClient.bottom };
					HBRUSH br = ::CreateSolidBrush(FaceColor());
					::FillRect(hdc, &rcEmpty, br);
					::DeleteObject(br);
					::ReleaseDC(hWnd, hdc);
				}
			}
		}

		// A list-view's column header (SysHeader32) is not darkened by SetWindowTheme
		// and it sends its NM_CUSTOMDRAW to the list-view (its parent), not to the
		// dialog, so we subclass the list-view and paint the header ourselves. When the
		// list originally had grid lines, dwRefData is 1 and we also draw a dark grid.
		const UINT_PTR kListViewSubclassId = 2;

		LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR dwRefData) {
			if (msg == WM_CTLCOLOREDIT) {
				// The in-place label-edit control a list-view creates for renaming an item (e.g. the
				// Organize Favorites list, the Subresync grid) is a child of the list-view, so its
				// WM_CTLCOLOREDIT is sent here, not to the dialog. Give it the dark edit colours so it
				// isn't a bright white box hovering over the dark list.
				CDC* pDC = CDC::FromHandle(reinterpret_cast<HDC>(wParam));
				if (HBRUSH hbr = OnCtlColor(pDC, CTLCOLOR_EDIT)) {
					return reinterpret_cast<LRESULT>(hbr);
				}
			}
			if (msg == WM_PAINT) {
				const LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
				FillListEmptyArea(hWnd);      // dark over the light empty strip / stray column separator
				if (dwRefData) {
					DrawListGridlines(hWnd);  // dark grid (only for lists that originally had grid lines)
				}
				return res;
			}
			if (msg == WM_HSCROLL && dwRefData) {
				// A horizontal scroll bit-blts the client, smearing our overpainted vertical grid lines
				// (they end up doubled / misaligned, e.g. the Keys list). Force a full repaint so they
				// are redrawn at the new offset. Only needed for grid lists (double-buffered → no flicker).
				const LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
				::InvalidateRect(hWnd, nullptr, FALSE);
				return res;
			}
			if (msg == WM_NOTIFY) {
				NMHDR* pNM = reinterpret_cast<NMHDR*>(lParam);
				HWND hHeader = reinterpret_cast<HWND>(::SendMessageW(hWnd, LVM_GETHEADER, 0, 0));
				if (pNM && pNM->hwndFrom == hHeader
						&& (pNM->code == HDN_ITEMCHANGEDA || pNM->code == HDN_ITEMCHANGEDW
							|| pNM->code == HDN_ENDTRACKA || pNM->code == HDN_ENDTRACKW)) {
					// A column resize — dragging a header divider, or double-clicking the divider
					// "gripper" to auto-size the column — bit-blts the rows to the right of that divider,
					// which smears our hand-drawn grid lines / dark header-fill (they double up or stay at
					// the old offsets). Let the list process the size change, then force a clean repaint.
					const LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
					::InvalidateRect(hWnd, nullptr, FALSE);
					return res;
				}
				if (pNM && pNM->code == NM_CUSTOMDRAW && pNM->hwndFrom == hHeader) {
					LPNMCUSTOMDRAW p = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
					switch (p->dwDrawStage) {
						case CDDS_PREPAINT: {
							CDC* pDC = CDC::FromHandle(p->hdc);
							CRect rcHdr;
							::GetClientRect(hHeader, &rcHdr); // p->rc is often empty for headers
							pDC->FillSolidRect(rcHdr, FaceColor());
							return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
						}
						case CDDS_ITEMPREPAINT: {
							CDC* pDC = CDC::FromHandle(p->hdc);
							CRect rc(p->rc);
							pDC->FillSolidRect(rc, FaceColor());
							pDC->FillSolidRect(rc.right - 1, rc.top, 1, rc.Height(), ThemeRGB(70, 75, 80));
							pDC->FillSolidRect(rc.left, rc.bottom - 1, rc.Width(), 1, ThemeRGB(70, 75, 80));

							wchar_t buf[256] = {};
							HDITEMW hdi = {};
							hdi.mask = HDI_TEXT | HDI_FORMAT;
							hdi.pszText = buf;
							hdi.cchTextMax = _countof(buf) - 1;
							::SendMessageW(hHeader, HDM_GETITEMW, p->dwItemSpec, reinterpret_cast<LPARAM>(&hdi));

							HFONT hFont = reinterpret_cast<HFONT>(::SendMessageW(hHeader, WM_GETFONT, 0, 0));
							CFont* pOldFont = hFont ? pDC->SelectObject(CFont::FromHandle(hFont)) : nullptr;

							pDC->SetBkMode(TRANSPARENT);
							pDC->SetTextColor(TextColor());
							CRect rcText = rc;
							rcText.left += 6;
							rcText.right -= 6;
							const UINT just = hdi.fmt & HDF_JUSTIFYMASK;
							const UINT fmt = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS
								| (just == HDF_CENTER ? DT_CENTER : just == HDF_RIGHT ? DT_RIGHT : DT_LEFT);
							if (buf[0] && rcText.right > rcText.left) {
								pDC->DrawTextW(buf, rcText, fmt);
							}

							if (pOldFont) {
								pDC->SelectObject(pOldFont);
							}
							return CDRF_SKIPDEFAULT;
						}
						case CDDS_POSTPAINT: {
							// Windows repaints the empty area past the last column with the
							// light theme background; overpaint it dark so it does not show
							// up as a white "extra column".
							const int count = static_cast<int>(::SendMessageW(hHeader, HDM_GETITEMCOUNT, 0, 0));
							if (count > 0) {
								RECT rcLast{};
								if (::SendMessageW(hHeader, HDM_GETITEMRECT, count - 1, reinterpret_cast<LPARAM>(&rcLast))) {
									CRect rcHdr;
									::GetClientRect(hHeader, &rcHdr);
									if (rcLast.right < rcHdr.right) {
										CDC* pDC = CDC::FromHandle(p->hdc);
										pDC->FillSolidRect(rcLast.right, rcHdr.top, rcHdr.right - rcLast.right, rcHdr.Height(), FaceColor());
									}
								}
							}
							return CDRF_DODEFAULT;
						}
					}
					return CDRF_DODEFAULT;
				}
			} else if (msg == WM_NCDESTROY) {
				RemoveWindowSubclass(hWnd, ListViewSubclassProc, kListViewSubclassId);
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// Draws a small filled triangle (spin-button arrow) centred in rc.
		void DrawArrow(CDC* pDC, CRect rc, bool up, COLORREF clr) {
			const int w = 7;                     // arrow width in px
			const int h = 4;                     // arrow height in px
			const int cx = rc.left + rc.Width() / 2;
			const int cy = rc.top + rc.Height() / 2;
			POINT pts[3];
			if (up) {
				pts[0] = { cx,          cy - h / 2 - 1 };
				pts[1] = { cx - w / 2,  cy + h / 2 };
				pts[2] = { cx + w / 2 + 1, cy + h / 2 };
			} else {
				pts[0] = { cx - w / 2,  cy - h / 2 };
				pts[1] = { cx + w / 2 + 1, cy - h / 2 };
				pts[2] = { cx,          cy + h / 2 + 1 };
			}
			CBrush br(clr);
			CBrush* pOldBr = pDC->SelectObject(&br);
			CPen pen(PS_SOLID, 1, clr);
			CPen* pOldPen = pDC->SelectObject(&pen);
			pDC->SetPolyFillMode(WINDING);
			pDC->Polygon(pts, 3);
			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBr);
		}

		// Same as DrawArrow but pointing left/right (for horizontal up-down controls).
		void DrawArrowLR(CDC* pDC, CRect rc, bool left, COLORREF clr) {
			const int w = 7;                     // arrow length along its axis
			const int h = 4;                     // arrow half-thickness base
			const int cx = rc.left + rc.Width() / 2;
			const int cy = rc.top + rc.Height() / 2;
			POINT pts[3];
			if (left) {
				pts[0] = { cx - h / 2 - 1, cy };
				pts[1] = { cx + h / 2,     cy - w / 2 };
				pts[2] = { cx + h / 2,     cy + w / 2 + 1 };
			} else {
				pts[0] = { cx - h / 2,     cy - w / 2 };
				pts[1] = { cx + h / 2 + 1, cy };
				pts[2] = { cx - h / 2,     cy + w / 2 + 1 };
			}
			CBrush br(clr);
			CBrush* pOldBr = pDC->SelectObject(&br);
			CPen pen(PS_SOLID, 1, clr);
			CPen* pOldPen = pDC->SelectObject(&pen);
			pDC->SetPolyFillMode(WINDING);
			pDC->Polygon(pts, 3);
			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBr);
		}

		// Spin (up-down) controls are drawn by Windows as two light 3D buttons that
		// native dark mode does not darken, so we owner-draw them: a dark button face
		// with a light-grey arrow in each half and a subtle border/divider.
		const UINT_PTR kSpinSubclassId = 4;

		LRESULT CALLBACK SpinSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_ERASEBKGND:
					return 1;
				case WM_PAINT: {
					PAINTSTRUCT ps;
					CDC* pDC = CDC::FromHandle(::BeginPaint(hWnd, &ps));

					CRect rc;
					::GetClientRect(hWnd, &rc);

					const COLORREF clrFace   = ThemeRGB(38, 44, 50);
					const COLORREF clrBorder = ThemeRGB(70, 75, 80);
					const COLORREF clrArrow   = RGB(170, 175, 180); // fixed: foreground glyph never tints

					pDC->FillSolidRect(rc, clrFace);

					if (::GetWindowLongW(hWnd, GWL_STYLE) & UDS_HORZ) {
						// Two side-by-side buttons with left/right arrows.
						const int mid = rc.left + rc.Width() / 2;
						CRect rcL(rc.left, rc.top, mid, rc.bottom);
						CRect rcR(mid, rc.top, rc.right, rc.bottom);
						DrawArrowLR(pDC, rcL, true,  clrArrow);
						DrawArrowLR(pDC, rcR, false, clrArrow);
						pDC->Draw3dRect(rc, clrBorder, clrBorder);            // outer border
						pDC->FillSolidRect(mid, rc.top, 1, rc.Height(), clrBorder); // divider
					} else {
						// Two stacked buttons with up/down arrows.
						const int mid = rc.top + rc.Height() / 2;
						CRect rcUp(rc.left, rc.top, rc.right, mid);
						CRect rcDn(rc.left, mid, rc.right, rc.bottom);
						DrawArrow(pDC, rcUp, true,  clrArrow);
						DrawArrow(pDC, rcDn, false, clrArrow);
						pDC->Draw3dRect(rc, clrBorder, clrBorder);            // outer border
						pDC->FillSolidRect(rc.left, mid, rc.Width(), 1, clrBorder); // divider
					}

					::EndPaint(hWnd, &ps);
					return 0;
				}
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, SpinSubclassProc, kSpinSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// Some trackbars stop honouring the NM_CUSTOMDRAW channel colour after a
		// programmatic value change (e.g. the "Reset" button on the subtitle Default Style
		// page repaints them with the light theme channel). For those we fully owner-draw
		// the trackbar here instead of relying on custom draw: dark background, dark groove
		// and a celeste thumb, painted deterministically in WM_PAINT.
		const UINT_PTR kTrackbarSubclassId = 6;

		LRESULT CALLBACK TrackbarSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR dwData) {
			switch (msg) {
				case WM_ERASEBKGND:
					return 1; // background is painted in WM_PAINT
				case WM_PAINT: {
					PAINTSTRUCT ps;
					CDC* pDC = CDC::FromHandle(::BeginPaint(hWnd, &ps));

					CRect rc;
					::GetClientRect(hWnd, &rc);

					// The R/G/B/Brightness sliders (dwData != 0) paint from the committed snapshot so
					// the one being dragged doesn't recolour live; other sliders use the live colour.
					const bool frozen = (dwData != 0 && g_committedFace != CLR_INVALID);
					const COLORREF face   = frozen ? g_committedFace   : FaceColor();
					const COLORREF groove = frozen ? g_committedGroove : ThemeRGB(10, 14, 18);
					const COLORREF border = frozen ? g_committedBorder : ThemeRGB(60, 65, 70);
					pDC->FillSolidRect(rc, face);

					RECT rcCh{};
					::SendMessageW(hWnd, TBM_GETCHANNELRECT, 0, reinterpret_cast<LPARAM>(&rcCh));
					CRect ch(rcCh);
					pDC->FillSolidRect(ch, groove);      // dark groove
					pDC->Draw3dRect(ch, border, border); // subtle border

					RECT rcTh{};
					::SendMessageW(hWnd, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&rcTh));
					CRect th(rcTh);
					// Thumb states: dragged (pressed) > hovered > normal, so the slider gives the same
					// visual feedback as a native one. Disabled overrides all.
					const bool disabled = (::GetWindowLongW(hWnd, GWL_STYLE) & WS_DISABLED) != 0;
					const bool pressed  = ::GetPropW(hWnd, L"MPC_TB_PRESSED") != nullptr;
					const bool hover    = ::GetPropW(hWnd, L"MPC_TB_HOVER")   != nullptr;
					COLORREF thumbClr;
					if (disabled)     { thumbClr = ThemeRGB(90, 95, 100); }
					else if (pressed) { thumbClr = RGB(150, 224, 255); } // brightest while dragging
					else if (hover)   { thumbClr = RGB(115, 210, 255); } // lighter on hover
					else              { thumbClr = RGB(76, 194, 255); }  // celeste
					pDC->FillSolidRect(th, thumbClr);

					::EndPaint(hWnd, &ps);
					return 0;
				}
				case WM_MOUSEMOVE: {
					// Hover only when the cursor is over the thumb itself, not the whole track.
					RECT rcTh{};
					::SendMessageW(hWnd, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&rcTh));
					const POINT pt = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
					const bool over = ::PtInRect(&rcTh, pt) != FALSE;
					if (over != (::GetPropW(hWnd, L"MPC_TB_HOVER") != nullptr)) {
						if (over) { ::SetPropW(hWnd, L"MPC_TB_HOVER", reinterpret_cast<HANDLE>(1)); }
						else      { ::RemovePropW(hWnd, L"MPC_TB_HOVER"); }
						::InvalidateRect(hWnd, nullptr, FALSE);
					}
					if (over) {
						TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
						::TrackMouseEvent(&tme); // so we get WM_MOUSELEAVE to clear the hover
					}
					break; // pass through so the trackbar still handles the move
				}
				case WM_MOUSELEAVE:
					if (::GetPropW(hWnd, L"MPC_TB_HOVER")) {
						::RemovePropW(hWnd, L"MPC_TB_HOVER");
						::InvalidateRect(hWnd, nullptr, FALSE);
					}
					break;
				case WM_LBUTTONDOWN:
					::SetPropW(hWnd, L"MPC_TB_PRESSED", reinterpret_cast<HANDLE>(1));
					::InvalidateRect(hWnd, nullptr, FALSE);
					break; // pass through so the trackbar drags
				case WM_LBUTTONUP:
					if (::GetPropW(hWnd, L"MPC_TB_PRESSED")) {
						::RemovePropW(hWnd, L"MPC_TB_PRESSED");
						::InvalidateRect(hWnd, nullptr, FALSE);
					}
					break;
				case WM_NCDESTROY:
					::RemovePropW(hWnd, L"MPC_TB_HOVER");
					::RemovePropW(hWnd, L"MPC_TB_PRESSED");
					RemoveWindowSubclass(hWnd, TrackbarSubclassProc, kTrackbarSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// Subclass for auxiliary top-level dialogs (opened from the Options pages, e.g. the
		// "Add filter" choosers): paints the dialog background and control colours dark via
		// WM_CTLCOLOR* / WM_ERASEBKGND, mirroring what CPPageBase does for the pages.
		const UINT_PTR kDialogSubclassId = 7;

		LRESULT CALLBACK DialogSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_CTLCOLORMSGBOX:
				case WM_CTLCOLOREDIT:
				case WM_CTLCOLORLISTBOX:
				case WM_CTLCOLORBTN:
				case WM_CTLCOLORDLG:
				case WM_CTLCOLORSCROLLBAR:
				case WM_CTLCOLORSTATIC: {
					CDC* pDC = CDC::FromHandle(reinterpret_cast<HDC>(wParam));
					// WM_CTLCOLOR* run consecutively starting at WM_CTLCOLORMSGBOX, matching
					// the CTLCOLOR_* constants in the same order.
					if (HBRUSH hbr = OnCtlColor(pDC, msg - WM_CTLCOLORMSGBOX)) {
						return reinterpret_cast<LRESULT>(hbr);
					}
					break;
				}
				case WM_NOTIFY: {
					// Radio buttons keep black text under native dark mode (checkboxes go light);
					// draw radios/checkboxes (and any sliders) ourselves via NM_CUSTOMDRAW, like the
					// Options pages do, so their captions match.
					NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
					if (pNMHDR && pNMHDR->code == NM_CUSTOMDRAW) {
						LRESULT result = 0;
						if (TrackbarCustomDraw(pNMHDR, &result) || ButtonCustomDraw(pNMHDR, &result)) {
							return result;
						}
					}
					break;
				}
				case WM_ERASEBKGND: {
					CDC* pDC = CDC::FromHandle(reinterpret_cast<HDC>(wParam));
					CRect rc;
					::GetClientRect(hWnd, &rc);
					pDC->FillSolidRect(rc, FaceColor());
					return 1;
				}
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, DialogSubclassProc, kDialogSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// Edits, tree-views and list-views keep a light sunken border under native dark
		// mode. We repaint just the outer border frame with the shared dark border
		// colour (not the whole non-client ring, so a control's scrollbars are left
		// alone), matching the spin buttons, colour wells and group boxes.
		const UINT_PTR kBorderSubclassId = 5;

		LRESULT CALLBACK BorderSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_NCPAINT: {
					// Let the default proc paint the non-client (scroll bars + its own border), then
					// overpaint our dark frame on top. Drawing over whatever the theme rendered keeps the
					// border dark on every system — skipping / clipping the default looked fine locally but
					// rendered a WHITE border on other machines (nothing dark was painted over the edge).
					const LRESULT r = DefSubclassProc(hWnd, msg, wParam, lParam);
					RECT wr;
					::GetWindowRect(hWnd, &wr);
					POINT org = { 0, 0 };
					::ClientToScreen(hWnd, &org);
					const int edge = org.y - wr.top; // NC border: 0 = none, 1 = WS_BORDER, 2 = client edge
					// Only overpaint when the control actually reserves a non-client border. A borderless
					// control (NOT WS_BORDER, e.g. the MediaInfo edit) has edge 0; a 1px frame there lands in
					// the *client*, which the edit's ScrollWindowEx then drags into the middle of the text on
					// scroll. Skip it — such controls stay borderless, as their template intends.
					if (edge >= 1) {
						if (HDC hdc = ::GetWindowDC(hWnd)) {
							RECT rc = { 0, 0, wr.right - wr.left, wr.bottom - wr.top };
							HBRUSH br = ::CreateSolidBrush(CtrlBorderColor());
							for (int k = 0; k < edge; ++k) {
								::FrameRect(hdc, &rc, br);
								::InflateRect(&rc, -1, -1);
							}
							::DeleteObject(br);
							::ReleaseDC(hWnd, hdc);
						}
					}
					return r;
				}
				case WM_PAINT:
				case WM_SETFOCUS:
				case WM_KILLFOCUS: {
					// Combo boxes draw their border in the CLIENT area (not the non-client), and repaint it
					// in a light 'focused' state on focus — which the WM_NCPAINT overpaint above never
					// covers, so an active combo showed a white border (Add-to-Favorites). Only for combos,
					// overpaint the client-edge frame dark after each WM_PAINT, and force a repaint on focus
					// change. Gated on the ComboBox class so edits / lists / trees / statics are untouched.
					wchar_t cls[16] = {};
					::GetClassNameW(hWnd, cls, _countof(cls));
					if (_wcsicmp(cls, L"ComboBox") != 0) {
						break; // not a combo — fall through to the default handling
					}
					const LRESULT r = DefSubclassProc(hWnd, msg, wParam, lParam);
					if (msg == WM_PAINT) {
						if (HDC hdc = ::GetDC(hWnd)) {
							RECT rc;
							::GetClientRect(hWnd, &rc);
							HBRUSH br = ::CreateSolidBrush(CtrlBorderColor());
							::FrameRect(hdc, &rc, br);
							::DeleteObject(br);
							::ReleaseDC(hWnd, hdc);
						}
					} else {
						::InvalidateRect(hWnd, nullptr, FALSE); // focus changed — repaint so the frame stays dark
					}
					return r;
				}
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, BorderSubclassProc, kBorderSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		void ApplyDarkBorder(HWND hCtrl) {
			SetWindowSubclass(hCtrl, BorderSubclassProc, kBorderSubclassId, 0);
			::SetWindowPos(hCtrl, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // force NC repaint
		}

		// The colour-well buttons on the Interface / OSD / Subtitle-style pages are push buttons
		// that each page paints with the selected colour via NM_CUSTOMDRAW (its OnCustomDrawBtns).
		// Owner-drawing them here would steal WM_PAINT and show the button caption ("B", "O", ...)
		// instead of the colour swatch, so ThemeControl skips them and lets the page draw them.
		bool IsOwnerColorButton(HWND hCtrl) {
			switch (::GetDlgCtrlID(hCtrl)) {
				case IDC_BUTTON_CLRFACE:
				case IDC_BUTTON_CLROUTLINE:
				case IDC_BUTTON_CLRFONT:
				case IDC_BUTTON_CLRGRAD1:
				case IDC_BUTTON_CLRGRAD2:
				case IDC_COLORPRI:
				case IDC_COLORSEC:
				case IDC_COLOROUTL:
				case IDC_COLORSHAD:
					return true;
				default:
					return false;
			}
		}

		// Disabled text statics are drawn by Windows with an embossed grey (a light 1px shadow),
		// which looks bad on a dark background. Owner-draw disabled text labels flat instead — a
		// plain grey caption, no emboss. Enabled statics fall through to the default painting.
		const UINT_PTR kStaticSubclassId = 10;

		LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_ENABLE:
					::InvalidateRect(hWnd, nullptr, TRUE); // repaint when the enable state changes
					break;
				case WM_PAINT:
					if (::GetWindowLongW(hWnd, GWL_STYLE) & WS_DISABLED) {
						PAINTSTRUCT ps;
						CDC* pDC = CDC::FromHandle(::BeginPaint(hWnd, &ps));
						CRect rc;
						::GetClientRect(hWnd, &rc);
						pDC->FillSolidRect(rc, FaceColor());

						HFONT hFont = reinterpret_cast<HFONT>(::SendMessageW(hWnd, WM_GETFONT, 0, 0));
						CFont* pOldFont = hFont ? pDC->SelectObject(CFont::FromHandle(hFont)) : nullptr;
						pDC->SetBkMode(TRANSPARENT);
						pDC->SetTextColor(RGB(110, 115, 120)); // flat disabled grey, no emboss

						CString text;
						const int len = ::GetWindowTextLengthW(hWnd);
						if (len > 0) {
							::GetWindowTextW(hWnd, text.GetBuffer(len + 1), len + 1);
							text.ReleaseBuffer();
						}

						const LONG st = ::GetWindowLongW(hWnd, GWL_STYLE) & SS_TYPEMASK;
						UINT fmt = DT_NOPREFIX;
						if (st == SS_CENTER)     fmt |= DT_CENTER;
						else if (st == SS_RIGHT) fmt |= DT_RIGHT;
						if (st == SS_LEFTNOWORDWRAP || st == SS_SIMPLE) fmt |= DT_SINGLELINE | DT_VCENTER;
						else                                            fmt |= DT_WORDBREAK;
						pDC->DrawTextW(text, rc, fmt);

						if (pOldFont) {
							pDC->SelectObject(pOldFont);
						}
						::EndPaint(hWnd, &ps);
						return 0;
					}
					break; // enabled: let the default painting run (colour via WM_CTLCOLORSTATIC)
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, StaticSubclassProc, kStaticSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		void ThemeControl(HWND hCtrl) {
			if (pAllowDarkModeForWindow) {
				pAllowDarkModeForWindow(hCtrl, true);
			}

			wchar_t cls[64] = {};
			GetClassNameW(hCtrl, cls, _countof(cls));

			if (_wcsicmp(cls, L"Button") == 0) {
				const LONG bt = GetWindowLongW(hCtrl, GWL_STYLE) & BS_TYPEMASK;
				if (bt == BS_GROUPBOX) {
					SetWindowSubclass(hCtrl, GroupBoxSubclassProc, kGroupBoxSubclassId, 0);
					InvalidateRect(hCtrl, nullptr, TRUE);
				} else if (bt == BS_PUSHBUTTON || bt == BS_DEFPUSHBUTTON) {
					if (IsOwnerColorButton(hCtrl)) {
						// Colour-well button: leave it for its page's NM_CUSTOMDRAW handler, which
						// fills it with the selected colour (owner-drawing would show its caption).
					} else {
						// Owner-draw so they stay dark AND keep the Win11 rounding + icon.
						SetWindowSubclass(hCtrl, ButtonSubclassProc, kButtonSubclassId, 0);
						InvalidateRect(hCtrl, nullptr, TRUE);
					}
				} else {
					// checkboxes, radios
					SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
				}
			} else if (_wcsicmp(cls, L"ComboBox") == 0) {
				SetWindowTheme(hCtrl, L"DarkMode_CFD", nullptr);
				// The CFD combo border stays light on some machines (e.g. the Add-Favorite dropdown);
				// overpaint it dark like the edits.
				ApplyDarkBorder(hCtrl);
			} else if (_wcsicmp(cls, L"Edit") == 0) {
				const LONG est = GetWindowLongW(hCtrl, GWL_STYLE);
				if (est & (WS_VSCROLL | WS_HSCROLL)) {
					SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr); // dark scrollbar
				} else {
					// Single-line edits: DISABLE the visual style (empty theme) rather than DarkMode_CFD.
					// CFD owns a hover/hot border state and self-invalidates the edit's non-client on every
					// mouse-move; our WM_NCPAINT overpaint then flashed light->dark on each hover repaint
					// (About / History / Format / Keys filter / Logo fields). A style-less edit has no hover
					// border, so nothing repaints the NC on hover — the dark overpaint below is drawn once
					// (on show/focus/resize) and stays. Interior stays dark via WM_CTLCOLOR* (OnCtlColor),
					// which a classic edit honours fully, so dropping CFD doesn't lighten the interior.
					SetWindowTheme(hCtrl, L"", L"");
				}
				// Repaint the light client-edge dark. This self-skips on a borderless edit (NOT WS_BORDER,
				// e.g. the MediaInfo dump) — where a drawn frame would land in the client and get dragged
				// into the text on scroll — so only edits with a real border (the Details/Clip multiline
				// field, single-line value boxes) get the dark frame. No white edge, no scroll drag.
				ApplyDarkBorder(hCtrl);
			} else if (_wcsicmp(cls, L"SysListView32") == 0) {
				// List-view controls ignore WM_CTLCOLOR: their background (the area
				// not covered by columns/rows) must be set explicitly, otherwise it
				// stays the default white.
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
				const COLORREF clrBk = FaceColor();
				::SendMessageW(hCtrl, LVM_SETBKCOLOR,     0, static_cast<LPARAM>(clrBk));
				::SendMessageW(hCtrl, LVM_SETTEXTBKCOLOR, 0, static_cast<LPARAM>(clrBk));
				::SendMessageW(hCtrl, LVM_SETTEXTCOLOR,   0, static_cast<LPARAM>(TextColor()));
				// Double-buffer so the full repaint we force on horizontal scroll (to clear the
				// hand-drawn grid-line ghosts) doesn't flicker.
				::SendMessageW(hCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
				// Replace the light native grid lines with our own dark ones: strip the
				// style and remember (via the subclass ref-data) that this list wants a grid.
				// Only do this the first time — on re-application the style is already gone,
				// so preserve the remembered flag instead of clearing it.
				DWORD_PTR gridFlag = 0;
				if (!GetWindowSubclass(hCtrl, ListViewSubclassProc, kListViewSubclassId, &gridFlag)) {
					gridFlag = (::SendMessageW(hCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0) & LVS_EX_GRIDLINES) ? 1 : 0;
					if (gridFlag) {
						::SendMessageW(hCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES, 0);
					}
				}
				// Dark-paint the column header (and grid) via a subclass (see ListViewSubclassProc).
				SetWindowSubclass(hCtrl, ListViewSubclassProc, kListViewSubclassId, gridFlag);
				if (HWND hHeader = reinterpret_cast<HWND>(::SendMessageW(hCtrl, LVM_GETHEADER, 0, 0))) {
					InvalidateRect(hHeader, nullptr, TRUE);
				}
				ApplyDarkBorder(hCtrl); // dark outer border to match everything else
			} else if (_wcsicmp(cls, L"SysTreeView32") == 0) {
				// Tree-views, like list-views, need their background/text colors set
				// explicitly (SetWindowTheme only handles the glyphs and scrollbar).
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
				::SendMessageW(hCtrl, TVM_SETBKCOLOR,   0, static_cast<LPARAM>(FaceColor()));
				::SendMessageW(hCtrl, TVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(TextColor()));
				ApplyDarkBorder(hCtrl); // dark outer border to match everything else
			} else if (_wcsicmp(cls, UPDOWN_CLASSW) == 0) {
				// Spin buttons: fully owner-drawn (native dark mode leaves them light).
				SetWindowSubclass(hCtrl, SpinSubclassProc, kSpinSubclassId, 0);
				InvalidateRect(hCtrl, nullptr, TRUE);
			} else if (_wcsicmp(cls, L"ListBox") == 0) {
				// Plain list boxes (e.g. DVD preferred-language, the Internal Filters lists) get their
				// interior from WM_CTLCOLORLISTBOX (handled by the page), but their sunken border stays
				// light — repaint it dark. Unlike a multiline edit, a list box doesn't drag the overpaint
				// on scroll, so keep the custom border even when it scrolls (the theme's own border
				// rendered white on some machines).
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
				ApplyDarkBorder(hCtrl);
			} else if (_wcsicmp(cls, L"Static") == 0) {
				// Sunken value boxes (e.g. Brightness/Contrast/Hue/Saturation on the
				// Color correction page are SS_SUNKEN RTEXT statics) keep a light 3D edge
				// under native dark mode. Their interior comes from WM_CTLCOLORSTATIC
				// (handled by the page); repaint just the edge with the dark border.
				const LONG st = GetWindowLongW(hCtrl, GWL_STYLE);
				const LONG ex = GetWindowLongW(hCtrl, GWL_EXSTYLE);
				const LONG sType = st & SS_TYPEMASK;
				if ((st & SS_SUNKEN) || (ex & (WS_EX_CLIENTEDGE | WS_EX_STATICEDGE))) {
					ApplyDarkBorder(hCtrl);
				} else if (sType == SS_LEFT || sType == SS_CENTER || sType == SS_RIGHT
						|| sType == SS_LEFTNOWORDWRAP || sType == SS_SIMPLE) {
					// Plain text labels: owner-draw disabled ones flat (Windows would emboss them,
					// which looks bad on dark). Enabled ones keep the default painting.
					SetWindowSubclass(hCtrl, StaticSubclassProc, kStaticSubclassId, 0);
				}
			} else if (_wcsicmp(cls, TRACKBAR_CLASSW) == 0) {
				// Owner-draw every slider (dark groove + celeste thumb with hover / pressed states).
				// The native DarkMode trackbar thumb renders ugly black-on-hover / white-when-pressed
				// states otherwise. Skip trackbars already owner-drawn explicitly: the theme
				// R/G/B/Brightness sliders carry dwData = 1 for their frozen-colour snapshot, and
				// re-subclassing here (dwData = 0) would clobber that.
				DWORD_PTR existing = 0;
				if (!GetWindowSubclass(hCtrl, TrackbarSubclassProc, kTrackbarSubclassId, &existing)) {
					SetWindowSubclass(hCtrl, TrackbarSubclassProc, kTrackbarSubclassId, 0);
					InvalidateRect(hCtrl, nullptr, TRUE);
				}
			} else {
				// Note: SysTabControl32 is handled by CDarkTabCtrl (a CTabCtrl-derived
				// owner-drawn control), not here — installing a comctl subclass would sit
				// in front of MFC's WndProc and steal WM_PAINT from that class.
				// up-down, scrollbar, ...
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
			}
		}

		BOOL CALLBACK EnumChildProc(HWND hChild, LPARAM) {
			ThemeControl(hChild);
			return TRUE;
		}

		// Reverses everything ThemeControl may have installed on a control, so it returns to its
		// default (light) look when the dark theme is switched off while the Options dialog is
		// open. Removing a subclass that isn't present is a safe no-op.
		BOOL CALLBACK StripThemeChildProc(HWND hChild, LPARAM) {
			RemoveWindowSubclass(hChild, GroupBoxSubclassProc, kGroupBoxSubclassId);
			RemoveWindowSubclass(hChild, ButtonSubclassProc,   kButtonSubclassId);
			RemoveWindowSubclass(hChild, SpinSubclassProc,     kSpinSubclassId);
			RemoveWindowSubclass(hChild, BorderSubclassProc,   kBorderSubclassId);
			RemoveWindowSubclass(hChild, TrackbarSubclassProc, kTrackbarSubclassId);
			RemoveWindowSubclass(hChild, StaticSubclassProc,   kStaticSubclassId);

			DWORD_PTR gridFlag = 0;
			const bool hadGrid = GetWindowSubclass(hChild, ListViewSubclassProc, kListViewSubclassId, &gridFlag) && gridFlag;
			RemoveWindowSubclass(hChild, ListViewSubclassProc, kListViewSubclassId);

			wchar_t cls[64] = {};
			GetClassNameW(hChild, cls, _countof(cls));
			if (_wcsicmp(cls, L"SysListView32") == 0) {
				::SendMessageW(hChild, LVM_SETBKCOLOR,     0, static_cast<LPARAM>(::GetSysColor(COLOR_WINDOW)));
				::SendMessageW(hChild, LVM_SETTEXTBKCOLOR, 0, static_cast<LPARAM>(::GetSysColor(COLOR_WINDOW)));
				::SendMessageW(hChild, LVM_SETTEXTCOLOR,   0, static_cast<LPARAM>(::GetSysColor(COLOR_WINDOWTEXT)));
				if (hadGrid) { // we had stripped the native grid lines; put them back
					::SendMessageW(hChild, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
				}
			} else if (_wcsicmp(cls, L"SysTreeView32") == 0) {
				::SendMessageW(hChild, TVM_SETBKCOLOR,   0, static_cast<LPARAM>(-1)); // -1 = default
				::SendMessageW(hChild, TVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(-1));
			}

			SetWindowTheme(hChild, nullptr, nullptr); // drop the DarkMode_* visual style override
			if (pAllowDarkModeForWindow) {
				pAllowDarkModeForWindow(hChild, false);
			}
			::SetWindowPos(hChild, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			InvalidateRect(hChild, nullptr, TRUE);
			return TRUE;
		}
	} // anonymous namespace

	bool IsActive() {
		return AfxGetAppSettings().bUseDarkTheme && SysVersion::IsWin10v1809orLater();
	}

	// The background palette follows the R/G/B/Brightness sliders (ThemeRGB), so the Options
	// dialog tints in real time to match the player. The TEXT colour is deliberately FIXED (not
	// run through ThemeRGB) so it stays readable wherever the sliders are, instead of being
	// driven to black when a channel is lowered.
	COLORREF FaceColor()       { return ThemeRGB(22, 27, 32); }
	COLORREF TextColor()       { return RGB(165, 170, 175); }
	COLORREF CtrlBackColor()   { return ThemeRGB(10, 14, 18); }
	COLORREF CtrlBorderColor() { return ThemeRGB(70, 75, 80); }
	COLORREF GridlineColor()   { return ThemeRGB(40, 45, 50); }

	void AllowDarkModeForApp() {
		if (!IsActive()) {
			return;
		}
		LoadApi();
		if (!g_bApiOk || g_bAppAllowed) {
			return;
		}
		if (pSetPreferredAppMode) {
			pSetPreferredAppMode(APPMODE_FORCEDARK);
		} else if (pAllowDarkModeForApp) {
			pAllowDarkModeForApp(true);
		}
		if (pRefreshImmersiveColorPolicyState) {
			pRefreshImmersiveColorPolicyState();
		}
		if (pFlushMenuThemes) {
			pFlushMenuThemes();
		}
		g_bAppAllowed = true;
	}

	void DisallowDarkModeForApp() {
		LoadApi();
		if (!g_bApiOk) {
			return;
		}
		// Undo the process-wide force-dark set by AllowDarkModeForApp, so turning the dark theme off
		// at runtime reverts the main window's menus: they use the immersive dark menu theme, which
		// would otherwise stay dark until the app restarts. FORCELIGHT keeps the app light here.
		if (pSetPreferredAppMode) {
			pSetPreferredAppMode(APPMODE_FORCELIGHT);
		} else if (pAllowDarkModeForApp) {
			pAllowDarkModeForApp(false);
		}
		if (pRefreshImmersiveColorPolicyState) {
			pRefreshImmersiveColorPolicyState();
		}
		if (pFlushMenuThemes) {
			pFlushMenuThemes();
		}
		g_bAppAllowed = false;
	}

	void EnableForWindow(HWND hWnd) {
		if (!IsActive() || !hWnd) {
			return;
		}
		LoadApi();
		if (g_bApiOk && pAllowDarkModeForWindow) {
			pAllowDarkModeForWindow(hWnd, true);
			if (pRefreshImmersiveColorPolicyState) {
				pRefreshImmersiveColorPolicyState();
			}
		}

		// Dark title bar: attribute 20 (DWMWA_USE_IMMERSIVE_DARK_MODE) on 2004+,
		// attribute 19 on 1809/1903.
		BOOL bDark = TRUE;
		if (FAILED(DwmSetWindowAttribute(hWnd, 20, &bDark, sizeof(bDark)))) {
			DwmSetWindowAttribute(hWnd, 19, &bDark, sizeof(bDark));
		}
		// Win11: tint the title-bar background with the same colour as the player's caption
		// (ThemeRGB(45,50,55)), so the Options title matches the player and follows the sliders.
		if (SysVersion::IsWin11orLater()) {
			COLORREF cap = ThemeRGB(45, 50, 55);
			DwmSetWindowAttribute(hWnd, 35 /*DWMWA_CAPTION_COLOR*/, &cap, sizeof(cap));
		}
	}

	void ApplyThemeToChildren(HWND hWndParent) {
		if (!IsActive() || !hWndParent) {
			return;
		}
		LoadApi();
		if (!g_bApiOk) {
			return;
		}
		EnumChildWindows(hWndParent, EnumChildProc, 0);
	}

	void FixGroupBoxes(HWND hWndParent) {
		if (!IsActive() || !hWndParent) {
			return;
		}
		// Collect the group boxes first — moving a window's z-order while walking the sibling
		// list would disturb the walk. Only the direct children of the page can overlap here.
		HWND boxes[32];
		int n = 0;
		for (HWND c = ::GetWindow(hWndParent, GW_CHILD); c && n < _countof(boxes); c = ::GetWindow(c, GW_HWNDNEXT)) {
			wchar_t cls[16] = {};
			GetClassNameW(c, cls, _countof(cls));
			if (_wcsicmp(cls, L"Button") == 0 && (GetWindowLongW(c, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX) {
				boxes[n++] = c;
			}
		}
		for (int i = 0; i < n; ++i) {
			SetWindowLongW(boxes[i], GWL_STYLE, GetWindowLongW(boxes[i], GWL_STYLE) | WS_CLIPSIBLINGS);
			::SetWindowPos(boxes[i], HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}

	void ApplyThemeToControl(HWND hCtrl) {
		if (!IsActive() || !hCtrl) {
			return;
		}
		LoadApi();
		if (!g_bApiOk) {
			return;
		}
		ThemeControl(hCtrl);
	}

	void RefreshThemeForControl(HWND hCtrl) {
		if (!hCtrl) {
			return;
		}
		LoadApi();
		if (IsActive()) {
			if (g_bApiOk) {
				ThemeControl(hCtrl);
			}
		} else {
			// Undo everything ThemeControl installed (subclasses that paint dark unconditionally,
			// the DarkMode_* visual style, the dark list/tree background) so the control goes back to
			// its native light look instead of leaving a half-dark control after a runtime toggle-off.
			StripThemeChildProc(hCtrl, 0);
		}
		::InvalidateRect(hCtrl, nullptr, TRUE);
	}

	void CommitThemeColors() {
		// Snapshot the current theme colours for the R/G/B/Brightness sliders. Call this when a
		// slider drag ends so all four repaint together to the final colour (see TrackbarSubclassProc).
		g_committedFace   = FaceColor();
		g_committedGroove = ThemeRGB(10, 14, 18);
		g_committedBorder = ThemeRGB(60, 65, 70);
	}

	void MakeTrackbarOwnerDrawn(HWND hTrackbar, bool bThemeControl) {
		if (!IsActive() || !hTrackbar) {
			return;
		}
		SetWindowSubclass(hTrackbar, TrackbarSubclassProc, kTrackbarSubclassId, bThemeControl ? 1 : 0);
		::InvalidateRect(hTrackbar, nullptr, TRUE);
	}

	void MarkUacShield(HWND hButton) {
		if (!hButton) {
			return;
		}
		// Flag a push button so its owner-draw (ButtonSubclassProc) paints the UAC elevation shield.
		// Needed because BCM_SETSHIELD's glyph is drawn internally and lost when we owner-draw.
		::SetPropW(hButton, L"MPC_UAC_SHIELD", reinterpret_cast<HANDLE>(1));
		::InvalidateRect(hButton, nullptr, FALSE);
	}

	void ThemeDialog(HWND hDlg) {
		if (!IsActive() || !hDlg) {
			return;
		}
		LoadApi();
		// Ensure the process-wide immersive dark mode is on. Without it the native scrollbars of
		// child edits/lists render light and the DarkMode_CFD border stays a light "frame" — visible
		// when an auxiliary dialog (Command Line Switches, Shader Editor, ...) is opened directly,
		// before the Options sheet (which used to be the only caller of this) has ever been shown.
		AllowDarkModeForApp();
		EnableForWindow(hDlg);                 // dark title bar + allow dark mode
		SetWindowSubclass(hDlg, DialogSubclassProc, kDialogSubclassId, 0); // dark bg / ctl colours
		ApplyThemeToChildren(hDlg);            // theme the child controls
		// Repaint the dialog AND its child controls: InvalidateRect alone doesn't reach the
		// child windows, so freshly-themed checkboxes/statics would keep their stale light paint.
		// RDW_UPDATENOW forces a synchronous repaint so controls that otherwise only redraw on the
		// next interaction (some third-party filter property-page widgets stayed light until hovered)
		// pick up the theme immediately; RDW_FRAME also refreshes their non-client border.
		::RedrawWindow(hDlg, nullptr, nullptr,
			RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
	}

	// --- Dark message boxes (MessageBox / AfxMessageBox) via a WH_CBT hook -------------------------
	// A message box is a standard "#32770" dialog the OS creates and paints with system colours; the
	// only way to theme it is to catch it as it activates and re-theme it. A persistent hook on the UI
	// thread (InstallMessageBoxHook) covers all main-thread message boxes; CDarkMessageBoxHook is a
	// scoped guard for the few shown from a worker thread (a thread hook only sees its own thread).
	namespace {
		HHOOK g_msgBoxHook = nullptr;           // scoped (CDarkMessageBoxHook)
		HHOOK g_persistentMsgBoxHook = nullptr; // process-lifetime (InstallMessageBoxHook)
		const UINT_PTR kMsgBoxSubclassId = 11;

		// The message box repaints its lower button "band" with a light system colour in its own
		// WM_PAINT, over the dark erase ThemeDialog installs. Paint the whole client dark ourselves and
		// skip the default paint so the band stays dark; the icon / text / buttons are child windows and
		// repaint on top. Installed on top of ThemeDialog's subclass, so WM_CTLCOLOR* still go dark.
		LRESULT CALLBACK MsgBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_PAINT: {
					PAINTSTRUCT ps;
					HDC hdc = ::BeginPaint(hWnd, &ps);
					CRect rc;
					::GetClientRect(hWnd, &rc);
					CDC::FromHandle(hdc)->FillSolidRect(rc, FaceColor());
					::EndPaint(hWnd, &ps);
					return 0;
				}
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, MsgBoxSubclassProc, kMsgBoxSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// A standard message box holds only static labels / icon and push buttons. Any other control
		// class (edit, combo, tree, tab, list, or a nested dialog / property page) means it's a real
		// dialog that themes itself — the hook must not blanket-theme those (that would re-theme the
		// Options sheet and every aux dialog on activation and risk breaking them).
		BOOL CALLBACK MsgBoxChildProc(HWND hChild, LPARAM lParam) {
			auto* flags = reinterpret_cast<bool*>(lParam); // [0] = only statics/buttons, [1] = has a button
			wchar_t cls[32] = {};
			GetClassNameW(hChild, cls, _countof(cls));
			if (_wcsicmp(cls, L"Button") == 0) {
				flags[1] = true;
			} else if (_wcsicmp(cls, L"Static") != 0) {
				flags[0] = false;
				return FALSE; // found a non-message-box control, stop
			}
			return TRUE;
		}

		bool LooksLikeMessageBox(HWND hWnd) {
			bool flags[2] = { true, false };
			EnumChildWindows(hWnd, MsgBoxChildProc, reinterpret_cast<LPARAM>(flags));
			return flags[0] && flags[1];
		}

		void ThemeMessageBoxWindow(HWND hWnd) {
			if (!IsActive() || !hWnd) {
				return;
			}
			wchar_t cls[16] = {};
			::GetClassNameW(hWnd, cls, _countof(cls));
			if (wcscmp(cls, L"#32770") != 0) {
				return; // not the dialog class MessageBox uses
			}
			DWORD_PTR ref = 0;
			if (GetWindowSubclass(hWnd, DialogSubclassProc, kDialogSubclassId, &ref)) {
				return; // one of our own dialogs (it themes itself) — leave it alone
			}
			if (!LooksLikeMessageBox(hWnd)) {
				return; // a real dialog, not a message box
			}
			ThemeDialog(hWnd);
			SetWindowSubclass(hWnd, MsgBoxSubclassProc, kMsgBoxSubclassId, 0);
			::RedrawWindow(hWnd, nullptr, nullptr,
				RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
		}

		LRESULT CALLBACK MsgBoxCBTProc(int code, WPARAM wParam, LPARAM lParam) {
			if (code == HCBT_ACTIVATE) {
				ThemeMessageBoxWindow(reinterpret_cast<HWND>(wParam));
			}
			return ::CallNextHookEx(nullptr, code, wParam, lParam); // hhk is ignored on modern Windows
		}
	}

	void InstallMessageBoxHook() {
		// Persistent, checks IsActive() per message box (so it follows the theme toggle), installed once.
		if (!g_persistentMsgBoxHook) {
			g_persistentMsgBoxHook = ::SetWindowsHookExW(WH_CBT, MsgBoxCBTProc, nullptr, ::GetCurrentThreadId());
		}
	}

	CDarkMessageBoxHook::CDarkMessageBoxHook() : m_hooked(false) {
		// One scoped hook per thread at a time; nested message boxes reuse the outer guard's hook.
		if (IsActive() && !g_msgBoxHook) {
			g_msgBoxHook = ::SetWindowsHookExW(WH_CBT, MsgBoxCBTProc, nullptr, ::GetCurrentThreadId());
			m_hooked = (g_msgBoxHook != nullptr);
		}
	}

	CDarkMessageBoxHook::~CDarkMessageBoxHook() {
		if (m_hooked && g_msgBoxHook) {
			::UnhookWindowsHookEx(g_msgBoxHook);
			g_msgBoxHook = nullptr;
		}
	}

	void RefreshTheme(HWND hRoot) {
		if (!hRoot) {
			return;
		}
		LoadApi();
		if (IsActive()) {
			// Turned on at runtime: (re)apply to the sheet and every already-created page.
			AllowDarkModeForApp();             // re-arm the process-wide force-dark (menus etc.)
			EnableForWindow(hRoot);            // dark title bar (checks IsActive internally)
			ApplyThemeToChildren(hRoot);       // recurses into all descendant controls
		} else {
			// Turned off at runtime: strip every subclass/override so everything goes light, and
			// undo the process-wide force-dark so the main window's menus revert to light (they use
			// the immersive dark menu theme, which otherwise stays dark until the app restarts).
			DisallowDarkModeForApp();
			if (g_bApiOk && pAllowDarkModeForWindow) {
				pAllowDarkModeForWindow(hRoot, false);
			}
			BOOL bDark = FALSE;                 // restore the light title bar
			if (FAILED(DwmSetWindowAttribute(hRoot, 20, &bDark, sizeof(bDark)))) {
				DwmSetWindowAttribute(hRoot, 19, &bDark, sizeof(bDark));
			}
			EnumChildWindows(hRoot, StripThemeChildProc, 0);
		}
		::RedrawWindow(hRoot, nullptr, nullptr,
			RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
	}

	void RefreshColors(HWND hRoot) {
		if (!IsActive() || !hRoot) {
			return;
		}
		// Re-apply the themed background colours the sliders just changed (tree/list backgrounds
		// are stored in the control, so they don't re-tint on their own), then invalidate the
		// sheet. WM_CTLCOLOR* refreshes the cached brushes (EnsureBrushes) for everything else.
		// The text colour is fixed, so nothing there needs updating. Called when a slider drag
		// ends (not on every tick), so this one repaint doesn't produce visible flicker.
		EnumChildWindows(hRoot, [](HWND h, LPARAM) -> BOOL {
			wchar_t cls[32] = {};
			GetClassNameW(h, cls, _countof(cls));
			if (_wcsicmp(cls, L"SysTreeView32") == 0) {
				::SendMessageW(h, TVM_SETBKCOLOR, 0, static_cast<LPARAM>(FaceColor()));
			} else if (_wcsicmp(cls, L"SysListView32") == 0) {
				const COLORREF bk = FaceColor();
				::SendMessageW(h, LVM_SETBKCOLOR,     0, static_cast<LPARAM>(bk));
				::SendMessageW(h, LVM_SETTEXTBKCOLOR, 0, static_cast<LPARAM>(bk));
			}
			return TRUE;
		}, 0);
		// Also re-tint the title bar (Win11) so the caption follows the sliders, like the player's.
		if (SysVersion::IsWin11orLater()) {
			COLORREF cap = ThemeRGB(45, 50, 55);
			DwmSetWindowAttribute(hRoot, 35 /*DWMWA_CAPTION_COLOR*/, &cap, sizeof(cap));
		}
		::RedrawWindow(hRoot, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}

	bool MakeCheckStateImageList(CImageList& il, int size, HWND hRef, bool bDisabled) {
		if (!IsActive() || size <= 0) {
			return false;
		}

		if (bDisabled) {
			// Hand-drawn grey checkboxes for the inactive (locked) list. The theme's
			// disabled glyph renders almost fully transparent through buffered paint,
			// which made the checkboxes vanish; a solid grey box + grey tick reads clearly
			// as an inactive checkbox and never disappears.
			const COLORREF mask   = RGB(255, 0, 255);
			const COLORREF fill   = ThemeRGB(34, 39, 44);
			const COLORREF border = ThemeRGB(90, 95, 100);
			const COLORREF mark   = ThemeRGB(120, 125, 130);

			CClientDC screen(nullptr);
			CDC dc;
			dc.CreateCompatibleDC(&screen);
			CBitmap bmp;
			bmp.CreateCompatibleBitmap(&screen, size * 3, size);
			CBitmap* pOld = dc.SelectObject(&bmp);
			dc.FillSolidRect(0, 0, size * 3, size, mask);

			for (int i = 1; i <= 2; ++i) { // index 0 = blank, 1 = unchecked, 2 = checked
				CRect box(i * size, 0, i * size + size, size);
				box.DeflateRect(2, 2);
				dc.FillSolidRect(box, fill);
				dc.Draw3dRect(box, border, border);
				if (i == 2) {
					CPen pen(PS_SOLID, 2, mark);
					CPen* pp = dc.SelectObject(&pen);
					const int l = box.left, t = box.top, w = box.Width(), h = box.Height();
					dc.MoveTo(l + w * 25 / 100, t + h * 50 / 100);
					dc.LineTo(l + w * 45 / 100, t + h * 70 / 100);
					dc.LineTo(l + w * 75 / 100, t + h * 28 / 100);
					dc.SelectObject(pp);
				}
			}

			dc.SelectObject(pOld);
			il.DeleteImageList();
			il.Create(size, size, ILC_COLOR24 | ILC_MASK, 3, 0);
			il.Add(&bmp, mask);
			return il.GetImageCount() == 3;
		}

		AllowDarkModeForApp(); // ensure force-dark so the BUTTON theme is the dark one

		HTHEME hTheme = ::OpenThemeData(hRef, L"BUTTON");
		if (!hTheme) {
			return false;
		}

		il.DeleteImageList();
		if (!il.Create(size, size, ILC_COLOR32, 3, 0)) {
			::CloseThemeData(hTheme);
			return false;
		}

		::BufferedPaintInit();

		// State index 0 = no image, 1 = unchecked, 2 = checked (LVS_EX_CHECKBOXES layout).
		const int states[3] = { 0, CBS_UNCHECKEDNORMAL, CBS_CHECKEDNORMAL };
		HDC hdcScreen = ::GetDC(nullptr);
		HDC hdcMem    = ::CreateCompatibleDC(hdcScreen);
		bool ok = true;

		for (int i = 0; i < 3 && ok; ++i) {
			BITMAPINFO bmi = {};
			bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth       = size;
			bmi.bmiHeader.biHeight      = -size; // top-down
			bmi.bmiHeader.biPlanes      = 1;
			bmi.bmiHeader.biBitCount    = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			void* pBits = nullptr;
			HBITMAP hDib = ::CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
			if (!hDib) {
				ok = false;
				break;
			}
			HBITMAP hOldBmp = (HBITMAP)::SelectObject(hdcMem, hDib);

			RECT rcFull = { 0, 0, size, size };
			BP_PAINTPARAMS pp = { sizeof(pp) };
			pp.dwFlags = BPPF_ERASE;
			HDC hdcPaint = nullptr;
			HPAINTBUFFER hbp = ::BeginBufferedPaint(hdcMem, &rcFull, BPBF_TOPDOWNDIB, &pp, &hdcPaint);
			if (hbp) {
				if (states[i] != 0) { // index 0 stays fully transparent (no checkbox)
					SIZE gsz = { size, size };
					::GetThemePartSize(hTheme, hdcPaint, BP_CHECKBOX, states[i], nullptr, TS_DRAW, &gsz);
					RECT rg;
					rg.left   = (size - gsz.cx) / 2; if (rg.left < 0) { rg.left = 0; }
					rg.top    = (size - gsz.cy) / 2; if (rg.top  < 0) { rg.top  = 0; }
					rg.right  = rg.left + gsz.cx;
					rg.bottom = rg.top  + gsz.cy;
					::DrawThemeBackground(hTheme, hdcPaint, BP_CHECKBOX, states[i], &rg, nullptr);
				}
				::EndBufferedPaint(hbp, TRUE);
			} else {
				ok = false;
			}

			::SelectObject(hdcMem, hOldBmp);
			if (ok) {
				il.Add(CBitmap::FromHandle(hDib), (CBitmap*)nullptr);
			}
			::DeleteObject(hDib);
		}

		::DeleteDC(hdcMem);
		::ReleaseDC(nullptr, hdcScreen);
		::BufferedPaintUnInit();
		::CloseThemeData(hTheme);

		if (ok && il.GetImageCount() == 3) {
			return true;
		}
		il.DeleteImageList();
		return false;
	}

	HBRUSH OnCtlColor(CDC* pDC, UINT nCtlColor) {
		if (!IsActive() || !pDC) {
			return nullptr;
		}
		EnsureBrushes();

		switch (nCtlColor) {
			case CTLCOLOR_EDIT:
			case CTLCOLOR_LISTBOX:
				pDC->SetTextColor(TextColor());
				pDC->SetBkColor(g_clrCtrl);
				return g_hbrCtrl;
			default: // CTLCOLOR_DLG, CTLCOLOR_STATIC, CTLCOLOR_BTN, CTLCOLOR_MSGBOX, CTLCOLOR_SCROLLBAR
				pDC->SetTextColor(TextColor());
				pDC->SetBkColor(g_clrFace);
				return g_hbrFace;
		}
	}

	bool TrackbarCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
		if (!IsActive() || !pNMHDR || !pResult) {
			return false;
		}

		wchar_t cls[64] = {};
		GetClassNameW(pNMHDR->hwndFrom, cls, _countof(cls));
		if (_wcsicmp(cls, L"msctls_trackbar32") != 0) {
			return false;
		}

		LPNMCUSTOMDRAW p = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
		switch (p->dwDrawStage) {
			case CDDS_PREPAINT: {
				// Fill the whole trackbar dark up front. Otherwise the background relies on
				// WM_CTLCOLOR, which is not honoured on some repaints (e.g. after the
				// "Reset" button on the subtitle Default Style page), leaving it white.
				CDC* pDC = CDC::FromHandle(p->hdc);
				pDC->FillSolidRect(&p->rc, FaceColor());
				*pResult = CDRF_NOTIFYITEMDRAW;
				return true;
			}
			case CDDS_ITEMPREPAINT:
				if (p->dwItemSpec == TBCD_CHANNEL) {
					CDC* pDC = CDC::FromHandle(p->hdc);
					CRect rc(p->rc);
					pDC->FillSolidRect(rc, ThemeRGB(10, 14, 18));                        // dark groove
					pDC->Draw3dRect(rc, ThemeRGB(60, 65, 70), ThemeRGB(60, 65, 70));     // subtle border
					*pResult = CDRF_SKIPDEFAULT;
				} else {
					*pResult = CDRF_DODEFAULT; // keep the default thumb and tick marks
				}
				return true;
		}

		*pResult = CDRF_DODEFAULT;
		return true;
	}

	bool ButtonCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
		if (!IsActive() || !pNMHDR || !pResult) {
			return false;
		}

		HWND hCtrl = pNMHDR->hwndFrom;
		wchar_t cls[64] = {};
		GetClassNameW(hCtrl, cls, _countof(cls));
		if (_wcsicmp(cls, L"Button") != 0) {
			return false;
		}

		const LONG style = GetWindowLongW(hCtrl, GWL_STYLE);
		const LONG btype = style & BS_TYPEMASK;
		const bool isCheck = (btype == BS_CHECKBOX || btype == BS_AUTOCHECKBOX || btype == BS_3STATE || btype == BS_AUTO3STATE);
		const bool isRadio = (btype == BS_RADIOBUTTON || btype == BS_AUTORADIOBUTTON);
		const bool isPush  = (btype == BS_PUSHBUTTON || btype == BS_DEFPUSHBUTTON);
		if (!isCheck && !isRadio && !isPush) {
			return false; // group boxes and owner-draw color pickers are handled elsewhere
		}

		LPNMCUSTOMDRAW p = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
		if (p->dwDrawStage != CDDS_PREPAINT) {
			*pResult = CDRF_DODEFAULT;
			return true;
		}

		CDC* pDC = CDC::FromHandle(p->hdc);
		CRect rc(p->rc);
		pDC->FillSolidRect(rc, FaceColor());

		const bool disabled = (p->uItemState & CDIS_DISABLED) != 0;
		const bool pressed  = (p->uItemState & CDIS_SELECTED) != 0;
		const bool hot      = (p->uItemState & CDIS_HOT) != 0;

		if (isPush) {
			// Flat dark push button (face darkens when pressed, lightens on hover).
			const bool focus = (p->uItemState & CDIS_FOCUS) != 0;
			const COLORREF face = disabled ? ThemeRGB(30, 34, 38)
								: pressed  ? ThemeRGB(28, 33, 38)
								: hot      ? ThemeRGB(52, 59, 66)
								:            ThemeRGB(44, 50, 56);
			pDC->FillSolidRect(rc, face);
			pDC->Draw3dRect(rc, ThemeRGB(80, 86, 92), ThemeRGB(80, 86, 92));

			CString btext;
			const int blen = ::GetWindowTextLengthW(hCtrl);
			if (blen > 0) {
				::GetWindowTextW(hCtrl, btext.GetBuffer(blen + 1), blen + 1);
				btext.ReleaseBuffer();
			}

			HFONT hbf = reinterpret_cast<HFONT>(::SendMessageW(hCtrl, WM_GETFONT, 0, 0));
			CFont* pOldBf = hbf ? pDC->SelectObject(CFont::FromHandle(hbf)) : nullptr;
			pDC->SetBkMode(TRANSPARENT);
			pDC->SetTextColor(disabled ? RGB(110, 115, 120) : TextColor());

			// Centre the caption vertically even when it wraps to two lines.
			CRect rcCalc = rc;
			pDC->DrawTextW(btext, rcCalc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
			CRect rcText = rc;
			if (rcCalc.Height() < rc.Height()) {
				rcText.top += (rc.Height() - rcCalc.Height()) / 2;
			}
			pDC->DrawTextW(btext, rcText, DT_CENTER | DT_WORDBREAK);

			if (pOldBf) {
				pDC->SelectObject(pOldBf);
			}
			if (focus) {
				CRect rf = rc;
				rf.DeflateRect(3, 3);
				pDC->DrawFocusRect(rf);
			}
			*pResult = CDRF_SKIPDEFAULT;
			return true;
		}

		const LRESULT check = ::SendMessageW(hCtrl, BM_GETCHECK, 0, 0);

		const int partId = isRadio ? BP_RADIOBUTTON : BP_CHECKBOX;
		int stateBase;
		if (isRadio) {
			stateBase = (check == BST_CHECKED) ? RBS_CHECKEDNORMAL : RBS_UNCHECKEDNORMAL;
		} else if (check == BST_INDETERMINATE) {
			stateBase = CBS_MIXEDNORMAL;
		} else if (check == BST_CHECKED) {
			stateBase = CBS_CHECKEDNORMAL;
		} else {
			stateBase = CBS_UNCHECKEDNORMAL;
		}
		// each group is ordered Normal, Hot, Pressed, Disabled
		const int stateId = stateBase + (disabled ? 3 : pressed ? 2 : hot ? 1 : 0);

		SIZE glyph = { 13, 13 };
		HTHEME hTheme = OpenThemeData(hCtrl, L"Button");
		if (hTheme) {
			GetThemePartSize(hTheme, pDC->GetSafeHdc(), partId, stateId, nullptr, TS_DRAW, &glyph);
		}

		CRect rcGlyph;
		rcGlyph.left   = rc.left;
		rcGlyph.top    = (style & BS_MULTILINE) ? rc.top + 1 : rc.top + (rc.Height() - glyph.cy) / 2;
		rcGlyph.right  = rcGlyph.left + glyph.cx;
		rcGlyph.bottom = rcGlyph.top + glyph.cy;

		if (hTheme) {
			DrawThemeBackground(hTheme, pDC->GetSafeHdc(), partId, stateId, &rcGlyph, nullptr);
			CloseThemeData(hTheme);
		} else {
			UINT dfcState = (isRadio ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK)
				| (check == BST_CHECKED ? DFCS_CHECKED : 0u)
				| (disabled ? DFCS_INACTIVE : 0u);
			pDC->DrawFrameControl(rcGlyph, DFC_BUTTON, dfcState);
		}

		CString text;
		const int len = ::GetWindowTextLengthW(hCtrl);
		if (len > 0) {
			::GetWindowTextW(hCtrl, text.GetBuffer(len + 1), len + 1);
			text.ReleaseBuffer();
		}

		CRect rcText = rc;
		rcText.left = rcGlyph.right + 4;

		HFONT hFont = reinterpret_cast<HFONT>(::SendMessageW(hCtrl, WM_GETFONT, 0, 0));
		CFont* pOldFont = hFont ? pDC->SelectObject(CFont::FromHandle(hFont)) : nullptr;

		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(disabled ? RGB(110, 115, 120) : TextColor());

		UINT fmt = DT_LEFT;
		if (style & BS_MULTILINE) {
			fmt |= DT_WORDBREAK;
		} else {
			fmt |= DT_SINGLELINE | DT_VCENTER;
		}
		pDC->DrawTextW(text, rcText, fmt);

		if (pOldFont) {
			pDC->SelectObject(pOldFont);
		}

		*pResult = CDRF_SKIPDEFAULT;
		return true;
	}
}
