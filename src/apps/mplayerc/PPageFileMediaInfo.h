/*
 * (C) 2012-2025 see Authors.txt
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

// CPPageFileMediaInfo dialog

class CPPageFileMediaInfo : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPageFileMediaInfo)

private:
	CDPI* m_pSheetDpi;

	CComboBox m_cbFilename;
	CEdit m_edMediainfo;
	CFont m_font;

	std::list<CStringW> m_files;
	int m_fileindex = -1;

public:
	CPPageFileMediaInfo(const std::list<CStringW>& files, CDPI* pSheetDpi);
	virtual ~CPPageFileMediaInfo() = default;

	enum { IDD = IDD_FILEMEDIAINFO };
	CStringW MI_File;
	CStringW MI_Text;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	CRect m_rCrt;

public:
	afx_msg BOOL OnSetActive();
	afx_msg BOOL OnKillActive();
	afx_msg void OnComboFileChange();
};
