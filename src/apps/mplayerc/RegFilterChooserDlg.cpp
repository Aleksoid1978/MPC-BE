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
#include <dmo.h>
#include "RegFilterChooserDlg.h"
#include "FGFilter.h"
#include "DSUtil/FileHandle.h"
#include "DSUtil/std_helper.h"
#include "PPageExternalFilters.h"

// CRegFilterChooserDlg dialog

CRegFilterChooserDlg::CRegFilterChooserDlg(CWnd* pParent)
	: CResizableDialog(CRegFilterChooserDlg::IDD, pParent)
{
}

void CRegFilterChooserDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST2, m_list);
}

void CRegFilterChooserDlg::AddToList(IMoniker* pMoniker)
{
	CComPtr<IPropertyBag> pPB;
	if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPB)))) {
		CComVariant var;
		if (SUCCEEDED(pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr))) {
			CComVariant var2;
			if (SUCCEEDED(pPB->Read(L"CLSID", &var2, nullptr))) {
				CStringW clsid(var2.bstrVal);
				if (!clsid.IsEmpty() && IsSupportedExternalVideoRenderer(GUIDFromCString(clsid))) {
					return;
				}
			}

			m_monikers.emplace_back(pMoniker);
			int iItem = m_list.InsertItem(m_list.GetItemCount(), CStringW(var.bstrVal));
			m_list.SetItemData(iItem, (DWORD_PTR)pMoniker);
		}
	}

}

BEGIN_MESSAGE_MAP(CRegFilterChooserDlg, CResizableDialog)
	ON_LBN_DBLCLK(IDC_LIST1, OnLbnDblclkList1)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOK)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, OnNMDblclkList2)
END_MESSAGE_MAP()

// CRegFilterChooserDlg message handlers

BOOL CRegFilterChooserDlg::OnInitDialog()
{
	__super::OnInitDialog();

	BeginEnumSysDev(CLSID_LegacyAmFilterCategory, pMoniker) {
		AddToList(pMoniker);
	}
	EndEnumSysDev

	BeginEnumSysDev(DMOCATEGORY_VIDEO_EFFECT, pMoniker) {
		AddToList(pMoniker);
	}
	EndEnumSysDev

	BeginEnumSysDev(DMOCATEGORY_AUDIO_EFFECT, pMoniker) {
		AddToList(pMoniker);
	}
	EndEnumSysDev

	AddAnchor(IDC_LIST2, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON1, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	SetMinTrackSize(CSize(300,100));

	return TRUE;
}

void CRegFilterChooserDlg::OnLbnDblclkList1()
{
	SendMessageW(WM_COMMAND, IDOK);
}

void CRegFilterChooserDlg::OnUpdateOK(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_list.GetFirstSelectedItemPosition());
}

void CRegFilterChooserDlg::OnBnClickedOk()
{
	CComPtr<IMoniker> pMoniker;

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	if (pos) {
		const IMoniker* ptr = (IMoniker*)m_list.GetItemData(m_list.GetNextSelectedItem(pos));
		auto it = FindInListByPointer(m_monikers, ptr);
		if (it != m_monikers.end()) {
			pMoniker = *it;
		}
	}
	if (pMoniker) {
		CFGFilterRegistry fgf(pMoniker);
		auto& f = m_filters.emplace_back(DNew FilterOverride);
		f->type      = FilterOverride::REGISTERED;
		f->name      = fgf.GetName();
		f->dispname  = fgf.GetDisplayName();
		f->clsid     = fgf.GetCLSID();
		f->guids     = fgf.GetTypes();
		f->dwMerit   = fgf.GetMeritForDirectShow();
		f->iLoadType = FilterOverride::MERIT;
	}

	__super::OnOK();
}

void CRegFilterChooserDlg::OnBnClickedButton1()
{
	CAppSettings& s = AfxGetAppSettings();

	CFileDialog dlg(TRUE, nullptr, nullptr,
					OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_NOCHANGEDIR,
					L"DirectShow Filters (*.ax,*.dll)|*.ax;*.dll|", this, 0);
	dlg.m_ofn.lpstrInitialDir = s.strLastOpenFilterDir;

	if (dlg.DoModal() == IDOK) {
		CString fname = dlg.GetPathName();
		s.strLastOpenFilterDir = GetAddSlash(GetFolderPath(fname));

		CFilterMapper2 fm2(false);
		fm2.Register(fname);
		m_filters.splice(m_filters.cend(), fm2.m_filters); // transfer elements from fm2.m_filters to the end of m_filters

		__super::OnOK();
	}
}

void CRegFilterChooserDlg::OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_list.GetFirstSelectedItemPosition()) {
		OnBnClickedOk();
	}

	*pResult = 0;
}
