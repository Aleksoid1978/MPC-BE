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

#include "stdafx.h"
#include "SaveTextFileDialog.h"

// CSaveTextFileDialog

IMPLEMENT_DYNAMIC(CSaveTextFileDialog, CFileDialog)
CSaveTextFileDialog::CSaveTextFileDialog(
	CTextFile::enc e,
	LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
	LPCTSTR lpszFilter, CWnd* pParentWnd,
	BOOL bDisableExternalStyleCheckBox, BOOL bSaveExternalStyleFile)
	: CFileDialog(FALSE, lpszDefExt, lpszFileName,
				  OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
				  lpszFilter, pParentWnd)
	, m_e(e)
	, m_bDisableExternalStyleCheckBox(bDisableExternalStyleCheckBox)
	, m_bSaveExternalStyleFile(bSaveExternalStyleFile)
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		pfdc->StartVisualGroup(IDS_TEXTFILE_ENC, ResStr(IDS_TEXTFILE_ENC));
		pfdc->AddComboBox(IDC_COMBO1);
		pfdc->AddControlItem(IDC_COMBO1, CTextFile::ASCII, _T("ANSI"));
		pfdc->AddControlItem(IDC_COMBO1, CTextFile::LE16,  _T("Unicode 16-LE"));
		pfdc->AddControlItem(IDC_COMBO1, CTextFile::BE16,  _T("Unicode 16-BE"));
		pfdc->AddControlItem(IDC_COMBO1, CTextFile::UTF8,  _T("UTF-8"));
		pfdc->SetSelectedControlItem(IDC_COMBO1, m_e);
		pfdc->EndVisualGroup();
		pfdc->MakeProminent(IDS_TEXTFILE_ENC);

		pfdc->AddCheckButton(IDC_CHECK1, ResStr(IDS_SUB_SAVE_EXTERNAL_STYLE_FILE), m_bSaveExternalStyleFile);
		pfdc->SetControlState(IDC_CHECK1, m_bDisableExternalStyleCheckBox ? CDCS_INACTIVE : CDCS_ENABLEDVISIBLE);

		pfdc->Release();
	}
}

CSaveTextFileDialog::~CSaveTextFileDialog()
{
}

BOOL CSaveTextFileDialog::OnFileNameOK()
{
	DWORD result;
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		pfdc->GetSelectedControlItem(IDC_COMBO1, &result);
		pfdc->Release();
		m_e = (CTextFile::enc)result;
	}

	pfdc->GetCheckButtonState(IDC_CHECK1, &m_bSaveExternalStyleFile);

	return __super::OnFileNameOK();
}
