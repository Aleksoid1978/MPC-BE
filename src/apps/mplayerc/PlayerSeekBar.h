/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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

#include "PngImage.h"
#include "../../DSUtil/DSMPropertyBag.h"

#define SHOW_DELAY 200
#define AUTOPOP_DELAY 1000

// CPlayerSeekBar

class CMainFrame;

class CPlayerSeekBar : public CDialogBar
{
	DECLARE_DYNAMIC(CPlayerSeekBar)

private:
	enum tooltip_state_t { TOOLTIP_HIDDEN, TOOLTIP_TRIGGERED, TOOLTIP_VISIBLE };

	REFERENCE_TIME m_start, m_stop, m_pos, m_posreal, m_pos2, m_posreal2;
	CPoint pt2;
	CRgn m_CustomRgn;
	bool m_fEnabled;
	CToolTipCtrl m_tooltip;
	TOOLINFO m_ti;
	tooltip_state_t m_tooltipState;
	REFERENCE_TIME m_tooltipPos, m_tooltipLastPos;
	UINT_PTR m_tooltipTimer;

	CMPCPngImage m_BackGroundbm;

	void MoveThumb(CPoint point);
	REFERENCE_TIME CalculatePosition(CPoint point);
	void SetPosInternal(REFERENCE_TIME pos);

	void MoveThumb2(CPoint point);
	void SetPosInternal2(REFERENCE_TIME pos);

	CCritSec m_CBLock;
	CComPtr<IDSMChapterBag> m_pChapterBag;
	CString strChap;
	void UpdateTooltip(CPoint point);

	CRect GetChannelRect();
	CRect GetThumbRect();
	CRect GetInnerThumbRect();
	CRect r_Lock;

	CMainFrame* m_pMainFrame;

public:
	CPlayerSeekBar(CMainFrame* pMainFrame);
	virtual ~CPlayerSeekBar();

	void Enable(bool fEnable);

	void GetRange(REFERENCE_TIME& start, REFERENCE_TIME& stop);
	void SetRange(REFERENCE_TIME start, REFERENCE_TIME stop);
	REFERENCE_TIME GetPos(), GetPosReal();
	void SetPos(REFERENCE_TIME pos);

	void HideToolTip();
	void UpdateToolTipPosition(CPoint& point);
	void UpdateToolTipText();

	void SetChapterBag(CComPtr<IDSMChapterBag>& pCB);

	BOOL HasDuration() { return m_stop && m_start < m_stop; };

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlayerSeekBar)
	virtual BOOL Create(CWnd* pParentWnd);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CPlayerSeekBar)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnPlayStop(UINT nID);
};
