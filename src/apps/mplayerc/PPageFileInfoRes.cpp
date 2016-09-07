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
#include "PPageFileInfoRes.h"
#include "Misc.h"

// CPPageFileInfoRes dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoRes, CPPageBase)
CPPageFileInfoRes::CPPageFileInfoRes(CString fn, IFilterGraph* pFG)
	: CPPageBase(CPPageFileInfoRes::IDD, CPPageFileInfoRes::IDD)
	, m_fn(fn)
	, m_hIcon(NULL)
{
	m_fn.TrimRight('/');
	int i = max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));

	if (i >= 0 && i < m_fn.GetLength() - 1) {
		m_fn = m_fn.Mid(i + 1);
	}

	BeginEnumFilters(pFG, pEF, pBF) {
		if (CComQIPtr<IDSMResourceBag> pRB = pBF)
		if (pRB && pRB->ResGetCount() > 0) {
			for (DWORD i = 0; i < pRB->ResGetCount(); i++) {
				CComBSTR name, desc, mime;
				BYTE* pData = NULL;
				DWORD len = 0;
				if (SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, NULL))) {
					CDSMResource r;
					r.name = name;
					r.desc = desc;
					r.mime = mime;
					r.data.SetCount(len);
					memcpy(r.data.GetData(), pData, r.data.GetCount());
					CoTaskMemFree(pData);
					m_res.AddTail(r);
				}
			}
		}
	}
	EndEnumFilters;
}

CPPageFileInfoRes::~CPPageFileInfoRes()
{
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

void CPPageFileInfoRes::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

#define SETPAGEFOCUS WM_APP+252 // arbitrary number, can be changed if necessary
BEGIN_MESSAGE_MAP(CPPageFileInfoRes, CPPageBase)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON1, OnSaveAs)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON1, OnUpdateSaveAs)
	ON_MESSAGE(SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

// CPPageFileInfoRes message handlers

BOOL CPPageFileInfoRes::OnInitDialog()
{
	__super::OnInitDialog();

	m_hIcon = LoadIcon(m_fn, false);
	if (m_hIcon) {
		m_icon.SetIcon(m_hIcon);
	}

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	m_list.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 187);
	m_list.InsertColumn(1, _T("Mime Type"), LVCFMT_LEFT, 127);

	POSITION pos = m_res.GetHeadPosition();
	while (pos) {
		CDSMResource res = m_res.GetAt(pos);

		int iItem = m_list.InsertItem(m_list.GetItemCount(), res.name);
		m_list.SetItemText(iItem, 1, res.mime);
		m_list.SetItemData(iItem, (DWORD_PTR)pos);

		m_res.GetNext(pos);
	}

	UpdateData(FALSE);

	return TRUE;
}

void CPPageFileInfoRes::OnSaveAs()
{
	int i = m_list.GetSelectionMark();

	if (i < 0) {
		return;
	}

	if (m_list.GetItemData(i) == NULL) {
		return;
	}

	CDSMResource& r = m_res.GetAt((POSITION)m_list.GetItemData(i));

	CString fname(r.name);
	CString ext = ::PathFindExtension(fname);

	CString ext_list = _T("All files|*.*|");
	CString mime(r.mime);
	mime.MakeLower();
	if (mime == _T("application/x-truetype-font") || mime == _T("application/x-font-ttf")) {
		ext_list = _T("TrueType Font (*.ttf)|*.ttf|");
		if (ext.IsEmpty()) {
			fname += _T(".ttf");
		}
	} else if (mime == _T("image/jpeg") || mime == _T("image/jpg")) {
		ext_list = _T("JPG - JPEG Image (*.jpg)|*.jpg|");
		if (ext.IsEmpty()) {
			fname += _T(".jpg");
		}
	} else if (mime == _T("image/png")) {
		ext_list = _T("PNG - Portable Network Graphics (*.png)|*.png|");
		if (ext.IsEmpty()) {
			fname += _T(".png");
		}
	}

	CFileDialog fd(FALSE, NULL, fname,
				   OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR,
				   ext_list, this, 0);
	if (fd.DoModal() == IDOK) {
		FILE* f = NULL;
		if (!_tfopen_s(&f, fd.GetPathName(), _T("wb"))) {
			fwrite(r.data.GetData(), 1, r.data.GetCount(), f);
			fclose(f);
		}
	}
}

void CPPageFileInfoRes::OnUpdateSaveAs(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_list.GetSelectedCount());
}

void CPPageFileInfoRes::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	CRect r(0, 0, 0, 0);
	if (::IsWindow(m_list.GetSafeHwnd())) {
		m_list.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_list.SetWindowPos(NULL, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}

	HDWP hDWP = ::BeginDeferWindowPos(1);
	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if (pChild->SendMessage(WM_GETDLGCODE) & DLGC_BUTTON) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.top += dy;
			r.bottom += dy;
			::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, r.left, r.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
		} else if (pChild != GetDlgItem(IDC_LIST1) && pChild != GetDlgItem(IDC_DEFAULTICON)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.right += dx;
			::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
		}
	}
	::EndDeferWindowPos(hDWP);
}

BOOL CPPageFileInfoRes::OnSetActive()
{
	BOOL ret = __super::OnSetActive();
	PostMessage(SETPAGEFOCUS, 0, 0L);

	return ret;
}

LRESULT CPPageFileInfoRes::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->GetTabControl()->SetFocus();

	SendDlgItemMessage(IDC_EDIT1, EM_SETSEL, 0, 0);

	return 0;
}