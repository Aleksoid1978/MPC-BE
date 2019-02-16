/*
 * (C) 2012-2019 see Authors.txt
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
#include <atlutil.h>
#include "Log.h"
#include <list>

#define SOCKET_DUMPLOGFILE 0

class CMPCSocket : public CSocket
{
protected:
	int			m_nTimerID;

	BOOL		m_bProxyEnable;
	CString		m_sProxyServer;
	DWORD		m_nProxyPort;

	CUrl		m_url;
	CStringA	m_sUserAgent;

	CStringA	m_RequestHdr;
	CStringA	m_Hdr;

	std::list<CStringA> m_AddHeaderParams;

	UINT		m_uConnectTimeOut;
	UINT		m_uReceiveTimeOut;

	virtual BOOL OnMessagePending();

public:
	CMPCSocket();

	virtual ~CMPCSocket() {
		KillTimeOut();
	}

	BOOL Connect(CString url, BOOL bConnectOnly = FALSE);
	BOOL Connect(CUrl url, BOOL bConnectOnly = FALSE);

	void SetTimeOut(UINT uConnectTimeOut, UINT uReceiveTimeOut);
	BOOL SetTimeOut(UINT uTimeOut);
	BOOL KillTimeOut();

	BOOL SendRequest();
	CStringA GetHeader() { return m_Hdr; };

	void SetProxy(CString ProxyServer, DWORD ProxyPort);
	void SetUserAgent(CStringA UserAgent);

	void AddHeaderParams(CStringA sHeaderParam);
	void ClearHeaderParams();

	CMPCSocket& operator = (const CMPCSocket& soc) {
		m_bProxyEnable	= soc.m_bProxyEnable;
		m_sProxyServer	= soc.m_sProxyServer;
		m_nProxyPort	= soc.m_nProxyPort;
		m_url			= soc.m_url;
		m_sUserAgent	= soc.m_sUserAgent;
		m_RequestHdr	= soc.m_RequestHdr;

		return *this;
	}
};
