/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include "MainFrm.h"
#include "SaveDlg.h"
#include "../../filters/filters.h"
#include "OpenImage.h"

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

IMPLEMENT_DYNAMIC(CSaveDlg, CCmdUIDialog)
CSaveDlg::CSaveDlg(CString in, CString name, CString out, CWnd* pParent)
	: CCmdUIDialog(CSaveDlg::IDD, pParent)
	, m_in(in)
	, m_name(name)
	, m_out(out)
	, m_nIDTimerEvent((UINT_PTR)-1)
{
}

CSaveDlg::~CSaveDlg()
{
}

void CSaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CCmdUIDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_ANIMATE1, m_anim);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
	DDX_Control(pDX, IDC_REPORT, m_report);
	DDX_Control(pDX, IDC_FROMTO, m_fromto);
}

BEGIN_MESSAGE_MAP(CSaveDlg, CCmdUIDialog)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CSaveDlg message handlers

BOOL CSaveDlg::OnInitDialog()
{
	CCmdUIDialog::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	CString str, in = m_name, out = m_out;

	if (m_in.Find(_T("http://")) != -1 || m_in.Find(_T("ftp://")) != -1) {
		m_anim.SendMessage(ACM_OPEN, (WPARAM)AfxGetInstanceHandle(), (LPARAM)IDR_AVI_WEB_FILECOPY);
	} else {
		m_anim.SendMessage(ACM_OPEN, (WPARAM)AfxGetInstanceHandle(), (LPARAM)IDR_AVI_FILECOPY);
	}
	m_anim.Play(0, (UINT)-1, (UINT)-1);

	if (in.GetLength() > 60) {
		in = in.Left(17) + _T("..") + in.Right(43);
	}

	if (out.GetLength() > 60) {
		out = out.Left(17) + _T("..") + out.Right(43);
	}

	str.Format(_T("%s\r\n%s"), in, out);
	m_fromto.SetWindowText(str);

	m_progress.SetRange(0, 100);

	if (OpenImageCheck(m_in) && s.strSnapShotExt != _T(".*")) {
		OpenImageDIB(m_in, m_out, s.iThumbQuality, 0);
		EndDialog(IDOK);
		return TRUE;
	}

	if (FAILED(pGB.CoCreateInstance(CLSID_FilterGraph)) || !(pMC = pGB) || !(pME = pGB) || !(pMS = pGB)
			|| FAILED(pME->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0))) {
		m_report.SetWindowText(_T("Error"));
		return FALSE;
	}

	HRESULT hr;

	CStringW fnw = m_in;
	CComPtr<IFileSourceFilter> pReader;

	if (!pReader && m_in.Mid(m_in.ReverseFind('.')+1).MakeLower() == _T("cda")) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDDAReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDXAReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader /*&& ext == _T("ifo")*/) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CVTSReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		} else {
			CPath pout(m_out);
			pout.RenameExtension(_T(".ifo"));
			CopyFile(m_in, pout, FALSE);
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		hr = pUnk.CoCreateInstance(CLSID_AsyncReader);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		pUnk.CoCreateInstance(CLSID_3DYDYoutubeSource);

		if (CComQIPtr<IBaseFilter> pSrc = pUnk) {
			hr = pGB->AddFilter(pSrc, fnw);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(hr = pReader->Load(fnw, NULL))) {
				pReader.Release();
				pGB->RemoveFilter(pSrc);
			}
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		hr = pUnk.CoCreateInstance(CLSID_URLReader);

		if (CComQIPtr<IBaseFilter> pSrc = pUnk) {
			hr = pGB->AddFilter(pSrc, fnw);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(hr = pReader->Load(fnw, NULL))) {
				pReader.Release();
				pGB->RemoveFilter(pSrc);
			}
		}
	}

	CComQIPtr<IBaseFilter> pSrc = pReader;

	if (FAILED(pGB->AddFilter(pSrc, fnw))) {
		m_report.SetWindowText(_T("Sorry, can't save this file, press cancel"));

		return FALSE;
	}

	CComQIPtr<IBaseFilter> pMid = DNew CStreamDriveThruFilter(NULL, &hr);

	if (FAILED(pGB->AddFilter(pMid, L"StreamDriveThru"))) {
		m_report.SetWindowText(_T("Error"));

		return FALSE;
	}

	CComQIPtr<IBaseFilter> pDst;
	pDst.CoCreateInstance(CLSID_FileWriter);
	CComQIPtr<IFileSinkFilter2> pFSF = pDst;
	pFSF->SetFileName(CStringW(m_out), NULL);
	pFSF->SetMode(AM_FILE_OVERWRITE);

	if (FAILED(pGB->AddFilter(pDst, L"File Writer"))) {
		m_report.SetWindowText(_T("Error"));

		return FALSE;
	}

	hr = pGB->Connect(
			 GetFirstPin((pSrc), PINDIR_OUTPUT),
			 GetFirstPin((pMid), PINDIR_INPUT));

	if (FAILED(hr)) {
		m_report.SetWindowText(_T("Error Connect pSrc / pMid"));
		return FALSE;
	}

	hr = pGB->Connect(
			 GetFirstPin((pMid), PINDIR_OUTPUT),
			 GetFirstPin((pDst), PINDIR_INPUT));
	if (FAILED(hr)) {
		m_report.SetWindowText(_T("Error Connect pMid / pDst"));
		return FALSE;
	}

	pMS = pMid;

	pMC->Run();

	m_nIDTimerEvent = SetTimer(1, 500, NULL);

	return TRUE;
}

void CSaveDlg::OnBnClickedCancel()
{
	if (pMC) {
		pMC->Stop();
	}

	OnCancel();
}

LRESULT CSaveDlg::OnGraphNotify(WPARAM wParam, LPARAM lParam)
{
	LONG evCode, evParam1, evParam2;

	while (pME && SUCCEEDED(pME->GetEvent(&evCode, (LONG_PTR*)&evParam1, (LONG_PTR*)&evParam2, 0))) {
		HRESULT hr = pME->FreeEventParams(evCode, evParam1, evParam2);
		UNREFERENCED_PARAMETER(hr);

		if (EC_COMPLETE == evCode) {
			EndDialog(IDOK);
		} else if (EC_ERRORABORT == evCode) {
			//TRACE(_T("CSaveDlg::OnGraphNotify / EC_ERRORABORT, hr = %08x\n"), (HRESULT)evParam1);
			m_report.SetWindowText(_T("Copying unexpectedly terminated!"));
		}
	}

	return 0;
}

void CSaveDlg::OnTimer(UINT_PTR nIDEvent)
{
	static UINT sizeUnits[]  = { IDS_SIZE_UNIT_K,  IDS_SIZE_UNIT_M,  IDS_SIZE_UNIT_G  };
	static UINT speedUnits[] = { IDS_SPEED_UNIT_K, IDS_SPEED_UNIT_M, IDS_SPEED_UNIT_G };

	if (nIDEvent == m_nIDTimerEvent && pGB && pMS) {
		CString str;
		REFERENCE_TIME pos = 0, dur = 0;
		pMS->GetCurrentPosition(&pos);
		pMS->GetDuration(&dur);
		REFERENCE_TIME time = 0;
		CComQIPtr<IMediaSeeking>(pGB)->GetCurrentPosition(&time);
		REFERENCE_TIME speed = time > 0 ? pos * 10000000 / time : 0;

		double dPos = pos / 1024.;
		unsigned int unitPos = AdaptUnit(dPos, _countof(sizeUnits));
		double dDur = dur / 1024.;
		unsigned int unitDur = AdaptUnit(dDur, _countof(sizeUnits));
		double dSpeed = speed / 1024.;
		unsigned int unitSpeed = AdaptUnit(dSpeed, _countof(speedUnits));

		str.Format(_T("%.2lf %s / %.2lf %s , %.2lf %s"),
				   dPos, ResStr(sizeUnits[unitPos]), dDur, ResStr(sizeUnits[unitDur]),
				   dSpeed, ResStr(speedUnits[unitSpeed]));

		if (speed > 0) {
			str.Append(_T(","));
			REFERENCE_TIME sec = (dur - pos) / speed;

			DVD_HMSF_TIMECODE tcDur = {
				(BYTE)(sec / 3600),
				(BYTE)(sec / 60 % 60),
				(BYTE)(sec % 60),
				0
			};

			if (tcDur.bHours > 0) {
				str.AppendFormat(_T(" %0.2dh"), tcDur.bHours);
			}
			if (tcDur.bMinutes > 0) {
				str.AppendFormat(_T(" %0.2dm"), tcDur.bMinutes);
			}
			if (tcDur.bSeconds > 0) {
				str.AppendFormat(_T(" %0.2ds"), tcDur.bSeconds);
			}
		}

		m_report.SetWindowText(str);

		m_progress.SetPos(dur > 0 ? (int)(100 * pos / dur) : 0);
	}

	CCmdUIDialog::OnTimer(nIDEvent);
}

// CSaveTaskDlg dialog

IMPLEMENT_DYNAMIC(CSaveTaskDlg, CTaskDialog)
CSaveTaskDlg::CSaveTaskDlg(CString in, CString name, CString out)
	: CTaskDialog(_T(""), name + _T("\n") + out, ResStr(IDS_SAVE_FILE), TDCBF_CANCEL_BUTTON, TDF_CALLBACK_TIMER|TDF_POSITION_RELATIVE_TO_WINDOW)
	, m_in(in)
	, m_out(out)
	, m_TaskDlgHwnd(0)
{
	m_hIcon = (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (m_hIcon != NULL) {
		SetMainIcon(m_hIcon);
	}

	SetProgressBarMarquee();
	SetProgressBarRange(0, 100);
	SetProgressBarPosition(0);

	SetDialogWidth(250);

	InitFileCopy();
}

CSaveTaskDlg::~CSaveTaskDlg()
{
	if (pMC) {
		pMC->Stop();
	}

	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

HRESULT CSaveTaskDlg::OnInit()
{
	m_TaskDlgHwnd = ::GetActiveWindow();

	if (OpenImageCheck(m_in)) {
		OpenImageDIB(m_in, m_out, AfxGetAppSettings().iThumbQuality, 0);
		::SendMessage(m_TaskDlgHwnd, TDM_CLICK_BUTTON, static_cast<WPARAM>(TDCBF_CANCEL_BUTTON), 0);
	}

	return S_OK;
}

HRESULT CSaveTaskDlg::InitFileCopy()
{
	if (FAILED(pGB.CoCreateInstance(CLSID_FilterGraph)) || !(pMC = pGB) || !(pME = pGB) || !(pMS = pGB)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));
		return S_FALSE;
	}

	HRESULT hr;

	CStringW fnw = m_in;
	CComPtr<IFileSourceFilter> pReader;

	if (!pReader && m_in.Mid(m_in.ReverseFind('.')+1).MakeLower() == _T("cda")) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDDAReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDXAReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CVTSReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		} else {
			CPath pout(m_out);
			pout.RenameExtension(_T(".ifo"));
			CopyFile(m_in, pout, FALSE);
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		pUnk.CoCreateInstance(CLSID_AsyncReader);

		if (!(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		pUnk.CoCreateInstance(CLSID_3DYDYoutubeSource);

		if (CComQIPtr<IBaseFilter> pSrc = pUnk) {
			pGB->AddFilter(pSrc, fnw);

			if (!(pReader = pUnk) || FAILED(hr = pReader->Load(fnw, NULL))) {
				pReader.Release();
				pGB->RemoveFilter(pSrc);
			}
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		pUnk.CoCreateInstance(CLSID_URLReader);

		if (CComQIPtr<IBaseFilter> pSrc = pUnk) {
			pGB->AddFilter(pSrc, fnw);

			if (!(pReader = pUnk) || FAILED(hr = pReader->Load(fnw, NULL))) {
				pReader.Release();
				pGB->RemoveFilter(pSrc);
			}
		}
	}

	CComQIPtr<IBaseFilter> pSrc = pReader;

	if (FAILED(pGB->AddFilter(pSrc, fnw))) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(_T("Sorry, can't save this file, press Cancel"));
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
		SetFooterText(_T("Error Connect pSrc / pMid"));

		return S_FALSE;
	}

	hr = pGB->Connect(
			 GetFirstPin((pMid), PINDIR_OUTPUT),
			 GetFirstPin((pDst), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(_T("Error Connect pMid / pDst"));

		return S_FALSE;
	}

	pMS = pMid;

	pMC->Run();

	return S_OK;
}

HRESULT CSaveTaskDlg::OnTimer(_In_ long lTime)
{
	static UINT sizeUnits[]  = { IDS_SIZE_UNIT_K,  IDS_SIZE_UNIT_M,  IDS_SIZE_UNIT_G  };
	static UINT speedUnits[] = { IDS_SPEED_UNIT_K, IDS_SPEED_UNIT_M, IDS_SPEED_UNIT_G };

	if (pGB && pMS) {
		CString str;
		REFERENCE_TIME pos = 0, dur = 0;
		pMS->GetCurrentPosition(&pos);
		pMS->GetDuration(&dur);
		REFERENCE_TIME time = 0;
		CComQIPtr<IMediaSeeking>(pGB)->GetCurrentPosition(&time);
		REFERENCE_TIME speed = time > 0 ? pos * 10000000 / time : 0;

		double dPos = pos / 1024.;
		unsigned int unitPos = AdaptUnit(dPos, _countof(sizeUnits));
		double dDur = dur / 1024.;
		unsigned int unitDur = AdaptUnit(dDur, _countof(sizeUnits));
		double dSpeed = speed / 1024.;
		unsigned int unitSpeed = AdaptUnit(dSpeed, _countof(speedUnits));

		str.Format(_T("%.2lf %s / %.2lf %s , %.2lf %s"),
				   dPos, ResStr(sizeUnits[unitPos]), dDur, ResStr(sizeUnits[unitDur]),
				   dSpeed, ResStr(speedUnits[unitSpeed]));

		if (speed > 0) {
			str.Append(_T(","));
			REFERENCE_TIME sec = (dur - pos) / speed;

			DVD_HMSF_TIMECODE tcDur = {
				(BYTE)(sec / 3600),
				(BYTE)(sec / 60 % 60),
				(BYTE)(sec % 60),
				0
			};

			if (tcDur.bHours > 0) {
				str.AppendFormat(_T(" %0.2dh"), tcDur.bHours);
			}
			if (tcDur.bMinutes > 0) {
				str.AppendFormat(_T(" %0.2dm"), tcDur.bMinutes);
			}
			if (tcDur.bSeconds > 0) {
				str.AppendFormat(_T(" %0.2ds"), tcDur.bSeconds);
			}
		}

		SetContent(str);

		SetProgressBarPosition(dur > 0 ? (int)(100 * pos / dur) : 0);

		if (dur && pos >= dur) {
			::SendMessage(m_TaskDlgHwnd, TDM_CLICK_BUTTON, static_cast<WPARAM>(TDCBF_CANCEL_BUTTON), 0);
			return S_FALSE;
		}
	}

	return S_OK;
}
