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

#pragma once

#include "FileDialogs.h"

// CSaveFFmpegDialog

class CSaveFFmpegDialog : public CSaveFileDialog
{
	DECLARE_DYNAMIC(CSaveFFmpegDialog)

public:
	CSaveFFmpegDialog(
		const bool bEnableFFmpeg, const bool bMuxFFmpeg,
		LPCWSTR lpszDefExt, LPCWSTR lpszFileName,
		LPCWSTR lpszFilter, CWnd* pParentWnd);
	~CSaveFFmpegDialog() = default;

protected:
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnFileNameOK() override;

public:
	bool m_bMuxFFmpeg;
};
