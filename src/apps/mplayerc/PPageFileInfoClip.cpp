/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include <qnetwork.h>
#include "../../DSUtil/WinAPIUtils.h"


// CPPageFileInfoClip dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoClip, CPropertyPage)
CPPageFileInfoClip::CPPageFileInfoClip(CString fn, IFilterGraph* pFG)
	: CPropertyPage(CPPageFileInfoClip::IDD, CPPageFileInfoClip::IDD)
	, m_fn(fn)
	, m_clip(ResStr(IDS_AG_NONE))
	, m_author(ResStr(IDS_AG_NONE))
	, m_copyright(ResStr(IDS_AG_NONE))
	, m_rating(ResStr(IDS_AG_NONE))
	, m_location_str(ResStr(IDS_AG_NONE))
	, m_album(ResStr(IDS_AG_NONE))
	, m_hIcon(NULL)
{
	auto pFrame = AfxGetMainFrame();

	BeginEnumFilters(pFG, pEF, pBF) {
		if (CComQIPtr<IPropertyBag> pPB = pBF) {
			if (!pFrame->CheckMainFilter(pBF)) {
				continue;
			}

			CComVariant var;
			if (SUCCEEDED(pPB->Read(CComBSTR(_T("ALBUM")), &var, NULL))) {
				m_album = var.bstrVal;
			}

			var.Clear();
			if (SUCCEEDED(pPB->Read(CComBSTR(_T("LYRICS")), &var, NULL))) {
				m_descText = var.bstrVal;
				if (m_descText.Find('\n') && m_descText.Find(L"\r\n") == -1) {
					m_descText.Replace(L"\n", L"\r\n");
				}
			}
		}

		if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
			if (!pFrame->CheckMainFilter(pBF)) {
				continue;
			}

			CComBSTR bstr;
			if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
				m_clip = bstr.m_str;
				bstr.Empty();
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
				m_descText.Replace(L";", L"\r\n");
				bstr.Empty();
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
		CString path = m_location_str;

		if (path[path.GetLength() - 1] != '\\') {
			path += _T("\\");
		}

		path += m_fn;

		if (path.Find(_T("://")) == -1 && ExploreToFile(path)) {
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

#define SETPAGEFOCUS WM_APP+252 // arbitrary number, can be changed if necessary
BEGIN_MESSAGE_MAP(CPPageFileInfoClip, CPropertyPage)
	ON_WM_SIZE()
	ON_MESSAGE(SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

// CPPageFileInfoClip message handlers

BOOL CPPageFileInfoClip::OnInitDialog()
{
	__super::OnInitDialog();

	m_hIcon = LoadIcon(m_fn, false);
	if (m_hIcon) {
		m_icon.SetIcon(m_hIcon);
	}

	m_fn.TrimRight('/');

	if (m_fn.Find(_T("://")) > 0) {
		if (m_fn.Find(_T("/"), m_fn.Find(_T("://")) + 3) < 0) {
			m_location_str = m_fn;
		}
	}

	if (m_location_str.IsEmpty() || m_location_str == ResStr(IDS_AG_NONE)) {
		int i = max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));

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
			m_descText.Format(ResStr(IDS_PUBLISHED) + L"%04hu-%02hu-%02hu %02hu:%02hu:%02hu\r\n\r\n",
							  pMainFrame->m_youtubeFields.dtime.wYear,
							  pMainFrame->m_youtubeFields.dtime.wMonth,
							  pMainFrame->m_youtubeFields.dtime.wDay,
							  pMainFrame->m_youtubeFields.dtime.wHour,
							  pMainFrame->m_youtubeFields.dtime.wMinute,
							  pMainFrame->m_youtubeFields.dtime.wSecond);
		}

		if (!pMainFrame->m_youtubeFields.content.IsEmpty()) {
			m_descText += pMainFrame->m_youtubeFields.content;

		}
	}

	m_location.SetWindowText(m_location_str);
	m_desc.SetWindowText(m_descText);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageFileInfoClip::OnSetActive()
{
	BOOL ret = __super::OnSetActive();
	PostMessage(SETPAGEFOCUS, 0, 0L);

	return ret;
}

LRESULT CPPageFileInfoClip::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->GetTabControl()->SetFocus();

	SendDlgItemMessage(IDC_EDIT1, EM_SETSEL, 0, 0);

	return 0;
}

void CPPageFileInfoClip::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	CRect r(0, 0, 0, 0);
	if (::IsWindow(m_desc.GetSafeHwnd())) {
		m_desc.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_desc.SetWindowPos(NULL, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}

	HDWP hDWP = ::BeginDeferWindowPos(1);
	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if (pChild != GetDlgItem(IDC_EDIT7) && pChild != GetDlgItem(IDC_DEFAULTICON)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.right += dx;
			::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
		}
	}
	::EndDeferWindowPos(hDWP);
}
