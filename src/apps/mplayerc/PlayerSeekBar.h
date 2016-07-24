/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

// CPlayerSeekBar

class CMainFrame;

class CPlayerSeekBar : public CDialogBar
{
	DECLARE_DYNAMIC(CPlayerSeekBar)

private:
	enum tooltip_state { TOOLTIP_HIDDEN, TOOLTIP_TRIGGERED, TOOLTIP_VISIBLE };

	REFERENCE_TIME m_stop           = 0;
	REFERENCE_TIME m_pos            = 0;
	REFERENCE_TIME m_posreal        = 0;
	REFERENCE_TIME m_pos2           = 0;
	REFERENCE_TIME m_posreal2       = 0;
	bool           m_bEnabled       = false;
	tooltip_state  m_tooltipState   = TOOLTIP_HIDDEN;
	REFERENCE_TIME m_tooltipPos     = 0;
	REFERENCE_TIME m_tooltipLastPos = -1;
	UINT_PTR       m_tooltipTimer   = 1;

	TOOLINFO        m_ti            = {};
	CToolTipCtrl    m_tooltip;
	CMPCPngImage    m_BackGroundbm;
	CCritSec        m_CBLock;
	CComPtr<IDSMChapterBag> m_pChapterBag;
	CString         m_strChap;
	CRect           m_rLock;

	CMainFrame*     m_pMainFrame;

	CPoint          m_CurrentPoint;

	void MoveThumb(CPoint point);
	REFERENCE_TIME CalculatePosition(CPoint point);
	void SetPosInternal(REFERENCE_TIME pos);

	void MoveThumb2(CPoint point);
	void SetPosInternal2(REFERENCE_TIME pos);
	void UpdateTooltip(CPoint point);

	CRect GetChannelRect();
	CRect GetThumbRect();
	CRect GetInnerThumbRect();

public:
	CPlayerSeekBar(CMainFrame* pMainFrame);
	virtual ~CPlayerSeekBar();

	void Enable(bool bEnable);

	void GetRange(REFERENCE_TIME& stop);
	void SetRange(REFERENCE_TIME stop);
	void SetPos(REFERENCE_TIME pos);

	REFERENCE_TIME GetPos() const { return m_pos; }
	REFERENCE_TIME GetPosReal() const { return m_posreal; }
	BOOL HasDuration() const { return m_stop > 0; }

	void HideToolTip();
	void UpdateToolTipPosition(CPoint& point);
	void UpdateToolTipText();

	void SetChapterBag(CComPtr<IDSMChapterBag>& pCB);

	virtual BOOL Create(CWnd* pParentWnd);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override;

	BOOL OnPlayStop(UINT nID);

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual ULONG GetGestureStatus(CPoint) { return 0; }

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC) { return TRUE; }
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};
