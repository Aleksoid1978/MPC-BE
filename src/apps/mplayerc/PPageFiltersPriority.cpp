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
#include "PPageFiltersPriority.h"
#include "FGFilter.h"
#include "moreuuids.h"

// CPPageFiltersPriority dialog

IMPLEMENT_DYNAMIC(CPPageFiltersPriority, CPPageBase)
CPPageFiltersPriority::CPPageFiltersPriority()
	: CPPageBase(CPPageFiltersPriority::IDD, CPPageFiltersPriority::IDD)
{
}

CPPageFiltersPriority::~CPPageFiltersPriority()
{
}

void CPPageFiltersPriority::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_HTTP);
	DDX_Control(pDX, IDC_COMBO2, m_AVI);
	DDX_Control(pDX, IDC_COMBO3, m_MKV);
	DDX_Control(pDX, IDC_COMBO4, m_MPEGTS);
	DDX_Control(pDX, IDC_COMBO5, m_MPEG);
	DDX_Control(pDX, IDC_COMBO6, m_MP4);
	DDX_Control(pDX, IDC_COMBO7, m_FLV);
	DDX_Control(pDX, IDC_COMBO8, m_WMV);
}

BEGIN_MESSAGE_MAP(CPPageFiltersPriority, CPPageBase)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CPPageFiltersPriority message handlers

BOOL CPPageFiltersPriority::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);
	GetDlgItem(IDC_STATIC1)->EnableWindow(FALSE);
	m_HTTP.EnableWindow(FALSE);

	Init();

	CreateToolTip();

	return TRUE;
}

BOOL CPPageFiltersPriority::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.FiltersPrioritySettings.SetDefault();
	if (POSITION pos = (POSITION)m_AVI.GetItemDataPtr(m_AVI.GetCurSel())) {
		s.FiltersPrioritySettings.values[_T("avi")] = m_pFilters.GetAt(pos)->clsid;
	}
	if (POSITION pos = (POSITION)m_MKV.GetItemDataPtr(m_MKV.GetCurSel())) {
		s.FiltersPrioritySettings.values[_T("mkv")] = m_pFilters.GetAt(pos)->clsid;
	}
	if (POSITION pos = (POSITION)m_MPEGTS.GetItemDataPtr(m_MPEGTS.GetCurSel())) {
		s.FiltersPrioritySettings.values[_T("mpegts")] = m_pFilters.GetAt(pos)->clsid;
	}
	if (POSITION pos = (POSITION)m_MPEG.GetItemDataPtr(m_MPEG.GetCurSel())) {
		s.FiltersPrioritySettings.values[_T("mpeg")] = m_pFilters.GetAt(pos)->clsid;
	}
	if (POSITION pos = (POSITION)m_MP4.GetItemDataPtr(m_MP4.GetCurSel())) {
		s.FiltersPrioritySettings.values[_T("mp4")] = m_pFilters.GetAt(pos)->clsid;
	}
	if (POSITION pos = (POSITION)m_FLV.GetItemDataPtr(m_FLV.GetCurSel())) {
		s.FiltersPrioritySettings.values[_T("flv")] = m_pFilters.GetAt(pos)->clsid;
	}
	if (POSITION pos = (POSITION)m_WMV.GetItemDataPtr(m_WMV.GetCurSel())) {
		s.FiltersPrioritySettings.values[_T("wmv")] = m_pFilters.GetAt(pos)->clsid;
	}

	return __super::OnApply();
}

void CPPageFiltersPriority::Init()
{
	CAppSettings& s = AfxGetAppSettings();
	
	m_HTTP.ResetContent();
	m_HTTP.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_HTTP.SetCurSel(0);

	m_AVI.ResetContent();
	m_AVI.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_AVI.SetCurSel(0);

	m_MKV.ResetContent();
	m_MKV.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_MKV.SetCurSel(0);

	m_MPEGTS.ResetContent();
	m_MPEGTS.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_MPEGTS.SetCurSel(0);

	m_MPEG.ResetContent();
	m_MPEG.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_MPEG.SetCurSel(0);

	m_MP4.ResetContent();
	m_MP4.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_MP4.SetCurSel(0);

	m_FLV.ResetContent();
	m_FLV.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_FLV.SetCurSel(0);

	m_WMV.ResetContent();
	m_WMV.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_WMV.SetCurSel(0);

	m_pFilters.RemoveAll();

	POSITION pos = s.m_filters.GetHeadPosition();
	while (pos) {
		CAutoPtr<FilterOverride> f(DNew FilterOverride(s.m_filters.GetNext(pos)));

		CString name;

		if (f->iLoadType == FilterOverride::BLOCK) {
			continue;
		}

		if (f->type == FilterOverride::REGISTERED) {
			name = CFGFilterRegistry(f->dispname).GetName();
			if (name.IsEmpty()) {
				continue;
			}
		} else if (f->type == FilterOverride::EXTERNAL) {
			name = f->name;
			if (f->fTemporary) {
				continue;
			}

			if (!::PathFileExists(MakeFullPath(f->path))) {
				continue;
			}
		}

		if (name.IsEmpty()) {
			continue;
		}

		CAtlList<CLSID> CLSID_List;

		bool bIsSource		= (f->guids.GetCount() == 0);
		bool bIsSplitter	= false;
		if (!bIsSource) {
			POSITION pos = f->guids.GetHeadPosition();
			while (pos) {
				CLSID major	= f->guids.GetNext(pos);
				CLSID sub	= f->guids.GetNext(pos);

				if (major == MEDIATYPE_Stream) {
					bIsSplitter = true;
					CLSID_List.AddHead(sub);
				}
			}
		} else {
			CLSID_List.AddHead(MEDIASUBTYPE_NULL);
		}

		if (!bIsSource && !bIsSplitter) {
			continue;
		}

		CLSID clsid = f->clsid;
		POSITION pos2 = m_pFilters.AddTail(f);

		if (CLSID_List.Find(MEDIASUBTYPE_Avi) || CLSID_List.Find(MEDIASUBTYPE_NULL)) {
			int i = m_AVI.AddString(name);
			m_AVI.SetItemDataPtr(i, pos2);
			if (s.FiltersPrioritySettings.values[_T("avi")] == clsid) {
				m_AVI.SetCurSel(i);
			}
		}
		if (CLSID_List.Find(MEDIASUBTYPE_Matroska) || CLSID_List.Find(MEDIASUBTYPE_NULL)) {
			int i = m_MKV.AddString(name);
			m_MKV.SetItemDataPtr(i, pos2);
			if (s.FiltersPrioritySettings.values[_T("mkv")] == clsid) {
				m_MKV.SetCurSel(i);
			}
		}

		if (CLSID_List.Find(MEDIASUBTYPE_MPEG1System)
			|| CLSID_List.Find(MEDIASUBTYPE_MPEG2_PROGRAM)
			|| CLSID_List.Find(MEDIASUBTYPE_MPEG2_TRANSPORT)
			|| CLSID_List.Find(MEDIASUBTYPE_MPEG2_PVA)
			|| CLSID_List.Find(MEDIASUBTYPE_NULL)) {
			int i = m_MPEGTS.AddString(name);
			m_MPEGTS.SetItemDataPtr(i, pos2);
			if (s.FiltersPrioritySettings.values[_T("mpegts")] == clsid) {
				m_MPEGTS.SetCurSel(i);
			}

			i = m_MPEG.AddString(name);
			m_MPEG.SetItemDataPtr(i, pos2);
			if (s.FiltersPrioritySettings.values[_T("mpeg")] == clsid) {
				m_MPEG.SetCurSel(i);
			}
		}

		if (CLSID_List.Find(MEDIASUBTYPE_MP4) || CLSID_List.Find(MEDIASUBTYPE_NULL)) {
			int i = m_MP4.AddString(name);
			m_MP4.SetItemDataPtr(i, pos2);
			if (s.FiltersPrioritySettings.values[_T("mp4")] == clsid) {
				m_MP4.SetCurSel(i);
			}
		}

		if (CLSID_List.Find(MEDIASUBTYPE_FLV) || CLSID_List.Find(MEDIASUBTYPE_NULL)) {
			int i = m_FLV.AddString(name);
			m_FLV.SetItemDataPtr(i, pos2);
			if (s.FiltersPrioritySettings.values[_T("flv")] == clsid) {
				m_FLV.SetCurSel(i);
			}
		}

		if (CLSID_List.Find(MEDIASUBTYPE_Asf) || CLSID_List.Find(MEDIASUBTYPE_NULL)) {
			int i = m_WMV.AddString(name);
			m_WMV.SetItemDataPtr(i, pos2);
			if (s.FiltersPrioritySettings.values[_T("wmv")] == clsid) {
				m_WMV.SetCurSel(i);
			}
		}

		CLSID_List.RemoveAll();
	}

	UpdateData(FALSE);
}

void CPPageFiltersPriority::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);

	if (bShow) {
		Init();
	}
}
