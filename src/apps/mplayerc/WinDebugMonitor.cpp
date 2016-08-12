//////////////////////////////////////////////////////////////
//
//
//         File: WinDebugMonitor.cpp
//  Description: Implementation of class CWinDebugMonitor
//      Created: 2007-12-6
//       Author: Ken Zhang
//       E-Mail: cpp.china@hotmail.com
//
//  Adaptation for MPC-BE (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
//
//////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WinDebugMonitor.h"

// set to 1 to enable dump debug log to file
#define DBGLOG_FILE 0

// ----------------------------------------------------------------------------
//  PROPERTIES OF OBJECTS
// ----------------------------------------------------------------------------
//	NAME		|	DBWinMutex		DBWIN_BUFFER_READY		DBWIN_DATA_READY
// ----------------------------------------------------------------------------
//	TYPE		|	Mutex			Event					Event
//	ACCESS		|	All				All						Sync
//	INIT STATE	|	?				Signaled				Nonsignaled
//	PROPERTY	|	?				Auto-Reset				Auto-Reset
// ----------------------------------------------------------------------------

CWinDebugMonitor::CWinDebugMonitor(DWORD dwProcessId)
{
	bIsInitialize = FALSE;
	m_dwProcessId = dwProcessId;

#if defined(_DEBUG) && DBGLOG_FILE
	bIsInitialize = TRUE;
	if (Initialize() != 0) {
		bIsInitialize = FALSE;
		DbgLog((LOG_TRACE, 3, "CWinDebugMonitor::Initialize() failed"));
	}
#endif
}

CWinDebugMonitor::~CWinDebugMonitor()
{
#if defined(_DEBUG) && DBGLOG_FILE
	Unintialize();
#endif
}

DWORD CWinDebugMonitor::Initialize()
{
	DWORD errorCode = 0;
	BOOL bSuccessful = FALSE;

	SetLastError(0);

	// Mutex: DBWin
	// ---------------------------------------------------------
	CComBSTR DBWinMutex = L"DBWinMutex";
	m_hDBWinMutex = ::OpenMutex(
		SYNCHRONIZE,
		FALSE,
		DBWinMutex
		);

	if (m_hDBWinMutex == NULL) {
		errorCode = GetLastError();
		return errorCode;
	}

	// Event: buffer ready
	// ---------------------------------------------------------
	CComBSTR DBWIN_BUFFER_READY = L"DBWIN_BUFFER_READY";
	m_hEventBufferReady = ::OpenEvent(
		EVENT_ALL_ACCESS,
		FALSE,
		DBWIN_BUFFER_READY
		);

	if (m_hEventBufferReady == NULL) {
		m_hEventBufferReady = ::CreateEvent(
			NULL,
			FALSE,	// auto-reset
			TRUE,	// initial state: signaled
			DBWIN_BUFFER_READY
			);

		if (m_hEventBufferReady == NULL) {
			errorCode = GetLastError();
			return errorCode;
		}
	}

	// Event: data ready
	// ---------------------------------------------------------
	CComBSTR DBWIN_DATA_READY = L"DBWIN_DATA_READY";
	m_hEventDataReady = ::OpenEvent(
		SYNCHRONIZE,
		FALSE,
		DBWIN_DATA_READY
		);

	if (m_hEventDataReady == NULL) {
		m_hEventDataReady = ::CreateEvent(
			NULL,
			FALSE,	// auto-reset
			FALSE,	// initial state: nonsignaled
			DBWIN_DATA_READY
			);

		if (m_hEventDataReady == NULL) {
			errorCode = GetLastError();
			return errorCode;
		}
	}

	// Shared memory
	// ---------------------------------------------------------
	CComBSTR DBWIN_BUFFER = L"DBWIN_BUFFER";
	m_hDBMonBuffer = ::OpenFileMapping(
		FILE_MAP_READ,
		FALSE,
		DBWIN_BUFFER
		);

	if (m_hDBMonBuffer == NULL) {
		m_hDBMonBuffer = ::CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			sizeof(struct dbwin_buffer),
			DBWIN_BUFFER
			);

		if (m_hDBMonBuffer == NULL) {
			errorCode = GetLastError();
			return errorCode;
		}
	}

	m_pDBBuffer = (struct dbwin_buffer *)::MapViewOfFile(
		m_hDBMonBuffer,
		SECTION_MAP_READ,
		0,
		0,
		0
		);

	if (m_pDBBuffer == NULL) {
		errorCode = GetLastError();
		return errorCode;
	}

	// Monitoring thread
	// ---------------------------------------------------------
	m_bWinDebugMonStopped = FALSE;

	m_hWinDebugMonitorThread = ::CreateThread(
		NULL,
		0,
		WinDebugMonitorThread,
		this,
		0,
		NULL
		);

	if (m_hWinDebugMonitorThread == NULL) {
		m_bWinDebugMonStopped = TRUE;
		errorCode = GetLastError();
		return errorCode;
	}

	// set monitor thread's priority to highest
	// ---------------------------------------------------------
	bSuccessful = ::SetPriorityClass(
		::GetCurrentProcess(),
		REALTIME_PRIORITY_CLASS
		);

	bSuccessful = ::SetThreadPriority(
		m_hWinDebugMonitorThread,
		THREAD_PRIORITY_TIME_CRITICAL
		);

	return errorCode;
}

void CWinDebugMonitor::Unintialize()
{
	if (m_hWinDebugMonitorThread != NULL) {
		m_bWinDebugMonStopped = TRUE;
		::WaitForSingleObject(m_hWinDebugMonitorThread, INFINITE);
	}

	if (m_hDBWinMutex != NULL) {
		CloseHandle(m_hDBWinMutex);
		m_hDBWinMutex = NULL;
	}

	if (m_hDBMonBuffer != NULL) {
		::UnmapViewOfFile(m_pDBBuffer);
		CloseHandle(m_hDBMonBuffer);
		m_hDBMonBuffer = NULL;
	}

	if (m_hEventBufferReady != NULL) {
		CloseHandle(m_hEventBufferReady);
		m_hEventBufferReady = NULL;
	}

	if (m_hEventDataReady != NULL) {
		CloseHandle(m_hEventDataReady);
		m_hEventDataReady = NULL;
	}

	m_pDBBuffer = NULL;
}

DWORD CWinDebugMonitor::WinDebugMonitorProcess()
{
	DWORD ret = 0;

	// wait for data ready
	ret = ::WaitForSingleObject(m_hEventDataReady, TIMEOUT_WIN_DEBUG);

	if (ret == WAIT_OBJECT_0) {
		if (m_pDBBuffer->dwProcessId == m_dwProcessId) {
			OutputWinDebugString(m_pDBBuffer->data);
		}

		// signal buffer ready
		SetEvent(m_hEventBufferReady);
	}

	return ret;
}

DWORD WINAPI CWinDebugMonitor::WinDebugMonitorThread(void *pData)
{
	CWinDebugMonitor *_this = (CWinDebugMonitor *)pData;

	if (_this != NULL) {
		while (!_this->m_bWinDebugMonStopped) {
			_this->WinDebugMonitorProcess();
		}
	}

	return 0;
}
