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

#include "stdafx.h"
#include "MultiMonitor.h"

CMonitor::CMonitor() : m_hMonitor( nullptr )
{
}

CMonitor::CMonitor( const CMonitor& monitor  )
{
	m_hMonitor = (HMONITOR)monitor;
}

CMonitor::~CMonitor()
{
}

void CMonitor::Attach( const HMONITOR hMonitor )
{
	ASSERT( CMonitors::IsMonitor( hMonitor ) );

	m_hMonitor = hMonitor;
}

HMONITOR CMonitor::Detach()
{
	HMONITOR hMonitor = m_hMonitor;
	m_hMonitor = nullptr;
	return hMonitor;
}

HDC CMonitor::CreateDC() const
{
	ASSERT( IsMonitor() );

	CString name;
	GetName( name );

	HDC hdc = ::CreateDC( name, name, nullptr, nullptr );
	ASSERT( hdc != nullptr );

	CRect rect;
	GetMonitorRect( &rect );

	::SetViewportOrgEx( hdc, -rect.left, -rect.top, nullptr );
	::SetViewportExtEx( hdc, rect.Width(), rect.Height(), nullptr );

	return hdc;
}

int CMonitor::GetBitsPerPixel() const
{
	HDC hdc = CreateDC();
	int ret = ::GetDeviceCaps( hdc, BITSPIXEL ) * ::GetDeviceCaps( hdc, PLANES );
	VERIFY( ::DeleteDC( hdc ) );

	return ret;
}

void CMonitor::GetName( CString& string ) const
{
	ASSERT( IsMonitor() );

	MONITORINFOEX mi;
	mi.cbSize = sizeof( mi );
	::GetMonitorInfo( m_hMonitor, &mi );

	string = mi.szDevice;
}

void CMonitor::GetDeviceId( CString& string ) const
{
	ASSERT( IsMonitor() );

	MONITORINFOEX mi;
	mi.cbSize = sizeof( mi );
	::GetMonitorInfo( m_hMonitor, &mi );

	DISPLAY_DEVICEW dd = { sizeof(dd) };
	DWORD iDevNum = 0;
	while (EnumDisplayDevicesW(mi.szDevice, iDevNum, &dd, 0)) {
		if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE && !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
			const CString DeviceID(dd.DeviceID);
			string = DeviceID.Mid(8, DeviceID.Find(L"\\", 9) - 8);
			return;
		}
		iDevNum++;
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
	}
}

BOOL CMonitor::IsOnMonitor( const POINT pt ) const
{
	CRect rect;
	GetMonitorRect( rect );

	return rect.PtInRect( pt );
}

BOOL CMonitor::IsOnMonitor( const CWnd* pWnd ) const
{
	CRect rect;
	GetMonitorRect( rect );

	ASSERT( ::IsWindow( pWnd->GetSafeHwnd() ) );
	CRect wndRect;
	pWnd->GetWindowRect( &wndRect );

	return rect.IntersectRect( rect, wndRect );
}

BOOL CMonitor::IsOnMonitor( const LPRECT lprc ) const
{
	CRect rect;
	GetMonitorRect( rect );

	return rect.IntersectRect( rect, lprc );
}

void CMonitor::GetMonitorRect( LPRECT lprc ) const
{
	ASSERT( IsMonitor() );

	MONITORINFO mi;
	RECT        rc;

	mi.cbSize = sizeof( mi );
	::GetMonitorInfo( m_hMonitor, &mi );
	rc = mi.rcMonitor;

	::SetRect( lprc, rc.left, rc.top, rc.right, rc.bottom );
}

void CMonitor::GetWorkAreaRect( LPRECT lprc ) const
{
	ASSERT( IsMonitor() );

	MONITORINFO mi;
	RECT        rc;

	mi.cbSize = sizeof( mi );
	::GetMonitorInfo( m_hMonitor, &mi );
	rc = mi.rcWork;

	::SetRect( lprc, rc.left, rc.top, rc.right, rc.bottom );
}

void CMonitor::CenterRectToMonitor( LPRECT lprc, const BOOL UseWorkAreaRect ) const
{
	int  w = lprc->right - lprc->left;
	int  h = lprc->bottom - lprc->top;

	CRect rect;
	if ( UseWorkAreaRect ) {
		GetWorkAreaRect( &rect );
	} else {
		GetMonitorRect( &rect );
	}

	// Added rounding to get exactly the same rect as the CWnd::CenterWindow method returns.
	lprc->left = lround(rect.left + ( rect.Width() - w ) / 2.0);
	lprc->top = lround(rect.top + ( rect.Height() - h ) / 2.0);
	lprc->right	= lprc->left + w;
	lprc->bottom = lprc->top + h;
}

void CMonitor::CenterWindowToMonitor( CWnd* const pWnd, const BOOL UseWorkAreaRect ) const
{
	ASSERT( IsMonitor() );
	ASSERT( pWnd );
	ASSERT( ::IsWindow( pWnd->m_hWnd ) );

	CRect rect;
	pWnd->GetWindowRect( &rect );
	CenterRectToMonitor( &rect, UseWorkAreaRect );
	// Check if we are a child window and modify the coordinates accordingly
	if (pWnd->GetStyle() & WS_CHILD) {
		pWnd->GetParent()->ScreenToClient(&rect);
	}
	pWnd->SetWindowPos( nullptr, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
}

void CMonitor::ClipRectToMonitor( LPRECT lprc, const BOOL UseWorkAreaRect ) const
{
	int w = lprc->right - lprc->left;
	int h = lprc->bottom - lprc->top;

	CRect rect;
	if ( UseWorkAreaRect ) {
		GetWorkAreaRect( &rect );
	} else {
		GetMonitorRect( &rect );
	}

	lprc->left   = std::max(rect.left, std::min(rect.right - w, lprc->left));
	lprc->top    = std::max(rect.top,  std::min(rect.bottom - h, lprc->top));
	lprc->right  = lprc->left + w;
	lprc->bottom = lprc->top  + h;
}

BOOL CMonitor::IsPrimaryMonitor() const
{
	ASSERT( IsMonitor() );

	MONITORINFO mi;

	mi.cbSize = sizeof( mi );
	::GetMonitorInfo( m_hMonitor, &mi );

	return mi.dwFlags == MONITORINFOF_PRIMARY;
}

BOOL CMonitor::IsMonitor() const
{
	return CMonitors::IsMonitor( m_hMonitor );
}

CMonitors::CMonitors()
{
	// WARNING : GetSystemMetrics(SM_CMONITORS) return only visible display monitors, and  EnumDisplayMonitors
	// enumerate visible and pseudo invisible monitors !!!
	// m_MonitorArray.SetSize( GetMonitorCount() );

	ADDMONITOR addMonitor;
	addMonitor.pMonitors = &m_MonitorArray;
	addMonitor.currentIndex = 0;

	::EnumDisplayMonitors( nullptr, nullptr, AddMonitorsCallBack, (LPARAM)&addMonitor );
}

CMonitors::~CMonitors()
{
	for ( int i = 0; i < m_MonitorArray.GetSize(); i++ ) {
		delete m_MonitorArray.GetAt( i );
	}
}

BOOL CALLBACK CMonitors::AddMonitorsCallBack( HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData )
{
	LPADDMONITOR pAddMonitor = (LPADDMONITOR)dwData;

	CMonitor* pMonitor = DNew CMonitor;
	pMonitor->Attach( hMonitor );

	pAddMonitor->pMonitors->Add( pMonitor );
	pAddMonitor->currentIndex++;

	return TRUE;
}

CMonitor CMonitors::GetPrimaryMonitor()
{
	HMONITOR hMonitor = ::MonitorFromPoint( CPoint( 0,0 ), MONITOR_DEFAULTTOPRIMARY );
	ASSERT( IsMonitor( hMonitor ) );

	CMonitor monitor;
	monitor.Attach( hMonitor );
	ASSERT( monitor.IsPrimaryMonitor() );

	return monitor;
}

BOOL CMonitors::IsMonitor( const HMONITOR hMonitor )
{
	if ( hMonitor == nullptr ) {
		return FALSE;
	}

	MATCHMONITOR match;
	match.target = hMonitor;
	match.foundMatch = FALSE;

	::EnumDisplayMonitors( nullptr, nullptr, FindMatchingMonitorHandle, (LPARAM)&match );

	return match.foundMatch;
}

BOOL CALLBACK CMonitors::FindMatchingMonitorHandle( HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData )
{
	LPMATCHMONITOR pMatch = (LPMATCHMONITOR)dwData;

	if ( hMonitor == pMatch->target ) {

		pMatch->foundMatch = TRUE;
		return FALSE;
	}

	pMatch->foundMatch = FALSE;
	return TRUE;
}

BOOL CMonitors::AllMonitorsShareDisplayFormat()
{
	return ::GetSystemMetrics( SM_SAMEDISPLAYFORMAT );
}

int CMonitors::GetMonitorCount()
{
	return ::GetSystemMetrics(SM_CMONITORS);
}

CMonitor CMonitors::GetMonitor( const int index ) const
{
	ASSERT( index >= 0 && index < m_MonitorArray.GetCount() );

	CMonitor* pMonitor = static_cast<CMonitor*>(m_MonitorArray.GetAt( index ));

	return *pMonitor;
}

void CMonitors::GetVirtualDesktopRect( LPRECT lprc )
{
	::SetRect( lprc,
			   ::GetSystemMetrics( SM_XVIRTUALSCREEN ),
			   ::GetSystemMetrics( SM_YVIRTUALSCREEN ),
			   ::GetSystemMetrics( SM_CXVIRTUALSCREEN ),
			   ::GetSystemMetrics( SM_CYVIRTUALSCREEN ) );

}

BOOL CMonitors::IsOnScreen( const LPRECT lprc )
{
	return ::MonitorFromRect( lprc, MONITOR_DEFAULTTONULL ) != nullptr;
}

BOOL CMonitors::IsOnScreen( const POINT pt )
{
	return ::MonitorFromPoint( pt, MONITOR_DEFAULTTONULL ) != nullptr;
}

BOOL CMonitors::IsOnScreen( const CWnd* pWnd )
{
	return ::MonitorFromWindow( pWnd->GetSafeHwnd(), MONITOR_DEFAULTTONULL ) != nullptr;
}

CMonitor CMonitors::GetNearestMonitor( const LPRECT lprc )
{
	CMonitor monitor;
	monitor.Attach( ::MonitorFromRect( lprc, MONITOR_DEFAULTTONEAREST ) );

	return monitor;
}

CMonitor CMonitors::GetNearestMonitor( const POINT pt )
{
	CMonitor monitor;
	monitor.Attach( ::MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST ) );

	return monitor;
}

CMonitor CMonitors::GetNearestMonitor( const CWnd* pWnd )
{
	ASSERT( pWnd );
	ASSERT( ::IsWindow( pWnd->m_hWnd ) );

	CMonitor monitor;
	monitor.Attach( ::MonitorFromWindow( pWnd->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST ) );

	return monitor;
}
