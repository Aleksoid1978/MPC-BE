/*
 * (C) 2015-2018 see Authors.txt
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
#include "FocusThread.h"

IMPLEMENT_DYNCREATE(CFocusThread, CWinThread)

LRESULT CALLBACK FocusWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_NCACTIVATE) {
		if (wp) {
			AfxGetMainWnd()->SetForegroundWindow();
		}
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wp, lp);
}

CFocusThread::CFocusThread()
	: m_hWnd(nullptr)
	, m_hEvtInit(nullptr)
{
	WNDCLASS wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wndclass.lpfnWndProc = FocusWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = nullptr;
	wndclass.hIcon = nullptr;
	wndclass.hCursor = nullptr;
	wndclass.hbrBackground = nullptr;
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = L"D3DFocusClass";

	if (!RegisterClassW(&wndclass)) {
		DLog(L"CFocusThread::CFocusThread() : Registering focus window failed");
	}

	m_hEvtInit = CreateEventW(nullptr, TRUE, FALSE, nullptr);
}

CFocusThread::~CFocusThread()
{
	SAFE_CLOSE_HANDLE(m_hEvtInit);
	UnregisterClassW(L"D3DFocusClass", nullptr);
}

BOOL CFocusThread::InitInstance()
{
	SetThreadName(DWORD_MAX, "FocusThread");
	m_hWnd = CreateWindowExW(0L, L"D3DFocusClass", L"D3D Focus Window", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
	SetEvent(m_hEvtInit);
	if (!m_hWnd) {
		DLog(L"CFocusThread::InitInstance() : Creating focus window failed");
		return FALSE;
	}
	return TRUE;
}

int CFocusThread::ExitInstance()
{
	if (m_hWnd) {
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
	return __super::ExitInstance();
}

HWND CFocusThread::GetFocusWindow()
{
	if (!m_hWnd) {
		WaitForSingleObject(m_hEvtInit, 10000);
	}
	return m_hWnd;
}
