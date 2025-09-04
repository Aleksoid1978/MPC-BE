/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
#include "PlayerSubresyncBar.h"


// CPlayerSubresyncBar

IMPLEMENT_DYNAMIC(CPlayerSubresyncBar, CPlayerBar)

CPlayerSubresyncBar::CPlayerSubresyncBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
{
}

CPlayerSubresyncBar::~CPlayerSubresyncBar()
{
}

BOOL CPlayerSubresyncBar::Create(CWnd* pParentWnd, UINT defDockBarID, CCritSec* pSubLock)
{
	if (!__super::Create(ResStr(IDS_SUBRESYNC_CAPTION), pParentWnd, ID_VIEW_SUBRESYNC, defDockBarID, L"Subresync")) {
		return FALSE;
	}

	m_pSubLock = pSubLock;

	m_list.CreateEx(
		WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT/* |LVS_SHOWSELALWAYS*/ | LVS_AUTOARRANGE | LVS_NOSORTHEADER,
		CRect(0, 0, 100, 100), this, IDC_SUBRESYNCLIST);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	return TRUE;
}

void CPlayerSubresyncBar::ReloadTranslatableResources()
{
	SetWindowText(ResStr(IDS_SUBRESYNC_CAPTION));

	CHeaderCtrl* pHeaderCtrl = m_list.GetHeaderCtrl();
	if (pHeaderCtrl && pHeaderCtrl->GetItemCount() > 4) {
		auto setColumnHeaderText = [pHeaderCtrl](int nPos, CString str) {
			HDITEMW item;
			item.mask = HDI_TEXT;
			item.pszText = (LPWSTR)(LPCWSTR)str;
			item.cchTextMax = str.GetLength() + 1;
			VERIFY(pHeaderCtrl->SetItem(nPos, &item));
		};

		setColumnHeaderText(COL_START, ResStr(IDS_SUBRESYNC_CLN_TIME));
		setColumnHeaderText(COL_END, ResStr(IDS_SUBRESYNC_CLN_END));
		setColumnHeaderText(COL_PREVSTART, ResStr(IDS_SUBRESYNC_CLN_PREVIEW));
		setColumnHeaderText(COL_PREVEND, ResStr(IDS_SUBRESYNC_CLN_END));

		if (m_mode == VOBSUB) {
			setColumnHeaderText(COL_VOBID, ResStr(IDS_SUBRESYNC_CLN_VOB_ID));
			setColumnHeaderText(COL_CELLID, ResStr(IDS_SUBRESYNC_CLN_CELL_ID));
			setColumnHeaderText(COL_FORCED, ResStr(IDS_SUBRESYNC_CLN_FORCED));
		}
		else if (m_mode == TEXTSUB) {
			setColumnHeaderText(COL_TEXT, ResStr(IDS_SUBRESYNC_CLN_TEXT));
			setColumnHeaderText(COL_STYLE, ResStr(IDS_SUBRESYNC_CLN_STYLE));
			setColumnHeaderText(COL_FONT, ResStr(IDS_SUBRESYNC_CLN_FONT));
			setColumnHeaderText(COL_CHARSET, ResStr(IDS_SUBRESYNC_CLN_CHARSET));
			setColumnHeaderText(COL_LAYER, ResStr(IDS_SUBRESYNC_CLN_LAYER));
			setColumnHeaderText(COL_ACTOR, ResStr(IDS_SUBRESYNC_CLN_ACTOR));
			setColumnHeaderText(COL_EFFECT, ResStr(IDS_SUBRESYNC_CLN_EFFECT));
		}
	}
}

BOOL CPlayerSubresyncBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!__super::PreCreateWindow(cs)) {
		return FALSE;
	}

	return TRUE;
}

BOOL CPlayerSubresyncBar::PreTranslateMessage(MSG* pMsg)
{
	if (IsWindow(pMsg->hwnd) && IsVisible() && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
			m_pMainFrame->ShowControlBarInternal(this, FALSE);
			return TRUE;
		}

		if (IsShortCut(pMsg) || IsDialogMessageW(pMsg)) {
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CPlayerSubresyncBar::SetTime(REFERENCE_TIME rt)
{
	m_rt = rt;
	int curSegment;

	if (!m_sts.SearchSubs((int)(rt / 10000), 25, &curSegment)) {
		curSegment = -1;
	}

	if (m_lastSegment != curSegment) {
		m_list.Invalidate();
	}
	m_lastSegment = curSegment;
}

void CPlayerSubresyncBar::SetFPS(double fps)
{
	if (m_fps != fps) {
		m_fps = fps;

		ReloadSubtitle();
	}
}

void CPlayerSubresyncBar::SetSubtitle(ISubStream* pSubStream, double fps)
{
	// Avoid reloading the same subtitles again
	if (m_pSubStream != pSubStream || m_fps != fps) {
		m_pSubStream = pSubStream;
		m_fps = fps;

		ReloadSubtitle();
	}
}

void CPlayerSubresyncBar::ReloadSubtitle()
{
	m_mode = NONE;
	m_lastSegment = -1;
	m_sts.Empty();

	ResetSubtitle();

	if (!m_pSubStream) {
		return;
	}

	CLSID clsid;
	m_pSubStream->GetClassID(&clsid);

	if (clsid == __uuidof(CVobSubFile)) {
		CVobSubFile* pVSF = (CVobSubFile*)m_pSubStream.p;

		m_mode = VOBSUB;

		ASSERT(pVSF->m_nLang >= 0);
		std::vector<CVobSubFile::SubPos>& sp = pVSF->m_langs[pVSF->m_nLang].subpos;

		for (size_t i = 0, j = sp.size(); i < j; i++) {
			CString str;
			str.Format(L"%d,%d,%d,%Iu", sp[i].vobid, sp[i].cellid, sp[i].bForced, i);
			m_sts.Add(str, false, (int)sp[i].start, (int)sp[i].stop);
		}

		m_sts.CreateDefaultStyle(DEFAULT_CHARSET);

		pVSF->m_bOnlyShowForcedSubs = false;

		for (int i = 0, j = m_list.GetHeaderCtrl()->GetItemCount(); i < j; i++) {
			m_list.DeleteColumn(0);
		}
		m_list.InsertColumn(COL_START, ResStr(IDS_SUBRESYNC_CLN_TIME), LVCFMT_LEFT, 80);
		m_list.InsertColumn(COL_END, ResStr(IDS_SUBRESYNC_CLN_END), LVCFMT_LEFT, 80);
		m_list.InsertColumn(COL_PREVSTART, ResStr(IDS_SUBRESYNC_CLN_PREVIEW), LVCFMT_LEFT, 80);
		m_list.InsertColumn(COL_PREVEND, ResStr(IDS_SUBRESYNC_CLN_END), LVCFMT_LEFT, 80);
		m_list.InsertColumn(COL_VOBID, ResStr(IDS_SUBRESYNC_CLN_VOB_ID), LVCFMT_CENTER, 60);
		m_list.InsertColumn(COL_CELLID, ResStr(IDS_SUBRESYNC_CLN_CELL_ID), LVCFMT_CENTER, 60);
		m_list.InsertColumn(COL_FORCED, ResStr(IDS_SUBRESYNC_CLN_FORCED), LVCFMT_CENTER, 60);
	} else if (clsid == __uuidof(CRenderedTextSubtitle)) {
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;

		m_mode = TEXTSUB;

		m_sts.Copy(*pRTS);
		m_sts.ConvertToTimeBased(m_fps);
		m_sts.Sort(true); /*!!m_fUnlink*/

		for (int i = 0, j = m_list.GetHeaderCtrl()->GetItemCount(); i < j; i++) {
			m_list.DeleteColumn(0);
		}
		m_list.InsertColumn(COL_START, ResStr(IDS_SUBRESYNC_CLN_TIME), LVCFMT_LEFT, 90);
		m_list.InsertColumn(COL_END, ResStr(IDS_SUBRESYNC_CLN_END), LVCFMT_LEFT, 4);
		m_list.InsertColumn(COL_PREVSTART, ResStr(IDS_SUBRESYNC_CLN_PREVIEW), LVCFMT_LEFT, 80);
		m_list.InsertColumn(COL_PREVEND, ResStr(IDS_SUBRESYNC_CLN_END), LVCFMT_LEFT, 4);
		m_list.InsertColumn(COL_TEXT, ResStr(IDS_SUBRESYNC_CLN_TEXT), LVCFMT_LEFT, 275);
		m_list.InsertColumn(COL_STYLE, ResStr(IDS_SUBRESYNC_CLN_STYLE), LVCFMT_LEFT, 80);
		m_list.InsertColumn(COL_FONT, ResStr(IDS_SUBRESYNC_CLN_FONT), LVCFMT_LEFT, 60);
		m_list.InsertColumn(COL_CHARSET, ResStr(IDS_SUBRESYNC_CLN_CHARSET), LVCFMT_CENTER, 20);
		m_list.InsertColumn(COL_LAYER, ResStr(IDS_SUBRESYNC_CLN_LAYER), LVCFMT_CENTER, 50);
		m_list.InsertColumn(COL_ACTOR, ResStr(IDS_SUBRESYNC_CLN_ACTOR), LVCFMT_LEFT, 80);
		m_list.InsertColumn(COL_EFFECT, ResStr(IDS_SUBRESYNC_CLN_EFFECT), LVCFMT_LEFT, 80);
	}

	m_subtimes.resize(m_sts.GetCount());

	for (size_t i = 0, j = m_sts.GetCount(); i < j; i++) {
		m_subtimes[i].orgstart = m_sts[i].start;
		m_subtimes[i].orgend   = m_sts[i].end;
	}

	ResetSubtitle();
}

void CPlayerSubresyncBar::ResetSubtitle()
{
	m_list.SetRedraw(FALSE);
	m_list.DeleteAllItems();

	if (m_mode == VOBSUB || m_mode == TEXTSUB) {
		WCHAR buff[32] = { 0 };

		int prevstart = INT_MIN;

		int nCount = (int)m_sts.GetCount();
		m_list.SetItemCount(nCount);

		for (int i = 0; i < nCount; i++) {
			m_subtimes[i].newstart = m_subtimes[i].orgstart;
			m_subtimes[i].newend = m_subtimes[i].orgend;

			FormatTime(i, buff, std::size(buff), 0, false);
			m_list.InsertItem(i, buff);
			m_list.SetItemText(i, COL_PREVSTART, buff);
			FormatTime(i, buff, std::size(buff), 0, true);
			m_list.SetItemText(i, COL_END, buff);
			m_list.SetItemText(i, COL_PREVEND, buff);

			if (prevstart > m_subtimes[i].orgstart) {
				m_list.SetItemData(i, (DWORD_PTR)TSEP);
			}
			prevstart = m_subtimes[i].orgstart;

			// Since all items in COL_START and COL_PREVSTART have the same text size,
			// we can compute it for the first element only so that it's faster.
			if (i == 0) {
				m_list.SetColumnWidth(COL_START, LVSCW_AUTOSIZE);
				m_list.SetColumnWidth(COL_PREVSTART, LVSCW_AUTOSIZE);
			}
		}

		UpdateStrings();
	}

	m_list.SetRedraw(TRUE);
	m_list.Invalidate();
	m_list.UpdateWindow();
}

void CPlayerSubresyncBar::SaveSubtitle()
{
	CLSID clsid;
	m_pSubStream->GetClassID(&clsid);

	if (clsid == __uuidof(CVobSubFile) && m_mode == VOBSUB) {
		CVobSubFile* pVSF = (CVobSubFile*)m_pSubStream.p;

		CAutoLock cAutoLock(m_pSubLock);

		ASSERT(pVSF->m_nLang >= 0);
		std::vector<CVobSubFile::SubPos>& sp = pVSF->m_langs[pVSF->m_nLang].subpos;

		for (size_t i = 0, j = sp.size(); i < j; i++) {
			sp[i].bValid = false;
		}

		for (int i = 0, j = (int)m_sts.GetCount(); i < j; i++) {
			int vobid, cellid, forced, spnum;
			WCHAR c;
			if (swscanf_s(m_sts.GetStrW(i), L"%d%c%d%c%d%c%d", &vobid,
						   &c, 1, &cellid, &c, 1, &forced, &c, 1, &spnum) != 7) {
				continue;
			}
			sp[spnum].start = m_sts[i].start;
			sp[spnum].stop = m_sts[i].end;
			sp[spnum].bValid = true;
		}
	} else if (clsid == __uuidof(CRenderedTextSubtitle) && m_mode == TEXTSUB) {
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;

		CAutoLock cAutoLock(m_pSubLock);

		pRTS->Copy(m_sts);
	} else {
		return;
	}

	m_pMainFrame->InvalidateSubtitle();
}

void CPlayerSubresyncBar::UpdatePreview()
{
	if (m_mode == VOBSUB || m_mode == TEXTSUB) {
		if (0/*m_fUnlink*/) {
			for (int i = 0, j = (int)m_sts.GetCount(); i < j; i++) {
				bool fStartMod, fEndMod, fStartAdj, fEndAdj;
				GetCheck(i, fStartMod, fEndMod, fStartAdj, fEndAdj);
				m_sts[i].start = (fStartMod || fStartAdj) ? m_subtimes[i].newstart : m_subtimes[i].orgstart;
				m_sts[i].end = (fEndMod || fEndAdj) ? m_subtimes[i].newend : m_subtimes[i].orgend;
			}
		} else {
			for (int i = 0, j = (int)m_sts.GetCount(); i < j;) {
				std::vector<int> schk;

				int start = i, end;

				for (end = i; end < j; end++) {
					int data = (int)m_list.GetItemData(end);
					if ((data & TSEP) && end > i) {
						break;
					}
					if (data & (TSMOD | TSADJ)) {
						schk.emplace_back(end);
					}
				}

				if (schk.empty()) {
					for (; start < end; start++) {
						m_sts[start].start = m_subtimes[start].orgstart;
						m_sts[start].end = m_subtimes[start].orgend;
					}
				} else if (schk.size() == 1) {
					int k = schk[0];
					int dt = m_subtimes[k].newstart - m_subtimes[k].orgstart;
					for (; start < end; start++) {
						m_sts[start].start = m_subtimes[start].orgstart + dt;
						m_sts[start].end = (m_list.GetItemData(start)&TEMOD)
										   ? m_subtimes[start].newend
										   : (m_subtimes[start].orgend + dt);
					}
				} else if (schk.size() >= 2) {
					int i0 = 0;
					int i1 = 0;
					int ti0 = 0;
					int ds = 0;
					double m = 0;

					int k, l;
					for (k = 0, l = (int)schk.size() - 1; k < l; k++) {
						i0 = schk[k];
						i1 = schk[k + 1];

						ti0 = m_subtimes[i0].orgstart;
						ds = m_subtimes[i1].orgstart - ti0;

						if (ds == 0) {
							SetSTS0(start, i1, ti0);
						} else {
							m = double(m_subtimes[i1].newstart - m_subtimes[i0].newstart) / ds;
							SetSTS1(start, i1, ti0, m, i0);
						}

					}

					ASSERT(k > 0);
					if (ds == 0) {
						SetSTS0(start, end, ti0);
					} else {
						SetSTS1(start, end, ti0, m, i0);
					}
				}

				i = end;
			}
		}

		m_sts.CreateSegments();

		for (int i = 0, j = (int)m_sts.GetCount(); i < j; i++) {
			WCHAR buff[32];
			FormatTime(i, buff, std::size(buff), 2, false);
			m_list.SetItemText(i, COL_PREVSTART, buff);
			FormatTime(i, buff, std::size(buff), 2, true);
			m_list.SetItemText(i, COL_PREVEND, buff);
		}

		if (IsWindowVisible()) {
			SaveSubtitle();
		}
	}
}

void CPlayerSubresyncBar::SetSTS0(int& start, int end, int ti0)
{
	for (; start < end; start++) {
		m_sts[start].start = ti0;
		int endpos;
		if (m_list.GetItemData(start)&TEMOD) {
			endpos = m_subtimes[start].newend;
		} else {
			endpos = ti0 + m_subtimes[start].orgend - m_subtimes[start].orgstart;
		}
		m_sts[start].end = endpos;
	}
}

void CPlayerSubresyncBar::SetSTS1(int& start, int end, int ti0, double m, int i0)
{
	for (; start < end; start++) {
		m_sts[start].start = int((m_subtimes[start].orgstart - ti0) * m + m_subtimes[i0].newstart);
		int endpos;
		if (m_list.GetItemData(start)&TEMOD) {
			endpos = m_subtimes[start].newend;
		} else {
			int diff = m_subtimes[start].orgend - m_subtimes[start].orgstart;
			if (m_mode == VOBSUB) {
				endpos = m_sts[start].start + diff;
			} else {
				endpos = m_sts[start].start + int(diff * m);
			}
		}
		m_sts[start].end = endpos;
	}
}

void CPlayerSubresyncBar::UpdateStrings()
{
	CString str;

	if (m_mode == TEXTSUB) {
		for (int i = 0, j = (int)m_sts.GetCount(); i < j; i++) {
			STSStyle stss;
			m_sts.GetStyle(i, stss);

			m_list.SetItemText(i, COL_TEXT, m_sts.GetStrW(i, true));
			m_list.SetItemText(i, COL_STYLE, m_sts[i].style);
			m_list.SetItemText(i, COL_FONT, stss.fontName);
			str.Format(L"%d", stss.charSet);
			m_list.SetItemText(i, COL_CHARSET, str);
			str.Format(L"%d", m_sts[i].layer);
			m_list.SetItemText(i, COL_LAYER, str);
			m_list.SetItemText(i, COL_ACTOR, m_sts[i].actor);
			m_list.SetItemText(i, COL_EFFECT, m_sts[i].effect);
		}
	} else if (m_mode == VOBSUB) {
		for (int i = 0, j = (int)m_sts.GetCount(); i < j; i++) {
			int vobid, cellid, forced;
			WCHAR c;
			if (swscanf_s(m_sts.GetStrW(i), L"%d%c%d%c%d", &vobid,
						   &c, 1, &cellid, &c, 1, &forced) != 5) {
				continue;
			}
			if (vobid < 0) {
				str = L"-";
			} else {
				str.Format(L"%d", vobid);
			}
			m_list.SetItemText(i, COL_VOBID, str);
			if (cellid < 0) {
				str = L"-";
			} else {
				str.Format(L"%d", cellid);
			}
			m_list.SetItemText(i, COL_CELLID, str);
			str = forced ? L"Yes" : L"";
			m_list.SetItemText(i, COL_FORCED, str);
		}
	}
}

void CPlayerSubresyncBar::GetCheck(int iItem, bool& fStartMod, bool& fEndMod, bool& fStartAdj, bool& fEndAdj)
{
	if (0 <= iItem && (size_t)iItem < m_sts.GetCount()) {
		int nCheck = (int)m_list.GetItemData(iItem);
		fStartMod = !!(nCheck & TSMOD);
		fEndMod = !!(nCheck & TEMOD);
		fStartAdj = !!(nCheck & TSADJ);
		fEndAdj = !!(nCheck & TEADJ);
	}
}

void CPlayerSubresyncBar::SetCheck(int iItem, bool fStart, bool fEnd)
{
	if (0 <= iItem && (size_t)iItem < m_sts.GetCount()) {
		SubTime& st = m_subtimes[iItem];

		int nCheck = (int)m_list.GetItemData(iItem) & TSEP;

		if (fStart) {
			nCheck |= TSMOD;
		} else if (abs(st.orgstart - st.newstart)) {
			nCheck |= TSADJ;
		}
		if (fEnd) {
			nCheck |= TEMOD;
		} else if (abs(st.orgend - st.newend)) {
			nCheck |= TEADJ;
		}

		m_list.SetItemData(iItem, (DWORD)nCheck);

		WCHAR buff[32];
		FormatTime(iItem, buff, std::size(buff), fStart, false);
		m_list.SetItemText(iItem, COL_START, buff);
		FormatTime(iItem, buff, std::size(buff), fEnd, true);
		m_list.SetItemText(iItem, COL_END, buff);
	}
}

bool CPlayerSubresyncBar::ModStart(int iItem, int t, bool fReset)
{
	bool fRet = false;
	bool fStartMod, fEndMod, fStartAdj, fEndAdj;

	GetCheck(iItem, fStartMod, fEndMod, fStartAdj, fEndAdj);

	SubTime& st = m_subtimes[iItem];

	//if (fStartMod || fStartAdj || st.orgstart != t || fReset)
	{
		fRet = (st.newstart != t);

		st.newstart = t;
		if (!fEndMod) {
			st.newend = st.newstart + (st.orgend - st.orgstart);
		} else if (fReset) {
			st.newstart = st.newend - (st.orgend - st.orgstart);
		}

		SetCheck(iItem, !fReset, fEndMod);
	}

	return fRet;
}

bool CPlayerSubresyncBar::ModEnd(int iItem, int t, bool fReset)
{
	bool fRet = false;
	bool fStartMod, fEndMod, fStartAdj, fEndAdj;

	GetCheck(iItem, fStartMod, fEndMod, fStartAdj, fEndAdj);

	SubTime& st = m_subtimes[iItem];

	//if (fEndMod || fEndAdj || st.orgend != t || fReset)
	{
		fRet = (st.newend != t);

		st.newend = t;
		if (!fStartMod) {
			st.newstart = st.newend - (st.orgend - st.orgstart);
		} else if (fReset) {
			st.newend = st.newstart + (st.orgend - st.orgstart);
		}

		SetCheck(iItem, fStartMod, !fReset);
	}

	return fRet;
}

void CPlayerSubresyncBar::FormatTime(int iItem, WCHAR* buff, size_t buffLen, int time, bool fEnd)
{
	int t = !fEnd
			? (time == 2 ? m_sts[iItem].start
			   : time == 1 ? m_subtimes[iItem].newstart
			   : m_subtimes[iItem].orgstart)
			: (time == 2 ? m_sts[iItem].end
			   : time == 1 ? m_subtimes[iItem].newend
			   : m_subtimes[iItem].orgend);

	swprintf_s(buff, buffLen, t >= 0
				? L"%02d:%02d:%02d.%03d"
				: L"-%02d:%02d:%02d.%03d",
				abs(t) / 60 / 60 / 1000,
				(abs(t) / 60 / 1000) % 60,
				(abs(t) / 1000) % 60,
				abs(t) % 1000);
}


BEGIN_MESSAGE_MAP(CPlayerSubresyncBar, CPlayerBar)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_BEGINLABELEDITW, IDC_SUBRESYNCLIST, OnBeginlabeleditList)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_SUBRESYNCLIST, OnDolabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDITW, IDC_SUBRESYNCLIST, OnEndlabeleditList)
	ON_NOTIFY(NM_RCLICK, IDC_SUBRESYNCLIST, OnRclickList)
	ON_NOTIFY(NM_DBLCLK, IDC_SUBRESYNCLIST, OnNMDblclkList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_SUBRESYNCLIST, OnLvnKeydownList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SUBRESYNCLIST, OnCustomdrawList)
END_MESSAGE_MAP()


// CPlayerSubresyncBar message handlers

void CPlayerSubresyncBar::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (::IsWindow(m_list.m_hWnd)) {
		CRect r;
		GetClientRect(r);
		r.DeflateRect(2, 2);
		m_list.MoveWindow(r);
	}
}

static bool ParseTime(CString str, int& ret, bool fWarn = true)
{
	int sign = 1, h, m, s, ms;
	WCHAR c;

	str.Trim();
	if (!str.IsEmpty() && str[0] == '-') {
		sign = -1;
	}

	int n = swscanf_s(str, L"%d%c%d%c%d%c%d", &h, &c, 1, &m, &c, 1, &s, &c, 1, &ms);

	h = abs(h);

	if (n == 7
			&& 0 <= h && h < 24
			&& 0 <= m && m < 60
			&& 0 <= s && s < 60
			&& 0 <= ms && ms < 1000) {
		ret = sign * (h * 60 * 60 * 1000 + m * 60 * 1000 + s * 1000 + ms);
		return true;
	}

	if (fWarn) {
		AfxMessageBox(ResStr(IDS_SUBRESYNC_TIME_FORMAT), MB_ICONEXCLAMATION | MB_OK, 0);
	}
	return false;
}

void CPlayerSubresyncBar::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (pItem->iItem >= 0) {
		if ((pItem->iSubItem == COL_START || pItem->iSubItem == COL_END || pItem->iSubItem == COL_TEXT
				|| pItem->iSubItem == COL_STYLE || pItem->iSubItem == COL_LAYER
				|| pItem->iSubItem == COL_ACTOR || pItem->iSubItem == COL_EFFECT)
				&& m_mode == TEXTSUB) {
			*pResult = TRUE;
		} else if ((pItem->iSubItem == COL_START)
				   && m_mode == VOBSUB) {
			*pResult = TRUE;
		}
	}
}

void CPlayerSubresyncBar::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (pItem->iItem >= 0) {
		if ((pItem->iSubItem == COL_START || pItem->iSubItem == COL_END || pItem->iSubItem == COL_TEXT
				|| pItem->iSubItem == COL_STYLE || pItem->iSubItem == COL_LAYER
				|| pItem->iSubItem == COL_ACTOR || pItem->iSubItem == COL_EFFECT)
				&& m_mode == TEXTSUB) {
			m_list.ShowInPlaceEdit(pItem->iItem, pItem->iSubItem);
			*pResult = TRUE;
		} else if ((pItem->iSubItem == COL_START)
				   && m_mode == VOBSUB) {
			m_list.ShowInPlaceEdit(pItem->iItem, pItem->iSubItem);
			*pResult = TRUE;
		}
	}
}

void CPlayerSubresyncBar::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (!m_list.m_fInPlaceDirty) {
		return;
	}

	bool fNeedsUpdate = false;

	if (pItem->iItem >= 0 && pItem->pszText && (m_mode == VOBSUB || m_mode == TEXTSUB)) {
		switch (pItem->iSubItem) {
			case COL_START: {
				int t;
				if (ParseTime(pItem->pszText, t)) {
					fNeedsUpdate = ModStart(pItem->iItem, t);

					*pResult = TRUE;
				}
			}
			break;
			case COL_END:
				if (m_mode == TEXTSUB) {
					int t;
					if (ParseTime(pItem->pszText, t)) {
						fNeedsUpdate = ModEnd(pItem->iItem, t);

						*pResult = TRUE;
					}
				}
				break;
			case COL_TEXT:
				if (m_mode == TEXTSUB) {
					CString str = m_sts.GetStrW(pItem->iItem, true);

					if (str != pItem->pszText) {
						fNeedsUpdate = true;
						m_sts.SetStr(pItem->iItem, CString(pItem->pszText), true);
						m_list.SetItemText(pItem->iItem, pItem->iSubItem, m_sts.GetStrW(pItem->iItem, true));
					}
				}
				break;
			case COL_STYLE:
				if (m_mode == TEXTSUB) {
					CString str(pItem->pszText);
					str.Trim();

					if (!str.IsEmpty() && m_sts[pItem->iItem].style != str) {
						fNeedsUpdate = true;

						if (!m_sts.m_styles.Lookup(str)) {
							m_sts.AddStyle(str, DNew STSStyle());
						}

						m_sts[pItem->iItem].style = str;

						m_list.SetItemText(pItem->iItem, pItem->iSubItem, pItem->pszText);
					}
				}
				break;
			case COL_LAYER:
				if (m_mode == TEXTSUB) {
					int l;
					if (swscanf_s(pItem->pszText, L"%d", &l) == 1) {
						fNeedsUpdate = true;
						m_sts[pItem->iItem].layer = l;
						CString str;
						str.Format(L"%d", l);
						m_list.SetItemText(pItem->iItem, pItem->iSubItem, str);
					}
				}
				break;
			case COL_ACTOR:
				if (m_mode == TEXTSUB) {
					CString str(pItem->pszText);
					str.Trim();
					if (!str.IsEmpty()) {
						fNeedsUpdate = true;
						m_sts[pItem->iItem].actor = str;
						m_list.SetItemText(pItem->iItem, pItem->iSubItem, str);
					}
				}
				break;
			case COL_EFFECT:
				if (m_mode == TEXTSUB) {
					CString str(pItem->pszText);
					str.Trim();
					if (!str.IsEmpty()) {
						fNeedsUpdate = true;
						m_sts[pItem->iItem].effect = str;
						m_list.SetItemText(pItem->iItem, pItem->iSubItem, str);
					}
				}
				break;
		}
	}

	if (fNeedsUpdate) {
		UpdatePreview();
	}
}

void CPlayerSubresyncBar::OnRclickList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;

	if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0) {
		enum {
			TOGSEP = 1,
			DUPITEM, DELITEM,
			RESETS, SETOS, SETCS, RESETE, SETOE, SETCE,
			STYLEFIRST, STYLELAST = STYLEFIRST + 1000, STYLEEDIT,
			LAYERDEC, LAYERINC,
			ACTORFIRST, ACTORLAST = ACTORFIRST + 1000,
			EFFECTFIRST, EFFECTLAST = EFFECTFIRST + 1000
		};

		CStringArray styles;
		CStringArray actors;
		CStringArray effects;

		CMenu m;
		m.CreatePopupMenu();

		if (m_mode == VOBSUB || m_mode == TEXTSUB) {
			m.AppendMenuW(MF_STRING | MF_ENABLED, TOGSEP, ResStr(IDS_SUBRESYNC_SEPARATOR));
			m.AppendMenuW(MF_SEPARATOR);
			if (m_mode == TEXTSUB) {
				m.AppendMenuW(MF_STRING | MF_ENABLED, DUPITEM, ResStr(IDS_SUBRESYNC_DUPLICATE));
			}
			m.AppendMenuW(MF_STRING | MF_ENABLED, DELITEM, ResStr(IDS_SUBRESYNC_DELETE));
		}

		switch (lpnmlv->iSubItem) {
			case COL_START:
				if (m_mode == VOBSUB || m_mode == TEXTSUB) {
					m.AppendMenuW(MF_SEPARATOR);
					m.AppendMenuW(MF_STRING | MF_ENABLED, RESETS, ResStr(IDS_SUBRESYNC_RESET) + L"\tF1");
					m.AppendMenuW(MF_STRING | MF_ENABLED, SETOS, ResStr(IDS_SUBRESYNC_ORIGINAL) + L"\tF3");
					m.AppendMenuW(MF_STRING | MF_ENABLED, SETCS, ResStr(IDS_SUBRESYNC_CURRENT) + L"\tF5");
				}
				break;
			case COL_END:
				if (m_mode == TEXTSUB) {
					m.AppendMenuW(MF_SEPARATOR);
					m.AppendMenuW(MF_STRING | MF_ENABLED, RESETE, ResStr(IDS_SUBRESYNC_RESET) + L"\tF2");
					m.AppendMenuW(MF_STRING | MF_ENABLED, SETOE, ResStr(IDS_SUBRESYNC_ORIGINAL) + L"\tF4");
					m.AppendMenuW(MF_STRING | MF_ENABLED, SETCE, ResStr(IDS_SUBRESYNC_CURRENT) + L"\tF6");
				}
				break;
			case COL_STYLE:
				if (m_mode == TEXTSUB) {
					m.AppendMenuW(MF_SEPARATOR);

					int id = STYLEFIRST;

					POSITION pos = m_sts.m_styles.GetStartPosition();
					while (pos && id <= STYLELAST) {
						CString key;
						STSStyle* val;
						m_sts.m_styles.GetNextAssoc(pos, key, val);
						styles.Add(key);
						m.AppendMenuW(MF_STRING | MF_ENABLED, id++, key);
					}

					if (id > STYLEFIRST && m_list.GetSelectedCount() == 1) {
						m.AppendMenuW(MF_SEPARATOR);
						m.AppendMenuW(MF_STRING | MF_ENABLED, STYLEEDIT, ResStr(IDS_SUBRESYNC_EDIT));
					}
				}
				break;
			case COL_LAYER:
				if (m_mode == TEXTSUB) {
					m.AppendMenuW(MF_SEPARATOR);
					m.AppendMenuW(MF_STRING | MF_ENABLED, LAYERDEC, ResStr(IDS_SUBRESYNC_DECREASE));
					m.AppendMenuW(MF_STRING | MF_ENABLED, LAYERINC, ResStr(IDS_SUBRESYNC_INCREASE));
				}
				break;
			case COL_ACTOR:
				if (m_mode == TEXTSUB) {
					CMapStringToPtr actormap;

					for (size_t i = 0, j = m_sts.GetCount(); i < j; i++) {
						actormap[m_sts[i].actor] = nullptr;
					}

					actormap.RemoveKey(L"");

					if (!actormap.IsEmpty()) {
						m.AppendMenuW(MF_SEPARATOR);

						int id = ACTORFIRST;

						POSITION pos = actormap.GetStartPosition();
						while (pos && id <= ACTORLAST) {
							CString key;
							void* val;
							actormap.GetNextAssoc(pos, key, val);

							actors.Add(key);

							m.AppendMenuW(MF_STRING | MF_ENABLED, id++, key);
						}
					}
				}
				break;
			case COL_EFFECT:
				if (m_mode == TEXTSUB) {
					CMapStringToPtr effectmap;

					for (size_t i = 0, j = m_sts.GetCount(); i < j; i++) {
						effectmap[m_sts[i].effect] = nullptr;
					}

					effectmap.RemoveKey(L"");

					if (!effectmap.IsEmpty()) {
						m.AppendMenuW(MF_SEPARATOR);

						int id = EFFECTFIRST;

						POSITION pos = effectmap.GetStartPosition();
						while (pos && id <= EFFECTLAST) {
							CString key;
							void* val;
							effectmap.GetNextAssoc(pos, key, val);

							effects.Add(key);

							m.AppendMenuW(MF_STRING | MF_ENABLED, id++, key);
						}
					}
				}
				break;
		}

		CPoint p = lpnmlv->ptAction;
		::MapWindowPoints(pNMHDR->hwndFrom, HWND_DESKTOP, &p, 1);

		UINT id = m.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RETURNCMD, p.x, p.y, this);

		bool fNeedsUpdate = false;

		POSITION pos = m_list.GetFirstSelectedItemPosition();
		while (pos) {
			int iItem = m_list.GetNextSelectedItem(pos);

			SubTime& st = m_subtimes[iItem];

			switch (id) {
				case TOGSEP:
					m_list.SetItemData(iItem, m_list.GetItemData(iItem) ^ TSEP);
					m_list.Invalidate();
					fNeedsUpdate = true;
					break;
				case DUPITEM: {
					std::vector<int> items;
					pos = m_list.GetFirstSelectedItemPosition();
					while (pos) {
						items.emplace_back(m_list.GetNextSelectedItem(pos));
					}

					std::sort(items.begin(), items.end(), std::greater<int>());

					for (INT_PTR i = 0, l = items.size(); i < l; i++) {
						iItem = items[i];

						STSEntry stse = m_sts[iItem];
						m_sts.InsertAt(iItem + 1, stse);

						SubTime st = m_subtimes[iItem];
						m_subtimes.insert(m_subtimes.begin() + iItem + 1, st);

						CHeaderCtrl* pHeader = (CHeaderCtrl*)m_list.GetDlgItem(0);
						int nColumnCount = pHeader->GetItemCount();

						CStringArray sa;
						sa.SetSize(nColumnCount);
						for (int col = 0; col < nColumnCount; col++) {
							sa[col] = m_list.GetItemText(iItem, col);
						}

						DWORD_PTR data = m_list.GetItemData(iItem);
						m_list.InsertItem(iItem + 1, sa[0]);
						m_list.SetItemData(iItem + 1, data);
						for (int col = 1; col < nColumnCount; col++) {
							m_list.SetItemText(iItem + 1, col, sa[col]);
						}
					}
				}

				fNeedsUpdate = true;
				break;
				case DELITEM: {
					std::vector<int> items;
					pos = m_list.GetFirstSelectedItemPosition();
					while (pos) {
						items.emplace_back(m_list.GetNextSelectedItem(pos));
					}

					std::sort(items.begin(), items.end(), std::greater<int>());

					for (INT_PTR i = 0, l = items.size(); i < l; i++) {
						iItem = items[i];
						m_sts.RemoveAt(iItem);
						m_subtimes.erase(m_subtimes.begin() + iItem);
						m_list.DeleteItem(iItem);
					}

					iItem = items[items.size() - 1];
					if (iItem >= m_list.GetItemCount()) {
						iItem = m_list.GetItemCount() - 1;
					}

					m_list.SetSelectionMark(iItem);
				}
				fNeedsUpdate = true;
				break;
				case RESETS: /*if (*/
					ModStart(iItem, st.orgstart, true);/*)*/
					fNeedsUpdate = true;
					break;
				case SETOS: /*if (*/
					ModStart(iItem, st.orgstart);/*)*/
					fNeedsUpdate = true;
					break;
				case SETCS: /*if (*/
					ModStart(iItem, (int)(m_rt / 10000)); /*)*/
					fNeedsUpdate = true;
					break;
				case RESETE: /*if (*/
					ModEnd(iItem, st.orgend, true);/*)*/
					fNeedsUpdate = true;
					break;
				case SETOE: /*if (*/
					ModEnd(iItem, st.orgend);/*)*/
					fNeedsUpdate = true;
					break;
				case SETCE: /*if (*/
					ModEnd(iItem, (int)(m_rt / 10000)); /*)*/
					fNeedsUpdate = true;
					break;
				default:
					if (STYLEFIRST <= id && id <= STYLELAST) {
						CString s = styles[id - STYLEFIRST];
						if (m_sts[iItem].style != s) {
							fNeedsUpdate = true;
						}
						m_sts[iItem].style = s;
						m_list.SetItemText(iItem, lpnmlv->iSubItem, s);
					} else if (id == STYLEEDIT) {
						std::vector<std::unique_ptr<CPPageSubStyle>> pages;
						std::vector<STSStyle*> styles;

						STSStyle* stss = m_sts.GetStyle(iItem);
						int iSelPage = 0;

						POSITION pos = m_sts.m_styles.GetStartPosition();
						for (int i = 0; pos; i++) {
							CString key;
							STSStyle* val;
							m_sts.m_styles.GetNextAssoc(pos, key, val);

							auto page = std::make_unique<CPPageSubStyle>();
							page->InitSubStyle(key, val);
							pages.emplace_back(std::move(page));
							styles.emplace_back(val);

							if (stss == val) {
								iSelPage = i;
							}
						}

						CPropertySheet dlg(L"Styles...", this, iSelPage);
						for (const auto& page : pages) {
							dlg.AddPage(page.get());
						}

						if (dlg.DoModal() == IDOK) {
							for (size_t j = 0, l = pages.size(); j < l; j++) {
								stss = styles[j];
								pages[j]->GetSubStyle(stss);

								for (int i = 0; i < (int)m_sts.GetCount(); i++) {
									if (m_sts.GetStyle(i) == stss) {
										CString str;
										m_list.SetItemText(i, COL_TEXT, m_sts.GetStrW(i, true));
										m_list.SetItemText(i, COL_FONT, stss->fontName);
										str.Format(L"%d", stss->charSet);
										m_list.SetItemText(i, COL_CHARSET, str);
										str.Format(L"%d", m_sts[i].layer);
									}
								}
							}

							fNeedsUpdate = true;
						}
					} else if (id == LAYERDEC || id == LAYERINC) {
						int d = (id == LAYERDEC) ? -1 : 1;
						fNeedsUpdate = true;
						m_sts[iItem].layer += d;
						CString s;
						s.Format(L"%d", m_sts[iItem].layer);
						m_list.SetItemText(iItem, lpnmlv->iSubItem, s);
					} else if (ACTORFIRST <= id && id <= ACTORLAST) {
						CString s = actors[id - ACTORFIRST];
						if (m_sts[iItem].actor != s) {
							fNeedsUpdate = true;
						}
						m_sts[iItem].actor = s;
						m_list.SetItemText(iItem, lpnmlv->iSubItem, s);
					} else if (EFFECTFIRST <= id && id <= EFFECTLAST) {
						CString s = effects[id - EFFECTFIRST];
						if (m_sts[iItem].effect != s) {
							fNeedsUpdate = true;
						}
						m_sts[iItem].effect = s;
						m_list.SetItemText(iItem, lpnmlv->iSubItem, s);
					}
					break;
			}
		}

		if (fNeedsUpdate) {
			UpdatePreview();
		}
	}

	*pResult = 0;
}

void CPlayerSubresyncBar::OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;

	if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0 && (m_mode == VOBSUB || m_mode == TEXTSUB)) {
		int t = 0;
		if (lpnmlv->iSubItem > COL_PREVEND || !ParseTime(m_list.GetItemText(lpnmlv->iItem, lpnmlv->iSubItem), t, false)) {
			t = m_sts[lpnmlv->iItem].start;
		}

		REFERENCE_TIME rt = (REFERENCE_TIME)t * 10000;
		m_pMainFrame->SeekTo(rt);
	}

	*pResult = 0;
}

void CPlayerSubresyncBar::OnLvnKeydownList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	UNREFERENCED_PARAMETER(pLVKeyDown);

	*pResult = 0;
}

static CUIntArray s_itemGroups;
static int s_totalGroups;

void CPlayerSubresyncBar::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	*pResult = CDRF_DODEFAULT;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage) {
		s_itemGroups.SetSize(m_list.GetItemCount());
		s_totalGroups = 0;
		for (int i = 0, j = m_list.GetItemCount(); i < j; i++) {
			if (m_list.GetItemData(i)&TSEP) {
				s_totalGroups++;
			}
			s_itemGroups[i] = s_totalGroups;
		}

		*pResult = CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW;
	} else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage) {
		pLVCD->nmcd.uItemState &= ~CDIS_FOCUS;

		*pResult = CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYSUBITEMDRAW;
	} else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage) {
		COLORREF clrText;
		COLORREF clrTextBk;

		if ((pLVCD->iSubItem == COL_START || pLVCD->iSubItem == COL_END || pLVCD->iSubItem == COL_TEXT || pLVCD->iSubItem == COL_STYLE
				|| pLVCD->iSubItem == COL_LAYER || pLVCD->iSubItem == COL_ACTOR || pLVCD->iSubItem == COL_EFFECT)
				&& m_mode == TEXTSUB) {
			clrText = 0;
		} else if ((pLVCD->iSubItem == COL_START)
				   && m_mode == VOBSUB) {
			clrText = 0;
		} else {
			clrText = 0x606060;
		}

		clrTextBk = 0xffffff;
		//if (s_totalGroups > 0)
		clrTextBk -= ((s_itemGroups[pLVCD->nmcd.dwItemSpec] & 1) ? 0x100010 : 0x200020);

		if (m_sts[pLVCD->nmcd.dwItemSpec].start <= m_rt / 10000 && m_rt / 10000 < m_sts[pLVCD->nmcd.dwItemSpec].end) {
			clrText |= 0xFF;
		}

		int nCheck = (int)m_list.GetItemData((int)pLVCD->nmcd.dwItemSpec);

		if ((nCheck & 1) && (pLVCD->iSubItem == COL_START || pLVCD->iSubItem == COL_PREVSTART)) {
			clrTextBk = 0xffddbb;
		} else if ((nCheck & 4) && (/*pLVCD->iSubItem == COL_START ||*/ pLVCD->iSubItem == COL_PREVSTART)) {
			clrTextBk = 0xffeedd;
		}

		if ((nCheck & 2) && (pLVCD->iSubItem == COL_END || pLVCD->iSubItem == COL_PREVEND)) {
			clrTextBk = 0xffddbb;
		} else if ((nCheck & 8) && (/*pLVCD->iSubItem == COL_END ||*/ pLVCD->iSubItem == COL_PREVEND)) {
			clrTextBk = 0xffeedd;
		}

		pLVCD->clrText = clrText;
		pLVCD->clrTextBk = clrTextBk;

		*pResult = CDRF_NOTIFYPOSTPAINT;
	} else if ((CDDS_ITEMPOSTPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage) {
		//	  *pResult = CDRF_DODEFAULT;
	} else if (CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage) {
		int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

		LVITEM rItem;
		ZeroMemory(&rItem, sizeof(LVITEM));
		rItem.mask  = LVIF_IMAGE | LVIF_STATE;
		rItem.iItem = nItem;
		rItem.stateMask = LVIS_SELECTED;
		m_list.GetItem(&rItem);

		{
			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);

			CRect rcItem;
			m_list.GetItemRect(nItem, &rcItem, LVIR_BOUNDS);

			{
				bool fSeparator = nItem < m_list.GetItemCount() - 1 && (m_list.GetItemData(nItem + 1)&TSEP);
				CPen p(PS_INSIDEFRAME, 1, fSeparator ? 0x404040 : 0xe0e0e0);
				CPen* old = pDC->SelectObject(&p);
				pDC->MoveTo(CPoint(rcItem.left, rcItem.bottom - 1));
				pDC->LineTo(CPoint(rcItem.right, rcItem.bottom - 1));
				pDC->SelectObject(old);
			}

			{
				CPen p(PS_INSIDEFRAME, 1, 0xe0e0e0);
				CPen* old = pDC->SelectObject(&p);

				CHeaderCtrl* pHeader = (CHeaderCtrl*)m_list.GetDlgItem(0);
				int nColumnCount = pHeader->GetItemCount();

				// Get the column offset
				int offset = rcItem.left;
				for (int i = 0; i < nColumnCount; i++) {
					offset += m_list.GetColumnWidth(i);
					pDC->MoveTo(CPoint(offset, rcItem.top));
					pDC->LineTo(CPoint(offset, rcItem.bottom));
				}

				pDC->SelectObject(old);
			}

			*pResult = CDRF_SKIPDEFAULT;
		}
	} else if (CDDS_POSTPAINT == pLVCD->nmcd.dwDrawStage) {
	}
}

bool CPlayerSubresyncBar::IsShortCut(const MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && VK_F1 <= pMsg->wParam && pMsg->wParam <= VK_F6) {
		int iItem = -1;

		bool fNeedsUpdate = false;
		bool fStep = false;

		POSITION pos = m_list.GetFirstSelectedItemPosition();
		while (pos) {
			iItem = m_list.GetNextSelectedItem(pos);

			SubTime& st = m_subtimes[iItem];

			switch (pMsg->wParam) {
				case VK_F1: /*if (*/
					ModStart(iItem, st.orgstart, true);/*)*/
					fNeedsUpdate = true;
					break;
				case VK_F3: /*if (*/
					ModStart(iItem, st.orgstart);/*)*/
					fNeedsUpdate = true;
					break;
				case VK_F5: /*if (*/
					ModStart(iItem, (int)(m_rt / 10000)); /*)*/
					fNeedsUpdate = true;
					break;
				case VK_F2: /*if (*/
					ModEnd(iItem, st.orgend, true);/*)*/
					fNeedsUpdate = true;
					break;
				case VK_F4: /*if (*/
					ModEnd(iItem, st.orgend);/*)*/
					fNeedsUpdate = true;
					break;
				case VK_F6: /*if (*/
					ModEnd(iItem, (int)(m_rt / 10000)); /*)*/
					fNeedsUpdate = fStep = true;
					break;
			}
		}

		if (fNeedsUpdate) {
			if (fStep && m_list.GetSelectedCount() == 1 && iItem < m_list.GetItemCount() - 1) {
				m_list.SetItemState(iItem, 0, LVIS_SELECTED);
				m_list.SetItemState(iItem + 1, LVIS_SELECTED, LVIS_SELECTED);
				m_list.SetSelectionMark(iItem + 1);
				m_list.EnsureVisible(iItem + 1, false);
			}

			UpdatePreview();

			return true;
		}
	}

	return false;
}

int CPlayerSubresyncBar::FindNearestSub(REFERENCE_TIME& rtPos, bool bForward)
{
	if (m_subtimes.empty()) {
		return -2;
	}

	int lCurTime = (int)(rtPos / 10000) + (bForward ? 1 : -1);
	int subCount = (int)m_sts.GetCount();

	if (lCurTime < m_subtimes[0].newstart) {
		rtPos = (REFERENCE_TIME)m_subtimes[0].newstart * 10000;
		return bForward ? 0 : -1;
	} else if (lCurTime > m_subtimes[subCount - 1].newstart) {
		rtPos = (REFERENCE_TIME)m_subtimes[subCount - 1].newstart * 10000;
		return bForward ? -1 : subCount - 1;
	}

	for (int i = 1; i < subCount; i++) {
		if ((lCurTime >= m_subtimes[i - 1].newstart) && (lCurTime < m_subtimes[i].newstart)) {
			if (!bForward) {
				i--;
				if (i >= 1 && m_subtimes[i].newend >= lCurTime) {
					i--;
				}
			}
			rtPos = (REFERENCE_TIME)m_subtimes[i].newstart * 10000;
			return i;
		}
	}

	return -1;
}

bool CPlayerSubresyncBar::ShiftSubtitle(int nItem, long lValue, REFERENCE_TIME& rtPos)
{
	bool bRet = false;

	if ((nItem == 0) || (m_subtimes[nItem - 1].newend < m_subtimes[nItem].newstart + lValue)) {
		for (size_t i = nItem; i < m_sts.GetCount(); i++) {
			m_subtimes[i].newstart += lValue;
			m_subtimes[i].newend   += lValue;
			m_subtimes[i].orgstart += lValue;
			m_subtimes[i].orgend   += lValue;
		}
		UpdatePreview();
		SaveSubtitle();
		bRet = true;
		rtPos = (REFERENCE_TIME)m_subtimes[nItem].newstart * 10000;
	}
	return bRet;
}

bool CPlayerSubresyncBar::SaveToDisk()
{
	return m_sts.SaveAs(m_sts.m_path, m_sts.m_subtitleType);
}
