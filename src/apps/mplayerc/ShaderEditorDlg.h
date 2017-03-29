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

#pragma once

#include <PixelShaderCompiler.h>
#include "LineNumberEdit.h"
#include "ShaderAutoCompleteDlg.h"
#include <ResizableLib/ResizableDialog.h>

// CShaderEdit

class CShaderEdit : public CLineNumberEdit
{
	int m_nEndChar;
	UINT_PTR m_nIDEvent;

public:
	CShaderEdit();
	~CShaderEdit();

	CShaderAutoCompleteDlg m_acdlg;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdate();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

// CShaderEditorDlg dialog

class CPixelShaderCompiler;

class CShaderEditorDlg : public CResizableDialog
{
private:
	bool m_fSplitterGrabbed;
	CPixelShaderCompiler* m_pPSC;
	ShaderC* m_pShader;

	bool HitTestSplitter(CPoint p);
	void NewShader();
	void DeleteShader();


public:
	CShaderEditorDlg();
	virtual ~CShaderEditorDlg();

	BOOL Create(CWnd* pParent = NULL);

	enum { IDD = IDD_SHADEREDITOR_DLG };
	CComboBox m_cbLabels;
	CComboBox m_cbProfile;
	CShaderEdit m_edSrcdata;
	CEdit m_edOutput;
	CFont m_Font;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK() {}
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedMenu();
	afx_msg void OnBnClickedApply();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};
