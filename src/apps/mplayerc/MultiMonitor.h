/*
 *
 * Author: Donald Kackman
 * Email: don@itsEngineering.com
 * Copyright 2002, Donald Kackman
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

class CMonitor : public CObject
{
public:
	CMonitor();
	CMonitor( const CMonitor& monitor );
	virtual ~CMonitor();

	void Attach( const HMONITOR hMonitor );
	HMONITOR Detach();

	void ClipRectToMonitor( LPRECT lprc, const BOOL UseWorkAreaRect = FALSE ) const;
	void CenterRectToMonitor( LPRECT lprc, const BOOL UseWorkAreaRect = FALSE ) const;
	void CenterWindowToMonitor( CWnd* const pWnd, const BOOL UseWorkAreaRect = FALSE ) const;

	HDC CreateDC() const;

	void GetMonitorRect( LPRECT lprc ) const;
	void GetWorkAreaRect( LPRECT lprc ) const;

	void GetName( CString& string ) const;
	void GetDeviceId( CString& string ) const;

	int GetBitsPerPixel() const;

	BOOL IsOnMonitor( const POINT pt ) const;
	BOOL IsOnMonitor( const CWnd* pWnd ) const;
	BOOL IsOnMonitor( const LPRECT lprc ) const;

	BOOL IsPrimaryMonitor() const;
	BOOL IsMonitor() const;

	operator HMONITOR() const {
		return m_hMonitor;
	}

	BOOL operator ==( const CMonitor& monitor ) const {
		return m_hMonitor == (HMONITOR)monitor;
	}

	BOOL operator !=( const CMonitor& monitor ) const {
		return !( *this == monitor );
	}

	CMonitor& operator =( const CMonitor& monitor ) {
		m_hMonitor = (HMONITOR)monitor;
		return *this;
	}

private:
	HMONITOR m_hMonitor;
};

class CMonitors : public CObject
{
public:
	CMonitors();
	virtual ~CMonitors();

	CMonitor GetMonitor( const int index ) const;

	int GetCount() const {
		return (int)m_MonitorArray.GetCount();
	}

	static CMonitor GetNearestMonitor( const LPRECT lprc );
	static CMonitor GetNearestMonitor( const POINT pt );
	static CMonitor GetNearestMonitor( const CWnd* pWnd );

	static BOOL IsOnScreen( const POINT pt );
	static BOOL IsOnScreen( const CWnd* pWnd );
	static BOOL IsOnScreen( const LPRECT lprc );

	static void GetVirtualDesktopRect( LPRECT lprc );

	static BOOL IsMonitor( const HMONITOR hMonitor );

	static CMonitor GetPrimaryMonitor();
	static BOOL AllMonitorsShareDisplayFormat();

	static int GetMonitorCount();

private:
	CObArray m_MonitorArray;

	typedef struct tagMATCHMONITOR {
		HMONITOR target;
		BOOL foundMatch;
	} MATCHMONITOR, *LPMATCHMONITOR;

	static BOOL CALLBACK FindMatchingMonitorHandle(
		HMONITOR hMonitor,
		HDC hdcMonitor,
		LPRECT lprcMonitor,
		LPARAM dwData
	);

	typedef struct tagADDMONITOR {
		CObArray* pMonitors;
		int currentIndex;
	} ADDMONITOR, *LPADDMONITOR;

	static BOOL CALLBACK AddMonitorsCallBack(
		HMONITOR hMonitor,
		HDC hdcMonitor,
		LPRECT lprcMonitor,
		LPARAM dwData
	);
};
