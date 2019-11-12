/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2019 see Authors.txt
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

// CSaveImageDialog

class CSaveImageDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CSaveImageDialog)

public:
	CSaveImageDialog(
		const int quality, const int levelPNG, const bool bSnapShotSubtitles, const bool bSubtitlesEnabled,
		LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
		LPCWSTR lpszFilter, CWnd* pParentWnd);
	~CSaveImageDialog() = default;

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnFileNameOK();
	virtual void OnTypeChange();

public:
	int m_quality, m_levelPNG;
	bool m_bSnapShotSubtitles;
	CSpinButtonCtrl m_qualityctrl;
};

// CSaveThumbnailsDialog

class CSaveThumbnailsDialog : public CSaveImageDialog
{
	DECLARE_DYNAMIC(CSaveThumbnailsDialog)

public:
	CSaveThumbnailsDialog(
		const int rows, const int cols, const int width, const int quality, const int levelPNG, const bool bSnapShotSubtitles, bool bSubtitlesEnabled,
		LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
		LPCWSTR lpszFilter, CWnd* pParentWnd);
	~CSaveThumbnailsDialog() = default;

protected:
	virtual BOOL OnFileNameOK();

public:
	int m_rows, m_cols, m_width;
	CSpinButtonCtrl m_rowsctrl, m_colsctrl, m_widthctrl;
};
