/*
 * (C) 2023-2024 see Authors.txt
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
CSaveTaskDlg::CSaveTaskDlg(const std::list<SaveItem_t>& saveItems, const CStringW& dstPath, HRESULT& hr)
	: CTaskDialog(L"", L"", ResStr(IDS_SAVE_FILE), TDCBF_CANCEL_BUTTON, TDF_CALLBACK_TIMER | TDF_POSITION_RELATIVE_TO_WINDOW)
	, m_saveItems(saveItems.cbegin(), saveItems.cend())
{
	if (m_saveItems.empty() || dstPath.IsEmpty()) {
		hr = E_INVALIDARG;
		return;
	}

	SetLangDefault("en");

	const CStringW finalext = GetFileExt(dstPath).MakeLower();

	m_dstPaths.resize(m_saveItems.size());
	m_dstPaths[0] = dstPath;
	for (unsigned i = 1; i < m_saveItems.size(); ++i) {
		const auto& item = m_saveItems[i];
		switch (item.type) {
		case 'a':
			m_dstPaths[i] = GetRenameFileExt(dstPath, (finalext == L".mp4") ? L".audio.m4a" : L".audio.mka");
			break;
		case 's': {
			CStringW subext = L"." + item.title + L".vtt";
			FixFilename(subext);
			m_dstPaths[i] = GetRenameFileExt(dstPath, subext);
			break;
		}
		case 't': {
			GetTemporaryFilePath(item.title, m_dstPaths[i]);
			break;
		}
		}
	}

	m_hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (m_hIcon != nullptr) {
		SetMainIcon(m_hIcon);
	}

	SetMainInstruction(m_dstPaths.front());

	SetProgressBarMarquee();
	SetProgressBarRange(0, 1000);
	SetProgressBarPosition(0);

	SetDialogWidth(250);

	hr = InitFileCopy();
}

void CSaveTaskDlg::SetFFmpegPath(const CStringW& ffmpegpath)
{
	m_ffmpegPath = ffmpegpath;
}

void CSaveTaskDlg::SetLangDefault(const CStringA& langDefault)
{
	int isub = 0;

	for (unsigned i = 0; i < m_saveItems.size(); ++i) {
		const auto& item = m_saveItems[i];
		if (item.type == 's') {
			if (item.lang == langDefault) {
				m_iSubLangDefault = isub;
				break;
			}
			isub++;
		}
	}
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
				m_SaveThread = std::thread([this] { SaveHTTP(m_iSubLangDefault); });

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
			hr = FileOperation(m_saveItems.front().path, m_dstPaths.front(), FO_COPY, fileOpflags);
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
	pFSF->SetFileName(m_dstPaths.front(), nullptr);
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

	HANDLE hFile = CreateFileW(m_dstPaths.front(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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

		m_written += dwSizeRead;

		attempts = 0;
	}

	CloseHandle(hFile);
}

void CSaveTaskDlg::SaveHTTP(const int iSubLangDefault)
{
	if (m_protocol != protocol::PROTOCOL_HTTP) {
		return;
	}

	m_iProgress = -1;

	for (unsigned i = 0; i < m_saveItems.size(); ++i) {
		HRESULT hr = DownloadHTTP(m_saveItems[i].path, m_dstPaths[i]);
		if (FAILED(hr)) {
			m_bAbort = true;
			return;
		}
	}

	if (m_saveItems.size() >= 2 && m_ffmpegPath.GetLength()) {
		m_iProgress = PROGRESS_MERGING;

		const CStringW finalfile(m_dstPaths.front());
		const CStringW finalext = GetFileExt(finalfile).Mid(1).MakeLower();
		const CStringW tmpfile  = finalfile + L".tmp";
		const CStringW format =
			(finalext == L"m4a") ? CStringW(L"mp4") :
			(finalext == L"mka") ? CStringW(L"matroska") :
			finalext;

		CStringW strArgs = L"-y";
		CStringW mapping;
		CStringW metadata;
		unsigned isub = 0;

		for (unsigned i = 0; i < m_saveItems.size(); ++i) {
			const auto& item = m_saveItems[i];

			if (item.type == 't' && finalext == L"mka") {
				strArgs.AppendFormat(LR"( -attach "%s")", m_dstPaths[i]);
				if (item.title == L".jpg") {
					metadata.Append(L" -metadata:s:t mimetype=image/jpeg -metadata:s:t:0 filename=cover.jpg");
				}
				else if (item.title == L".webp") {
					metadata.Append(L" -metadata:s:t mimetype=image/webp -metadata:s:t:0 filename=cover.webp");
				}
			}
			else {
				strArgs.AppendFormat(LR"( -i "%s")", m_dstPaths[i]);
				mapping.AppendFormat(L" -map %u", i);
			}

			if (item.type == 's') {
				LPCSTR lang = ISO6391To6392(item.lang);
				if (lang[0]) {
					metadata.AppendFormat(LR"( -metadata:s:s:%u language="%S")", isub, lang);
				}
				if (item.title.GetLength()) {
					if (item.title != ISO6391ToLanguage(item.lang)) {
						metadata.AppendFormat(LR"( -metadata:s:s:%u title="%s")", isub, item.title);
					}
				}
				isub++;
			}
		}
		strArgs.Append(L" -c copy");
		if (finalext == L"mp4") {
			strArgs.Append(L" -c:s mov_text");
		}
		strArgs.Append(mapping);
		strArgs.Append(metadata);
		if (iSubLangDefault >= 0) {
			strArgs.AppendFormat(L" -disposition:s:%d default", iSubLangDefault);
		}
		if (m_saveItems.back().type == 't' && finalext == L"m4a") {
			if (EndsWith(m_dstPaths.back(), L".webp")) {
				strArgs.Append(L" -c:v:0 mjpeg");
			}
			strArgs.Append(L" -disposition:v:0 attached_pic");
		}
		if (format == L"mp4") {
			strArgs.Append(L" -movflags +faststart");
		}
		strArgs.AppendFormat(LR"( -f %s "%s")", format, tmpfile);

		SHELLEXECUTEINFOW execinfo = { sizeof(execinfo) };
		execinfo.lpFile = m_ffmpegPath.GetString();
		execinfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		execinfo.nShow = SW_HIDE;
		execinfo.lpParameters = strArgs.GetString();

		if (ShellExecuteExW(&execinfo)) {
			WaitForSingleObject(execinfo.hProcess, INFINITE);
			CloseHandle(execinfo.hProcess);
		}

		if (PathFileExistsW(tmpfile)) {
			for (const auto& dstpath : m_dstPaths) {
				DeleteFileW(dstpath);
			}
			MoveFileW(tmpfile, finalfile);
		}
	}

	m_iProgress = PROGRESS_COMPLETED;
}

HRESULT CSaveTaskDlg::DownloadHTTP(CStringW url, const CStringW filepath)
{
	m_length = 0;
	m_written = 0;
	++m_iProgress;

	const DWORD bufLen = 64 * KILOBYTE;
	std::vector<BYTE> pBuffer(bufLen);

	CHTTPAsync httpAsync;
	HRESULT hr = httpAsync.Connect(url, AfxGetAppSettings().iNetworkTimeout * 1000);
	if (FAILED(hr)) {
		return hr;
	}
	m_length = httpAsync.GetLenght();

	if (m_bAbort) { // check after connection
		return E_ABORT;
	}

	HANDLE hFile = CreateFileW(filepath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		httpAsync.Close();
		return E_FAIL;
	}

	if (m_length) {
		ULARGE_INTEGER usize;
		usize.QuadPart = m_length;
		HANDLE hMapping = CreateFileMappingW(hFile, nullptr, PAGE_READWRITE, usize.HighPart, usize.LowPart, nullptr);
		if (hMapping != INVALID_HANDLE_VALUE) {
			CloseHandle(hMapping);
		}
	}

	while (!m_bAbort) {
		DWORD dwSizeRead = 0;
		hr = httpAsync.Read(pBuffer.data(), bufLen, dwSizeRead, http::readTimeout);
		if (hr != S_OK) {
			break;
		}

		DWORD dwSizeWritten = 0;
		if (FALSE == WriteFile(hFile, (LPCVOID)pBuffer.data(), dwSizeRead, &dwSizeWritten, nullptr) || dwSizeRead != dwSizeWritten) {
			hr = E_FAIL;
			break;
		}

		m_written += dwSizeRead;
		if (m_length && m_length == m_written) {
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
	const int iProgress = m_iProgress.load();

	if (iProgress == PROGRESS_COMPLETED) {
		ClickCommandControl(IDCANCEL);
		return S_OK;
	}

	if (iProgress != m_iPrevState) {
		if (iProgress >= 0 && iProgress < (int)m_saveItems.size()) {
			CStringW path = m_dstPaths[iProgress];
			EllipsisPath(path, 100);
			SetMainInstruction(path);
			m_SaveStats.Reset();
		}
		else if (iProgress == PROGRESS_MERGING) {
			SetMainInstruction(m_saveItems.front().title);
			SetContent(ResStr(IDS_MERGING_FILES));
		}
		m_iPrevState = iProgress;
	}

	static UINT sizeUnits[]  = { IDS_UNIT_SIZE_KB,  IDS_UNIT_SIZE_MB,  IDS_UNIT_SIZE_GB };
	static UINT speedUnits[] = { IDS_UNIT_SPEED_KB_S, IDS_UNIT_SPEED_MB_S, IDS_UNIT_SPEED_GB_S };

	if (iProgress >= 0 && iProgress < (int)m_saveItems.size()) {
		const UINT64 written = m_written.load();
		const UINT64 length  = m_length.load();

		const long speed = m_SaveStats.AddValuesGetSpeed(written, clock());

		double dPos = written / 1024.0;
		const unsigned int unitPos = AdaptUnit(dPos, std::size(sizeUnits));
		double dSpeed = speed / 1024.0;
		const unsigned int unitSpeed = AdaptUnit(dSpeed, std::size(speedUnits));

		CString str;
		if (length) {
			double dDur = length / 1024.0;
			const unsigned int unitDur = AdaptUnit(dDur, std::size(sizeUnits));

			str.Format(L"%.2lf %s / %.2lf %s , %.2lf %s",
					   dPos, ResStr(sizeUnits[unitPos]),
					   dDur, ResStr(sizeUnits[unitDur]),
					   dSpeed, ResStr(speedUnits[unitSpeed]));

			if (speed > 0) {
				const UINT64 time = (length - written + (speed - 1)) / speed; // round up the remaining time
				if (time > 0 && time < 360000) {
					const unsigned hours   = time / 3600;
					const unsigned minutes = time / 60 % 60;
					const unsigned seconds = time % 60;

					str.AppendChar(L',');

					if (hours > 0) {
						str.AppendFormat(L" %2u %s", hours, ResStr(IDS_UNIT_TIME_H));
					}
					if (minutes > 0) {
						str.AppendFormat(L" %2u %s", minutes, ResStr(IDS_UNIT_TIME_M));
					}
					if (seconds > 0) {
						str.AppendFormat(L" %2u %s", seconds, ResStr(IDS_UNIT_TIME_S));
					}
				}
			}
		} else {
			str.Format(L"%.2lf %s , %.2lf %s",
					   dPos, ResStr(sizeUnits[unitPos]),
					   dSpeed, ResStr(speedUnits[unitSpeed]));
		}

		SetContent(str);

		if (length) {
			SetProgressBarPosition(static_cast<int>(1000 * written / length));
		}

		if (m_bAbort) {
			ClickCommandControl(IDCANCEL);
			return S_FALSE;
		}
	}
	else if (iProgress == PROGRESS_MERGING) {
		return S_OK;
	}
	else if (m_pGB && m_pMS) {
		CString str;
		REFERENCE_TIME pos = 0;
		REFERENCE_TIME dur = 0;
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
