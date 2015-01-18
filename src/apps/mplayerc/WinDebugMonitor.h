//////////////////////////////////////////////////////////////
//
//
//         File: WinDebugMonitor.h
//  Description: Interface of class CWinDebugMonitor
//      Created: 2007-12-6
//       Author: Ken Zhang
//       E-Mail: cpp.china@hotmail.com
//
//  Adaptation for MPC-BE (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
//
//////////////////////////////////////////////////////////////

#ifndef __WIN_DEBUG_BUFFER_H__
#define __WIN_DEBUG_BUFFER_H__

class CWinDebugMonitor
{
private:
	enum {
		TIMEOUT_WIN_DEBUG	=	100,
	};

	struct dbwin_buffer
	{
		DWORD   dwProcessId;
		char    data[4096-sizeof(DWORD)];
	};

private:
	HANDLE m_hDBWinMutex;
	HANDLE m_hDBMonBuffer;
	HANDLE m_hEventBufferReady;
	HANDLE m_hEventDataReady;

	HANDLE m_hWinDebugMonitorThread;
	BOOL m_bWinDebugMonStopped;
	struct dbwin_buffer *m_pDBBuffer;

	DWORD m_dwProcessId;

private:
	DWORD Initialize();
	void Unintialize();
	DWORD WinDebugMonitorProcess();
	static DWORD WINAPI WinDebugMonitorThread(void *pData);

public:
	CWinDebugMonitor(DWORD dwProcessId);
	~CWinDebugMonitor();

public:
	virtual void OutputWinDebugString(const char *str) {};

protected:
	BOOL bIsInitialize;
};

#endif
