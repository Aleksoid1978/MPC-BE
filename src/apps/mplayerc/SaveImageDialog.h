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

#pragma once

// CSaveImageDialog

class CSaveImageDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CSaveImageDialog)

public:
	CSaveImageDialog(
		int quality, int levelPNG,
		LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL,
		LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL);
	virtual ~CSaveImageDialog();

protected:
	DECLARE_MESSAGE_MAP()
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnFileNameOK();
	virtual void OnTypeChange();

public:
	int m_quality, m_levelPNG;
	CSpinButtonCtrl m_qualityctrl;
};

// CSaveThumbnailsDialog

class CSaveThumbnailsDialog : public CSaveImageDialog
{
	DECLARE_DYNAMIC(CSaveThumbnailsDialog)

public:
	CSaveThumbnailsDialog(
		int rows, int cols, int width, int quality, int levelPNG,
		LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL,
		LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL);
	virtual ~CSaveThumbnailsDialog();

protected:
	DECLARE_MESSAGE_MAP()
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnFileNameOK();

public:
	int m_rows, m_cols, m_width;
	CSpinButtonCtrl m_rowsctrl, m_colsctrl, m_widthctrl;
};
