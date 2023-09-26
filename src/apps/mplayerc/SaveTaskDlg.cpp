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
#include <afxglobals.h>
#include <clsids.h>
#include "MainFrm.h"
#include "SaveTaskDlg.h"
#include "DSUtil/FileHandle.h"
#include "DSUtil/UrlParser.h"
#include "filters/reader/CDDAReader/CDDAReader.h"
#include "filters/reader/CDXAReader/CDXAReader.h"
#include "filters/reader/VTSReader/VTSReader.h"
#include "filters/parser/StreamDriveThru/StreamDriveThru.h"

static unsigned int AdaptUnit(double& val, size_t unitsNb)
{
	unsigned int unit = 0;

	while (val > 1024 && unit < unitsNb) {
		val /= 1024;
		unit++;
	}

	return unit;
}

enum {
	PROGRESS_MERGING = 10000,
	PROGRESS_COMPLETED = INT16_MAX,
};

// CSaveTaskDlg dialog

IMPLEMENT_DYNAMIC(CSaveTaskDlg, CTaskDialog)
CSaveTaskDlg::CSaveTaskDlg(const std::list<SaveItem_t>& saveItems, HRESULT& hr)
	: CTaskDialog(L"", L"", ResStr(IDS_SAVE_FILE), TDCBF_CANCEL_BUTTON, TDF_CALLBACK_TIMER | TDF_POSITION_RELATIVE_TO_WINDOW)
	, m_saveItems(saveItems.cbegin(), saveItems.cend())
{
	if (m_saveItems.empty()) {
		hr = E_INVALIDARG;
		return;
	}

	m_hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (m_hIcon != nullptr) {
		SetMainIcon(m_hIcon);
	}

	SetMainInstruction(m_saveItems.front().title + L"\n" + m_saveItems.front().dstpath);

	SetProgressBarMarquee();
	SetProgressBarRange(0, 1000);
	SetProgressBarPosition(0);

	SetDialogWidth(250);

	hr = InitFileCopy();
}

void CSaveTaskDlg::SetFFmpegPath(const CStringW& ffmpegpath)
{
	m_ffmpegpath = ffmpegpath;
}

bool CSaveTaskDlg::IsCompleteOk()
{
	return m_iProgress == PROGRESS_COMPLETED;
}

HRESULT CSaveTaskDlg::InitFileCopy()
{
	if (FAILED(m_pGB.CoCreateInstance(CLSID_FilterGraph)) || !(m_pMC = m_pGB) || !(m_pMS = m_pGB)) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	HRESULT hr;

	const CString fn = m_saveItems.front().path;
	CComPtr<IFileSourceFilter> pReader;

	if (::PathIsURLW(fn)) {
		CUrlParser urlParser(fn.GetString());
		const CString protocol = urlParser.GetSchemeName();

		if (protocol == L"http" || protocol == L"https") {
			CComPtr<IUnknown> pUnk;
			pUnk.CoCreateInstance(CLSID_3DYDYoutubeSource);

			if (CComQIPtr<IBaseFilter> pSrc = pUnk.p) {
				m_pGB->AddFilter(pSrc, fn);

				if (!(pReader = pUnk) || FAILED(hr = pReader->Load(fn, nullptr))) {
					pReader.Release();
					m_pGB->RemoveFilter(pSrc);
				}
			}

			if (!pReader) {
				m_protocol = protocol::PROTOCOL_HTTP;
				m_SaveThread = std::thread([this] { SaveHTTP(); });

				return S_OK;
			}
		}
		else if (protocol == L"udp") {
			WSADATA wsaData = {};
			WSAStartup(MAKEWORD(2, 2), &wsaData);

			m_addr.sin_family      = AF_INET;
			m_addr.sin_port        = htons((u_short)urlParser.GetPortNumber());
			m_addr.sin_addr.s_addr = htonl(INADDR_ANY);

			ip_mreq imr = { 0 };
			if (InetPton(AF_INET, urlParser.GetHostName(), &imr.imr_multiaddr.s_addr) != 1) {
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

				if (::bind(m_UdpSocket, (struct sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR) {
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
				m_SaveThread = std::thread([this] { SaveUDP(); });

				return S_OK;
			}
		}
	}
	else {
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
			const DWORD fileOpflags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR | FOFX_NOMINIMIZEBOX;
			hr = FileOperation(m_saveItems.front().path, m_saveItems.front().dstpath, FO_COPY, fileOpflags);
			DLogIf(FAILED(hr), L"CSaveTaskDlg : file copy was aborted with error %s", HR2Str(hr));

			return E_ABORT;
		}
	}

fail:

	CComQIPtr<IBaseFilter> pSrc = pReader.p;
	if (FAILED(m_pGB->AddFilter(pSrc, fn))) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(L"Sorry, can't save this file, press \"Cancel\"");

		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pMid = DNew CStreamDriveThruFilter(nullptr, &hr);
	if (FAILED(m_pGB->AddFilter(pMid, L"StreamDriveThru"))) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pDst;
	if (FAILED(pDst.CoCreateInstance(CLSID_FileWriter))) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}
	CComQIPtr<IFileSinkFilter2> pFSF = pDst.p;
	pFSF->SetFileName(m_saveItems.front().dstpath, nullptr);
	pFSF->SetMode(AM_FILE_OVERWRITE);

	if (FAILED(m_pGB->AddFilter(pDst, L"File Writer"))) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	hr = m_pGB->Connect(
		GetFirstPin((pSrc), PINDIR_OUTPUT),
		GetFirstPin((pMid), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(L"Error Connect pSrc / pMid");

		return S_FALSE;
	}

	hr = m_pGB->Connect(
		GetFirstPin((pMid), PINDIR_OUTPUT),
		GetFirstPin((pDst), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCEW(IDI_ERROR));
		SetFooterText(L"Error Connect pMid / pDst");

		return S_FALSE;
	}

	m_pMS = pMid;

	m_pMC->Run();

	return S_OK;
}

void CSaveTaskDlg::SaveUDP()
{
	if (m_protocol != protocol::PROTOCOL_UDP) {
		return;
	}

	m_iProgress = -1;

	HANDLE hFile = CreateFileW(m_saveItems.front().dstpath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	m_iProgress = 0;

	const DWORD bufLen = 64 * KILOBYTE;
	std::vector<BYTE> pBuffer(bufLen);

	int attempts = 0;
	while (!m_bAbort && attempts <= 20) {
		DWORD dwSizeRead = 0;
		const DWORD dwResult = WSAWaitForMultipleEvents(1, &m_WSAEvent, FALSE, 200, FALSE);
		if (dwResult == WSA_WAIT_EVENT_0) {
			WSAResetEvent(m_WSAEvent);
			static int fromlen = sizeof(m_addr);
			dwSizeRead = recvfrom(m_UdpSocket, (char*)pBuffer.data(), bufLen, 0, (SOCKADDR*)&m_addr, &fromlen);
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

		DWORD dwSizeWritten = 0;
		if (FALSE == WriteFile(hFile, (LPCVOID)pBuffer.data(), dwSizeRead, &dwSizeWritten, nullptr) || dwSizeRead != dwSizeWritten) {
			m_bAbort = true;
			break;
		}

		m_pos += dwSizeRead;

		attempts = 0;
	}

	CloseHandle(hFile);
}

void CSaveTaskDlg::SaveHTTP()
{
	if (m_protocol != protocol::PROTOCOL_HTTP) {
		return;
	}

	m_iProgress = -1;

	for (const auto& item : m_saveItems) {
		HRESULT hr = DownloadHTTP(item.path, item.dstpath);
		if (FAILED(hr)) {
			m_bAbort = true;
			return;
		}
	}

	if (m_saveItems.size() >= 2 && m_ffmpegpath.GetLength()) {
		const CPathW finalfile = m_saveItems.front().dstpath;
		const CStringW finalext = finalfile.GetExtension().Mid(1).MakeLower();
		const CStringW tmpfile = finalfile + L".tmp";

		CStringW mapping;
		CStringW metadata;
		unsigned isub = 0;
		CStringW strArgs = L"-y";
		for (unsigned i = 0; i < m_saveItems.size(); ++i) {
			strArgs.AppendFormat(LR"( -i "%s")", m_saveItems[i].dstpath);
			mapping.AppendFormat(L" -map %u", i);
			if (m_saveItems[i].type == 's') {
				mapping.AppendFormat(LR"( -metadata:s:s:%u title="%s")", isub, m_saveItems[i].title);
				isub++;
			}
		}
		strArgs.Append(L" -c copy");
		if (finalext == L"mp4") {
			strArgs.Append(L" -c:s mov_text");
		}
		strArgs.Append(mapping);
		strArgs.AppendFormat(LR"( -f %s "%s")", finalext, tmpfile);

		SHELLEXECUTEINFOW execinfo = { sizeof(execinfo) };
		execinfo.lpFile = m_ffmpegpath.GetString();
		execinfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		execinfo.nShow = SW_HIDE;
		execinfo.lpParameters = strArgs.GetString();

		if (ShellExecuteExW(&execinfo)) {
			WaitForSingleObject(execinfo.hProcess, INFINITE);
			CloseHandle(execinfo.hProcess);
		}

		if (PathFileExistsW(tmpfile)) {
			for (const auto& item : m_saveItems) {
				DeleteFileW(item.dstpath);
			}
			MoveFileW(tmpfile, finalfile);
		}
	}

	m_iProgress = PROGRESS_COMPLETED;
}

HRESULT CSaveTaskDlg::DownloadHTTP(CStringW url, const CStringW filepath)
{
	const DWORD bufLen = 64 * KILOBYTE;
	std::vector<BYTE> pBuffer(bufLen);

	CHTTPAsync httpAsync;
	HRESULT hr = httpAsync.Connect(url, AfxGetAppSettings().iNetworkTimeout * 1000);
	if (FAILED(hr)) {
		return hr;
	}
	m_pos = 0;
	m_len = httpAsync.GetLenght();

	++m_iProgress;

	if (m_bAbort) { // check after connection
		return E_ABORT;
	}

	HANDLE hFile = CreateFileW(filepath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		httpAsync.Close();
		return E_FAIL;
	}

	if (m_len) {
		ULARGE_INTEGER usize;
		usize.QuadPart = m_len;
		HANDLE hMapping = CreateFileMappingW(hFile, nullptr, PAGE_READWRITE, usize.HighPart, usize.LowPart, nullptr);
		if (hMapping != INVALID_HANDLE_VALUE) {
			CloseHandle(hMapping);
		}
	}

	int attempts = 0;
	while (!m_bAbort && attempts <= 20) {
		DWORD dwSizeRead = 0;
		hr = httpAsync.Read(pBuffer.data(), bufLen, dwSizeRead);
		if (hr != S_OK) {
			break;
		}

		DWORD dwSizeWritten = 0;
		if (FALSE == WriteFile(hFile, (LPCVOID)pBuffer.data(), dwSizeRead, &dwSizeWritten, nullptr) || dwSizeRead != dwSizeWritten) {
			hr = E_FAIL;
			break;
		}

		m_pos += dwSizeRead;
		if (m_len && m_len == m_pos) {
			hr = S_OK;
			break;
		}
	}

	CloseHandle(hFile);

	return m_bAbort ? E_ABORT : hr;
}

HRESULT CSaveTaskDlg::OnDestroy()
{
	if (m_pMC) {
		m_pMC->Stop();
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

	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}

	return S_OK;
}

HRESULT CSaveTaskDlg::OnTimer(_In_ long lTime)
{
	const int iProgress = m_iProgress;

	if (iProgress == PROGRESS_COMPLETED) {
		ClickCommandControl(IDCANCEL);
		return S_OK;
	}

	if (iProgress != m_iPrevState) {
		if (iProgress >= 0 && iProgress < (int)m_saveItems.size()) {
			CStringW path = m_saveItems[iProgress].dstpath;
			EllipsisPath(path, 50);
			SetMainInstruction(m_saveItems.front().title + L"\n" + path);
			m_SaveStats.Reset();
		}
		else if (iProgress == PROGRESS_MERGING) {
			SetMainInstruction(m_saveItems.front().title);
			SetContent(L"Merging files...");
		}
		m_iPrevState = iProgress;
	}

	static UINT sizeUnits[]  = { IDS_SIZE_UNIT_K,  IDS_SIZE_UNIT_M,  IDS_SIZE_UNIT_G  };
	static UINT speedUnits[] = { IDS_SPEED_UNIT_K, IDS_SPEED_UNIT_M, IDS_SPEED_UNIT_G };

	if (iProgress >= 0 && iProgress < (int)m_saveItems.size()) {
		const UINT64 pos = m_pos.load(); // bytes
		const long speed = m_SaveStats.AddValuesGetSpeed(pos, clock());

		double dPos = pos / 1024.0;
		const unsigned int unitPos = AdaptUnit(dPos, std::size(sizeUnits));
		double dSpeed = speed / 1024.0;
		const unsigned int unitSpeed = AdaptUnit(dSpeed, std::size(speedUnits));

		CString str;
		if (m_len) {
			double dDur = m_len / 1024.0;
			const unsigned int unitDur = AdaptUnit(dDur, std::size(sizeUnits));

			str.Format(L"%.2lf %s / %.2lf %s , %.2lf %s",
					   dPos, ResStr(sizeUnits[unitPos]),
					   dDur, ResStr(sizeUnits[unitDur]),
					   dSpeed, ResStr(speedUnits[unitSpeed]));

			if (speed > 0) {
				const REFERENCE_TIME sec = (m_len - pos) / speed;
				if (sec > 0 && sec < 921600) {
					DVD_HMSF_TIMECODE tcDur = {
						(BYTE)(sec / 3600),
						(BYTE)(sec / 60 % 60),
						(BYTE)(sec % 60),
						0
					};

					str.AppendChar(L',');

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
	} else if (m_pGB && m_pMS) {
		CString str;
		REFERENCE_TIME pos = 0, dur = 0;
		m_pMS->GetCurrentPosition(&pos);
		m_pMS->GetDuration(&dur);
		REFERENCE_TIME time = 0;
		CComQIPtr<IMediaSeeking>(m_pGB)->GetCurrentPosition(&time);
		const REFERENCE_TIME speed = time > 0 ? pos * 10000000 / time : 0;

		double dPos = pos / 1024.0;
		const unsigned int unitPos = AdaptUnit(dPos, std::size(sizeUnits));
		double dDur = dur / 1024.0;
		const unsigned int unitDur = AdaptUnit(dDur, std::size(sizeUnits));
		double dSpeed = speed / 1024.0;
		const unsigned int unitSpeed = AdaptUnit(dSpeed, std::size(speedUnits));

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

				str.AppendChar(L',');

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
			m_iProgress = PROGRESS_COMPLETED;
			return S_FALSE;
		}
	}

	return S_OK;
}

HRESULT CSaveTaskDlg::OnCommandControlClick(_In_ int nCommandControlID)
{
	if (nCommandControlID == IDCANCEL) {
		if (m_pGB) {
			m_pGB->Abort();
		}
		if (m_pMC) {
			m_pMC->Stop();
		}
	}

	return S_OK;
}
