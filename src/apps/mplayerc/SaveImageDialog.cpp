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

#include "stdafx.h"
#include "SaveImageDialog.h"

// CSaveImageDialog

IMPLEMENT_DYNAMIC(CSaveImageDialog, CFileDialog)
CSaveImageDialog::CSaveImageDialog(
	int quality, int levelPNG, bool bSnapShotSubtitles,
	LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
	LPCWSTR lpszFilter, CWnd* pParentWnd)
	: CFileDialog(FALSE, lpszDefExt, lpszFileName,
				  OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
				  lpszFilter, pParentWnd)
	, m_quality(quality)
	, m_levelPNG(levelPNG)
	, m_bSnapShotSubtitles(bSnapShotSubtitles)
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		CString str;

		pfdc->StartVisualGroup(IDS_THUMB_IMAGE_QUALITY, ResStr(IDS_THUMB_IMAGE_QUALITY));
		pfdc->AddText(IDS_THUMB_QUALITY, ResStr(IDS_THUMB_QUALITY));
		str.Format(L"%d", clamp(m_quality, 70, 100));
		pfdc->AddEditBox(IDC_EDIT1, str);

		pfdc->AddText(IDS_THUMB_LEVEL, ResStr(IDS_THUMB_LEVEL));
		str.Format(L"%d", clamp(m_levelPNG, 1, 9));
		pfdc->AddEditBox(IDC_EDIT5, str);
		pfdc->EndVisualGroup();

		pfdc->AddCheckButton(IDS_SNAPSHOT_SUBTITLES, ResStr(IDS_SNAPSHOT_SUBTITLES), m_bSnapShotSubtitles);

		pfdc->Release();
	}
}

CSaveImageDialog::~CSaveImageDialog()
{
}

BOOL CSaveImageDialog::OnInitDialog()
{
	__super::OnInitDialog();

	OnTypeChange();

	return TRUE;
}

// CSaveImageDialog message handlers

BOOL CSaveImageDialog::OnFileNameOK()
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		WCHAR* result;

		pfdc->GetEditBoxText(IDC_EDIT1, &result);
		m_quality = _wtoi(result);
		CoTaskMemFree(result);

		pfdc->GetEditBoxText(IDC_EDIT5, &result);
		m_levelPNG = _wtoi(result);
		CoTaskMemFree(result);

		BOOL bChecked;
		pfdc->GetCheckButtonState(IDS_SNAPSHOT_SUBTITLES, &bChecked);
		m_bSnapShotSubtitles = !!bChecked;

		pfdc->Release();
	}

	m_levelPNG = clamp(m_levelPNG, 1, 9);
	m_quality = clamp(m_quality, 70, 100);

	return __super::OnFileNameOK();
}

void CSaveImageDialog::OnTypeChange()
{
	__super::OnTypeChange();

	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		switch (m_pOFN->nFilterIndex) {
			case 1:
				pfdc->SetControlState(IDS_THUMB_IMAGE_QUALITY, CDCS_INACTIVE);
				pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_INACTIVE);
				pfdc->SetControlState(IDC_EDIT1, CDCS_INACTIVE);

				pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_INACTIVE);
				pfdc->SetControlState(IDC_EDIT5, CDCS_INACTIVE);
				break;
			case 2:
				pfdc->SetControlState(IDS_THUMB_IMAGE_QUALITY, CDCS_ENABLEDVISIBLE);
				pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_ENABLEDVISIBLE);
				pfdc->SetControlState(IDC_EDIT1, CDCS_ENABLEDVISIBLE);

				pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_INACTIVE);
				pfdc->SetControlState(IDC_EDIT5, CDCS_INACTIVE);
				break;
			case 3:
				pfdc->SetControlState(IDS_THUMB_IMAGE_QUALITY, CDCS_ENABLEDVISIBLE);
				pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_INACTIVE);
				pfdc->SetControlState(IDC_EDIT1, CDCS_INACTIVE);

				pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_ENABLEDVISIBLE);
				pfdc->SetControlState(IDC_EDIT5, CDCS_ENABLEDVISIBLE);
				break;
			default :
				break;
		}

		pfdc->Release();
	}
}

// CSaveThumbnailsDialog

IMPLEMENT_DYNAMIC(CSaveThumbnailsDialog, CSaveImageDialog)
CSaveThumbnailsDialog::CSaveThumbnailsDialog(
	int rows, int cols, int width, int quality, int levelPNG, bool bSnapShotSubtitles,
	LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
	LPCWSTR lpszFilter, CWnd* pParentWnd)
	: CSaveImageDialog(quality, levelPNG, bSnapShotSubtitles,
					   lpszDefExt, lpszFileName,
					   lpszFilter, pParentWnd)
	, m_rows(rows)
	, m_cols(cols)
	, m_width(width)
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		CString str;

		pfdc->StartVisualGroup(IDS_THUMB_THUMBNAILS, ResStr(IDS_THUMB_THUMBNAILS));
		pfdc->AddText(IDS_THUMB_ROWNUMBER, ResStr(IDS_THUMB_ROWNUMBER));
		str.Format(L"%d", clamp(m_rows, 1, 20));
		pfdc->AddEditBox(IDC_EDIT4, str);

		pfdc->AddText(IDS_THUMB_COLNUMBER, ResStr(IDS_THUMB_COLNUMBER));
		str.Format(L"%d", clamp(m_cols, 1, 10));
		pfdc->AddEditBox(IDC_EDIT2, str);
		pfdc->EndVisualGroup();

		pfdc->StartVisualGroup(IDS_THUMB_IMAGE_WIDTH, ResStr(IDS_THUMB_IMAGE_WIDTH));
		pfdc->AddText(IDS_THUMB_PIXELS, ResStr(IDS_THUMB_PIXELS));
		str.Format(L"%d", clamp(m_width, 256, 2560));
		pfdc->AddEditBox(IDC_EDIT3, str);
		pfdc->EndVisualGroup();

		pfdc->Release();
	}
}

CSaveThumbnailsDialog::~CSaveThumbnailsDialog()
{
}

// CSaveThumbnailsDialog message handlers

BOOL CSaveThumbnailsDialog::OnFileNameOK()
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		WCHAR* result;

		pfdc->GetEditBoxText(IDC_EDIT4, &result);
		m_rows = _wtoi(result);
		CoTaskMemFree(result);

		pfdc->GetEditBoxText(IDC_EDIT2, &result);
		m_cols = _wtoi(result);
		CoTaskMemFree(result);

		pfdc->GetEditBoxText(IDC_EDIT3, &result);
		m_width = _wtoi(result);
		CoTaskMemFree(result);

		pfdc->Release();
	}

	return __super::OnFileNameOK();
}
