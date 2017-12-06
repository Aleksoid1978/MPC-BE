/*
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

#include <atlcoll.h>
#include "../../../DSUtil/HdmvClipInfo.h"

class CMultiFiles : public CObject
{
	DECLARE_DYNAMIC(CMultiFiles)

protected:
	std::vector<CString>        m_strFiles;
	std::vector<LONGLONG>       m_FilesSize;
	std::vector<REFERENCE_TIME> m_rtPtsOffsets;

	LONGLONG                    m_llTotalLength     = 0;
	HANDLE                      m_hFile             = INVALID_HANDLE_VALUE;
	size_t                      m_nCurPart          = SIZE_T_MAX;
	REFERENCE_TIME*             m_pCurrentPTSOffset = nullptr;

private:
	BOOL     OpenPart(size_t nPart);
	void     ClosePart();
	void     Reset();
	LONGLONG GetAbsolutePosition(LONGLONG lOff, UINT nFrom);

public:
	CMultiFiles();
	virtual ~CMultiFiles();

	virtual BOOL Open(LPCTSTR lpszFileName);
	virtual BOOL OpenFiles(CHdmvClipInfo::CPlaylist& files);

	virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
	virtual ULONGLONG GetLength() const;
	virtual UINT Read(BYTE* lpBuf, UINT nCount, DWORD& dwError);
	virtual void Close();
};
