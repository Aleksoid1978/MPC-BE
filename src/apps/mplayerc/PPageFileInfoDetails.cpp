/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2019 see Authors.txt
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
#include "PPageFileInfoDetails.h"
#include <atlbase.h>
#include <d3d9.h>
#include <vmr9.h>
#include <moreuuids.h>
#include "MainFrm.h"

static bool GetProperty(IFilterGraph* pFG, LPCOLESTR propName, VARIANT* vt)
{
	BeginEnumFilters(pFG, pEF, pBF) {
		if (CComQIPtr<IPropertyBag> pPB = pBF)
			if (SUCCEEDED(pPB->Read(propName, vt, nullptr))) {
				return true;
			}
	}
	EndEnumFilters;

	return false;
}

static CString FormatDateTime(FILETIME tm)
{
	SYSTEMTIME st;
	FileTimeToSystemTime(&tm, &st);
	WCHAR buff[256];
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, nullptr, buff, 256);
	CString ret(buff);
	ret += L" ";
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, nullptr, buff, 256);
	ret += buff;
	return ret;
}

// CPPageFileInfoDetails dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoDetails, CPropertyPage)
CPPageFileInfoDetails::CPPageFileInfoDetails(const CString& fn, IFilterGraph* pFG, ISubPicAllocatorPresenter3* pCAP, IDvdInfo2* pDVDI)
	: CPropertyPage(CPPageFileInfoDetails::IDD, CPPageFileInfoDetails::IDD)
	, m_fn(fn)
	, m_hIcon(nullptr)
	, m_type(ResStr(IDS_AG_NOT_KNOWN))
	, m_size(ResStr(IDS_AG_NOT_KNOWN))
	, m_time(ResStr(IDS_AG_NOT_KNOWN))
	, m_resolution(ResStr(IDS_AG_NOT_KNOWN))
	, m_created(ResStr(IDS_AG_NOT_KNOWN))
{
	CComVariant vt;

	CString createdDate;
	if (::GetProperty(pFG, L"CurFile.TimeCreated", &vt)) {
		if (V_VT(&vt) == VT_UI8) {
			ULARGE_INTEGER uli;
			uli.QuadPart = V_UI8(&vt);

			FILETIME ft;
			ft.dwLowDateTime = uli.LowPart;
			ft.dwHighDateTime = uli.HighPart;

			createdDate = FormatDateTime(ft);
		}
	}

	WIN32_FIND_DATAW wfd;
	HANDLE hFind = FindFirstFileW(m_fn, &wfd);

	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);

		__int64 size = (__int64(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
		const int MAX_FILE_SIZE_BUFFER = 65;
		WCHAR szFileSize[MAX_FILE_SIZE_BUFFER];
		StrFormatByteSizeW(size, szFileSize, MAX_FILE_SIZE_BUFFER);
		CString szByteSize;
		szByteSize.Format(L"%I64d", size);
		m_size.Format(L"%s (%s bytes)", szFileSize, FormatNumber(szByteSize));

		if (createdDate.IsEmpty()) {
			createdDate = FormatDateTime(wfd.ftCreationTime);
		}
	}

	if (!createdDate.IsEmpty()) {
		m_created = createdDate;
	}

	REFERENCE_TIME rtDur = 0;
	CComQIPtr<IMediaSeeking> pMS = pFG;

	if (pMS && SUCCEEDED(pMS->GetDuration(&rtDur)) && rtDur > 0) {
		m_time = ReftimeToString2(rtDur);
	}

	CSize wh(0, 0), arxy(0, 0);

	if (pCAP) {
		wh = pCAP->GetVideoSize();
		arxy = pCAP->GetVideoSizeAR();
	} else {
		if (CComQIPtr<IBasicVideo> pBV = pFG) {
			if (SUCCEEDED(pBV->GetVideoSize(&wh.cx, &wh.cy))) {
				if (CComQIPtr<IBasicVideo2> pBV2 = pFG) {
					pBV2->GetPreferredAspectRatio(&arxy.cx, &arxy.cy);
				}
			} else {
				wh.SetSize(0, 0);
			}
		}

		if (wh.cx == 0 && wh.cy == 0) {
			BeginEnumFilters(pFG, pEF, pBF) {
				if (CComQIPtr<IBasicVideo> pBV = pBF) {
					pBV->GetVideoSize(&wh.cx, &wh.cy);
					if (CComQIPtr<IBasicVideo2> pBV2 = pBF) {
						pBV2->GetPreferredAspectRatio(&arxy.cx, &arxy.cy);
					}
					break;
				} else if (CComQIPtr<IVMRWindowlessControl> pWC = pBF) {
					pWC->GetNativeVideoSize(&wh.cx, &wh.cy, &arxy.cx, &arxy.cy);
					break;
				} else if (CComQIPtr<IVMRWindowlessControl9> pWC = pBF) {
					pWC->GetNativeVideoSize(&wh.cx, &wh.cy, &arxy.cx, &arxy.cy);
					break;
				} else if (CComQIPtr<IMFGetService> pMFGS = pBF) {
					CComPtr<IMFVideoDisplayControl> pMFVDC;
					if (SUCCEEDED(pMFGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pMFVDC)))) {
						pMFVDC->GetNativeVideoSize(&wh, &arxy);
						break;
					}
				}
			}
			EndEnumFilters;
		}
	}

	if (wh.cx > 0 && wh.cy > 0) {
		m_resolution.Format(L"%dx%d", wh.cx, wh.cy);
		ReduceDim(arxy);

		if (arxy.cx > 0 && arxy.cy > 0 && arxy.cx*wh.cy != arxy.cy*wh.cx) {
			CString ar;
			ar.Format(L" (AR %d:%d)", arxy.cx, arxy.cy);
			m_resolution += ar;
		}
	}

	InitEncoding(pFG, pDVDI);
}

CPPageFileInfoDetails::~CPPageFileInfoDetails()
{
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

void CPPageFileInfoDetails::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Text(pDX, IDC_EDIT4, m_type);
	DDX_Text(pDX, IDC_EDIT3, m_size);
	DDX_Text(pDX, IDC_EDIT2, m_time);
	DDX_Text(pDX, IDC_EDIT5, m_resolution);
	DDX_Text(pDX, IDC_EDIT6, m_created);
	DDX_Control(pDX, IDC_EDIT7, m_encoding);
}

#define SETPAGEFOCUS WM_APP + 252	// arbitrary number, can be changed if necessary
BEGIN_MESSAGE_MAP(CPPageFileInfoDetails, CPropertyPage)
	ON_WM_SIZE()
	ON_MESSAGE(SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

// CPPageFileInfoDetails message handlers

BOOL CPPageFileInfoDetails::OnInitDialog()
{
	__super::OnInitDialog();

	m_hIcon = LoadIconW(m_fn, false);
	if (m_hIcon) {
		m_icon.SetIcon(m_hIcon);
	}

	CString ext = m_fn.Left(m_fn.Find(L"://") + 1).TrimRight(':');

	if (ext.IsEmpty() || !ext.CompareNoCase(L"file")) {
		ext = L"." + m_fn.Mid(m_fn.ReverseFind('.')+1);
	}

	if (!LoadType(ext, m_type)) {
		m_type = ResStr(IDS_AG_NOT_KNOWN);
	}

	m_fn.TrimRight('/');
	m_fn.Replace('\\', '/');

	CString tmpStr;
	if (m_fn.Find(L"://") > 0) {
		if (m_fn.Find(L"/", m_fn.Find(L"://") + 3) < 0) {
			tmpStr = m_fn;
		}
	}

	m_fn = tmpStr.IsEmpty() ? m_fn.Mid(m_fn.ReverseFind('/')+1) : tmpStr;

	UpdateData(FALSE);

	m_encoding.SetWindowTextW(m_encodingText);

	return TRUE;
}

BOOL CPPageFileInfoDetails::OnSetActive()
{
	BOOL ret = __super::OnSetActive();
	PostMessageW(SETPAGEFOCUS, 0, 0L);

	return ret;
}

LRESULT CPPageFileInfoDetails::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->GetTabControl()->SetFocus();

	SendDlgItemMessageW(IDC_EDIT1, EM_SETSEL, 0, 0);

	return 0;
}

void CPPageFileInfoDetails::InitEncoding(IFilterGraph* pFG, IDvdInfo2* pDVDI)
{
	std::list<CString> videoStreams;
	std::list<CString> otherStreams;

	BeginEnumFilters(pFG, pEF, pBF) {
		CComPtr<IBaseFilter> pUSBF = GetUpStreamFilter(pBF);

		if (GetCLSID(pBF) == CLSID_NetShowSource) {
			continue;
		} else if (GetCLSID(pUSBF) != CLSID_NetShowSource) {
			if (IPin* pPin = GetFirstPin(pBF, PINDIR_INPUT)) {
				CMediaType mt;
				if (FAILED(pPin->ConnectionMediaType(&mt)) || mt.majortype != MEDIATYPE_Stream) {
					continue;
				}
			}

			if (IPin* pPin = GetFirstPin(pBF, PINDIR_OUTPUT)) {
				CMediaType mt;
				if (SUCCEEDED(pPin->ConnectionMediaType(&mt)) && mt.majortype == MEDIATYPE_Stream) {
					continue;
				}
			}
		}

		BOOL bUsePins = TRUE;
		if (CComQIPtr<IAMStreamSelect> pSS = pBF) {
			DWORD nCount;
			if (FAILED(pSS->Count(&nCount))) {
				nCount = 0;
			}

			for (DWORD i = 0; i < nCount; i++) {
				AM_MEDIA_TYPE* pmt = nullptr;
				WCHAR* pszName = nullptr;
				if (SUCCEEDED(pSS->Info(i, &pmt, nullptr, nullptr, nullptr, &pszName, nullptr, nullptr)) && pmt) {
					CMediaTypeEx mt(*pmt);
					CString str = mt.ToString();
					if (!str.IsEmpty()) {
						if (pszName && wcslen(pszName)) {
							str.AppendFormat(L" [%s]", pszName);
						}
						if (mt.majortype == MEDIATYPE_Video || mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO) {
							videoStreams.push_back(str);
						} else if (!pDVDI) {
							otherStreams.push_back(str);
						}

						bUsePins = FALSE;
					}
				}
				DeleteMediaType(pmt);
				CoTaskMemFree(pszName);
			}
		}

		if (bUsePins) {
			BeginEnumPins(pBF, pEP, pPin) {
				CMediaTypeEx mt;
				PIN_DIRECTION dir;
				if (FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
						|| FAILED(pPin->ConnectionMediaType(&mt))) {
					continue;
				}

				CString str = mt.ToString();
				if (!str.IsEmpty()) {
					str.AppendFormat(L" [%s]", GetPinName(pPin));
					if (mt.majortype == MEDIATYPE_Video || mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO) {
						videoStreams.push_back(str);
					} else if (!pDVDI) {
						otherStreams.push_back(str);
					}
				}

				FreeMediaType(mt);
			}
			EndEnumPins;
		}
	}
	EndEnumFilters;

	m_encodingText = Implode(videoStreams, L"\r\n");
	if (!m_encodingText.IsEmpty()) {
		m_encodingText += L"\r\n";
	}

	if (pDVDI) {
		{
			// DVD Audio
			ULONG ulStreamsAvailable, ulCurrentStream;
			if (SUCCEEDED(pDVDI->GetCurrentAudio(&ulStreamsAvailable, &ulCurrentStream))
						  && ulStreamsAvailable) {
				for (ULONG i = 0; i < ulStreamsAvailable; i++) {
					LCID Language;
					if (FAILED(pDVDI->GetAudioLanguage(i, &Language))) {
						continue;
					}

					CString str;
					if (Language) {
						int len = GetLocaleInfoW(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
						str.ReleaseBufferSetLength(std::max(len-1, 0));
					} else {
						str.Format(ResStr(IDS_AG_UNKNOWN), i+1);
					}

					DVD_AudioAttributes ATR;
					if (SUCCEEDED(pDVDI->GetAudioAttributes(i, &ATR))) {
						switch (ATR.LanguageExtension) {
							default:
							case DVD_AUD_EXT_NotSpecified:
								break;
							case DVD_AUD_EXT_Captions:
								str += L" (Captions)";
								break;
							case DVD_AUD_EXT_VisuallyImpaired:
								str += L" (Visually Impaired)";
								break;
							case DVD_AUD_EXT_DirectorComments1:
								str += ResStr(IDS_MAINFRM_121);
								break;
							case DVD_AUD_EXT_DirectorComments2:
								str += ResStr(IDS_MAINFRM_122);
								break;
						}

						CString format = AfxGetMainFrame()->GetDVDAudioFormatName(ATR);

						if (!format.IsEmpty()) {
							str.Format(ResStr(IDS_MAINFRM_11),
									   CString(str),
									   format,
									   ATR.dwFrequency,
									   ATR.bQuantization,
									   ATR.bNumberOfChannels,
									   (ATR.bNumberOfChannels > 1 ? ResStr(IDS_MAINFRM_13) : ResStr(IDS_MAINFRM_12)));
						}
					}

					otherStreams.push_back(L"Audio: " + str);
				}
			}
		}

		{
			// DVD subtitles
			ULONG ulStreamsAvailable, ulCurrentStream;
			BOOL bIsDisabled;
			if (SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
						  && ulStreamsAvailable) {
				for (ULONG i = 0; i < ulStreamsAvailable; i++) {
					LCID Language;
					if (FAILED(pDVDI->GetSubpictureLanguage(i, &Language))) {
						continue;
					}

					CString str;
					if (Language) {
						int len = GetLocaleInfoW(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
						str.ReleaseBufferSetLength(std::max(len - 1, 0));
					} else {
						str.Format(ResStr(IDS_AG_UNKNOWN), i + 1);
					}

					DVD_SubpictureAttributes ATR;
					if (SUCCEEDED(pDVDI->GetSubpictureAttributes(i, &ATR))) {
						switch (ATR.LanguageExtension) {
							case DVD_SP_EXT_NotSpecified:
							default:
							case DVD_SP_EXT_Caption_Normal:
								break;
							case DVD_SP_EXT_Caption_Big:
								str += L" (Big)";
								break;
							case DVD_SP_EXT_Caption_Children:
								str += L" (Children)";
								break;
							case DVD_SP_EXT_CC_Normal:
								str += L" (CC)";
								break;
							case DVD_SP_EXT_CC_Big:
								str += L" (CC Big)";
								break;
							case DVD_SP_EXT_CC_Children:
								str += L" (CC Children)";
								break;
							case DVD_SP_EXT_Forced:
								str += L" (Forced)";
								break;
							case DVD_SP_EXT_DirectorComments_Normal:
								str += L" (Director Comments)";
								break;
							case DVD_SP_EXT_DirectorComments_Big:
								str += L" (Director Comments, Big)";
								break;
							case DVD_SP_EXT_DirectorComments_Children:
								str += L" (Director Comments, Children)";
								break;
						}
					}

					otherStreams.push_back(L"Subtitle: " + str + L" [DVD Subtitles]");
				}
			}
		}
	}
	m_encodingText += Implode(otherStreams, L"\r\n");
}

void CPPageFileInfoDetails::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	CRect r(0, 0, 0, 0);
	if (::IsWindow(m_encoding.GetSafeHwnd())) {
		m_encoding.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_encoding.SetWindowPos(nullptr,0, 0, r.Width(), r.Height(),SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}

	HDWP hDWP = ::BeginDeferWindowPos(1);
	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != nullptr; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if (pChild != GetDlgItem(IDC_EDIT7) && pChild != GetDlgItem(IDC_DEFAULTICON)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.right += dx;
			::DeferWindowPos(hDWP, pChild->m_hWnd, nullptr, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
		}
	}
	::EndDeferWindowPos(hDWP);
}
