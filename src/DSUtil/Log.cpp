/*
 * (C) 2011-2014 see Authors.txt
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
#include <Shlobj.h>
#include "Log.h"

void HexDump(CString fileName, BYTE* buf, int size)
{
	if (size <= 0) {
		return;
	}

	CString dump_str;
	dump_str.Format(_T("Dump size = %d\n"), size);
	int len, i, j, c;

	for (i = 0; i < size; i +=16) {
		len = size - i;
		if (len > 16) {
			len = 16;
		}
		dump_str.AppendFormat(_T("%08x "), i);
		for (j = 0; j < 16; j++) {
			if (j < len) {
				dump_str.AppendFormat(_T(" %02x"), buf[i+j]);
			}
			else {
				dump_str.Append(_T("   "));
			}
		}
		dump_str.Append(_T(" "));
		for (j = 0; j < len; j++) {
			c = buf[i+j];
			if (c < ' ' || c > '~') {
				c = '.';
			}
			dump_str.AppendFormat(_T("%c"), c);
		}
		dump_str.Append(_T("\n"));
	}

	dump_str.Append(_T("\n"));

	if (!fileName.IsEmpty()) {
		CStdioFile file;
		if (file.Open(fileName, CFile::modeCreate|CFile::modeWrite)) {
			file.WriteString(dump_str);
			file.Close();
		}
	} else {
		TRACE(dump_str);
	}
}

void LOG2FILE(LPCTSTR fmt, ...)
{
	static CString fname;
	if (fname.IsEmpty()) {
		TCHAR szPath[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szPath))) {
			fname = CString(szPath) + L"\\mpc-be.log";
		}
	}

	va_list args;
	va_start(args, fmt);
	size_t len = _vsctprintf(fmt, args) + 1;
	if (TCHAR* buff = DNew TCHAR[len]) {
		_vstprintf(buff, len, fmt, args);
		if (FILE* f = _tfopen(fname, L"at, ccs=UTF-8")) {
			fseek(f, 0, 2);

			SYSTEMTIME st;
			::GetLocalTime(&st);

			CString lt;
			lt.Format(L"%04d.%02d.%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

			_ftprintf_s(f, _T("%s : %s\n"), lt, buff);
			fclose(f);
		}
		delete [] buff;
	}
	va_end(args);
}
