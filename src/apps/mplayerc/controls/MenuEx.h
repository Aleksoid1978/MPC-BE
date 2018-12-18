/*
 * (C) 2018 see Authors.txt
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

typedef struct tagMENUITEM
{
	CString strText;
	UINT    uID = 0;
	bool    bMainMenu = false;
	bool    bPopupMenu = false;
} MENUITEM;
typedef MENUITEM* LPMENUITEM;

class CMainFrame;

class CMenuEx
{
	static LRESULT CALLBACK MenuWndProc(HWND hWnd, UINT mess, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
        
	static inline HHOOK m_hookCBT = nullptr;

public:
	CMenuEx() = default;
	~CMenuEx() = default;

	static void ChangeStyle(CMenu *pMenu, const bool bMainMenu = false);
	static void MeasureItem(LPMEASUREITEMSTRUCT  lpMIS);
	static void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	static void SetColorMenu(
				const COLORREF crBkBar,
				const COLORREF crBN, const COLORREF crBNL, const COLORREF crBND,
				const COLORREF crBR, const COLORREF crBRL, const COLORREF crBRD,
				const COLORREF crSR, const COLORREF crSRL, const COLORREF crSRD,
				const COLORREF crTN, const COLORREF crTNL, const COLORREF crTG, const COLORREF crTGL);

	static void FreeResource();
	static void SetMain(CMainFrame* pMainFrame);
	static void ScaleFont();

	static void Hook();
	static void UnHook();
	static void EnableHook(const bool bEnable);

protected:
	static inline COLORREF m_crBkBar;

	static inline COLORREF m_crBN;
	static inline COLORREF m_crBNL;
	static inline COLORREF m_crBND;

	static inline COLORREF m_crBR;
	static inline COLORREF m_crBRL;
	static inline COLORREF m_crBRD;

	static inline COLORREF m_crBS;
	static inline COLORREF m_crBSL;
	static inline COLORREF m_crBSD;

	static inline COLORREF m_crTN;
	static inline COLORREF m_crTNL;
	static inline COLORREF m_crTG;
	static inline COLORREF m_crTGL;

	static inline int m_CYEDGE = 0;
	static inline int m_CYMENU = 0;
	static inline int m_CXMENUCHECK = 0;
	static inline int m_CYMENUCHECK = 0;

	static inline bool m_bUseDrawHook = false;

	static inline CFont m_font;
	static inline std::vector<MENUITEM*> m_pMenuItems;

	static inline CMainFrame* m_pMainFrame = nullptr;

	static void DrawCheckMark(CDC* pDC, CRect rect, const UINT uState, const bool bGrayed, const bool bSelected, CRect& rcMark);
	static void TextMenu(CDC *pDC, const CRect &rect, CRect rcText, const bool bSelected, const bool bGrayed, const LPMENUITEM lpItem);
};
