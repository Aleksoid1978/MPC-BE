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
#include "PlayerVolumeCtrl.h"

// CPlayerToolBar

class CMainFrame;

class CPlayerToolBar : public CToolBar
{
	DECLARE_DYNAMIC(CPlayerToolBar)

private:
	CMainFrame*	m_pMainFrame;

	int			m_nUseDarkTheme;
	bool		m_bMute;

	HICON		m_hDXVAIcon;
	LONG		m_nDXVAIconWidth;
	LONG		m_nDXVAIconHeight;

	bool		m_bDisableImgListRemap;

	CMPCPngImage m_BackGroundbm;
	CImageList*	m_pButtonsImages;
	CImageList	m_reImgListActive;
	CImageList	m_reImgListDisabled;

	int			m_nWidthIncrease = 0;

public:
	LONG		m_nButtonHeight;
	CVolumeCtrl	m_volctrl;

	BOOL		m_bAudioEnable = FALSE;
	BOOL		m_bSubtitleEnable = FALSE;

	CPlayerToolBar(CMainFrame* pMainFrame);
	virtual ~CPlayerToolBar();

	virtual BOOL Create(CWnd* pParentWnd);

	void SwitchTheme();

	int GetVolume();
	int GetMinWidth();
	void SetVolume(int volume);
	__declspec(property(get=GetVolume, put=SetVolume)) int Volume;

	void ScaleToolbar();

private:
	bool IsMuted();
	void SetMute(bool fMute = true);
	int getHitButtonIdx(CPoint point);
	void CreateRemappedImgList(UINT bmID, int nRemapState, CImageList& reImgList);
	void SwitchRemmapedImgList(UINT bmID, int nRemapState);

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual ULONG GetGestureStatus(CPoint) { return 0; };

	afx_msg void OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnInitialUpdate();
	afx_msg BOOL OnVolumeMute(UINT nID);
	afx_msg void OnUpdateVolumeMute(CCmdUI* pCmdUI);

	afx_msg BOOL OnPause(UINT nID);
	afx_msg BOOL OnPlay(UINT nID);
	afx_msg BOOL OnStop(UINT nID);

	afx_msg BOOL OnVolumeUp(UINT nID);
	afx_msg BOOL OnVolumeDown(UINT nID);

	afx_msg void OnNcPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnUpdateAudio(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSubtitle(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};
