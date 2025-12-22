/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include "PPageFileInfoClip.h"
#include <wmsdkidl.h>


// CPPageFileInfoClip dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoClip, CPropertyPage)
CPPageFileInfoClip::CPPageFileInfoClip(const CStringW& fn, IFilterGraph* pFG)
	: CPropertyPage(CPPageFileInfoClip::IDD, CPPageFileInfoClip::IDD)
	, m_fn(fn)
	, m_clip(ResStr(IDS_AG_NONE))
	, m_author(ResStr(IDS_AG_NONE))
	, m_copyright(ResStr(IDS_AG_NONE))
	, m_rating(ResStr(IDS_AG_NONE))
	, m_location_str(ResStr(IDS_AG_NONE))
	, m_album(ResStr(IDS_AG_NONE))
{
	HRESULT hr = E_NOT_SET;
	auto pFrame = AfxGetMainFrame();

	BeginEnumFilters(pFG, pEF, pBF) {
		if (!pFrame->CheckMainFilter(pBF)) {
			continue;
		}

		if (CComQIPtr<IPropertyBag> pPB = pBF.p) {
			CComVariant var;
			if (SUCCEEDED(pPB->Read(CComBSTR(L"ALBUM"), &var, nullptr))) {
				m_album = var.bstrVal;
			}

			var.Clear();
			if (SUCCEEDED(pPB->Read(CComBSTR(L"LYRICS"), &var, nullptr))) {
				m_descText = var.bstrVal;
				if (m_descText.Find(L'\n') && m_descText.Find(L"\r\n") == -1) {
					m_descText.Replace(L"\n", L"\r\n");
				}
			}
		}

		if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF.p) {
			CComBSTR bstr;
			if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
				m_clip = bstr.m_str;
				bstr.Empty();
				break;
			}
			if (SUCCEEDED(pAMMC->get_AuthorName(&bstr)) && bstr.Length()) {
				m_author = bstr.m_str;
				bstr.Empty();
			}
			if (SUCCEEDED(pAMMC->get_Copyright(&bstr)) && bstr.Length()) {
				m_copyright = bstr.m_str;
				bstr.Empty();
			}
			if (SUCCEEDED(pAMMC->get_Rating(&bstr)) && bstr.Length()) {
				m_rating = bstr.m_str;
				bstr.Empty();
			}
			if (SUCCEEDED(pAMMC->get_Description(&bstr)) && bstr.Length()) {
				m_descText = bstr.m_str;
				if (m_descText.Find(L'\n') && m_descText.Find(L"\r\n") == -1) {
					m_descText.Replace(L"\n", L"\r\n");
				}
				m_descText.Replace(L";", L"\r\n");
				bstr.Empty();
			}
		}
		else if (CComQIPtr<IWMHeaderInfo> pWMHI = pBF.p) {
			WORD streamNum = 0;
			WMT_ATTR_DATATYPE type;
			WORD length;
			std::vector<BYTE> value;

			hr = pWMHI->GetAttributeByName(&streamNum, L"Title", &type, nullptr, &length);
			if (SUCCEEDED(hr) && type == WMT_TYPE_STRING && length > sizeof(wchar_t)) {
				value.resize(length);
				hr = pWMHI->GetAttributeByName(&streamNum, L"Title", &type, value.data(), &length);
				if (SUCCEEDED(hr)) {
					m_clip.SetString((LPCWSTR)value.data(), length / sizeof(wchar_t));
				}
			}
			hr = pWMHI->GetAttributeByName(&streamNum, L"Author", &type, nullptr, &length);
			if (SUCCEEDED(hr) && type == WMT_TYPE_STRING && length > sizeof(wchar_t)) {
				value.resize(length);
				hr = pWMHI->GetAttributeByName(&streamNum, L"Author", &type, value.data(), &length);
				if (SUCCEEDED(hr)) {
					m_author.SetString((LPCWSTR)value.data(), length / sizeof(wchar_t));
				}
			}
		}
	}
	EndEnumFilters;
}

CPPageFileInfoClip::~CPPageFileInfoClip()
{
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

BOOL CPPageFileInfoClip::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_LBUTTONDBLCLK && pMsg->hwnd == m_location.m_hWnd && !m_location_str.IsEmpty()) {
		CStringW path = m_location_str;

		if (path[path.GetLength() - 1] != '\\') {
			path += L"\\";
		}

		path += m_fn;

		if (path.Find(L"://") == -1 && ExploreToFile(path)) {
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CPPageFileInfoClip::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Text(pDX, IDC_EDIT4, m_clip);
	DDX_Text(pDX, IDC_EDIT3, m_author);
	DDX_Text(pDX, IDC_EDIT8, m_album);
	DDX_Text(pDX, IDC_EDIT2, m_copyright);
	DDX_Text(pDX, IDC_EDIT5, m_rating);
	DDX_Control(pDX, IDC_EDIT6, m_location);
	DDX_Control(pDX, IDC_EDIT7, m_desc);
}

BEGIN_MESSAGE_MAP(CPPageFileInfoClip, CPropertyPage)
	ON_WM_SIZE()
	ON_MESSAGE(SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

// CPPageFileInfoClip message handlers

BOOL CPPageFileInfoClip::OnInitDialog()
{
	__super::OnInitDialog();

	m_hIcon = LoadIconW(m_fn, false);
	if (m_hIcon) {
		m_icon.SetIcon(m_hIcon);
	}

	m_fn.TrimRight('/');

	if (m_fn.Find(L"://") > 0) {
		if (m_fn.Find(L"/", m_fn.Find(L"://") + 3) < 0) {
			m_location_str = m_fn;
		}
	}

	if (m_location_str.IsEmpty() || m_location_str == ResStr(IDS_AG_NONE)) {
		int i = std::max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));

		if (i >= 0 && i < m_fn.GetLength() - 1) {
			m_location_str = m_fn.Left(i);
			m_fn = m_fn.Mid(i + 1);

			if (m_location_str.GetLength() == 2 && m_location_str[1] == ':') {
				m_location_str += '\\';
			}
		}
	}

	const CMainFrame* pMainFrame = AfxGetMainFrame();
	if (pMainFrame) {
		if (!pMainFrame->m_youtubeFields.title.IsEmpty()) {
			m_clip = pMainFrame->m_youtubeFields.title;
		}

		if (!pMainFrame->m_youtubeFields.author.IsEmpty()) {
			m_author = pMainFrame->m_youtubeFields.author;
		}

		if (pMainFrame->m_youtubeFields.dtime.wYear) {
			if (!pMainFrame->m_youtubeFields.dtime.wHour
					&& !pMainFrame->m_youtubeFields.dtime.wMinute
					&& !pMainFrame->m_youtubeFields.dtime.wSecond) {
				m_descText.Format(ResStr(IDS_PUBLISHED) + L"%04hu-%02hu-%02hu\r\n\r\n",
								  pMainFrame->m_youtubeFields.dtime.wYear,
								  pMainFrame->m_youtubeFields.dtime.wMonth,
								  pMainFrame->m_youtubeFields.dtime.wDay);
			} else {
				m_descText.Format(ResStr(IDS_PUBLISHED) + L"%04hu-%02hu-%02hu %02hu:%02hu:%02hu\r\n\r\n",
								  pMainFrame->m_youtubeFields.dtime.wYear,
								  pMainFrame->m_youtubeFields.dtime.wMonth,
								  pMainFrame->m_youtubeFields.dtime.wDay,
								  pMainFrame->m_youtubeFields.dtime.wHour,
								  pMainFrame->m_youtubeFields.dtime.wMinute,
								  pMainFrame->m_youtubeFields.dtime.wSecond);
			}
		}

		if (!pMainFrame->m_youtubeFields.content.IsEmpty()) {
			m_descText.Append(pMainFrame->m_youtubeFields.content);
		}
	}

	m_location.SetWindowTextW(m_location_str);
	m_desc.SetWindowTextW(m_descText);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageFileInfoClip::OnSetActive()
{
	BOOL ret = __super::OnSetActive();
	PostMessageW(SETPAGEFOCUS, 0, 0L);

	return ret;
}

LRESULT CPPageFileInfoClip::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->GetTabControl()->SetFocus();

	SendDlgItemMessageW(IDC_EDIT1, EM_SETSEL, 0, 0);

	return 0;
}

void CPPageFileInfoClip::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	CRect r;
	if (::IsWindow(m_desc.GetSafeHwnd())) {
		m_desc.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_desc.SetWindowPos(nullptr, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
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
