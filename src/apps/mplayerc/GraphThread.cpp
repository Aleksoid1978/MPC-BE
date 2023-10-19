/*
 * (C) 2023 see Authors.txt
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
#include "MainFrm.h"
#include "TunerScanDlg.h"
#include "GraphThread.h"
 
//
// CGraphThread
//

IMPLEMENT_DYNCREATE(CGraphThread, CWinThread)

BOOL CGraphThread::InitInstance()
{
	SetThreadName((DWORD)-1, "GraphThread");
	AfxSocketInit();
	return SUCCEEDED(CoInitialize(0)) ? TRUE : FALSE;
}

int CGraphThread::ExitInstance()
{
	CoUninitialize();
	return __super::ExitInstance();
}

BEGIN_MESSAGE_MAP(CGraphThread, CWinThread)
	ON_THREAD_MESSAGE(TM_EXIT, OnExit)
	ON_THREAD_MESSAGE(TM_OPEN, OnOpen)
	ON_THREAD_MESSAGE(TM_CLOSE, OnClose)
	ON_THREAD_MESSAGE(TM_RESIZE, OnResize)
	ON_THREAD_MESSAGE(TM_RESET, OnReset)
	ON_THREAD_MESSAGE(TM_TUNER_SCAN, OnTunerScan)
	ON_THREAD_MESSAGE(TM_DISPLAY_CHANGE, OnDisplayChange)
END_MESSAGE_MAP()

void CGraphThread::OnExit(WPARAM wParam, LPARAM lParam)
{
	PostQuitMessage(0);
	if (CAMEvent* e = (CAMEvent*)lParam) {
		e->Set();
	}
}

void CGraphThread::OnOpen(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		ASSERT(WaitForSingleObject(m_pMainFrame->m_hGraphThreadEventOpen, 0) == WAIT_TIMEOUT);
		if (m_pMainFrame->m_eMediaLoadState == MLS_LOADING) {
			std::unique_ptr<OpenMediaData> pOMD((OpenMediaData*)lParam);
			m_pMainFrame->OpenMediaPrivate(pOMD);
		}
		m_pMainFrame->m_hGraphThreadEventOpen.SetEvent();
	}
}

void CGraphThread::OnClose(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		ASSERT(WaitForSingleObject(m_pMainFrame->m_hGraphThreadEventClose, 0) == WAIT_TIMEOUT);
		if (m_pMainFrame->m_eMediaLoadState == MLS_CLOSING) {
			m_pMainFrame->CloseMediaPrivate();
		}
		m_pMainFrame->m_hGraphThreadEventClose.SetEvent();
	}
}

void CGraphThread::OnResize(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		BOOL* b = (BOOL*)wParam;
		BOOL bResult = m_pMainFrame->ResizeDevice();
		if (b) {
			*b = bResult;
		}
	}
	if (CAMEvent* e = (CAMEvent*)lParam) {
		e->Set();
	}
}

void CGraphThread::OnReset(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		BOOL* b = (BOOL*)wParam;
		BOOL bResult = m_pMainFrame->ResetDevice();
		if (b) {
			*b = bResult;
		}
	}
	if (CAMEvent* e = (CAMEvent*)lParam) {
		e->Set();
	}
}

void CGraphThread::OnTunerScan(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		std::unique_ptr<TunerScanData> pTSD((TunerScanData*)lParam);
		m_pMainFrame->DoTunerScan(pTSD.get());
	}
}

void CGraphThread::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		m_pMainFrame->DisplayChange();
	}
	if (CAMEvent* e = (CAMEvent*)lParam) {
		e->Set();
	}
}
