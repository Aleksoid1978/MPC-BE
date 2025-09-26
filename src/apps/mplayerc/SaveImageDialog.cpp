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

#include "stdafx.h"
#include "SaveImageDialog.h"

// CSaveImageDialog

IMPLEMENT_DYNAMIC(CSaveImageDialog, CSaveFileDialog)
CSaveImageDialog::CSaveImageDialog(
	const int quality, const int levelPNG,
	const bool bSnapShotSubtitles, const bool bSubtitlesEnabled,
	LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
	LPCWSTR lpszFilter, CWnd* pParentWnd)
	: CSaveFileDialog(lpszDefExt, lpszFileName,
				  OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
				  lpszFilter, pParentWnd)
	, m_JpegQuality(std::clamp(quality, 70, 100))
	, m_PngCompression(std::clamp(levelPNG, 1, 9))
	, m_bDrawSubtitles(bSnapShotSubtitles)
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		CStringW str;

		pfdc->StartVisualGroup(IDS_AG_OPTIONS, ResStr(IDS_AG_OPTIONS));
		pfdc->AddText(IDS_THUMB_QUALITY, ResStr(IDS_THUMB_QUALITY));
		str.Format(L"%d", m_JpegQuality);
		pfdc->AddEditBox(IDC_EDIT1, str);

		pfdc->AddText(IDS_THUMB_LEVEL, ResStr(IDS_THUMB_LEVEL));
		str.Format(L"%d", m_PngCompression);
		pfdc->AddEditBox(IDC_EDIT5, str);
		pfdc->EndVisualGroup();

		if (bSubtitlesEnabled) {
			pfdc->AddCheckButton(IDS_SNAPSHOT_SUBTITLES, ResStr(IDS_SNAPSHOT_SUBTITLES), m_bDrawSubtitles);
		}

		pfdc->Release();
	}
}

BOOL CSaveImageDialog::OnInitDialog()
{
	__super::OnInitDialog();

	OnTypeChange();

	return TRUE;
}

BOOL CSaveImageDialog::OnFileNameOK()
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		WCHAR* result;

		pfdc->GetEditBoxText(IDC_EDIT1, &result);
		m_JpegQuality = _wtoi(result);
		CoTaskMemFree(result);

		pfdc->GetEditBoxText(IDC_EDIT5, &result);
		m_PngCompression = _wtoi(result);
		CoTaskMemFree(result);

		BOOL bChecked;
		if (SUCCEEDED(pfdc->GetCheckButtonState(IDS_SNAPSHOT_SUBTITLES, &bChecked))) {
			m_bDrawSubtitles = !!bChecked;
		}

		pfdc->Release();
	}

	m_JpegQuality = std::clamp(m_JpegQuality, 70, 100);
	m_PngCompression = std::clamp(m_PngCompression, 1, 9);

	return __super::OnFileNameOK();
}

void CSaveImageDialog::OnTypeChange()
{
	__super::OnTypeChange();

	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		switch (m_pOFN->nFilterIndex) {
			case 1:
				pfdc->SetControlState(IDS_AG_OPTIONS, CDCS_INACTIVE);
				pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_INACTIVE);
				pfdc->SetControlState(IDC_EDIT1, CDCS_INACTIVE);

				pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_INACTIVE);
				pfdc->SetControlState(IDC_EDIT5, CDCS_INACTIVE);
				break;
			case 2:
				pfdc->SetControlState(IDS_AG_OPTIONS, CDCS_ENABLEDVISIBLE);
				pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_ENABLEDVISIBLE);
				pfdc->SetControlState(IDC_EDIT1, CDCS_ENABLEDVISIBLE);

				pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_INACTIVE);
				pfdc->SetControlState(IDC_EDIT5, CDCS_INACTIVE);
				break;
			case 3:
				pfdc->SetControlState(IDS_AG_OPTIONS, CDCS_ENABLEDVISIBLE);
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
	const int rows, const int cols, const int width,
	const int quality, const int levelPNG,
	const bool bSnapShotSubtitles, const bool bSubtitlesEnabled,
	LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
	LPCWSTR lpszFilter, CWnd* pParentWnd)
	: CSaveImageDialog(quality, levelPNG, bSnapShotSubtitles, bSubtitlesEnabled,
					   lpszDefExt, lpszFileName,
					   lpszFilter, pParentWnd)
	, m_rows(rows)
	, m_cols(cols)
	, m_width(width)
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		CStringW str;

		pfdc->StartVisualGroup(IDS_THUMB_THUMBNAILS, ResStr(IDS_THUMB_THUMBNAILS));
		pfdc->AddText(IDS_THUMB_ROWNUMBER, ResStr(IDS_THUMB_ROWNUMBER));
		str.Format(L"%d", std::clamp(m_rows, 1, 20));
		pfdc->AddEditBox(IDC_EDIT4, str);

		pfdc->AddText(IDS_THUMB_COLNUMBER, ResStr(IDS_THUMB_COLNUMBER));
		str.Format(L"%d", std::clamp(m_cols, 1, 10));
		pfdc->AddEditBox(IDC_EDIT2, str);
		pfdc->EndVisualGroup();

		pfdc->StartVisualGroup(IDS_THUMB_IMAGE_WIDTH, ResStr(IDS_THUMB_IMAGE_WIDTH));
		pfdc->AddText(IDS_THUMB_PIXELS, ResStr(IDS_THUMB_PIXELS));
		str.Format(L"%d", std::clamp(m_width, APP_THUMBWIDTH_MIN, APP_THUMBWIDTH_MAX));
		pfdc->AddEditBox(IDC_EDIT3, str);
		pfdc->EndVisualGroup();

		pfdc->Release();
	}
}

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
