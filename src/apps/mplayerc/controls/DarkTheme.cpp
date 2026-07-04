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
#include "../MainFrm.h" // AfxGetAppSettings(), ThemeRGB()
#include "DSUtil/SysVersion.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>  // BP_CHECKBOX / BP_RADIOBUTTON / CBS_* / RBS_*
#include <commctrl.h> // SetWindowSubclass
#include <ExtLib/ui/coolsb/coolscroll.h> // flat dark scrollbars (same as the playlist)

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
						pDC->SetTextColor(disabled ? ThemeRGB(110, 115, 120) : TextColor());
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

			// vertical line at each column's right edge, through the item rows only
			if (hHeader) {
				const int cols = static_cast<int>(::SendMessageW(hHeader, HDM_GETITEMCOUNT, 0, 0));
				for (int c = 0; c < cols; ++c) {
					RECT hr{};
					if (Header_GetItemRect(hHeader, c, &hr)) {
						::MoveToEx(hdc, hr.right - 1, headerH, nullptr);
						::LineTo(hdc, hr.right - 1, gridBottom);
					}
				}
			}

			::SelectObject(hdc, oldPen);
			::DeleteObject(pen);
			::ReleaseDC(hWnd, hdc);
		}

		// A list-view's column header (SysHeader32) is not darkened by SetWindowTheme
		// and it sends its NM_CUSTOMDRAW to the list-view (its parent), not to the
		// dialog, so we subclass the list-view and paint the header ourselves. When the
		// list originally had grid lines, dwRefData is 1 and we also draw a dark grid.
		const UINT_PTR kListViewSubclassId = 2;

		LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR dwRefData) {
			if (msg == WM_PAINT && dwRefData) {
				const LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);
				DrawListGridlines(hWnd);
				return res;
			}
			if (msg == WM_NOTIFY) {
				NMHDR* pNM = reinterpret_cast<NMHDR*>(lParam);
				HWND hHeader = reinterpret_cast<HWND>(::SendMessageW(hWnd, LVM_GETHEADER, 0, 0));
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
					const COLORREF clrArrow   = ThemeRGB(170, 175, 180);

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

		LRESULT CALLBACK TrackbarSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_ERASEBKGND:
					return 1; // background is painted in WM_PAINT
				case WM_PAINT: {
					PAINTSTRUCT ps;
					CDC* pDC = CDC::FromHandle(::BeginPaint(hWnd, &ps));

					CRect rc;
					::GetClientRect(hWnd, &rc);
					pDC->FillSolidRect(rc, FaceColor());

					RECT rcCh{};
					::SendMessageW(hWnd, TBM_GETCHANNELRECT, 0, reinterpret_cast<LPARAM>(&rcCh));
					CRect ch(rcCh);
					pDC->FillSolidRect(ch, ThemeRGB(10, 14, 18));                    // dark groove
					pDC->Draw3dRect(ch, ThemeRGB(60, 65, 70), ThemeRGB(60, 65, 70)); // subtle border

					RECT rcTh{};
					::SendMessageW(hWnd, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&rcTh));
					CRect th(rcTh);
					const bool disabled = (::GetWindowLongW(hWnd, GWL_STYLE) & WS_DISABLED) != 0;
					pDC->FillSolidRect(th, disabled ? ThemeRGB(90, 95, 100) : RGB(76, 194, 255)); // celeste thumb

					::EndPaint(hWnd, &ps);
					return 0;
				}
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, TrackbarSubclassProc, kTrackbarSubclassId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// CoolSB draws its thumb purely from its own stored scroll position (see CalcThumbSize)
		// and never updates it on its own: during a drag it only posts WM_VSCROLL(SB_THUMBTRACK)
		// and expects the owner to feed the new position back. Wheel/keyboard scrolling it does
		// not see at all. These controls (list-view, tree-view, list-box) scroll themselves and
		// keep their real scroll info, so we subclass them and, after every scroll, copy that
		// real info into CoolSB — otherwise the flat thumb snaps back to the top while the
		// content scrolls underneath it.
		const UINT_PTR kCoolSBSyncId = 8;

		// True when the control actually has something to scroll in this direction. Mirrors
		// CoolSB's own IsScrollInfoActive so our show/hide decision matches its thumb logic.
		bool IsBarScrollable(HWND hCtrl, int bar) {
			SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE };
			if (!::GetScrollInfo(hCtrl, bar, &si)) {
				return false;
			}
			return !(si.nPage > (UINT)si.nMax || si.nMax <= si.nMin || si.nMax == 0);
		}

		// Tree-views (e.g. the Options navigation tree) carry WS_HSCROLL and, once CoolSB's
		// vertical bar narrows their client, report a horizontal range — producing a phantom
		// bottom bar that was never there natively. They don't need horizontal scrolling here,
		// so we never manage or show their horizontal bar.
		// Decides whether a control should show a horizontal scrollbar. In the Options dialog the
		// only control that legitimately needs one is a multi-column report list-view (the Keys
		// list). Tree-views, list-boxes (the Internal/External Filters check lists) and single-
		// column lists (Formats, Select Filter) fit their content to the width, so any horizontal
		// bar there is the spurious one from the vertical-bar reservation — never show it.
		bool NeedsHorzBar(HWND hCtrl) {
			wchar_t cls[32] = {};
			GetClassNameW(hCtrl, cls, _countof(cls));

			if (_wcsicmp(cls, L"SysListView32") != 0) {
				return false; // tree-views, list-boxes, ... : single-column here
			}
			if ((GetWindowLongW(hCtrl, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT) {
				return IsBarScrollable(hCtrl, SB_HORZ); // list/icon modes scroll horizontally by nature
			}
			HWND hHeader = reinterpret_cast<HWND>(::SendMessageW(hCtrl, LVM_GETHEADER, 0, 0));
			const int cols = hHeader ? static_cast<int>(::SendMessageW(hHeader, HDM_GETITEMCOUNT, 0, 0)) : 0;
			if (cols <= 1) {
				return false; // a single column fits the width
			}
			// Multi-column: show it only if the columns really overflow, ignoring the ~vertical-
			// bar-width that the vertical bar reservation alone introduces.
			SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE };
			if (!::GetScrollInfo(hCtrl, SB_HORZ, &si)) {
				return false;
			}
			if (si.nPage > (UINT)si.nMax || si.nMax <= si.nMin || si.nMax == 0) {
				return false; // nothing to scroll horizontally
			}
			const int overflow = (si.nMax - si.nMin + 1) - static_cast<int>(si.nPage);
			return overflow > ::GetSystemMetrics(SM_CXVSCROLL) + 4;
		}

		// CoolSB reserves and draws a bar for every direction whose WS_*SCROLL style was set
		// when it was initialised, even with nothing to scroll. The native control hides an
		// unused bar; mirror that so no empty gutter shows up.
		void UpdateBarVisibility(HWND hCtrl) {
			CoolSB_ShowScrollBar(hCtrl, SB_VERT, IsBarScrollable(hCtrl, SB_VERT));
			CoolSB_ShowScrollBar(hCtrl, SB_HORZ, NeedsHorzBar(hCtrl));
		}

		void SyncCoolSB(HWND hCtrl) {
			// While CoolSB is actively dragging its thumb it already paints it following the
			// mouse; only update the stored position (no redraw) then, to avoid fighting that
			// paint. For wheel/keyboard scrolling we do need the redraw to move the thumb.
			const BOOL redraw = !CoolSB_IsThumbTracking(hCtrl);
			SCROLLINFO si = { sizeof(si), SIF_ALL };
			if (::GetScrollInfo(hCtrl, SB_VERT, &si)) {
				CoolSB_SetScrollInfo(hCtrl, SB_VERT, &si, redraw);
			}
			// Only feed the horizontal bar when it's genuinely wanted; otherwise CoolSB's
			// SetScrollInfo would auto-re-show a phantom/spurious bottom bar on every scroll.
			if (NeedsHorzBar(hCtrl)) {
				SCROLLINFO sih = { sizeof(sih), SIF_ALL };
				if (::GetScrollInfo(hCtrl, SB_HORZ, &sih)) {
					CoolSB_SetScrollInfo(hCtrl, SB_HORZ, &sih, redraw);
				}
			}
		}

		// Scrolls a CoolSB-managed control to an absolute position. CoolSB posts the target in
		// the SB_THUMB* message, but common controls (tree-view, list-view) read the drag
		// position from the scrollbar's nTrackPos — which cannot be set through the API — so a
		// posted SB_THUMBTRACK moves nothing on them (only CPlayerListCtrl overrides OnVScroll
		// to use the message value). Instead we step the control with line scrolls, which every
		// control honours (relative, no position needed), reading the real position back until
		// it reaches the target. Intermediate steps are not painted (all inside one message),
		// so there is no visible stepping.
		void ScrollToPos(HWND hCtrl, UINT bar, int target) {
			wchar_t cls[32] = {};
			GetClassNameW(hCtrl, cls, _countof(cls));

			// List-views can scroll to an absolute position in a single shot (LVM_SCROLL takes a
			// pixel delta), so avoid the per-line loop which freezes on large lists (e.g. Keys).
			// Mirrors CPlayerListCtrl::OnVScroll: pixel delta = span * (target - pos) / page.
			if (_wcsicmp(cls, L"SysListView32") == 0) {
				SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS };
				if (!::GetScrollInfo(hCtrl, bar, &si)) {
					return;
				}
				RECT rc; ::GetClientRect(hCtrl, &rc);
				const int span  = (bar == SB_VERT) ? rc.bottom : rc.right;
				const int denom = (si.nPage != 0) ? (int)si.nPage : (si.nMax + 1);
				if (denom <= 0) {
					return;
				}
				const int delta = (int)((LONGLONG)span * (target - si.nPos) / denom);
				if (delta != 0) {
					if (bar == SB_VERT) ::SendMessageW(hCtrl, LVM_SCROLL, 0, delta);
					else                ::SendMessageW(hCtrl, LVM_SCROLL, delta, 0);
				}
				return;
			}

			// Generic controls (tree-view, list-box): step there with line scrolls, which every
			// control honours. These have few scroll units, so the loop stays cheap.
			const UINT msg = (bar == SB_VERT) ? WM_VSCROLL : WM_HSCROLL;
			SCROLLINFO si = { sizeof(si), SIF_POS };
			if (!::GetScrollInfo(hCtrl, bar, &si)) {
				return;
			}
			int cur = si.nPos;
			for (int guard = 0; cur != target && guard < 20000; ++guard) {
				const bool down = target > cur; // SB_LINEDOWN == SB_LINERIGHT, SB_LINEUP == SB_LINELEFT
				DefSubclassProc(hCtrl, msg, MAKEWPARAM(down ? SB_LINEDOWN : SB_LINEUP, 0), 0);
				SCROLLINFO now = { sizeof(now), SIF_POS };
				::GetScrollInfo(hCtrl, bar, &now);
				if (now.nPos == cur) {
					break; // clamped at an end, cannot move further
				}
				const bool crossed = (now.nPos < target) != (cur < target);
				cur = now.nPos;
				if (crossed) {
					break; // one line stepped past the target (control's line > 1 unit); close enough
				}
			}
		}

		LRESULT CALLBACK CoolSBSyncProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uId*/, DWORD_PTR /*dw*/) {
			switch (msg) {
				case WM_VSCROLL:
				case WM_HSCROLL: {
					const UINT bar  = (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ;
					const int  code = LOWORD(wParam);
					LRESULT r = 0;
					if (code == SB_THUMBTRACK || code == SB_THUMBPOSITION) {
						ScrollToPos(hWnd, bar, HIWORD(wParam)); // drive the scroll ourselves
					} else {
						r = DefSubclassProc(hWnd, msg, wParam, lParam);
					}
					// A horizontal scroll bitblts the content and leaves our hand-drawn vertical
					// column grid lines behind as ghosts (only the newly exposed strip repaints).
					// Force a full repaint so they are redrawn cleanly.
					if (msg == WM_HSCROLL) {
						::InvalidateRect(hWnd, nullptr, TRUE);
					}
					SyncCoolSB(hWnd);
					return r;
				}
				case WM_MOUSEWHEEL:
				case WM_MOUSEHWHEEL:
				case WM_KEYDOWN:
				case WM_KEYUP: {
					const LRESULT r = DefSubclassProc(hWnd, msg, wParam, lParam);
					SyncCoolSB(hWnd);
					return r;
				}
				case WM_PAINT: {
					const LRESULT r = DefSubclassProc(hWnd, msg, wParam, lParam);
					// When the control's scroll range changes (e.g. the Keys list is filtered as
					// the user types) CoolSB doesn't hear about it. Detect the mismatch cheaply
					// and recompute the flat scrollbar; only acts when the range actually changed.
					SCROLLINFO real = { sizeof(real), SIF_RANGE | SIF_PAGE };
					SCROLLINFO cool = { sizeof(cool), SIF_RANGE | SIF_PAGE };
					if (::GetScrollInfo(hWnd, SB_VERT, &real) && CoolSB_GetScrollInfo(hWnd, SB_VERT, &cool)
							&& (real.nMin != cool.nMin || real.nMax != cool.nMax || real.nPage != cool.nPage)) {
						SyncCoolSB(hWnd);
						UpdateBarVisibility(hWnd);
					}
					return r;
				}
				case WM_NCDESTROY:
					RemoveWindowSubclass(hWnd, CoolSBSyncProc, kCoolSBSyncId);
					break;
			}
			return DefSubclassProc(hWnd, msg, wParam, lParam);
		}

		// Scrollbars are replaced with CoolSB -- the same flat dark scrollbar the playlist uses
		// (InitializeCoolSB is handed our ThemeRGB palette). CoolSB actually replaces the native
		// scrollbar (it removes the WS_*SCROLL styles and owns the non-client area), so unlike
		// painting over the native bar nothing of the original ever shows through while scrolling.
		void ApplyScrollBars(HWND hCtrl) {
			if (CoolSB_IsCoolScrollEnabled(hCtrl)) {
				return; // already initialised (ApplyThemeToChildren may run more than once)
			}
			// Install the sync subclass first so it sits behind CoolSB's window proc and sees the
			// scroll messages after the control has processed (and scrolled) them.
			SetWindowSubclass(hCtrl, CoolSBSyncProc, kCoolSBSyncId, 0);
			if (!InitializeCoolSB(hCtrl, ThemeRGB)) {
				RemoveWindowSubclass(hCtrl, CoolSBSyncProc, kCoolSBSyncId);
				return;
			}
			CoolSB_SetStyle(hCtrl, SB_VERT, CSBS_HOTTRACKED);
			CoolSB_SetStyle(hCtrl, SB_HORZ, CSBS_HOTTRACKED);
			if (SysVersion::IsWin8orLater()) {
				CoolSB_SetSize(hCtrl, SB_VERT, ::GetSystemMetrics(SM_CYVSCROLL), ::GetSystemMetrics(SM_CXVSCROLL));
				CoolSB_SetSize(hCtrl, SB_HORZ, ::GetSystemMetrics(SM_CXHSCROLL), ::GetSystemMetrics(SM_CYHSCROLL));
			}
			// Seed CoolSB with the control's current scroll range/position.
			SyncCoolSB(hCtrl);
			// Hide the bars that have nothing to scroll (e.g. the tree's horizontal bar).
			UpdateBarVisibility(hCtrl);
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
					const LRESULT r = DefSubclassProc(hWnd, msg, wParam, lParam); // draws scrollbars first
					HDC hdc = ::GetWindowDC(hWnd);
					if (hdc) {
						RECT wr;
						::GetWindowRect(hWnd, &wr);
						RECT rc = { 0, 0, wr.right - wr.left, wr.bottom - wr.top };
						POINT org = { 0, 0 };
						::ClientToScreen(hWnd, &org);
						int edge = org.y - wr.top; // border thickness: 1 (WS_BORDER) or 2 (client edge)
						if (edge < 1) {
							edge = 1;
						}
						HBRUSH br = ::CreateSolidBrush(CtrlBorderColor());
						for (int k = 0; k < edge; ++k) {
							::FrameRect(hdc, &rc, br);
							::InflateRect(&rc, -1, -1);
						}
						::DeleteObject(br);
						::ReleaseDC(hWnd, hdc);
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

		void ThemeControl(HWND hCtrl) {
			if (pAllowDarkModeForWindow) {
				pAllowDarkModeForWindow(hCtrl, true);
			}

			wchar_t cls[64] = {};
			GetClassNameW(hCtrl, cls, _countof(cls));

			if (_wcsicmp(cls, L"Button") == 0) {
				if ((GetWindowLongW(hCtrl, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX) {
					SetWindowSubclass(hCtrl, GroupBoxSubclassProc, kGroupBoxSubclassId, 0);
					InvalidateRect(hCtrl, nullptr, TRUE);
				} else {
					// checkboxes, radios, push buttons
					SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
				}
			} else if (_wcsicmp(cls, L"ComboBox") == 0) {
				SetWindowTheme(hCtrl, L"DarkMode_CFD", nullptr);
			} else if (_wcsicmp(cls, L"Edit") == 0) {
				SetWindowTheme(hCtrl, L"DarkMode_CFD", nullptr);
				ApplyDarkBorder(hCtrl);
				if (GetWindowLongW(hCtrl, GWL_STYLE) & ES_MULTILINE) {
					ApplyScrollBars(hCtrl); // multiline edits can show scrollbars
				}
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
				ApplyScrollBars(hCtrl); // flat MPC-HC-style scrollbars
			} else if (_wcsicmp(cls, L"SysTreeView32") == 0) {
				// Tree-views, like list-views, need their background/text colors set
				// explicitly (SetWindowTheme only handles the glyphs and scrollbar).
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
				::SendMessageW(hCtrl, TVM_SETBKCOLOR,   0, static_cast<LPARAM>(FaceColor()));
				::SendMessageW(hCtrl, TVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(TextColor()));
				ApplyDarkBorder(hCtrl); // dark outer border to match everything else
				ApplyScrollBars(hCtrl); // flat MPC-HC-style scrollbars
			} else if (_wcsicmp(cls, UPDOWN_CLASSW) == 0) {
				// Spin buttons: fully owner-drawn (native dark mode leaves them light).
				SetWindowSubclass(hCtrl, SpinSubclassProc, kSpinSubclassId, 0);
				InvalidateRect(hCtrl, nullptr, TRUE);
			} else if (_wcsicmp(cls, L"ListBox") == 0) {
				// Plain list boxes (e.g. DVD preferred-language) get their interior from
				// WM_CTLCOLORLISTBOX (handled by the page), but their sunken border stays
				// light — repaint it with the shared dark border like edits/lists/trees.
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
				ApplyDarkBorder(hCtrl);
				ApplyScrollBars(hCtrl); // flat MPC-HC-style scrollbars
			} else if (_wcsicmp(cls, L"Static") == 0) {
				// Sunken value boxes (e.g. Brightness/Contrast/Hue/Saturation on the
				// Color correction page are SS_SUNKEN RTEXT statics) keep a light 3D edge
				// under native dark mode. Their interior comes from WM_CTLCOLORSTATIC
				// (handled by the page); repaint just the edge with the dark border.
				const LONG st = GetWindowLongW(hCtrl, GWL_STYLE);
				const LONG ex = GetWindowLongW(hCtrl, GWL_EXSTYLE);
				if ((st & SS_SUNKEN) || (ex & (WS_EX_CLIENTEDGE | WS_EX_STATICEDGE))) {
					ApplyDarkBorder(hCtrl);
				}
			} else {
				// Note: SysTabControl32 is handled by CDarkTabCtrl (a CTabCtrl-derived
				// owner-drawn control), not here — installing a comctl subclass would sit
				// in front of MFC's WndProc and steal WM_PAINT from that class.
				// up-down, scrollbar, trackbar, ...
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
			RemoveWindowSubclass(hChild, SpinSubclassProc,     kSpinSubclassId);
			RemoveWindowSubclass(hChild, BorderSubclassProc,   kBorderSubclassId);
			RemoveWindowSubclass(hChild, TrackbarSubclassProc, kTrackbarSubclassId);
			RemoveWindowSubclass(hChild, CoolSBSyncProc,       kCoolSBSyncId);

			DWORD_PTR gridFlag = 0;
			const bool hadGrid = GetWindowSubclass(hChild, ListViewSubclassProc, kListViewSubclassId, &gridFlag) && gridFlag;
			RemoveWindowSubclass(hChild, ListViewSubclassProc, kListViewSubclassId);

			if (CoolSB_IsCoolScrollEnabled(hChild)) {
				UninitializeCoolSB(hChild); // restores the native scrollbars + window proc
			}

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

	COLORREF FaceColor()       { return ThemeRGB(22, 27, 32); }
	COLORREF TextColor()       { return ThemeRGB(165, 170, 175); }
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

	void MakeTrackbarOwnerDrawn(HWND hTrackbar) {
		if (!IsActive() || !hTrackbar) {
			return;
		}
		SetWindowSubclass(hTrackbar, TrackbarSubclassProc, kTrackbarSubclassId, 0);
		::InvalidateRect(hTrackbar, nullptr, TRUE);
	}

	void ThemeScrollBars(HWND hCtrl) {
		if (!IsActive() || !hCtrl) {
			return;
		}
		ApplyScrollBars(hCtrl);
	}

	void ThemeDialog(HWND hDlg) {
		if (!IsActive() || !hDlg) {
			return;
		}
		LoadApi();
		EnableForWindow(hDlg);                 // dark title bar + allow dark mode
		SetWindowSubclass(hDlg, DialogSubclassProc, kDialogSubclassId, 0); // dark bg / ctl colours
		ApplyThemeToChildren(hDlg);            // theme the child controls
		::InvalidateRect(hDlg, nullptr, TRUE);
	}

	void RefreshTheme(HWND hRoot) {
		if (!hRoot) {
			return;
		}
		LoadApi();
		if (IsActive()) {
			// Turned on at runtime: (re)apply to the sheet and every already-created page.
			EnableForWindow(hRoot);            // dark title bar (checks IsActive internally)
			ApplyThemeToChildren(hRoot);       // recurses into all descendant controls
		} else {
			// Turned off at runtime: strip every subclass/override so everything goes light.
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
			pDC->SetTextColor(disabled ? ThemeRGB(110, 115, 120) : TextColor());

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
		pDC->SetTextColor(disabled ? ThemeRGB(110, 115, 120) : TextColor());

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
