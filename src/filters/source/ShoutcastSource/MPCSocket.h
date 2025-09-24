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

#pragma once

#include <afxsock.h>
#include "DSUtil/UrlParser.h"

class CMPCSocket : public CSocket
{
protected:
	int			m_nTimerID = 0;

	BOOL		m_bProxyEnable = FALSE;
	CStringW	m_sProxyServer;
	DWORD		m_nProxyPort = 0;

	CStringA	m_sUserAgent = "MPC-BE";

	CStringA	m_RequestHdr;
	CStringA	m_Hdr;

	std::list<CStringA> m_AddHeaderParams;

	UINT		m_uConnectTimeOut = 0;
	UINT		m_uReceiveTimeOut = 0;

	virtual BOOL OnMessagePending();

public:
	CMPCSocket();

	virtual ~CMPCSocket() {
		KillTimeOut();
	}

	BOOL Connect(const CStringW& url, const BOOL bConnectOnly = FALSE);
	BOOL Connect(const CUrlParser& urlParser, const BOOL bConnectOnly = FALSE);

	void SetTimeOut(const UINT uConnectTimeOut, const UINT uReceiveTimeOut);
	BOOL SetTimeOut(const UINT uTimeOut);
	BOOL KillTimeOut();

	BOOL SendRequest();
	CStringA GetHeader() { return m_Hdr; };

	void SetProxy(const CStringW& ProxyServer, const DWORD ProxyPort);
	void SetUserAgent(const CStringA& UserAgent);

	void AddHeaderParams(const CStringA& sHeaderParam);
	void ClearHeaderParams();

	CMPCSocket& operator = (const CMPCSocket& soc) {
		m_bProxyEnable	= soc.m_bProxyEnable;
		m_sProxyServer	= soc.m_sProxyServer;
		m_nProxyPort	= soc.m_nProxyPort;
		m_sUserAgent	= soc.m_sUserAgent;
		m_RequestHdr	= soc.m_RequestHdr;

		return *this;
	}
};
