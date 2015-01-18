/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include "../../DSUtil/SysVersion.h"

// CSaveImageDialog

IMPLEMENT_DYNAMIC(CSaveImageDialog, CFileDialog)
CSaveImageDialog::CSaveImageDialog(
	int quality, int levelPNG,
	LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
	LPCTSTR lpszFilter, CWnd* pParentWnd)
	: CFileDialog(FALSE, lpszDefExt, lpszFileName,
				  OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
				  lpszFilter, pParentWnd)
	, m_quality(quality)
	, m_levelPNG(levelPNG)
{
	if (IsWinVistaOrLater()) {
		IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
		if (pfdc) {
			CString str;

			pfdc->StartVisualGroup(IDS_THUMB_IMAGE_QUALITY, ResStr(IDS_THUMB_IMAGE_QUALITY));
			pfdc->AddText(IDS_THUMB_QUALITY, ResStr(IDS_THUMB_QUALITY));
			str.Format(L"%d", max(70, min(100, m_quality)));
			pfdc->AddEditBox(IDC_EDIT1, str);

			pfdc->AddText(IDS_THUMB_LEVEL, ResStr(IDS_THUMB_LEVEL));
			str.Format(L"%d", max(1, min(9, m_levelPNG)));
			pfdc->AddEditBox(IDC_EDIT5, str);
			pfdc->EndVisualGroup();

			pfdc->Release();
		}
	} else {
		SetTemplate(0, IDD_SAVEIMAGEDIALOGTEMPL);
	}
}

CSaveImageDialog::~CSaveImageDialog()
{
}

void CSaveImageDialog::DoDataExchange(CDataExchange* pDX)
{
	if (!IsWinVistaOrLater()) {
		DDX_Control(pDX, IDC_SPIN1, m_qualityctrl);
	}

	__super::DoDataExchange(pDX);
}

BOOL CSaveImageDialog::OnInitDialog()
{
	__super::OnInitDialog();

	if (!IsWinVistaOrLater()) {
		m_qualityctrl.SetRange(70, 100);
		m_qualityctrl.SetPos(m_quality);
	}

	OnTypeChange();

	return TRUE;
}

BEGIN_MESSAGE_MAP(CSaveImageDialog, CFileDialog)
END_MESSAGE_MAP()

// CSaveImageDialog message handlers

BOOL CSaveImageDialog::OnFileNameOK()
{
	if (IsWinVistaOrLater()) {
		IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
		if (pfdc) {
			WCHAR* result;

			pfdc->GetEditBoxText(IDC_EDIT1, &result);
			m_quality = _wtoi(result);
			CoTaskMemFree(result);

			pfdc->GetEditBoxText(IDC_EDIT5, &result);
			m_levelPNG = _wtoi(result);
			CoTaskMemFree(result);

			pfdc->Release();
		}
	} else {
		m_quality = m_qualityctrl.GetPos();
	}

	m_levelPNG = max(1, min(9, m_levelPNG));
	m_quality = max(1, min(100, m_quality));

	return __super::OnFileNameOK();
}

void CSaveImageDialog::OnTypeChange()
{
	__super::OnTypeChange();

	if (IsWinVistaOrLater()) {
		IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
		if (pfdc) {
			switch (m_pOFN->nFilterIndex)
			{
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
	} else {
		m_qualityctrl.EnableWindow(m_pOFN->nFilterIndex == 2);
	}
}

// CSaveThumbnailsDialog

IMPLEMENT_DYNAMIC(CSaveThumbnailsDialog, CSaveImageDialog)
CSaveThumbnailsDialog::CSaveThumbnailsDialog(
	int rows, int cols, int width, int quality, int levelPNG,
	LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
	LPCTSTR lpszFilter, CWnd* pParentWnd)
	: CSaveImageDialog(quality, levelPNG,
					   lpszDefExt, lpszFileName,
					   lpszFilter, pParentWnd)
	, m_rows(rows)
	, m_cols(cols)
	, m_width(width)
{
	if (IsWinVistaOrLater()) {
		IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
		if (pfdc) {
			CString str;

			pfdc->StartVisualGroup(IDS_THUMB_THUMBNAILS, ResStr(IDS_THUMB_THUMBNAILS));
			pfdc->AddText(IDS_THUMB_ROWNUMBER, ResStr(IDS_THUMB_ROWNUMBER));
			str.Format(L"%d", max(1, min(20, m_rows)));
			pfdc->AddEditBox(IDC_EDIT4, str);

			pfdc->AddText(IDS_THUMB_COLNUMBER, ResStr(IDS_THUMB_COLNUMBER));
			str.Format(L"%d", max(1, min(10, m_cols)));
			pfdc->AddEditBox(IDC_EDIT2, str);
			pfdc->EndVisualGroup();

			pfdc->StartVisualGroup(IDS_THUMB_IMAGE_WIDTH, ResStr(IDS_THUMB_IMAGE_WIDTH));
			pfdc->AddText(IDS_THUMB_PIXELS, ResStr(IDS_THUMB_PIXELS));
			str.Format(L"%d", max(256, min(2560, m_width)));
			pfdc->AddEditBox(IDC_EDIT3, str);
			pfdc->EndVisualGroup();

			pfdc->Release();
		}
	} else {
		SetTemplate(0, IDD_SAVETHUMBSDIALOGTEMPL);
	}
}

CSaveThumbnailsDialog::~CSaveThumbnailsDialog()
{
}

void CSaveThumbnailsDialog::DoDataExchange(CDataExchange* pDX)
{
	if (!IsWinVistaOrLater()) {
		DDX_Control(pDX, IDC_SPIN4, m_rowsctrl);
		DDX_Control(pDX, IDC_SPIN2, m_colsctrl);
		DDX_Control(pDX, IDC_SPIN3, m_widthctrl);
	}

	__super::DoDataExchange(pDX);
}

BOOL CSaveThumbnailsDialog::OnInitDialog()
{
	__super::OnInitDialog();

	if (!IsWinVistaOrLater()) {
		m_rowsctrl.SetRange(1, 20);
		m_colsctrl.SetRange(1, 10);
		m_widthctrl.SetRange(256, 2560);

		m_rowsctrl.SetPos(m_rows);
		m_colsctrl.SetPos(m_cols);
		m_widthctrl.SetPos(m_width);
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CSaveThumbnailsDialog, CFileDialog)
END_MESSAGE_MAP()

// CSaveThumbnailsDialog message handlers

BOOL CSaveThumbnailsDialog::OnFileNameOK()
{
	if (IsWinVistaOrLater()) {
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
	} else {
		m_rows = m_rowsctrl.GetPos();
		m_cols = m_colsctrl.GetPos();
		m_width = m_widthctrl.GetPos();
	}

	return __super::OnFileNameOK();
}
