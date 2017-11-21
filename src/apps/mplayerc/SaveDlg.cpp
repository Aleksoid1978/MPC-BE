/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include <afxglobals.h>
#include "MainFrm.h"
#include "SaveDlg.h"
#include "../../DSUtil/FileHandle.h"
#include "../../filters/filters.h"

static unsigned int AdaptUnit(double& val, size_t unitsNb)
{
	unsigned int unit = 0;

	while (val > 1024 && unit < unitsNb) {
		val /= 1024;
		unit++;
	}

	return unit;
}

// CSaveDlg dialog

IMPLEMENT_DYNAMIC(CSaveDlg, CTaskDialog)
CSaveDlg::CSaveDlg(LPCWSTR in, LPCWSTR name, LPCWSTR out, HRESULT& hr)
	: CTaskDialog(L"", CString(name) + L"\n" + out, ResStr(IDS_SAVE_FILE), TDCBF_CANCEL_BUTTON, TDF_CALLBACK_TIMER|TDF_POSITION_RELATIVE_TO_WINDOW)
	, m_in(in)
	, m_out(out)
{
	m_hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (m_hIcon != nullptr) {
		SetMainIcon(m_hIcon);
	}

	SetProgressBarMarquee();
	SetProgressBarRange(0, 1000);
	SetProgressBarPosition(0);

	SetDialogWidth(250);

	hr = InitFileCopy();
}

CSaveDlg::~CSaveDlg()
{
	if (pMC) {
		pMC->Stop();
	}

	if (m_SaveThread.joinable()) {
		m_bAbort = true;
		m_SaveThread.join();
	}

	if (m_protocol == protocol::PROTOCOL_UDP) {
		if (m_WSAEvent != nullptr) {
			WSACloseEvent(m_WSAEvent);
		}

		if (m_UdpSocket != INVALID_SOCKET) {
			closesocket(m_UdpSocket);
			m_UdpSocket = INVALID_SOCKET;
		}

		WSACleanup();
	}


	if (m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
	}

	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

static HRESULT CopyFiles(CString sourceFile, CString destFile)
{
	#define FOF_UI_FLAGS FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR

	HRESULT hr = S_OK;

	CComPtr<IShellItem> psiItem;
	hr = afxGlobalData.ShellCreateItemFromParsingName(sourceFile, nullptr, IID_PPV_ARGS(&psiItem));
	EXIT_ON_ERROR(hr);

	CComPtr<IShellItem> psiDestinationFolder;
	CString pszPath = AddSlash(GetFolderOnly(destFile));
	hr = afxGlobalData.ShellCreateItemFromParsingName(pszPath, nullptr, IID_PPV_ARGS(&psiDestinationFolder));
	EXIT_ON_ERROR(hr);

	CComPtr<IFileOperation> pFileOperation;
	hr = CoCreateInstance(CLSID_FileOperation,
						  nullptr,
						  CLSCTX_INPROC_SERVER,
						  IID_PPV_ARGS(&pFileOperation));
	EXIT_ON_ERROR(hr);

	hr = pFileOperation->SetOperationFlags(FOF_UI_FLAGS | FOFX_NOMINIMIZEBOX);
	EXIT_ON_ERROR(hr);

	CString pszCopyName = GetFileOnly(destFile);
	hr = pFileOperation->CopyItem(psiItem, psiDestinationFolder, pszCopyName, nullptr);
	EXIT_ON_ERROR(hr);

	hr = pFileOperation->PerformOperations();

	return hr;
}

HRESULT CSaveDlg::InitFileCopy()
{
	if (FAILED(pGB.CoCreateInstance(CLSID_FilterGraph)) || !(pMC = pGB) || !(pMS = pGB)) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	HRESULT hr;

	const CString fn = m_in;
	CComPtr<IFileSourceFilter> pReader;

	if (::PathIsURL(fn)) {
		CUrl url;
		url.CrackUrl(fn);
		CString protocol = url.GetSchemeName();
		protocol.MakeLower();
		if (protocol == L"http" || protocol == L"https") {
			CComPtr<IUnknown> pUnk;
			pUnk.CoCreateInstance(CLSID_3DYDYoutubeSource);

			if (CComQIPtr<IBaseFilter> pSrc = pUnk) {
				pGB->AddFilter(pSrc, fn);

				if (!(pReader = pUnk) || FAILED(hr = pReader->Load(fn, nullptr))) {
					pReader.Release();
					pGB->RemoveFilter(pSrc);
				}
			}

			if (!pReader
					&& (m_HTTPAsync.Connect(fn, 10000) == S_OK)) {
				m_len = m_HTTPAsync.GetLenght();
				m_protocol = protocol::PROTOCOL_HTTP;
			}
		} else if (protocol == L"udp") {
			WSADATA wsaData = { 0 };
			WSAStartup(MAKEWORD(2, 2), &wsaData);

			int addr_size = sizeof(m_addr);

			memset(&m_addr, 0, addr_size);
			m_addr.sin_family      = AF_INET;
			m_addr.sin_port        = htons((u_short)url.GetPortNumber());
			m_addr.sin_addr.s_addr = htonl(INADDR_ANY);

			ip_mreq imr = { 0 };
			if (InetPton(AF_INET, url.GetHostName(), &imr.imr_multiaddr.s_addr) != 1) {
				goto fail;
			}
			imr.imr_interface.s_addr = INADDR_ANY;

			if ((m_UdpSocket = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET) {
				m_WSAEvent = WSACreateEvent();
				WSAEventSelect(m_UdpSocket, m_WSAEvent, FD_READ);

				DWORD dw = 1;
				if (setsockopt(m_UdpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&dw, sizeof(dw)) == SOCKET_ERROR) {
					closesocket(m_UdpSocket);
					m_UdpSocket = INVALID_SOCKET;
				}

				if (::bind(m_UdpSocket, (struct sockaddr*)&m_addr, addr_size) == SOCKET_ERROR) {
					closesocket(m_UdpSocket);
					m_UdpSocket = INVALID_SOCKET;
				}

				if (IN_MULTICAST(htonl(imr.imr_multiaddr.s_addr))
						&& (setsockopt(m_UdpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&imr, sizeof(imr)) == SOCKET_ERROR)) {
					closesocket(m_UdpSocket);
					m_UdpSocket = INVALID_SOCKET;
				}

				dw = 65 * KILOBYTE;
				if (setsockopt(m_UdpSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&dw, sizeof(dw)) == SOCKET_ERROR) {
					;
				}

				// set non-blocking mode
				u_long param = 1;
				ioctlsocket(m_UdpSocket, FIONBIO, &param);
			}

			if (m_UdpSocket != INVALID_SOCKET) {
				m_protocol = protocol::PROTOCOL_UDP;
			}
		}

		if (m_protocol != protocol::PROTOCOL_NONE) {
			m_hFile = CreateFileW(m_out, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (m_hFile != INVALID_HANDLE_VALUE) {
				if (m_len) {
					ULARGE_INTEGER usize = { 0 };
					usize.QuadPart = m_len;
					HANDLE hMapping = CreateFileMapping(m_hFile, nullptr, PAGE_READWRITE, usize.HighPart, usize.LowPart, nullptr);
					if (hMapping != INVALID_HANDLE_VALUE) {
						CloseHandle(hMapping);
					}
				}

				m_SaveThread = std::thread([this] { Save(); });

				return S_OK;
			}
		}
	} else {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDDAReader(nullptr, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fn, nullptr))) {
			pReader.Release();
		}

		if (!pReader) {
			hr = S_OK;
			CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDXAReader(nullptr, &hr);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fn, nullptr))) {
				pReader.Release();
			}
		}

		if (!pReader) {
			hr = S_OK;
			CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CVTSReader(nullptr, &hr);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fn, nullptr))) {
				pReader.Release();
			}
		}

		if (!pReader) {
			CopyFiles(m_in, m_out);
			return E_ABORT;
		}
	}

fail:

	CComQIPtr<IBaseFilter> pSrc = pReader;
	if (FAILED(pGB->AddFilter(pSrc, fn))) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(L"Sorry, can't save this file, press \"Cancel\"");

		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pMid = DNew CStreamDriveThruFilter(nullptr, &hr);
	if (FAILED(pGB->AddFilter(pMid, L"StreamDriveThru"))) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pDst;
	pDst.CoCreateInstance(CLSID_FileWriter);
	CComQIPtr<IFileSinkFilter2> pFSF = pDst;
	pFSF->SetFileName(CStringW(m_out), nullptr);
	pFSF->SetMode(AM_FILE_OVERWRITE);

	if (FAILED(pGB->AddFilter(pDst, L"File Writer"))) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	hr = pGB->Connect(
		GetFirstPin((pSrc), PINDIR_OUTPUT),
		GetFirstPin((pMid), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(L"Error Connect pSrc / pMid");

		return S_FALSE;
	}

	hr = pGB->Connect(
		GetFirstPin((pMid), PINDIR_OUTPUT),
		GetFirstPin((pDst), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(L"Error Connect pMid / pDst");

		return S_FALSE;
	}

	pMS = pMid;

	pMC->Run();

	return S_OK;
}

void CSaveDlg::Save()
{
	if (m_hFile != INVALID_HANDLE_VALUE) {
		m_startTime = clock();

		const DWORD bufLen = 64 * KILOBYTE;
		BYTE pBuffer[bufLen] = { 0 };

		int attempts = 0;

		while (!m_bAbort && attempts <= 20) {
			DWORD dwSizeRead = 0;
			if (m_protocol == protocol::PROTOCOL_HTTP) {
				const HRESULT hr = m_HTTPAsync.Read(pBuffer, bufLen, &dwSizeRead);
				if (hr != S_OK) {
					m_bAbort = true;
					break;
				}
			} else if (m_protocol == protocol::PROTOCOL_UDP) {
				const DWORD dwResult = WSAWaitForMultipleEvents(1, &m_WSAEvent, FALSE, 200, FALSE);
				if (dwResult == WSA_WAIT_EVENT_0) {
					WSAResetEvent(m_WSAEvent);
					static int fromlen = sizeof(m_addr);
					dwSizeRead = recvfrom(m_UdpSocket, (char*)pBuffer, bufLen, 0, (SOCKADDR*)&m_addr, &fromlen);
					if (dwSizeRead <= 0) {
						const int wsaLastError = WSAGetLastError();
						if (wsaLastError != WSAEWOULDBLOCK) {
							attempts++;
						}
						continue;
					}
				} else {
					attempts++;
					continue;
				}
			}

			DWORD dwSizeWritten = 0;
			if (FALSE == WriteFile(m_hFile, (LPCVOID)pBuffer, dwSizeRead, &dwSizeWritten, nullptr) || dwSizeRead != dwSizeWritten) {
				m_bAbort = true;
				break;
			}

			m_pos += dwSizeRead;
			if (m_len && m_len == m_pos) {
				m_bAbort = true;
				break;
			}

			attempts = 0;
		}
	}

	ClickCommandControl(IDCANCEL);
}

HRESULT CSaveDlg::OnTimer(_In_ long lTime)
{
	static UINT sizeUnits[]  = { IDS_SIZE_UNIT_K,  IDS_SIZE_UNIT_M,  IDS_SIZE_UNIT_G  };
	static UINT speedUnits[] = { IDS_SPEED_UNIT_K, IDS_SPEED_UNIT_M, IDS_SPEED_UNIT_G };

	if (m_hFile) {
		const QWORD pos = m_pos;                             // bytes
		const clock_t time = (clock() - m_startTime);        // milliseconds
		const long speed = time > 0 ? pos * 1000 / time : 0; // bytes per second

		double dPos = pos / 1024.0;
		const unsigned int unitPos = AdaptUnit(dPos, _countof(sizeUnits));
		double dSpeed = speed / 1024.0;
		const unsigned int unitSpeed = AdaptUnit(dSpeed, _countof(speedUnits));

		CString str;
		if (m_len) {
			double dDur = m_len / 1024.0;
			const unsigned int unitDur = AdaptUnit(dDur, _countof(sizeUnits));

			str.Format(L"%.2lf %s / %.2lf %s , %.2lf %s",
					   dPos, ResStr(sizeUnits[unitPos]),
					   dDur, ResStr(sizeUnits[unitDur]),
					   dSpeed, ResStr(speedUnits[unitSpeed]));

			if (speed > 0) {
				const REFERENCE_TIME sec = (m_len - pos) / speed;
				if (sec > 0) {
					DVD_HMSF_TIMECODE tcDur = {
						(BYTE)(sec / 3600),
						(BYTE)(sec / 60 % 60),
						(BYTE)(sec % 60),
						0
					};

					str.Append(L",");

					if (tcDur.bHours > 0) {
						str.AppendFormat(L" %0.2dh", tcDur.bHours);
					}
					if (tcDur.bMinutes > 0) {
						str.AppendFormat(L" %0.2dm", tcDur.bMinutes);
					}
					if (tcDur.bSeconds > 0) {
						str.AppendFormat(L" %0.2ds", tcDur.bSeconds);
					}
				}
			}
		} else {
			str.Format(L"%.2lf %s , %.2lf %s",
					   dPos, ResStr(sizeUnits[unitPos]),
					   dSpeed, ResStr(speedUnits[unitSpeed]));
		}

		SetContent(str);

		if (m_len) {
			SetProgressBarPosition(static_cast<int>(1000 * pos / m_len));
		}

		if (m_bAbort) {
			ClickCommandControl(IDCANCEL);
			return S_FALSE;
		}
	} else if (pGB && pMS) {
		CString str;
		REFERENCE_TIME pos = 0, dur = 0;
		pMS->GetCurrentPosition(&pos);
		pMS->GetDuration(&dur);
		REFERENCE_TIME time = 0;
		CComQIPtr<IMediaSeeking>(pGB)->GetCurrentPosition(&time);
		const REFERENCE_TIME speed = time > 0 ? pos * 10000000 / time : 0;

		double dPos = pos / 1024.0;
		const unsigned int unitPos = AdaptUnit(dPos, _countof(sizeUnits));
		double dDur = dur / 1024.0;
		const unsigned int unitDur = AdaptUnit(dDur, _countof(sizeUnits));
		double dSpeed = speed / 1024.0;
		const unsigned int unitSpeed = AdaptUnit(dSpeed, _countof(speedUnits));

		str.Format(L"%.2lf %s / %.2lf %s , %.2lf %s",
				   dPos, ResStr(sizeUnits[unitPos]),
				   dDur, ResStr(sizeUnits[unitDur]),
				   dSpeed, ResStr(speedUnits[unitSpeed]));

		if (speed > 0 && m_len) {
			const REFERENCE_TIME sec = (dur - pos) / speed;
			if (sec > 0) {
				DVD_HMSF_TIMECODE tcDur = {
					(BYTE)(sec / 3600),
					(BYTE)(sec / 60 % 60),
					(BYTE)(sec % 60),
					0
				};

				str.Append(L",");

				if (tcDur.bHours > 0) {
					str.AppendFormat(L" %0.2dh", tcDur.bHours);
				}
				if (tcDur.bMinutes > 0) {
					str.AppendFormat(L" %0.2dm", tcDur.bMinutes);
				}
				if (tcDur.bSeconds > 0) {
					str.AppendFormat(L" %0.2ds", tcDur.bSeconds);
				}
			}
		}

		SetContent(str);

		SetProgressBarPosition(dur > 0 ? (int)(1000 * pos / dur) : 0);

		if (dur && pos >= dur) {
			ClickCommandControl(IDCANCEL);
			return S_FALSE;
		}
	}

	return S_OK;
}

HRESULT CSaveDlg::OnCommandControlClick(_In_ int nCommandControlID)
{
	if (nCommandControlID == IDCANCEL) {
		if (pGB) {
			pGB->Abort();
		}
		if (pMC) {
			pMC->Stop();
		}
	}

	return S_OK;
}