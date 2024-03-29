/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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

#include "DSUtil/DSMPropertyBag.h"

// CPlayerSeekBar

class CMainFrame;

class CPlayerSeekBar : public CDialogBar
{
	DECLARE_DYNAMIC(CPlayerSeekBar)

	friend class CMainFrame;

private:
	enum tooltip_state { TOOLTIP_HIDDEN, TOOLTIP_TRIGGERED, TOOLTIP_VISIBLE };

	REFERENCE_TIME m_stop           = 0;
	REFERENCE_TIME m_pos            = 0;
	REFERENCE_TIME m_posreal        = 0;
	REFERENCE_TIME m_pos_preview    = 0;
	bool           m_bEnabled       = false;
	tooltip_state  m_tooltipState   = TOOLTIP_HIDDEN;
	REFERENCE_TIME m_tooltipPos     = 0;
	REFERENCE_TIME m_tooltipLastPos = -1;
	UINT_PTR       m_tooltipTimer   = 1;

	TOOLINFOW       m_ti            = { sizeof(TOOLINFOW), TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE };
	CToolTipCtrl    m_tooltip;
	CCritSec        m_CBLock;
	CComPtr<IDSMChapterBag> m_pChapterBag;
	CString         m_strChap;
	CRect           m_rLock;

	CMainFrame*     m_pMainFrame;

	CPoint          m_CurrentPoint;

	CFont           m_font;

	REFERENCE_TIME CalculatePosition(const CPoint point);

	void MoveThumb(const CPoint point);
	void SetPosInternal(const REFERENCE_TIME pos);

	void MoveThumbPreview(const CPoint point);
	void SetPosInternalPreview(const REFERENCE_TIME pos);

	void UpdateTooltip(const CPoint point);

	CRect GetChannelRect();
	CRect GetThumbRect();
	CRect GetInnerThumbRect();

	struct t_color {
		int R,G,B;
	};

	t_color m_crBackground;
	TRIVERTEX tvBackground[2];

	CPen m_penPlayed1, m_penPlayed2, m_penChapters;
	CPen m_penRepeatAB;

	TRIVERTEX tvBufferingProgress[2];

	TRIVERTEX tvBackgroundEnabledLeft[2];
	TRIVERTEX tvBackgroundEnabledRight[2];

	COLORREF m_crText;
	COLORREF m_crHighlightedText;
	COLORREF m_crTimeText;

	int m_scaleX1 = 1;
	int m_scaleX4 = 4;
	int m_scaleX8 = 8;

	int m_scaleY2  = 2;
	int m_scaleY5  = 5;
	int m_scaleY7  = 7;
	int m_scaleY14 = 14;

	std::unique_ptr<CDC> m_pEnabledThumb;
	std::unique_ptr<CDC> m_pDisabledThumb;

public:
	CPlayerSeekBar(CMainFrame* pMainFrame);
	virtual ~CPlayerSeekBar() = default;

	void Enable(bool bEnable);

	REFERENCE_TIME GetRange() const { return m_stop; }
	void SetRange(const REFERENCE_TIME stop);
	void SetPos(const REFERENCE_TIME pos);

	REFERENCE_TIME GetPos() const { return m_pos; }
	REFERENCE_TIME GetPosReal() const { return m_posreal; }
	BOOL HasDuration() const { return m_stop > 0; }

	void HideToolTip();
	void UpdateToolTipPosition(CPoint point);
	void UpdateToolTipText();

	void SetChapterBag(CComPtr<IDSMChapterBag>& pCB);
	void RemoveChapters();

	virtual BOOL Create(CWnd* pParentWnd);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override;

	BOOL OnPlayStop(UINT nID);

	void PreviewWindowShow();

	void ScaleFont();

	void SetColor();

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
