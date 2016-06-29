/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
CSaveDlg::CSaveDlg(CString in, CString name, CString out, HRESULT& hr)
	: CTaskDialog(L"", name + L"\n" + out, ResStr(IDS_SAVE_FILE), TDCBF_CANCEL_BUTTON, TDF_CALLBACK_TIMER|TDF_POSITION_RELATIVE_TO_WINDOW)
	, m_in(in)
	, m_out(out)
{
	m_hIcon = (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (m_hIcon != NULL) {
		SetMainIcon(m_hIcon);
	}

	SetProgressBarMarquee();
	SetProgressBarRange(0, 100);
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
	hr = afxGlobalData.ShellCreateItemFromParsingName(sourceFile, NULL, IID_PPV_ARGS(&psiItem));
	EXIT_ON_ERROR(hr);

	CComPtr<IShellItem> psiDestinationFolder;
	CString pszPath = AddSlash(GetFolderOnly(destFile));
	hr = afxGlobalData.ShellCreateItemFromParsingName(pszPath, NULL, IID_PPV_ARGS(&psiDestinationFolder));
	EXIT_ON_ERROR(hr);

	CComPtr<IFileOperation> pFileOperation;
	hr = CoCreateInstance(CLSID_FileOperation,
						  NULL,
						  CLSCTX_INPROC_SERVER,
						  IID_PPV_ARGS(&pFileOperation));
	EXIT_ON_ERROR(hr);

	hr = pFileOperation->SetOperationFlags(FOF_UI_FLAGS | FOFX_NOMINIMIZEBOX);
	EXIT_ON_ERROR(hr);

	CString pszCopyName = GetFileOnly(destFile);
	hr = pFileOperation->CopyItem(psiItem, psiDestinationFolder, pszCopyName, NULL);
	EXIT_ON_ERROR(hr);

	hr = pFileOperation->PerformOperations();

	return hr;
}

HRESULT CSaveDlg::InitFileCopy()
{
	if (FAILED(pGB.CoCreateInstance(CLSID_FilterGraph)) || !(pMC = pGB) || !(pMS = pGB)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
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

				if (!(pReader = pUnk) || FAILED(hr = pReader->Load(fn, NULL))) {
					pReader.Release();
					pGB->RemoveFilter(pSrc);
				}
			}

			if (!pReader
					&& (m_HTTPAsync.Connect(fn, 3000) == S_OK)) {
				m_len = m_HTTPAsync.GetLenght();

				m_hFile = CreateFile(m_out, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (m_hFile != INVALID_HANDLE_VALUE) {
					if (m_len) {
						ULARGE_INTEGER usize = { 0 };
						usize.QuadPart = m_len;
						HANDLE hMapping = CreateFileMapping(m_hFile, NULL, PAGE_READWRITE, usize.HighPart, usize.LowPart, NULL);
						if (hMapping != INVALID_HANDLE_VALUE) {
							CloseHandle(hMapping);
						}
					}

					m_SaveThread = std::thread([this] { Save(); });

					return S_OK;
				}
			}
		} else if (protocol == L"udp") {
			// don't working ...
			/*
			hr = S_OK;
			CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CUDPReader(NULL, &hr);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fn, NULL))) {
				pReader.Release();
			}
			*/
		}
	} else {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDDAReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fn, NULL))) {
			pReader.Release();
		}

		if (!pReader) {
			hr = S_OK;
			CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDXAReader(NULL, &hr);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fn, NULL))) {
				pReader.Release();
			}
		}

		if (!pReader) {
			hr = S_OK;
			CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CVTSReader(NULL, &hr);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fn, NULL))) {
				pReader.Release();
			}
		}

		if (!pReader) {
			CopyFiles(m_in, m_out);
			return E_ABORT;
		}
	}

	CComQIPtr<IBaseFilter> pSrc = pReader;
	if (FAILED(pGB->AddFilter(pSrc, fn))) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(L"Sorry, can't save this file, press \"Cancel\"");

		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pMid = DNew CStreamDriveThruFilter(NULL, &hr);
	if (FAILED(pGB->AddFilter(pMid, L"StreamDriveThru"))) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pDst;
	pDst.CoCreateInstance(CLSID_FileWriter);
	CComQIPtr<IFileSinkFilter2> pFSF = pDst;
	pFSF->SetFileName(CStringW(m_out), NULL);
	pFSF->SetMode(AM_FILE_OVERWRITE);

	if (FAILED(pGB->AddFilter(pDst, L"File Writer"))) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	hr = pGB->Connect(
		GetFirstPin((pSrc), PINDIR_OUTPUT),
		GetFirstPin((pMid), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(L"Error Connect pSrc / pMid");

		return S_FALSE;
	}

	hr = pGB->Connect(
		GetFirstPin((pMid), PINDIR_OUTPUT),
		GetFirstPin((pDst), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
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

		while (!m_bAbort) {
			DWORD dwSizeRead = 0;
			const HRESULT hr = m_HTTPAsync.Read(pBuffer, bufLen, &dwSizeRead);
			if (hr != S_OK) {
				m_bAbort = true;
				break;
			}

			DWORD dwSizeWritten = 0;
			if (FALSE == WriteFile(m_hFile, (LPCVOID)pBuffer, dwSizeRead, &dwSizeWritten, NULL) || dwSizeRead != dwSizeWritten) {
				m_bAbort = true;
				break;
			}

			m_pos += dwSizeRead;
			if (m_len && m_len == m_pos) {
				m_bAbort = true;
				break;
			}
		}
	}

	ClickCommandControl(IDCANCEL);
}

HRESULT CSaveDlg::OnTimer(_In_ long lTime)
{
	static UINT sizeUnits[]  = { IDS_SIZE_UNIT_K,  IDS_SIZE_UNIT_M,  IDS_SIZE_UNIT_G  };
	static UINT speedUnits[] = { IDS_SPEED_UNIT_K, IDS_SPEED_UNIT_M, IDS_SPEED_UNIT_G };

	if (m_hFile) {
		const QWORD pos = m_pos;

		double dPos = pos / 1024.0;
		const unsigned int unitPos = AdaptUnit(dPos, _countof(sizeUnits));

		const clock_t timeSec = (clock() - m_startTime) / 1000;
		const LONG speed = timeSec > 0 ? pos / timeSec : 0;
		double dSpeed = speed / 1024.0;
		const unsigned int unitSpeed = AdaptUnit(dSpeed, _countof(speedUnits));

		CString str;
		if (m_len) {
			double dDur = m_len / 1024.0;
			const unsigned int unitDur = AdaptUnit(dDur, _countof(sizeUnits));

			str.Format(L"%.2lf %s / %.2lf %s , %.2lf %s",
					   dPos, ResStr(sizeUnits[unitPos]), dDur, ResStr(sizeUnits[unitDur]),
					   dSpeed, ResStr(speedUnits[unitSpeed]));
		} else {
			str.Format(L"%.2lf %s , %.2lf %s",
					   dPos, ResStr(sizeUnits[unitPos]),
					   dSpeed, ResStr(speedUnits[unitSpeed]));
		}

		if (speed > 0 && m_len) {
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

		SetContent(str);

		if (m_len) {
			SetProgressBarPosition(static_cast<int>(100 * m_pos / m_len));
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
				   dPos, ResStr(sizeUnits[unitPos]), dDur, ResStr(sizeUnits[unitDur]),
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

		SetProgressBarPosition(dur > 0 ? (int)(100 * pos / dur) : 0);

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