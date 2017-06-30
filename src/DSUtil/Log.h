/*
 * (C) 2011-2017 see Authors.txt
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

void HexDump(CString fName, BYTE* buf, int size);
void Log2File(LPCTSTR fmt, ...);
void Log2File(LPCSTR fmt, ...);

//#define _DEBUG_LOGFILE // Allow output to the log file

#ifndef DEBUG_OR_LOG
	#if defined(_DEBUG_LOGFILE) || defined(_DEBUG)
		#define DEBUG_OR_LOG
	#endif
#endif

#ifdef _DEBUG_LOGFILE
	#define DLog(...) Log2File(__VA_ARGS__)
	#define DLogError(...) Log2File(__VA_ARGS__)
#elif _DEBUG
	#define DLog(...) DbgLogInfo(LOG_TRACE, 3, __VA_ARGS__)
	#define DLogError(...) DbgLogInfo(LOG_ERROR, 3, __VA_ARGS__)
#else
	#define DLog(...) __noop
	#define DLogError(...) __noop
#endif

