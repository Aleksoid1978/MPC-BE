/*
 * (C) 2012-2025 see Authors.txt
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
#include "DSUtil/Log.h"
#include "DSUtil/text.h"

#define PROXY_TIMEOUT_FACTOR 0

CMPCSocket::CMPCSocket()
{
	CRegKey key;
	ULONG len			= MAX_PATH;
	DWORD ProxyEnable	= 0;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", KEY_READ)) {
		if (ERROR_SUCCESS == key.QueryDWORDValue(L"ProxyEnable", ProxyEnable) && ProxyEnable
				&& ERROR_SUCCESS == key.QueryStringValue(L"ProxyServer", m_sProxyServer.GetBufferSetLength(MAX_PATH), &len)) {
			m_sProxyServer.ReleaseBufferSetLength(len);

			std::list<CStringW> sl;
			m_sProxyServer = Explode(m_sProxyServer, sl, L';');
			for (const auto& item : sl) {
				std::list<CStringW> sl2;
				if (!Explode(item, sl2, L'=', 2).CompareNoCase(L"http") && sl2.size() == 2) {
					m_sProxyServer = sl2.back();
					break;
				}
			}

			m_sProxyServer = Explode(m_sProxyServer, sl, L':');
			if (sl.size() > 1) {
				m_nProxyPort = wcstol(sl.back(), nullptr, 10);
			}

			m_bProxyEnable = (ProxyEnable && !m_sProxyServer.IsEmpty() && m_nProxyPort);
		}

		key.Close();
	}
}

BOOL CMPCSocket::Connect(const CStringW& url, const BOOL bConnectOnly/* = FALSE*/)
{
	CUrlParser urlParser;

	return (urlParser.Parse(url.GetString()) && Connect(urlParser, bConnectOnly));
}

BOOL CMPCSocket::Connect(const CUrlParser& urlParser, const BOOL bConnectOnly/* = FALSE*/)
{
	m_Hdr.Empty();

	if (urlParser.GetScheme() != INTERNET_SCHEME_HTTP) {
		return FALSE;
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
			m_bProxyEnable ? m_sProxyServer.GetString() : urlParser.GetHostName(),
			m_bProxyEnable ? m_nProxyPort : urlParser.GetPortNumber())) {
		KillTimeOut();
		return FALSE;
	}

	if (m_uConnectTimeOut) {
		KillTimeOut();
	}

	CStringA host = CStringA(urlParser.GetHostName());
	CStringA path = CStringA(urlParser.GetUrlPath()) + CStringA(urlParser.GetExtraInfo());

	if (m_bProxyEnable) {
		path = urlParser.GetUrl();
	}

	CStringA sAddHeader;
	for (const auto& item : m_AddHeaderParams) {
		sAddHeader += (item + "\r\n");
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
		host, urlParser.GetPortNumber());

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

void CMPCSocket::SetTimeOut(const UINT uConnectTimeOut, const UINT uReceiveTimeOut)
{
	m_uConnectTimeOut = uConnectTimeOut;
	m_uReceiveTimeOut = uReceiveTimeOut;
}

BOOL CMPCSocket::SetTimeOut(const UINT uTimeOut)
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

	if (m_uReceiveTimeOut) {
		KillTimeOut();
	}

	return ret;
}

void CMPCSocket::SetProxy(const CStringW& ProxyServer, const DWORD ProxyPort)
{
	if (!ProxyServer.IsEmpty() && ProxyPort) {
		m_bProxyEnable	= TRUE;
		m_sProxyServer	= ProxyServer;
		m_nProxyPort	= ProxyPort;
	}
}

void CMPCSocket::SetUserAgent(const CStringA& UserAgent)
{
	m_sUserAgent = UserAgent;
}

void CMPCSocket::AddHeaderParams(const CStringA& sHeaderParam)
{
	m_AddHeaderParams.push_back(sHeaderParam);
}

void CMPCSocket::ClearHeaderParams()
{
	m_AddHeaderParams.clear();
}
