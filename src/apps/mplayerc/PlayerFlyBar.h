/*
 * (C) 2012-2021 see Authors.txt
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

#include "SvgHelper.h"

class CMainFrame;

// CFlyBar

class CFlyBar : public CWnd
{
	enum :int {
		IMG_EXIT = 0,
		IMG_EXIT_A,
		IMG_FULSCR,
		IMG_FULSCR_A,
		IMG_FULSCR_D,
		IMG_INFO,
		IMG_INFO_A,
		IMG_INFO_D,
		IMG_LOCK,
		IMG_LOCK_A,
		IMG_MAXWND,
		IMG_MAXWND_A,
		IMG_MAXWND_D,
		IMG_MINWND,
		IMG_MINWND_A,
		IMG_STDWND,
		IMG_STDWND_A,
		IMG_SETS,
		IMG_SETS_A,
		IMG_UNLOCK,
		IMG_UNLOCK_A,
		IMG_WINDOW,
		IMG_WINDOW_A,
		IMG_CLOSE,
		IMG_CLOSE_A,
	};

	DECLARE_DYNAMIC(CFlyBar)

private:
	CMainFrame* m_pMainFrame;

	CRect r_ExitIcon;
	CRect r_MinIcon;
	CRect r_RestoreIcon;
	CRect r_SettingsIcon;
	CRect r_InfoIcon;
	CRect r_FSIcon;
	CRect r_LockIcon;

	CToolTipCtrl m_tooltip;
	CImageList *m_pButtonImages = nullptr;

	int m_btIdx = 0;

	bool m_bTrackingMouseLeave = false;

public:
	CFlyBar(CMainFrame* pMainFrame);
	~CFlyBar();

	HRESULT Create(CWnd* pWnd);

	int iw;

	void CalcButtonsRect();
	void Scale();
	bool UpdateButtonImages();

private:
	void DrawButton(CDC *pDC, int nImage, int x, int z);
	void UpdateWnd(CPoint point);
	void DrawWnd();

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
