/*
* (C) 2012-2017 see Authors.txt
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
#include "MPCSocket.h"
#include "text.h"

#define PROXY_TIMEOUT_FACTOR 0

CMPCSocket::CMPCSocket()
	: m_nTimerID(0)
	, m_uConnectTimeOut(0)
	, m_uReceiveTimeOut(0)
	, m_bProxyEnable(FALSE)
	, m_nProxyPort(0)
	, m_sUserAgent("MPC-BE")
{
	CRegKey key;
	ULONG len			= MAX_PATH;
	DWORD ProxyEnable	= 0;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", KEY_READ)) {
		if (ERROR_SUCCESS == key.QueryDWORDValue(L"ProxyEnable", ProxyEnable) && ProxyEnable
				&& ERROR_SUCCESS == key.QueryStringValue(L"ProxyServer", m_sProxyServer.GetBufferSetLength(MAX_PATH), &len)) {
			m_sProxyServer.ReleaseBufferSetLength(len);

			CAtlList<CString> sl;
			m_sProxyServer = Explode(m_sProxyServer, sl, L';');
			POSITION pos = sl.GetHeadPosition();
			while (pos) {
				CAtlList<CString> sl2;
				if (!Explode(sl.GetNext(pos), sl2, L'=', 2).CompareNoCase(L"http")
						&& sl2.GetCount() == 2) {
					m_sProxyServer = sl2.GetTail();
					break;
				}
			}

			m_sProxyServer = Explode(m_sProxyServer, sl, L':');
			if (sl.GetCount() > 1) {
				m_nProxyPort = wcstol(sl.GetTail(), nullptr, 10);
			}

			m_bProxyEnable = (ProxyEnable && !m_sProxyServer.IsEmpty() && m_nProxyPort);
		}

		key.Close();
	}
}

BOOL CMPCSocket::Connect(CString url, BOOL bConnectOnly)
{
	CUrl Url;

	return (Url.CrackUrl(url) && Connect(Url, bConnectOnly));
}

BOOL CMPCSocket::Connect(CUrl url, BOOL bConnectOnly)
{
	m_Hdr.Empty();

	if (url.GetScheme() != ATL_URL_SCHEME_HTTP) {
		return FALSE;
	}
	if (url.GetUrlPathLength() == 0) {
		url.SetUrlPath(L"/");
	}
	if (url.GetPortNumber() == ATL_URL_INVALID_PORT_NUMBER) {
		url.SetPortNumber(ATL_URL_DEFAULT_HTTP_PORT);
	}

#if PROXY_TIMEOUT_FACTOR
	if (m_bProxyEnable) {
		m_uConnectTimeOut *= PROXY_TIMEOUT_FACTOR;
		m_uReceiveTimeOut *= PROXY_TIMEOUT_FACTOR;
	}
#endif

	if (m_uConnectTimeOut) {
		SetTimeOut(m_uConnectTimeOut);
	}

	if (!__super::Connect(
			m_bProxyEnable ? m_sProxyServer : url.GetHostName(),
			m_bProxyEnable ? m_nProxyPort : url.GetPortNumber())) {
		KillTimeOut();
		return FALSE;
	}

	if (m_uConnectTimeOut) {
		KillTimeOut();
	}

	m_url = url;

	CStringA host = CStringA(url.GetHostName());
	CStringA path = CStringA(url.GetUrlPath()) + CStringA(url.GetExtraInfo());

	if (m_bProxyEnable) {
		DWORD dwUrlLen	= url.GetUrlLength() + 1;
		WCHAR* szUrl	= new WCHAR[dwUrlLen];

		// Retrieve the contents of the CUrl object
		url.CreateUrl(szUrl, &dwUrlLen);
		path = CStringA(szUrl);

		delete [] szUrl;
	}

	CStringA sAddHeader;
	POSITION pos = m_AddHeaderParams.GetHeadPosition();
	while (pos) {
		sAddHeader += (m_AddHeaderParams.GetNext(pos) + "\r\n");
	}

	m_RequestHdr.Format(
		"GET %s HTTP/1.0\r\n"
		"Accept: */*\r\n"
		"User-Agent: %s\r\n"
		"%s"
		"%s"
		"Host: %s:%d\r\n"
		"\r\n",
		path,
		m_sUserAgent,
		m_bProxyEnable ? "Proxy-Connection: Keep-Alive\r\n" : "",
		sAddHeader,
		host, url.GetPortNumber());

	if (!bConnectOnly || m_bProxyEnable) {
		if (!SendRequest()) {
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CMPCSocket::OnMessagePending()
{
	MSG msg;

	if (::PeekMessageW(&msg, nullptr, WM_TIMER, WM_TIMER, PM_REMOVE)) {
		if (msg.wParam == (UINT) m_nTimerID) {
			DLog(L"CMPCSocket::OnMessagePending(WM_TIMER) PASSED!");
			// Remove the message and call CancelBlockingCall.
			::PeekMessageW(&msg, nullptr, WM_TIMER, WM_TIMER, PM_REMOVE);
			CancelBlockingCall();
			KillTimeOut();
			return FALSE;  // No need for idle time processing.
		};
	};

	return __super::OnMessagePending();
}

void CMPCSocket::SetTimeOut(UINT uConnectTimeOut, UINT uReceiveTimeOut)
{
	m_uConnectTimeOut = uConnectTimeOut;
	m_uReceiveTimeOut = uReceiveTimeOut;
}

BOOL CMPCSocket::SetTimeOut(UINT uTimeOut)
{
	m_nTimerID = SetTimer(nullptr, 0, uTimeOut, nullptr);
	return (m_nTimerID != 0);
}

BOOL CMPCSocket::KillTimeOut()
{
	return KillTimer(nullptr, m_nTimerID);
}

BOOL CMPCSocket::SendRequest()
{
	if (m_uReceiveTimeOut) {
		SetTimeOut(m_uReceiveTimeOut);
	}

	BOOL ret = (Send((LPCSTR)m_RequestHdr, m_RequestHdr.GetLength()) == m_RequestHdr.GetLength());

	m_Hdr.Empty();
	if (ret) {
		while (m_Hdr.Find("\r\n\r\n") == -1 && m_Hdr.GetLength() < 4096) {
			CStringA str;
			str.ReleaseBuffer(Receive(str.GetBuffer(1), 1)); // SOCKET_ERROR == -1, also suitable for ReleaseBuffer
			if (str.IsEmpty()) {
				break;
			}
			m_Hdr += str;
		}

		m_Hdr.Replace("\r\n\r\n", "");
	}

#if (SOCKET_DUMPLOGFILE)
	{
		DWORD dwUrlLen	= m_url.GetUrlLength() + 1;
		WCHAR* szUrl	= new WCHAR[dwUrlLen];
		// Retrieve the contents of the CUrl object
		m_url.CreateUrl(szUrl, &dwUrlLen);
		CString path = szUrl;
		delete [] szUrl;

		Log2File(L"===");
		Log2File(L"Request URL = %s", path);
		Log2File(L"Header:");
		CAtlList<CStringA> sl;
		CStringA hdr = GetHeader();
		Explode(hdr, sl, '\n');
		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			CStringA& hdrline = sl.GetNext(pos);
			Log2File(L"%s", CString(hdrline));
		}
	}
#endif

	if (m_uReceiveTimeOut) {
		KillTimeOut();
	}

	return ret;
}

void CMPCSocket::SetProxy(CString ProxyServer, DWORD ProxyPort)
{
	if (!ProxyServer.IsEmpty() && ProxyPort) {
		m_bProxyEnable	= TRUE;
		m_sProxyServer	= ProxyServer;
		m_nProxyPort	= ProxyPort;
	}
}

void CMPCSocket::SetUserAgent(CStringA UserAgent)
{
	m_sUserAgent = UserAgent;
}

void CMPCSocket::AddHeaderParams(CStringA sHeaderParam)
{
	m_AddHeaderParams.AddTail(sHeaderParam);
}

void CMPCSocket::ClearHeaderParams()
{
	m_AddHeaderParams.RemoveAll();
}