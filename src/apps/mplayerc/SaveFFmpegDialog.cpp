/*
 * (C) 2026 see Authors.txt
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
#include "SaveFFmpegDialog.h"

// CSaveFFmpegDialog

IMPLEMENT_DYNAMIC(CSaveFFmpegDialog, CSaveFileDialog)
CSaveFFmpegDialog::CSaveFFmpegDialog(
	const bool bEnableFFmpeg, const bool bMuxFFmpeg,
	LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
	LPCWSTR lpszFilter, CWnd* pParentWnd)
	: CSaveFileDialog(lpszDefExt, lpszFileName,
				  OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
				  lpszFilter, pParentWnd)
	, m_bMuxFFmpeg(bMuxFFmpeg)
{
	if (bEnableFFmpeg) {
		IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
		if (pfdc) {
			pfdc->AddCheckButton(IDS_MERGE_USING_FFMPEG, ResStr(IDS_MERGE_USING_FFMPEG), m_bMuxFFmpeg);

			pfdc->Release();
		}
	}
}

BOOL CSaveFFmpegDialog::OnInitDialog()
{
	__super::OnInitDialog();

	OnTypeChange();

	return TRUE;
}

BOOL CSaveFFmpegDialog::OnFileNameOK()
{
	IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
	if (pfdc) {
		BOOL bChecked;
		if (SUCCEEDED(pfdc->GetCheckButtonState(IDS_MERGE_USING_FFMPEG, &bChecked))) {
			m_bMuxFFmpeg = !!bChecked;
		}

		pfdc->Release();
	}

	return __super::OnFileNameOK();
}
