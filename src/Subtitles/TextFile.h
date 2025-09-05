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

#pragma once

#include <afx.h>
#include <stdio.h>
#include <memory>

class CTextFile
{
public:
	enum enc { // supported code pages
		ANSI    = 0,     // CP_ACP
		UTF16LE = 1200,
		UTF16BE = 1201,
		ASCII   = 20127,
		UTF8    = 65001, // CP_UTF8
	};

private:
	enc m_encoding, m_defaultencoding;
	int m_offset = 0;
	ULONGLONG m_posInFile = 0;
	std::unique_ptr<char[]> m_buffer;
	std::unique_ptr<WCHAR[]> m_wbuffer;
	LONGLONG m_posInBuffer = 0;
	LONGLONG m_nInBuffer = 0;

	std::unique_ptr<FILE, std::integral_constant<decltype(&fclose), &fclose>> m_pFile;
	std::unique_ptr<CStdioFile> m_pStdioFile;
	CStringW m_strFileName;

	bool OpenFile(LPCWSTR lpszFileName, LPCWSTR mode);

public:
	CTextFile(enc encoding = ASCII, enc defaultencoding = ASCII);
	virtual ~CTextFile();

	bool Open(LPCWSTR lpszFileName);
	bool Save(LPCWSTR lpszFileName, enc e /*= ASCII*/);
	void Close();

	enc GetEncoding() const;
	bool IsUnicode() const;

	CStringW GetFilePath() const;

	// CStdioFile

	ULONGLONG GetPosition() const;
	ULONGLONG GetLength() const;
	ULONGLONG Seek(LONGLONG lOff, UINT nFrom);

	void WriteString(LPCSTR lpsz/*CStringA str*/);
	void WriteString(LPCWSTR lpsz/*CStringW str*/);
	bool ReadString(CStringW& str);

protected:
	bool ReopenAsText();
	bool FillBuffer();
	ULONGLONG GetPositionFastBuffered() const;
};

class CWebTextFile final : public CTextFile
{
	LONGLONG m_llMaxSize;
	CStringW m_tempfn;

	CString m_url_redirect_str;

public:
	CWebTextFile(enc encoding = ASCII, enc defaultencoding = ASCII, LONGLONG llMaxSize = 1024 * 1024);
	~CWebTextFile();

	bool Open(LPCWSTR lpszFileName);
	void Close();

	const CString& GetRedirectURL() const;
};

extern CStringW AToT(CStringA str);
extern CStringA TToA(CStringW str);
