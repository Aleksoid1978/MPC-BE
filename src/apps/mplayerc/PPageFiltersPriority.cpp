/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2022 see Authors.txt
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
#include "FGFilter.h"
#include <moreuuids.h>
#include "DSUtil/std_helper.h"
#include "PPageFiltersPriority.h"

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
	DDX_Control(pDX, IDC_COMBO2, m_UDP);
	DDX_Control(pDX, IDC_COMBO3, m_AVI);
	DDX_Control(pDX, IDC_COMBO4, m_MKV);
	DDX_Control(pDX, IDC_COMBO5, m_MPEGTS);
	DDX_Control(pDX, IDC_COMBO6, m_MPEG);
	DDX_Control(pDX, IDC_COMBO7, m_MP4);
	DDX_Control(pDX, IDC_COMBO8, m_FLV);
	DDX_Control(pDX, IDC_COMBO9, m_WMV);
}

BEGIN_MESSAGE_MAP(CPPageFiltersPriority, CPPageBase)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CPPageFiltersPriority message handlers

BOOL CPPageFiltersPriority::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	Init();

	CreateToolTip();

	return TRUE;
}

BOOL CPPageFiltersPriority::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.FiltersPriority.SetDefault();
	if (CLSID* pclsid = (CLSID*)m_HTTP.GetItemDataPtr(m_HTTP.GetCurSel())) {
		s.FiltersPriority.values[L"http"] = *pclsid;
	}
	if (CLSID* pclsid = (CLSID*)m_UDP.GetItemDataPtr(m_UDP.GetCurSel())) {
		s.FiltersPriority.values[L"udp"] = *pclsid;
	}

	if (CLSID* pclsid = (CLSID*)m_AVI.GetItemDataPtr(m_AVI.GetCurSel())) {
		s.FiltersPriority.values[L"avi"] = *pclsid;
	}
	if (CLSID* pclsid = (CLSID*)m_MKV.GetItemDataPtr(m_MKV.GetCurSel())) {
		s.FiltersPriority.values[L"mkv"] = *pclsid;
	}
	if (CLSID* pclsid = (CLSID*)m_MPEGTS.GetItemDataPtr(m_MPEGTS.GetCurSel())) {
		s.FiltersPriority.values[L"mpegts"] = *pclsid;
	}
	if (CLSID* pclsid = (CLSID*)m_MPEG.GetItemDataPtr(m_MPEG.GetCurSel())) {
		s.FiltersPriority.values[L"mpeg"] = *pclsid;
	}
	if (CLSID* pclsid = (CLSID*)m_MP4.GetItemDataPtr(m_MP4.GetCurSel())) {
		s.FiltersPriority.values[L"mp4"] = *pclsid;
	}
	if (CLSID* pclsid = (CLSID*)m_FLV.GetItemDataPtr(m_FLV.GetCurSel())) {
		s.FiltersPriority.values[L"flv"] = *pclsid;
	}
	if (CLSID* pclsid = (CLSID*)m_WMV.GetItemDataPtr(m_WMV.GetCurSel())) {
		s.FiltersPriority.values[L"wmv"] = *pclsid;
	}

	return __super::OnApply();
}

void CPPageFiltersPriority::Init()
{
	CAppSettings& s = AfxGetAppSettings();

	m_HTTP.ResetContent();
	m_HTTP.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_HTTP.SetCurSel(0);

	m_UDP.ResetContent();
	m_UDP.AddString(ResStr(IDS_AG_DEFAULT_L));
	m_UDP.SetCurSel(0);

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

	m_ExtSrcFilters.clear();

	for (const auto& f : s.m_ExternalFilters) {
		if (f->iLoadType == FilterOverride::BLOCK) {
			continue;
		}

		CString name;
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

			if (!::PathFileExistsW(MakeFullPath(f->path))) {
				continue;
			}
		}

		if (name.IsEmpty()) {
			continue;
		}

		std::list<CLSID> CLSID_List;

		const bool bIsSource = f->guids.empty();
		bool bIsSplitter     = false;
		if (!bIsSource) {
			auto it = f->guids.cbegin();
			while (it != f->guids.cend() && std::next(it) != f->guids.cend()) {
				CLSID major = *it++;
				CLSID sub   = *it++;

				if (major == MEDIATYPE_Stream) {
					bIsSplitter = true;
					CLSID_List.push_front(sub);
				}
			}
		} else {
			CLSID_List.push_front(MEDIASUBTYPE_NULL);
		}

		if (!bIsSource && !bIsSplitter) {
			continue;
		}

		m_ExtSrcFilters.emplace_back(f->clsid);
		CLSID& clsid = m_ExtSrcFilters.back();

		if (bIsSource) {
			int i = m_HTTP.AddString(name);
			m_HTTP.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"http"] == clsid) {
				m_HTTP.SetCurSel(i);
			}

			i = m_UDP.AddString(name);
			m_UDP.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"udp"] == clsid) {
				m_UDP.SetCurSel(i);
			}
		}

		if (Contains(CLSID_List, MEDIASUBTYPE_Avi) || Contains(CLSID_List, MEDIASUBTYPE_NULL)) {
			int i = m_AVI.AddString(name);
			m_AVI.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"avi"] == clsid) {
				m_AVI.SetCurSel(i);
			}
		}
		if (Contains(CLSID_List, MEDIASUBTYPE_Matroska) || Contains(CLSID_List, MEDIASUBTYPE_NULL)) {
			int i = m_MKV.AddString(name);
			m_MKV.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"mkv"] == clsid) {
				m_MKV.SetCurSel(i);
			}
		}

		if (Contains(CLSID_List, MEDIASUBTYPE_MPEG1System)
			|| Contains(CLSID_List, MEDIASUBTYPE_MPEG2_PROGRAM)
			|| Contains(CLSID_List, MEDIASUBTYPE_MPEG2_TRANSPORT)
			|| Contains(CLSID_List, MEDIASUBTYPE_MPEG2_PVA)
			|| Contains(CLSID_List, MEDIASUBTYPE_NULL)) {
			int i = m_MPEGTS.AddString(name);
			m_MPEGTS.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"mpegts"] == clsid) {
				m_MPEGTS.SetCurSel(i);
			}

			i = m_MPEG.AddString(name);
			m_MPEG.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"mpeg"] == clsid) {
				m_MPEG.SetCurSel(i);
			}
		}

		if (Contains(CLSID_List, MEDIASUBTYPE_MP4) || Contains(CLSID_List, MEDIASUBTYPE_NULL)) {
			int i = m_MP4.AddString(name);
			m_MP4.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"mp4"] == clsid) {
				m_MP4.SetCurSel(i);
			}
		}

		if (Contains(CLSID_List, MEDIASUBTYPE_FLV) || Contains(CLSID_List, MEDIASUBTYPE_NULL)) {
			int i = m_FLV.AddString(name);
			m_FLV.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"flv"] == clsid) {
				m_FLV.SetCurSel(i);
			}
		}

		if (Contains(CLSID_List, MEDIASUBTYPE_Asf) || Contains(CLSID_List, MEDIASUBTYPE_NULL)) {
			int i = m_WMV.AddString(name);
			m_WMV.SetItemDataPtr(i, &clsid);
			if (s.FiltersPriority.values[L"wmv"] == clsid) {
				m_WMV.SetCurSel(i);
			}
		}

		CLSID_List.clear();
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
