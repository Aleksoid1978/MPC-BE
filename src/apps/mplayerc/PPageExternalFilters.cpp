/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include <atlpath.h>
#include <InitGuid.h>
#include <dmoreg.h>
#include "PPageExternalFilters.h"
#include "ComPropertySheet.h"
#include "RegFilterChooserDlg.h"
#include "SelectMediaType.h"
#include "FGFilter.h"
#include <moreuuids.h>
#include "../../DSUtil/std_helper.h"

static const std::vector<GUID> s_MajorTypes = {
	MEDIATYPE_NULL,
	MEDIATYPE_Video,
	MEDIATYPE_Audio,
	MEDIATYPE_Text,
	MEDIATYPE_Midi,
	MEDIATYPE_Stream,
	MEDIATYPE_Interleaved,
	MEDIATYPE_File,
	MEDIATYPE_ScriptCommand,
	MEDIATYPE_AUXLine21Data,
	MEDIATYPE_VBI,
	MEDIATYPE_Timecode,
	MEDIATYPE_LMRT,
	MEDIATYPE_URL_STREAM,
	MEDIATYPE_MPEG1SystemStream,
	MEDIATYPE_AnalogVideo,
	MEDIATYPE_AnalogAudio,
	MEDIATYPE_MPEG2_PACK,
	MEDIATYPE_MPEG2_PES,
	MEDIATYPE_MPEG2_SECTIONS,
	MEDIATYPE_DVD_ENCRYPTED_PACK,
	MEDIATYPE_DVD_NAVIGATION,
};

static const std::vector<GUID> s_SubTypes = {
	MEDIASUBTYPE_None,
	MEDIASUBTYPE_YUY2,
	MEDIASUBTYPE_MJPG,
	MEDIASUBTYPE_MJPA,
	MEDIASUBTYPE_MJPB,
	MEDIASUBTYPE_TVMJ,
	MEDIASUBTYPE_WAKE,
	MEDIASUBTYPE_CFCC,
	MEDIASUBTYPE_IJPG,
	MEDIASUBTYPE_Plum,
	MEDIASUBTYPE_DVCS,
	MEDIASUBTYPE_DVSD,
	MEDIASUBTYPE_MDVF,
	MEDIASUBTYPE_RGB1,
	MEDIASUBTYPE_RGB4,
	MEDIASUBTYPE_RGB8,
	MEDIASUBTYPE_RGB565,
	MEDIASUBTYPE_RGB555,
	MEDIASUBTYPE_RGB24,
	MEDIASUBTYPE_RGB32,
	MEDIASUBTYPE_ARGB1555,
	MEDIASUBTYPE_ARGB4444,
	MEDIASUBTYPE_ARGB32,
	MEDIASUBTYPE_A2R10G10B10,
	MEDIASUBTYPE_A2B10G10R10,
	MEDIASUBTYPE_AYUV,
	MEDIASUBTYPE_AI44,
	MEDIASUBTYPE_IA44,
	MEDIASUBTYPE_YV12,
	MEDIASUBTYPE_NV12,
	MEDIASUBTYPE_IMC1,
	MEDIASUBTYPE_IMC2,
	MEDIASUBTYPE_IMC3,
	MEDIASUBTYPE_IMC4,
	MEDIASUBTYPE_Overlay,
	MEDIASUBTYPE_MPEG1Packet,
	MEDIASUBTYPE_MPEG1Payload,
	MEDIASUBTYPE_MPEG1AudioPayload,
	MEDIASUBTYPE_MPEG1System,
	MEDIASUBTYPE_MPEG1VideoCD,
	MEDIASUBTYPE_MPEG1Video,
	MEDIASUBTYPE_MPEG1Audio,
	MEDIASUBTYPE_Avi,
	MEDIASUBTYPE_Asf,
	MEDIASUBTYPE_QTMovie,
	MEDIASUBTYPE_QTRpza,
	MEDIASUBTYPE_QTSmc,
	MEDIASUBTYPE_QTRle,
	MEDIASUBTYPE_QTJpeg,
	MEDIASUBTYPE_PCMAudio_Obsolete,
	MEDIASUBTYPE_PCM,
	MEDIASUBTYPE_WAVE,
	MEDIASUBTYPE_AU,
	MEDIASUBTYPE_AIFF,
	MEDIASUBTYPE_dvsd,
	MEDIASUBTYPE_dvhd,
	MEDIASUBTYPE_dvsl,
	MEDIASUBTYPE_dv25,
	MEDIASUBTYPE_dv50,
	MEDIASUBTYPE_dvh1,
	MEDIASUBTYPE_Line21_BytePair,
	MEDIASUBTYPE_Line21_GOPPacket,
	MEDIASUBTYPE_Line21_VBIRawData,
	MEDIASUBTYPE_TELETEXT,
	MEDIASUBTYPE_DRM_Audio,
	MEDIASUBTYPE_IEEE_FLOAT,
	MEDIASUBTYPE_DOLBY_AC3_SPDIF,
	MEDIASUBTYPE_RAW_SPORT,
	MEDIASUBTYPE_SPDIF_TAG_241h,
	MEDIASUBTYPE_DssVideo,
	MEDIASUBTYPE_DssAudio,
	MEDIASUBTYPE_VPVideo,
	MEDIASUBTYPE_VPVBI,
	MEDIASUBTYPE_ATSC_SI,
	MEDIASUBTYPE_DVB_SI,
	MEDIASUBTYPE_MPEG2DATA,
	MEDIASUBTYPE_MPEG2_VIDEO,
	MEDIASUBTYPE_MPEG2_PROGRAM,
	MEDIASUBTYPE_MPEG2_TRANSPORT,
	MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE,
	MEDIASUBTYPE_MPEG2_AUDIO,
	MEDIASUBTYPE_DOLBY_AC3,
	MEDIASUBTYPE_DVD_SUBPICTURE,
	MEDIASUBTYPE_DVD_LPCM_AUDIO,
	MEDIASUBTYPE_DTS,
	MEDIASUBTYPE_SDDS,
	MEDIASUBTYPE_DVD_NAVIGATION_PCI,
	MEDIASUBTYPE_DVD_NAVIGATION_DSI,
	MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER,
	MEDIASUBTYPE_I420,
	MEDIASUBTYPE_WAVE_DOLBY_AC3,
	MEDIASUBTYPE_DTS2,
};

// CPPageExternalFilters dialog

IMPLEMENT_DYNAMIC(CPPageExternalFilters, CPPageBase)
CPPageExternalFilters::CPPageExternalFilters()
	: CPPageBase(CPPageExternalFilters::IDD, CPPageExternalFilters::IDD)
	, m_iLoadType(FilterOverride::PREFERRED)
	, m_pLastSelFilter(nullptr)
{
}

CPPageExternalFilters::~CPPageExternalFilters()
{
}

void CPPageExternalFilters::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_filters);
	DDX_Radio(pDX, IDC_RADIO1, m_iLoadType);
	DDX_Control(pDX, IDC_EDIT1, m_dwMerit);
	DDX_Control(pDX, IDC_TREE2, m_tree);
}

void CPPageExternalFilters::StepUp(CCheckListBox& list)
{
	int i = list.GetCurSel();
	if (i < 1) {
		return;
	}

	CString str;
	list.GetText(i, str);
	DWORD_PTR dwItemData = list.GetItemData(i);
	int nCheck = list.GetCheck(i);
	list.DeleteString(i);
	i--;
	list.InsertString(i, str);
	list.SetItemData(i, dwItemData);
	list.SetCheck(i, nCheck);
	list.SetCurSel(i);

	SetModified();
}

void CPPageExternalFilters::StepDown(CCheckListBox& list)
{
	int i = list.GetCurSel();
	if (i < 0 || i >= list.GetCount()-1) {
		return;
	}

	CString str;
	list.GetText(i, str);
	DWORD_PTR dwItemData = list.GetItemData(i);
	int nCheck = list.GetCheck(i);
	list.DeleteString(i);
	i++;
	list.InsertString(i, str);
	list.SetItemData(i, dwItemData);
	list.SetCheck(i, nCheck);
	list.SetCurSel(i);

	SetModified();
}

FilterOverride* CPPageExternalFilters::GetCurFilter()
{
	int i = m_filters.GetCurSel();
	return i >= 0 ? (FilterOverride*)m_pFilters.GetAt((POSITION)m_filters.GetItemDataPtr(i)) : (FilterOverride*)nullptr;
}

BEGIN_MESSAGE_MAP(CPPageExternalFilters, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO1, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO2, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO3, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateFilterUp)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateFilterDown)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateFilterMerit)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON5, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON6, OnUpdateSubType)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON7, OnUpdateDeleteType)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON8, OnUpdateFilter)
	ON_BN_CLICKED(IDC_BUTTON1, OnAddRegistered)
	ON_BN_CLICKED(IDC_BUTTON2, OnRemoveFilter)
	ON_BN_CLICKED(IDC_BUTTON3, OnMoveFilterUp)
	ON_BN_CLICKED(IDC_BUTTON4, OnMoveFilterDown)
	ON_LBN_DBLCLK(IDC_LIST1, OnLbnDblclkFilter)
	ON_WM_VKEYTOITEM()
	ON_BN_CLICKED(IDC_BUTTON5, OnAddMajorType)
	ON_BN_CLICKED(IDC_BUTTON6, OnAddSubType)
	ON_BN_CLICKED(IDC_BUTTON7, OnDeleteType)
	ON_BN_CLICKED(IDC_BUTTON8, OnResetTypes)
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckChangeList1)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO3, OnBnClickedRadio)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE2, OnNMDblclkTree2)
	ON_NOTIFY(TVN_KEYDOWN, IDC_TREE2, OnTVNKeyDownTree2)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// CPPageExternalFilters message handlers

BOOL CPPageExternalFilters::OnInitDialog()
{
	__super::OnInitDialog();

	DragAcceptFiles(TRUE);

	CAppSettings& s = AfxGetAppSettings();

	m_pFilters.RemoveAll();

	m_filters.SetItemHeight(0, ScaleY(13)+3); // 16 without scale
	POSITION pos = s.m_filters.GetHeadPosition();
	while (pos) {
		CAutoPtr<FilterOverride> f(DNew FilterOverride(s.m_filters.GetNext(pos)));

		CString name(L"<unknown>");

		if (f->type == FilterOverride::REGISTERED) {
			name = CFGFilterRegistry(f->dispname).GetName();
			if (name.IsEmpty()) {
				name = f->name + L" <not registered>";
			}
		} else if (f->type == FilterOverride::EXTERNAL) {
			name = f->name;
			if (f->fTemporary) {
				name += L" <temporary>";
			}
			if (!CPath(MakeFullPath(f->path)).FileExists()) {
				name += L" <not found!>";
			}
		}

		int i = m_filters.AddString(name);
		m_filters.SetCheck(i, f->fDisabled ? 0 : 1);
		m_filters.SetItemDataPtr(i, m_pFilters.AddTail(f));
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageExternalFilters::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.m_filters.RemoveAll();

	for (int i = 0; i < m_filters.GetCount(); i++) {
		if (POSITION pos = (POSITION)m_filters.GetItemData(i)) {
			CAutoPtr<FilterOverride> f(DNew FilterOverride(m_pFilters.GetAt(pos)));
			f->fDisabled = !m_filters.GetCheck(i);
			s.m_filters.AddTail(f);
		}
	}

	s.SaveExternalFilters();

	return __super::OnApply();
}

void CPPageExternalFilters::OnUpdateFilter(CCmdUI* pCmdUI)
{
	if (FilterOverride* f = GetCurFilter()) {
		pCmdUI->Enable(!(pCmdUI->m_nID == IDC_RADIO2 && f->type == FilterOverride::EXTERNAL));
	} else {
		pCmdUI->Enable(FALSE);
	}
}

void CPPageExternalFilters::OnUpdateFilterUp(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_filters.GetCurSel() > 0);
}

void CPPageExternalFilters::OnUpdateFilterDown(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_filters.GetCurSel() >= 0 && m_filters.GetCurSel() < m_filters.GetCount()-1);
}

void CPPageExternalFilters::OnUpdateFilterMerit(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_iLoadType == FilterOverride::MERIT);
}

void CPPageExternalFilters::OnUpdateSubType(CCmdUI* pCmdUI)
{
	HTREEITEM node = m_tree.GetSelectedItem();
	pCmdUI->Enable(node != nullptr && m_tree.GetItemData(node) == NULL);
}

void CPPageExternalFilters::OnUpdateDeleteType(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_tree.GetSelectedItem());
}

void CPPageExternalFilters::OnAddRegistered()
{
	CRegFilterChooserDlg dlg(this);
	if (dlg.DoModal() == IDOK) {
		while (!dlg.m_filters.empty()) {
			FilterOverride* f = dlg.m_filters.front();
			dlg.m_filters.pop_front();

			if (f) {
				CAutoPtr<FilterOverride> p(f);

				if (f->name.IsEmpty() && !f->guids.size() && !f->dwMerit) {
					// skip something strange
					continue;
				}

				CString name = f->name;

				if (f->type == FilterOverride::EXTERNAL) {
					if (!CPath(MakeFullPath(f->path)).FileExists()) {
						name += L" <not found!>";
					}
				}

				int i = m_filters.AddString(name);
				m_filters.SetItemDataPtr(i, m_pFilters.AddTail(p));
				m_filters.SetCheck(i, 1);

				if (dlg.m_filters.empty()) {
					m_filters.SetCurSel(i);
					OnLbnSelchangeList1();
				}

				SetModified();
			}
		}
	}
}

void CPPageExternalFilters::OnRemoveFilter()
{
	int i = m_filters.GetCurSel();
	m_pFilters.RemoveAt((POSITION)m_filters.GetItemDataPtr(i));
	m_filters.DeleteString(i);

	if (i >= m_filters.GetCount()) {
		i--;
	}
	m_filters.SetCurSel(i);
	OnLbnSelchangeList1();

	SetModified();
}

void CPPageExternalFilters::OnMoveFilterUp()
{
	StepUp(m_filters);
}

void CPPageExternalFilters::OnMoveFilterDown()
{
	StepDown(m_filters);
}

void CPPageExternalFilters::OnLbnDblclkFilter()
{
	if (FilterOverride* f = GetCurFilter()) {
		CComPtr<IBaseFilter> pBF;
		CString name;

		if (f->type == FilterOverride::REGISTERED) {
			CStringW namew;
			if (CreateFilter(f->dispname, &pBF, namew)) {
				name = namew;
			}
		} else if (f->type == FilterOverride::EXTERNAL) {
			if (SUCCEEDED(LoadExternalFilter(f->path, f->clsid, &pBF))) {
				name = f->name;
			}
		}

		if (CComQIPtr<ISpecifyPropertyPages> pSPP = pBF) {
			CComPropertySheet ps(name, this);
			if (ps.AddPages(pSPP) > 0) {
				CComPtr<IFilterGraph> pFG;
				if (SUCCEEDED(pFG.CoCreateInstance(CLSID_FilterGraph))) {
					pFG->AddFilter(pBF, L"");
				}

				ps.DoModal();
			}
		}
	}
}

int CPPageExternalFilters::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
	if (nKey == VK_DELETE) {
		OnRemoveFilter();
		return -2;
	}

	return __super::OnVKeyToItem(nKey, pListBox, nIndex);
}

void CPPageExternalFilters::OnAddMajorType()
{
	FilterOverride* f = GetCurFilter();
	if (!f) {
		return;
	}

	CSelectMediaType dlg(s_MajorTypes, MEDIATYPE_NULL, this);
	if (dlg.DoModal() == IDOK) {
		auto it = f->guids.begin();
		while (it != f->guids.end() && std::next(it) != f->guids.end()) {
			if (*it++ == dlg.m_guid) {
				AfxMessageBox(ResStr(IDS_EXTERNAL_FILTERS_ERROR_MT), MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			it++;
		}

		f->guids.push_back(dlg.m_guid);
		it = --f->guids.end();
		f->guids.push_back(GUID_NULL);

		CString major = GetMediaTypeName(dlg.m_guid);
		CString sub = GetMediaTypeName(GUID_NULL);

		HTREEITEM node = m_tree.InsertItem(major);
		m_tree.SetItemData(node, NULL);

		node = m_tree.InsertItem(sub, node);
		m_tree.SetItemData(node, (DWORD_PTR)&(*it));

		SetModified();
	}
}

void CPPageExternalFilters::OnAddSubType()
{
	FilterOverride* f = GetCurFilter();
	if (!f) {
		return;
	}

	HTREEITEM node = m_tree.GetSelectedItem();
	if (!node) {
		return;
	}

	HTREEITEM child = m_tree.GetChildItem(node);
	if (!child) {
		return;
	}

	auto it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(child));
	if (it == f->guids.end()) {
		ASSERT(0);
		return;
	}

	GUID major = *it;

	CSelectMediaType dlg(s_SubTypes, MEDIASUBTYPE_NULL, this);
	if (dlg.DoModal() == IDOK) {
		for (child = m_tree.GetChildItem(node); child; child = m_tree.GetNextSiblingItem(child)) {
			it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(child));
			it++;
			if (*it == dlg.m_guid) {
				AfxMessageBox(ResStr(IDS_EXTERNAL_FILTERS_ERROR_MT), MB_ICONEXCLAMATION | MB_OK);
				return;
			}
		}

		f->guids.push_back(major);
		it = --f->guids.end(); // iterator for major
		f->guids.push_back(dlg.m_guid);

		CString sub = GetMediaTypeName(dlg.m_guid);

		node = m_tree.InsertItem(sub, node);
		m_tree.SetItemData(node, (DWORD_PTR)&(*it));

		SetModified();
	}
}

void CPPageExternalFilters::OnDeleteType()
{
	if (FilterOverride* f = GetCurFilter()) {
		HTREEITEM node = m_tree.GetSelectedItem();
		if (!node) {
			return;
		}

		auto it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(node));

		if (it == f->guids.end()) {
			for (HTREEITEM child = m_tree.GetChildItem(node); child; child = m_tree.GetNextSiblingItem(child)) {
				it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(child));

				f->guids.erase(it++);
				f->guids.erase(it++);
			}

			m_tree.DeleteItem(node);
		} else {
			HTREEITEM parent = m_tree.GetParentItem(node);

			m_tree.DeleteItem(node);

			if (!m_tree.ItemHasChildren(parent)) {
				auto it1 = it++;
				*it = GUID_NULL;
				node = m_tree.InsertItem(GetMediaTypeName(GUID_NULL), parent);
				m_tree.SetItemData(node, (DWORD_PTR)&(*it1));
			} else {
				f->guids.erase(it++);
				f->guids.erase(it++);
			}
		}

		SetModified();
	}
}

void CPPageExternalFilters::OnResetTypes()
{
	if (FilterOverride* f = GetCurFilter()) {
		if (f->type == FilterOverride::REGISTERED) {
			CFGFilterRegistry fgf(f->dispname);
			if (!fgf.GetName().IsEmpty()) {
				f->guids = fgf.GetTypes();
				f->backup = fgf.GetTypes();
			} else {
				f->guids = f->backup;
			}
		} else {
			CFilterMapper2 fm2(false);
			fm2.Register(f->path);
			for (const auto& filter : fm2.m_filters) {
				if (filter->clsid == f->clsid) {
					f->backup = filter->backup;
					break;
				}
			}
			f->guids = f->backup;
		}

		m_pLastSelFilter = nullptr;
		OnLbnSelchangeList1();

		SetModified();
	}
}

void CPPageExternalFilters::OnLbnSelchangeList1()
{
	if (FilterOverride* f = GetCurFilter()) {
		if (m_pLastSelFilter == f) {
			return;
		}
		m_pLastSelFilter = f;

		m_iLoadType = f->iLoadType;
		UpdateData(FALSE);
		m_dwMerit = f->dwMerit;

		HTREEITEM dummy_item = m_tree.InsertItem(L"", 0,0, nullptr, TVI_FIRST);
		if (dummy_item)
			for (HTREEITEM item = m_tree.GetNextVisibleItem(dummy_item); item; item = m_tree.GetNextVisibleItem(dummy_item)) {
				m_tree.DeleteItem(item);
			}

		CMapStringToPtr map;

		auto it = f->guids.begin();
		while (it != f->guids.end() && std::next(it) != f->guids.end()) {
			GUID* tmp = &(*it);
			CString major = GetMediaTypeName(*it++);
			CString sub = GetMediaTypeName(*it++);

			HTREEITEM node = nullptr;

			void* val = nullptr;
			if (map.Lookup(major, val)) {
				node = (HTREEITEM)val;
			} else {
				map[major] = node = m_tree.InsertItem(major);
			}
			m_tree.SetItemData(node, NULL);

			node = m_tree.InsertItem(sub, node);
			m_tree.SetItemData(node, (DWORD_PTR)tmp);
		}

		m_tree.DeleteItem(dummy_item);

		for (HTREEITEM item = m_tree.GetFirstVisibleItem(); item; item = m_tree.GetNextVisibleItem(item)) {
			m_tree.Expand(item, TVE_EXPAND);
		}

		m_tree.EnsureVisible(m_tree.GetRootItem());
	} else {
		m_pLastSelFilter = nullptr;

		m_iLoadType = FilterOverride::PREFERRED;
		UpdateData(FALSE);
		m_dwMerit = 0;

		m_tree.DeleteAllItems();
	}
}

void CPPageExternalFilters::OnCheckChangeList1()
{
	SetModified();
}

void CPPageExternalFilters::OnBnClickedRadio()
{
	UpdateData();

	FilterOverride* f = GetCurFilter();
	if (f && f->iLoadType != m_iLoadType) {
		f->iLoadType = m_iLoadType;

		SetModified();
	}
}

void CPPageExternalFilters::OnEnChangeEdit1()
{
	UpdateData();
	if (FilterOverride* f = GetCurFilter()) {
		DWORD dw;
		if (m_dwMerit.GetDWORD(dw) && f->dwMerit != dw) {
			f->dwMerit = dw;

			SetModified();
		}
	}
}

void CPPageExternalFilters::OnNMDblclkTree2(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	if (FilterOverride* f = GetCurFilter()) {
		HTREEITEM node = m_tree.GetSelectedItem();
		if (!node) {
			return;
		}

		auto it = FindInListByPointer(f->guids, (GUID*)m_tree.GetItemData(node));
		if (it == f->guids.end()) {
			ASSERT(0);
			return;
		}

		it++;
		if (it == f->guids.end()) {
			return;
		}

		CSelectMediaType dlg(s_SubTypes, *it, this);
		if (dlg.DoModal() == IDOK) {
			*it = dlg.m_guid;
			m_tree.SetItemText(node, GetMediaTypeName(dlg.m_guid));

			SetModified();
		}
	}
}

void CPPageExternalFilters::OnTVNKeyDownTree2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

	if (pTVKeyDown->wVKey == VK_DELETE) {
		OnDeleteType();
	}

	*pResult = 0;
}

void CPPageExternalFilters::OnDropFiles(HDROP hDropInfo)
{
	SetActiveWindow();

	UINT nFiles = ::DragQueryFileW(hDropInfo, (UINT)-1, nullptr, 0);
	for (UINT iFile = 0; iFile < nFiles; iFile++) {
		WCHAR szFileName[MAX_PATH];
		::DragQueryFileW(hDropInfo, iFile, szFileName, MAX_PATH);

		CFilterMapper2 fm2(false);
		fm2.Register(szFileName);

		while (!fm2.m_filters.empty()) {
			FilterOverride* f = fm2.m_filters.front();
			fm2.m_filters.pop_front();

			if (f) {
				CAutoPtr<FilterOverride> p(f);
				int i = m_filters.AddString(f->name);
				m_filters.SetItemDataPtr(i, m_pFilters.AddTail(p));
				m_filters.SetCheck(i, 1);

				if (fm2.m_filters.empty()) {
					m_filters.SetCurSel(i);
					OnLbnSelchangeList1();
				}

				SetModified();
			}
		}
	}
	::DragFinish(hDropInfo);
}
