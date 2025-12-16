/*
 * (C) 2011-2025 see Authors.txt
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

#include <mutex>

//#define _DEBUG_LOGFILE // Allow output to the log file

#ifdef _DEBUG_LOGFILE
#include <ShlObj_core.h>
#include <KnownFolders.h>

static const CString GetLogFileName()
{
	CString ret;

	PWSTR pathDesktop = nullptr;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &pathDesktop);
	if (SUCCEEDED(hr)) {
		ret = CStringW(pathDesktop) + L"\\mpc-be.log";
	} else {
		ret = L"mpc-be.log";
	}
	CoTaskMemFree(pathDesktop);

	return ret;
}

static const CString GetLocalTime()
{
	SYSTEMTIME st;
	::GetLocalTime(&st);

	CString time;
	time.Format(L"%04u.%02u.%02u %02u:%02u:%02u.%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	return time;
}

namespace Logger
{
	static std::mutex log_mutex;
	static const CString logFileName = GetLogFileName();

	template<typename ... Args>
	inline void Log2File(LPCWSTR fmt, Args ... args)
	{
		std::unique_lock<std::mutex> lock(log_mutex);
		FILE* f = nullptr;
		if (_wfopen_s(&f, logFileName, L"a, ccs=UTF-8") == 0) {
			const auto size = _scwprintf(fmt, args ...) + 1;
			if (size > 0) {
				std::unique_ptr<wchar_t[]> buf(new(std::nothrow) wchar_t[size]);
				if (buf) {
					_snwprintf_s(buf.get(), size, _TRUNCATE, fmt, args ...);
					fwprintf_s(f, L"%s : %s\n", GetLocalTime(), buf.get());
				}
			}
			fclose(f);
		}
	}

	template<typename ... Args>
	inline void Log2File(LPCSTR fmt, Args ... args)
	{
		std::unique_lock<std::mutex> lock(log_mutex);
		FILE* f = nullptr;
		if (_wfopen_s(&f, logFileName, L"a, ccs=UTF-8") == 0) {
			const auto size = _scprintf(fmt, args ...) + 1;
			if (size > 0) {
				std::unique_ptr<char[]> buf(new(std::nothrow) char[size]);
				if (buf) {
					_snprintf_s(buf.get(), size, _TRUNCATE, fmt, args ...);
					fwprintf_s(f, L"%s : %hs\n", GetLocalTime(), buf.get());
				}
			}
			fclose(f);
		}
	}
}
#endif

#ifndef DEBUG_OR_LOG
	#if defined(_DEBUG_LOGFILE) || defined(_DEBUG)
		#define DEBUG_OR_LOG
	#endif
#endif

#ifdef _DEBUG_LOGFILE
	#define DLog(...) Logger::Log2File(__VA_ARGS__)
	#define DLogIf(f,...) {if (f) Logger::Log2File(__VA_ARGS__);}
	#define DLogError(...) Logger::Log2File(__VA_ARGS__)
#elif _DEBUG
	#define DLog(...) DbgLogInfo(LOG_TRACE, 3, __VA_ARGS__)
	#define DLogIf(f,...) {if (f) DbgLogInfo(LOG_TRACE, 3, __VA_ARGS__);}
	#define DLogError(...) DbgLogInfo(LOG_ERROR, 3, __VA_ARGS__)
#else
	#define DLog(...) __noop
	#define DLogIf(f,...) __noop
	#define DLogError(...) __noop
#endif

inline void HexDump(const CString& fileName, BYTE* buf, int size)
{
	if (size <= 0) {
		return;
	}

	CStringW dump_str;
	dump_str.Format(L"Dump size = %d\n", size);

	for (int i = 0; i < size; i += 16) {
		int len = size - i;
		if (len > 16) {
			len = 16;
		}
		dump_str.AppendFormat(L"%08x:", i);
		for (int j = 0; j < 16; j++) {
			if (j < len) {
				dump_str.AppendFormat(L" %02x", buf[i+j]);
			}
			else {
				dump_str.Append(L"   ");
			}
		}
		dump_str.Append(L" | ");
		for (int j = 0; j < len; j++) {
			int c = buf[i+j];
			if (c < ' ' || c > '~') {
				c = '.';
			}
			dump_str.AppendFormat(L"%c", c);
		}
		dump_str.AppendChar('\n');
	}

	if (!fileName.IsEmpty()) {
		CStdioFile file;
		if (file.Open(fileName, CFile::modeCreate|CFile::modeWrite)) {
			file.WriteString(dump_str);
			file.Close();
		}
	} else {
		DLog(dump_str);
	}
}
