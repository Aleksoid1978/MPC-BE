/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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

class CMainFrame;

class CChildView : public CWnd
{
	CRect m_videoRect;
	CRect m_logoRect;

	CCritSec m_csLogo;
	CComPtr<IWICBitmap> m_pBitmap;
	CComPtr<IWICBitmap> m_pBitmapResized;

	HCURSOR m_hCursor = nullptr;
	CPoint m_lastMousePoint{ -1, -1 };

	CMainFrame* m_pMainFrame = nullptr;

public:
	CChildView(CMainFrame* pMainFrame);
	~CChildView() = default;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseLeave();

	DECLARE_DYNAMIC(CChildView)

	void SetVideoRect(CRect r = {});

	CRect GetVideoRect() const {
		return(m_videoRect);
	}
	CRect GetLogoRect() const {
		return(m_logoRect);
	}

	void LoadLogo();
	CSize GetLogoSize() const;

	void ClearResizedImage();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual ULONG GetGestureStatus(CPoint) { return 0; };

	virtual BOOL OnTouchInput(CPoint pt, int nInputNumber, int nInputsCount, PTOUCHINPUT pInput);

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);

	DECLARE_MESSAGE_MAP()

	bool m_bTrackingMouseLeave = false;
};
