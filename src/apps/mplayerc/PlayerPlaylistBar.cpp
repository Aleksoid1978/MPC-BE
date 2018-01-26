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
#include <atlutil.h>
#include <atlpath.h>
#include "MainFrm.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/Filehandle.h"
#include "SaveTextFileDialog.h"
#include "PlayerPlaylistBar.h"
#include "OpenDlg.h"
#include "Content.h"

static CString MakePath(CString path)
{
	if (::PathIsURL(path) || Youtube::CheckURL(path)) { // skip URLs
		if (path.Left(8).MakeLower() == L"file:///") {
			path.Delete(0, 8);
			path.Replace('/', '\\');
		}

		return path;
	}

	path.Replace('/', '\\');

	CPath c(path);
	c.Canonicalize();
	return CString(c);
}

struct CUETrack {
	REFERENCE_TIME m_rt;
	UINT m_trackNum;
	CString m_fn;
	CString m_Title;
	CString m_Performer;

	CUETrack() {
		m_rt		= _I64_MIN;
		m_trackNum	= 0;
	}

	CUETrack(CString fn, REFERENCE_TIME rt, UINT trackNum, CString Title, CString Performer) {
		m_rt		= rt;
		m_trackNum = trackNum;
		m_fn		= fn;
		m_Title		= Title;
		m_Performer	= Performer;
	}
};
static bool ParseCUESheetFile(CString fn, std::list<CUETrack> &CUETrackList, CString& Title, CString& Performer)
{
	CTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn) || f.GetLength() > 32 * 1024) {
		return false;
	}

	Title.Empty();
	Performer.Empty();

	CString cueLine;

	std::vector<CString> sFilesArray;
	std::vector<CString> sTrackArray;
	while (f.ReadString(cueLine)) {
		CString cmd = GetCUECommand(cueLine);
		if (cmd == L"TRACK") {
			sTrackArray.push_back(cueLine);
		} else if (cmd == L"FILE" && cueLine.Find(L" WAVE") > 0) {
			cueLine.Replace(L" WAVE", L"");
			cueLine.Trim(L" \"");
			sFilesArray.push_back(cueLine);
		}
	};

	if (sTrackArray.empty() || sFilesArray.empty()) {
		return false;
	}

	BOOL bMultiple = (sTrackArray.size() == sFilesArray.size());

	CString sTitle, sPerformer, sFileName, sFileName2;
	REFERENCE_TIME rt = _I64_MIN;
	BOOL fAudioTrack = FALSE;
	UINT trackNum = 0;

	UINT idx = 0;
	f.Seek(0, CFile::SeekPosition::begin);
	while (f.ReadString(cueLine)) {
		CString cmd = GetCUECommand(cueLine);

		if (cmd == L"TRACK") {
			if (rt != _I64_MIN) {
				CString fName = bMultiple ? sFilesArray[idx++] : sFilesArray.size() == 1 ? sFilesArray[0] : sFileName;
				CUETrackList.push_back(CUETrack(fName, rt, trackNum, sTitle, sPerformer));
			}
			rt = _I64_MIN;
			sFileName = sFileName2;
			if (!Performer.IsEmpty()) {
				sPerformer = Performer;
			}

			WCHAR type[256] = { 0 };
			trackNum = 0;
			fAudioTrack = FALSE;
			if (2 == swscanf_s(cueLine, L"%u %s", &trackNum, type, _countof(type) - 1)) {
				fAudioTrack = (wcscmp(type, L"AUDIO") == 0);
			}
		} else if (cmd == L"TITLE") {
			cueLine.Trim(L" \"");
			sTitle = cueLine;

			if (sFileName2.IsEmpty()) {
				Title = sTitle;
			}
		} else if (cmd == L"PERFORMER") {
			cueLine.Trim(L" \"");
			sPerformer = cueLine;

			if (sFileName2.IsEmpty() && Performer.IsEmpty()) {
				Performer = sPerformer;
			}
		} else if (cmd == L"FILE" && cueLine.Find(L" WAVE") > 0) {
			cueLine.Replace(L" WAVE", L"");
			cueLine.Trim(L" \"");
			if (sFileName.IsEmpty()) {
				sFileName = sFileName2 = cueLine;
			} else {
				sFileName2 = cueLine;
			}
		} else if (cmd == L"INDEX") {
			unsigned mm, ss, ff;
			if (3 == swscanf_s(cueLine, L"01 %u:%u:%u", &mm, &ss, &ff) && fAudioTrack) {
				// "INDEX 01" is required and denotes the start of the track
				rt = UNITS * (mm*60 + ss) + UNITS * ff / 75;
			}
		}
	}

	if (rt != _I64_MIN) {
		CString fName = bMultiple ? sFilesArray[idx] : sFilesArray.size() == 1 ? sFilesArray[0] : sFileName2;
		CUETrackList.push_back(CUETrack(fName, rt, trackNum, sTitle, sPerformer));
	}

	return CUETrackList.size() > 0;
}

UINT CPlaylistItem::m_globalid  = 0;

CPlaylistItem::CPlaylistItem()
	: m_type(file)
	, m_fInvalid(false)
	, m_duration(0)
	, m_vinput(-1)
	, m_vchannel(-1)
	, m_ainput(-1)
	, m_country(0)
{
	m_id = m_globalid++;
}

CPlaylistItem::~CPlaylistItem()
{
}

CPlaylistItem::CPlaylistItem(const CPlaylistItem& pli)
{
	*this = pli;
}

CPlaylistItem& CPlaylistItem::operator = (const CPlaylistItem& pli)
{
	if (this != &pli) {
		m_id = pli.m_id;
		m_label = pli.m_label;
		m_fns = pli.m_fns;
		m_subs = pli.m_subs;
		m_type = pli.m_type;
		m_fInvalid = pli.m_fInvalid;
		m_duration = pli.m_duration;
		m_vinput = pli.m_vinput;
		m_vchannel = pli.m_vchannel;
		m_ainput = pli.m_ainput;
		m_country = pli.m_country;
	}
	return(*this);
}

bool CPlaylistItem::FindFile(LPCTSTR path)
{
	for (const auto& fi : m_fns) {
		if (fi.GetName().CompareNoCase(path) == 0) {
			return true;
		}
	}

	return false;
}

CString CPlaylistItem::GetLabel(int i)
{
	CString str;

	if (i == 0) {
		if (!m_label.IsEmpty()) {
			str = m_label;
		} else if (!m_fns.empty()) {
			str = GetFileOnly(m_fns.front());
		}
	} else if (i == 1) {
		if (m_fInvalid) {
			return L"Invalid";
		}

		if (m_type == file) {
			REFERENCE_TIME rt = m_duration;

			if (rt > 0) {
				str = ReftimeToString2(rt);
			}
		} else if (m_type == device) {
			// TODO
		}

	}

	return str;
}

template<class T>
static bool FindFileInList(std::list<T>& sl, CString fn)
{
	for (const auto& item : sl) {
		if (CString(item).CompareNoCase(fn) == 0) {
			return true;
		}
	}

	return false;
}

static void StringToPaths(const CString& curentdir, const CString& str, std::vector<CString>& paths)
{
	int pos = 0;
	do {
		CString s = str.Tokenize(L";", pos);
		if (s.GetLength() == 0) {
			continue;
		}
		int bs = s.ReverseFind('\\');
		int a = s.Find('*');
		if (a >= 0 && a < bs) {
			continue; // the asterisk can only be in the last folder
		}

		CPath path(curentdir);
		path.Append(s);

		if (path.IsRoot() && path.FileExists()) {
			paths.push_back(path);
			continue;
		}

		WIN32_FIND_DATAW fd = { 0 };
		HANDLE hFind = FindFirstFileW(path, &fd);
		if (hFind == INVALID_HANDLE_VALUE) {
			continue;
		} else {
			CPath parentdir = path + L"\\..";
			parentdir.Canonicalize();

			do {
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) {
					CString folder = parentdir + '\\' + fd.cFileName + '\\';

					size_t index = 0;
					size_t count = paths.size();
					for (; index < count; index++) {
						if (folder.CompareNoCase(paths[index]) == 0) {
							break;
						}
					}
					if (index == count) {
						paths.push_back(folder);
					}
				}
			} while (FindNextFileW(hFind, &fd));
			FindClose(hFind);
		}
	} while (pos > 0);
}

void CPlaylistItem::AutoLoadFiles()
{
	if (m_fns.empty()) {
		return;
	}

	CString fn = m_fns.front();
	if (fn.Find(L"://") >= 0) { // skip URLs
		return;
	}

	int n = fn.ReverseFind('\\') + 1;
	CString curdir = fn.Left(n);
	CString name   = fn.Mid(n);
	CString ext;
	n = name.ReverseFind('.');
	if (n >= 0) {
		ext = name.Mid(n + 1).MakeLower();
		name.Truncate(n);
	}

	CString BDLabel, empty;
	AfxGetMainFrame()->MakeBDLabel(fn, empty, &BDLabel);

	CAppSettings& s = AfxGetAppSettings();

	if (s.fAutoloadAudio) {
		std::vector<CString> paths;
		StringToPaths(curdir, s.strAudioPaths, paths);

		for (const auto& apa : s.slAudioPathsAddons) {
			paths.push_back(apa);
		}

		CMediaFormats& mf = s.m_Formats;
		if (!mf.FindAudioExt(ext)) {
			for (const auto& path : paths) {
				WIN32_FIND_DATAW fd = {0};

				HANDLE hFind;
				std::vector<CString> searchPattern;
				searchPattern.push_back(path + name + L"*.*");
				if (!BDLabel.IsEmpty()) {
					searchPattern.push_back(path + BDLabel + L"*.*");
				}
				for (size_t j = 0; j < searchPattern.size(); j++) {
					hFind = FindFirstFileW(searchPattern[j], &fd);

					if (hFind != INVALID_HANDLE_VALUE) {
						do {
							if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
								continue;
							}

							CString ext2 = fd.cFileName;
							n = ext2.ReverseFind('.');
							if (n < 0) {
								continue;
							}
							ext2 = ext2.Mid(n + 1).MakeLower();
							CString fullpath = path + fd.cFileName;

							if (ext != ext2
									&& mf.FindAudioExt(ext2)
									&& !FindFileInList(m_fns, fullpath)
									&& s.GetFileEngine(fullpath) == DirectShow) {
								m_fns.push_back(fullpath);
							}
						} while (FindNextFileW(hFind, &fd));

						FindClose(hFind);
					}
				}
			}
		}
	}

	if (s.IsISRAutoLoadEnabled()) {
		std::vector<CString> paths;
		StringToPaths(curdir, s.strSubtitlePaths, paths);

		for (const auto& spa : s.slSubtitlePathsAddons) {
			paths.push_back(spa);
		}

		std::vector<CString> ret;
		Subtitle::GetSubFileNames(fn, paths, ret);

		for (const auto& sub_fn : ret) {
			if (!FindFileInList(m_subs, sub_fn)) {
				m_subs.push_back(sub_fn);
			}
		}

		if (!BDLabel.IsEmpty()) {
			ret.clear();
			Subtitle::GetSubFileNames(BDLabel, paths, ret);

			for (const auto& sub_fn : ret) {
				if (!FindFileInList(m_subs, sub_fn)) {
					m_subs.push_back(sub_fn);
				}
			}
		}
	}

	// cue-sheet file auto-load
	CString cuefn(fn);
	cuefn.Replace(ext, L"cue");
	if (::PathFileExistsW(cuefn)) {
		CString filter;
		CAtlArray<CString> mask;
		AfxGetAppSettings().m_Formats.GetAudioFilter(filter, mask);
		std::list<CString> sl;
		Explode(mask[0], sl, L';');

		BOOL bExists = false;
		for (CString _mask : sl) {
			_mask.Delete(0, 2);
			_mask.MakeLower();
			if (_mask == ext) {
				bExists = TRUE;
				break;
			}
		}

		if (bExists) {
			CFileItem* fi = &m_fns.front();
			if (!fi->GetChapterCount()) {

				CString Title, Performer;
				std::list<CUETrack> CUETrackList;
				if (ParseCUESheetFile(cuefn, CUETrackList, Title, Performer)) {
					std::list<CStringW> fileNames;

					for (const auto& cueTrack : CUETrackList) {
						if (std::find(fileNames.cbegin(), fileNames.cend(), cueTrack.m_fn) == fileNames.cend()) {
							fileNames.push_back(cueTrack.m_fn);
						}
					}

					if (fileNames.size() == 1) {
						// support opening cue-sheet file with only a single file inside, even if its name differs from the current
						MakeCUETitle(m_label, Title, Performer);

						for (const auto& cueTrack : CUETrackList) {
							CString cueTrackTitle;
							MakeCUETitle(cueTrackTitle, cueTrack.m_Title, cueTrack.m_Performer, cueTrack.m_trackNum);
							fi->AddChapter(Chapters{cueTrackTitle, cueTrack.m_rt});
						}
						fi->SetTitle(m_label);

						ChaptersList chaplist;
						fi->GetChapters(chaplist);
						BOOL bTrustedChap = FALSE;
						for (size_t i = 0; i < chaplist.size(); i++) {
							if (chaplist[i].rt > 0) {
								bTrustedChap = TRUE;
								break;
							}
						}
						if (!bTrustedChap) {
							fi->ClearChapter();
						}
					}
				}
			}
		}
	}
}

CPlaylist::CPlaylist()
	: m_pos(nullptr)
{
}

CPlaylist::~CPlaylist()
{
}

bool CPlaylist::RemoveAll()
{
	__super::RemoveAll();
	bool bWasPlaying = (m_pos != nullptr);
	m_pos = nullptr;
	return bWasPlaying;
}

bool CPlaylist::RemoveAt(POSITION pos)
{
	if (pos) {
		__super::RemoveAt(pos);
		if (m_pos == pos) {
			m_pos = nullptr;
			return true;
		}
	}

	return false;
}

struct plsort_str_t {
	CString str;
	POSITION pos;
};

void CPlaylist::SortByName()
{
	std::vector<plsort_str_t> a;
	a.reserve(GetCount());

	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		CString fn = GetAt(pos).m_fns.front();
		plsort_str_t item = { fn.Mid(std::max(fn.ReverseFind('/'), fn.ReverseFind('\\')) + 1), pos };
		a.emplace_back(item);
	}

	std::sort(a.begin(), a.end(), [](const plsort_str_t& a, const plsort_str_t& b) {
		return (StrCmpLogicalW(a.str, b.str) < 0);
	});

	for (size_t i = 0; i < a.size(); i++) {
		AddTail(GetAt(a[i].pos));
		__super::RemoveAt(a[i].pos);
		if (m_pos == a[i].pos) {
			m_pos = GetTailPosition();
		}
	}
}

void CPlaylist::SortByPath()
{
	std::vector<plsort_str_t> a;
	a.reserve(GetCount());

	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		plsort_str_t item = { GetAt(pos).m_fns.front(), pos };
		a.emplace_back(item);
	}

	std::sort(a.begin(), a.end(), [](const plsort_str_t& a, const plsort_str_t& b) {
		return (StrCmpLogicalW(a.str, b.str) < 0);
	});

	for (size_t i = 0; i < a.size(); i++) {
		AddTail(GetAt(a[i].pos));
		__super::RemoveAt(a[i].pos);
		if (m_pos == a[i].pos) {
			m_pos = GetTailPosition();
		}
	}
}

struct plsort_t {
	UINT n;
	POSITION pos;
};

void CPlaylist::SortById()
{
	std::vector<plsort_t> a;
	a.reserve(GetCount());

	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		plsort_t item = { GetAt(pos).m_id, pos };
		a.emplace_back(item);
	}

	std::sort(a.begin(), a.end(), [](const plsort_t& a, const plsort_t& b) {
		return a.n < b.n;
	});

	for (size_t i = 0; i < a.size(); i++) {
		AddTail(GetAt(a[i].pos));
		__super::RemoveAt(a[i].pos);
		if (m_pos == a[i].pos) {
			m_pos = GetTailPosition();
		}
	}
}

void CPlaylist::Randomize()
{
	std::vector<plsort_t> a;
	a.reserve(GetCount());

	srand((unsigned int)time(nullptr));
	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		plsort_t item = { rand(), pos };
		a.emplace_back(item);
	}

	std::sort(a.begin(), a.end(), [](const plsort_t& a, const plsort_t& b) {
		return a.n < b.n;
	});

	for (size_t i = 0; i < a.size(); i++) {
		AddTail(GetAt(a[i].pos));
		__super::RemoveAt(a[i].pos);
		if (m_pos == a[i].pos) {
			m_pos = GetTailPosition();
		}
	}
}

POSITION CPlaylist::GetPos() const
{
	return(m_pos);
}

void CPlaylist::SetPos(POSITION pos)
{
	m_pos = pos;
}

POSITION CPlaylist::Shuffle()
{
	static INT_PTR idx = 0;
	static INT_PTR count = 0;
	static std::vector<POSITION> a;

	ASSERT(GetCount() > 2);
	// insert or remove items in playlist, or index out of bounds then recalculate
	if ((count != GetCount()) || (idx >= GetCount())) {
		a.clear();
		idx = 0;
		a.reserve(count = GetCount());

		POSITION pos = GetHeadPosition();
		for (INT_PTR i = 0; pos; i++, GetNext(pos)) {
			a.emplace_back(pos); // initialize position array
		}

		//Use Fisher-Yates shuffle algorithm
		srand((unsigned)time(nullptr));
		for (INT_PTR i=0; i<(count-1); i++) {
			INT_PTR r = i + (rand() % (count-i));
			std::swap(a[i], a[r]);
		}
	}

	return a[idx++];
}

CPlaylistItem& CPlaylist::GetNextWrap(POSITION& pos)
{
	if (AfxGetAppSettings().bShufflePlaylistItems && GetCount() > 2) {
		pos = Shuffle();
	} else {
		GetNext(pos);
		if (!pos) {
			pos = GetHeadPosition();
		}
	}

	return(GetAt(pos));
}

CPlaylistItem& CPlaylist::GetPrevWrap(POSITION& pos)
{
	GetPrev(pos);
	if (!pos) {
		pos = GetTailPosition();
	}
	return(GetAt(pos));
}

IMPLEMENT_DYNAMIC(CPlayerPlaylistBar, CPlayerBar)

CPlayerPlaylistBar::CPlayerPlaylistBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_list(0)
	, m_nTimeColWidth(0)
	, m_bDragging(FALSE)
	, m_bHiddenDueToFullscreen(false)
	, m_bVisible(false)
{
}

CPlayerPlaylistBar::~CPlayerPlaylistBar()
{
}

BOOL CPlayerPlaylistBar::Create(CWnd* pParentWnd, UINT defDockBarID)
{
	if (!__super::Create(ResStr(IDS_PLAYLIST_CAPTION), pParentWnd, ID_VIEW_PLAYLIST, defDockBarID, L"Playlist")) {
		return FALSE;
	}

	m_list.CreateEx(
		0,//less margins//WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP
		| LVS_OWNERDRAWFIXED
		| LVS_NOCOLUMNHEADER
		| LVS_EDITLABELS
		| LVS_REPORT | /*LVS_SINGLESEL|*/ LVS_AUTOARRANGE | LVS_NOSORTHEADER, // TODO: remove LVS_SINGLESEL and implement multiple item repositioning (dragging is ready)
		CRect(0,0,100,100), this, IDC_PLAYLIST);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_list.InsertColumn(COL_NAME, L"Name", LVCFMT_LEFT);
	m_list.InsertColumn(COL_TIME, L"Time", LVCFMT_RIGHT);

	ScaleFontInternal();

	m_fakeImageList.Create(1, 16, ILC_COLOR4, 10, 10);
	m_list.SetImageList(&m_fakeImageList, LVSIL_SMALL);

	return TRUE;
}

static void GetNonClientMetrics(NONCLIENTMETRICS* ncm)
{
	ZeroMemory(ncm, sizeof(NONCLIENTMETRICS));
	ncm->cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm->cbSize, ncm, 0));
}

static void GetMessageFont(LOGFONT* lf)
{
	NONCLIENTMETRICS ncm;
	GetNonClientMetrics(&ncm);
	*lf = ncm.lfMessageFont;
	ASSERT(lf->lfHeight);
}

void CPlayerPlaylistBar::ScaleFontInternal()
{
	LOGFONT lf = { 0 };
	GetMessageFont(&lf);
	lf.lfHeight = m_pMainFrame->ScaleSystemToMonitorY(lf.lfHeight);

	m_font.DeleteObject();
	if (m_font.CreateFontIndirectW(&lf)) {
		m_list.SetFont(&m_font);
	}

	CDC* pDC = m_list.GetDC();
	CFont* old = pDC->SelectObject(GetFont());
	m_nTimeColWidth = pDC->GetTextExtent(L"000:00:00").cx + m_pMainFrame->ScaleX(5);
	pDC->SelectObject(old);
	m_list.ReleaseDC(pDC);

	m_list.SetColumnWidth(COL_TIME, m_nTimeColWidth);
}

void CPlayerPlaylistBar::ScaleFont()
{
	ScaleFontInternal();
	ResizeListColumn();
}

BOOL CPlayerPlaylistBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CSizingControlBarG::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.dwExStyle |= WS_EX_ACCEPTFILES;

	return TRUE;
}

BOOL CPlayerPlaylistBar::PreTranslateMessage(MSG* pMsg)
{
	if (IsWindow(pMsg->hwnd) && IsVisible() && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
		if (pMsg->message == WM_KEYDOWN) {
			if (pMsg->wParam == VK_ESCAPE) {
				GetParentFrame()->ShowControlBar(this, FALSE, TRUE);

				return TRUE;
			} else if (pMsg->wParam == VK_RETURN) {
				if (m_list.GetSelectedCount() == 1) {
					const int item = m_list.GetNextItem(-1, LVNI_SELECTED);

					m_pl.SetPos(FindPos(item));
					m_pMainFrame->OpenCurPlaylistItem();
					AfxGetMainWnd()->SetFocus();

					return TRUE;
				}
			} else if (pMsg->wParam == '7' && GetKeyState(VK_CONTROL) < 0) {
				GetParentFrame()->ShowControlBar(this, FALSE, TRUE);

				return TRUE;
			}
		}

		if (pMsg->message == WM_CHAR) {
			WCHAR chr = static_cast<WCHAR>(pMsg->wParam);
			switch (chr) {
				case 0x01: // Ctrl+A - select all
					if (GetKeyState(VK_CONTROL) < 0) {
						m_list.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
					}
					break;
				case 0x09: // Ctrl+I - inverse selection
					if (GetKeyState(VK_CONTROL) < 0) {
						for (int nItem = 0; nItem < m_list.GetItemCount(); nItem++) {
							m_list.SetItemState(nItem, ~m_list.GetItemState(nItem, LVIS_SELECTED), LVIS_SELECTED);
						}
					}
					break;
			}
		}

		if (IsDialogMessage(pMsg)) {
			return TRUE;
		}
	}

	return CSizingControlBarG::PreTranslateMessage(pMsg);
}

void CPlayerPlaylistBar::LoadState(CFrameWnd *pParent)
{
	CString section = L"ToolBars\\" + m_strSettingName;

	if (AfxGetMyApp()->GetProfileInt(section, L"Visible", FALSE)) {
		ShowWindow(SW_SHOW);
		m_bVisible = true;
	}

	__super::LoadState(pParent);
}

void CPlayerPlaylistBar::SaveState()
{
	__super::SaveState();

	CString section = L"ToolBars\\" + m_strSettingName;

	AfxGetMyApp()->WriteProfileInt(section, L"Visible",
								 IsWindowVisible() || (AfxGetAppSettings().bHidePlaylistFullScreen && m_bHiddenDueToFullscreen));
}

bool CPlayerPlaylistBar::IsHiddenDueToFullscreen() const
{
	return m_bHiddenDueToFullscreen;
}

void CPlayerPlaylistBar::SetHiddenDueToFullscreen(bool bHiddenDueToFullscreen)
{
	m_bHiddenDueToFullscreen = bHiddenDueToFullscreen;
}

void CPlayerPlaylistBar::AddItem(CString fn, CSubtitleItemList* subs)
{
	std::list<CString> sl;
	sl.push_back(fn);
	AddItem(sl, subs);
}

void CPlayerPlaylistBar::AddItem(std::list<CString>& fns, CSubtitleItemList* subs)
{
	CPlaylistItem pli;

	for (CString fn : fns) {
		if (!fn.Trim().IsEmpty()) {
			pli.m_fns.push_back(MakePath(fn));
		}
	}

	if (subs) {
		for (CString sub : *subs) {
			if (!sub.Trim().IsEmpty()) {
				pli.m_subs.push_back(sub);
			}
		}
	}

	pli.AutoLoadFiles();
	if (pli.m_fns.size() == 1) {
		Youtube::YoutubeFields y_fields;
		if (Youtube::Parse_URL(pli.m_fns.front(), y_fields)) {
			pli.m_label    = y_fields.title;
			pli.m_duration = y_fields.duration;
		}
	}

	m_pl.AddTail(pli);
}

static bool SearchFiles(CString path, std::list<CString>& sl, bool bSingleElement)
{
	if (path.Find(L"://") >= 0
			|| !::PathIsDirectoryW(path)) {
		return false;
	}

	sl.clear();

	CMediaFormats& mf = AfxGetAppSettings().m_Formats;

	path.Trim();
	path = AddSlash(path);
	const CString mask = path + L"*.*";

	{
		WIN32_FIND_DATAW fd;
		HANDLE h = FindFirstFileW(mask, &fd);
		if (h != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (bSingleElement
							&& (_wcsicmp(fd.cFileName, L"VIDEO_TS") == 0 || _wcsicmp(fd.cFileName, L"BDMV") == 0)) {
						SearchFiles(path + fd.cFileName, sl, bSingleElement);
					}
					continue;
				}

				const CString ext = GetFileExt(fd.cFileName).MakeLower();
				if (mf.FindExt(ext)) {
					for (size_t i = 0; i < mf.GetCount(); i++) {
						CMediaFormatCategory& mfc = mf.GetAt(i);
						/* playlist files are skipped when playing the contents of an entire directory */
						if (mfc.FindExt(ext) && mf[i].GetFileType() != TPlaylist) {
							sl.push_back(path + fd.cFileName);
							break;
						}
					}
				}

			} while (FindNextFileW(h, &fd));

			FindClose(h);

			if (sl.size() == 0 && mask.Find(L":\\") == 1) {
				if (CDROM_VideoCD == GetCDROMType(mask[0], sl)) {
					sl.clear();
				}
			}
		}
	}

	sl.sort([](const CString& a, const CString& b) {
		return (StrCmpLogicalW(a, b) < 0);
	});

	return(sl.size() > 1
		   || sl.size() == 1 && sl.front().CompareNoCase(mask)
		   || sl.size() == 0 && mask.FindOneOf(L"?*") >= 0);
}

void CPlayerPlaylistBar::ParsePlayList(CString fn, CSubtitleItemList* subs, bool bCheck/* = true*/)
{
	std::list<CString> sl;
	sl.push_back(fn);
	ParsePlayList(sl, subs, bCheck);
}

void CPlayerPlaylistBar::ResolveLinkFiles(std::list<CString> &fns)
{
	// resolve .lnk files

	CComPtr<IShellLink> pSL;
	pSL.CoCreateInstance(CLSID_ShellLink);
	CComQIPtr<IPersistFile> pPF = pSL;

	if (pSL && pPF) {
		for (auto& fn : fns) {
			if (CPath(fn).GetExtension().MakeLower() == L".lnk") {
				WCHAR buff[MAX_PATH] = { 0 };
				if (SUCCEEDED(pPF->Load(fn, STGM_READ))
						&& SUCCEEDED(pSL->Resolve(nullptr, SLR_ANY_MATCH | SLR_NO_UI))
						&& SUCCEEDED(pSL->GetPath(buff, _countof(buff), nullptr, 0))) {
					CString fnResolved(buff);
					if (!fnResolved.IsEmpty()) {
						fn = fnResolved;
					}
				}
			}
		}
	}
}

void CPlayerPlaylistBar::ParsePlayList(std::list<CString>& fns, CSubtitleItemList* subs, bool bCheck/* = true*/)
{
	if (fns.empty()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	ResolveLinkFiles(fns);

	if (bCheck && !Youtube::CheckURL(fns.front())) {
		std::list<CString> sl;
		if (SearchFiles(fns.front(), sl, m_bSingleElement)) {
			bool bDVD_BD = false;
			for (const auto& fn : sl) {
				if (m_pMainFrame->CheckBD(fn) || m_pMainFrame->CheckDVD(fn)) {
					fns.clear();
					fns.push_front(fn);
					bDVD_BD = true;
					break;
				}
			}

			if (!bDVD_BD) {
				if (sl.size() > 1) {
					subs = nullptr;
				}
				for (const auto& fn : sl) {
					ParsePlayList(fn, subs);
				}
				return;
			}
		}

		const CString fn = fns.front();
		Content::Online::Clear(fn);

		std::list<CString> redir;
		const CString ct = Content::GetType(fn, &redir);
		if (!redir.empty()) {
			for (const auto& r : redir) {
				ParsePlayList(r, subs);
			}
			return;
		}

		if (m_pMainFrame->CheckBD(fn)) {
			AddItem(fns, subs);

			CString empty, label;
			m_pMainFrame->MakeBDLabel(fn, empty, &label);
			m_pl.GetTail().m_label = label;
			return;
		} else if (m_pMainFrame->CheckDVD(fn)) {
			AddItem(fns, subs);
			CString empty, label;
			m_pMainFrame->MakeDVDLabel(fn, empty, &label);
			m_pl.GetTail().m_label = label;
			return;
		} else if (ct == L"application/x-mpc-playlist") {
			ParseMPCPlayList(fn);
			return;
		} else if (ct == L"audio/x-mpegurl" || ct == L"application/http-live-streaming-m3u") {
			ParseM3UPlayList(fn);
			return;
		} else if (ct == L"application/x-cue-metadata") {
			if (ParseCUEPlayList(fn)) {
				return;
			}
		}
	}

	AddItem(fns, subs);
}

static CString CombinePath(CPath p, CString fn)
{
	if (fn.Find(':') >= 0 || fn.Find(L"\\") == 0) {
		return fn;
	}

	p.Append(CPath(fn));
	CString out(p);
	if (out.Find(L"://")) {
		out.Replace('\\', '/');
	}
	return out;
}

bool CPlayerPlaylistBar::ParseMPCPlayList(CString fn)
{
	m_nSelected_idx = INT_MAX;

	CString str;
	CAtlMap<int, CPlaylistItem> pli;
	std::vector<int> idx;
	int selected_idx = -1;

	CWebTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn) || !f.ReadString(str) || str != L"MPCPLAYLIST" || f.GetLength() > MEGABYTE) {
		return false;
	}

	CPath base(fn);
	base.RemoveFileSpec();

	while (f.ReadString(str)) {
		std::list<CString> sl;
		Explode(str, sl, L',', 3);
		if (sl.size() != 3) {
			continue;
		}

		if (int i = _wtoi(sl.front())) {
			sl.pop_front();
			CString key = sl.front();
			sl.pop_front();
			CString value = sl.front();
			sl.pop_front();

			if (key == L"type") {
				pli[i].m_type = (CPlaylistItem::type_t)_wtol(value);
				idx.push_back(i);
			} else if (key == L"label") {
				pli[i].m_label = value;
			} else if (key == L"time") {
				pli[i].m_duration = StringToReftime2(value);
			} else if (key == L"selected") {
				if (value == L"1") {
					selected_idx = i - 1;
				}
			} else if (key == L"filename") {
				value = MakePath(CombinePath(base, value));
				pli[i].m_fns.push_back(value);
			} else if (key == L"subtitle") {
				value = CombinePath(base, value);
				pli[i].m_subs.push_back(value);
			} else if (key == L"video") {
				while (pli[i].m_fns.size() < 2) {
					pli[i].m_fns.push_back(L"");
				}
				pli[i].m_fns.front() = value;
			} else if (key == L"audio") {
				while (pli[i].m_fns.size() < 2) {
					pli[i].m_fns.push_back(L"");
				}
				pli[i].m_fns.back() = value;
			} else if (key == L"vinput") {
				pli[i].m_vinput = _wtol(value);
			} else if (key == L"vchannel") {
				pli[i].m_vchannel = _wtol(value);
			} else if (key == L"ainput") {
				pli[i].m_ainput = _wtol(value);
			} else if (key == L"country") {
				pli[i].m_country = _wtol(value);
			}
		}
	}

	const BOOL bIsEmpty = m_pl.IsEmpty();

	std::sort(idx.begin(), idx.end());

	for (size_t i = 0; i < idx.size(); i++) {
		m_pl.AddTail(pli[idx[i]]);
	}

	if (bIsEmpty && selected_idx >= 0 && selected_idx < m_pl.GetCount()) {
		m_nSelected_idx = selected_idx - 1;
	}

	return pli.GetCount() > 0;
}

bool CPlayerPlaylistBar::SaveMPCPlayList(CString fn, CTextFile::enc e, bool fRemovePath)
{
	CTextFile f;
	if (!f.Save(fn, e)) {
		return false;
	}

	f.WriteString(L"MPCPLAYLIST\n");

	POSITION cur_pos = m_pl.GetPos();

	POSITION pos = m_pl.GetHeadPosition();
	for (int i = 1; pos; i++) {
		bool selected = (cur_pos == pos) && (m_pl.GetCount() > 1);
		CPlaylistItem& pli = m_pl.GetNext(pos);

		CString idx;
		idx.Format(L"%d", i);

		CString str;
		str.Format(L"%d,type,%d", i, pli.m_type);
		f.WriteString(str + L"\n");

		if (!pli.m_label.IsEmpty()) {
			f.WriteString(idx + L",label," + pli.m_label + L"\n");
		}

		if (pli.m_duration > 0) {
			f.WriteString(idx + L",time," + pli.GetLabel(1) + L"\n");
		}

		if (selected) {
			f.WriteString(idx + L",selected,1\n");
		}

		if (pli.m_type == CPlaylistItem::file) {
			for (const auto& fi : pli.m_fns) {
				CString fn = fi.GetName();
				if (fRemovePath) {
					CPath p(fn);
					p.StripPath();
					fn = (LPCTSTR)p;
				}
				f.WriteString(idx + L",filename," + fn + L"\n");
			}

			for (const auto& si : pli.m_subs) {
				CString fn = si.GetName();
				if (fRemovePath) {
					CPath p(fn);
					p.StripPath();
					fn = (LPCTSTR)p;
				}
				f.WriteString(idx + L",subtitle," + fn + L"\n");
			}
		} else if (pli.m_type == CPlaylistItem::device && pli.m_fns.size() == 2) {
			f.WriteString(idx + L",video," + pli.m_fns.front().GetName() + L"\n");
			f.WriteString(idx + L",audio," + pli.m_fns.back().GetName() + L"\n");
			str.Format(L"%d,vinput,%d", i, pli.m_vinput);
			f.WriteString(str + L"\n");
			str.Format(L"%d,vchannel,%d", i, pli.m_vchannel);
			f.WriteString(str + L"\n");
			str.Format(L"%d,ainput,%d", i, pli.m_ainput);
			f.WriteString(str + L"\n");
			str.Format(L"%d,country,%d", i, pli.m_country);
			f.WriteString(str + L"\n");
		}
	}

	return true;
}

bool CPlayerPlaylistBar::ParseM3UPlayList(CString fn)
{
	CWebTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn)) {
		return false;
	}

	CPath base(fn);
	if (fn.Find(L"://") > 0) {
		CString tmp(fn);
		tmp.Truncate(tmp.ReverseFind('/'));
		base = (CPath)tmp;
	} else {
		base.RemoveFileSpec();
	}

	CPlaylistItem *pli = nullptr;

	INT_PTR c = m_pl.GetCount();
	CString str;
	while (f.ReadString(str)) {
		str = str.Trim();

		if (str.IsEmpty() || (str.Find(L"#EXTM3U") == 0)) {
			continue;
		}

		if (!pli) {
			pli = DNew CPlaylistItem;
		}

		if (str.Find(L"#") == 0) {
			if (str.Find(L"#EXTINF:") == 0) {
				int k = str.Find(L"#EXTINF:") + CString(L"#EXTINF:").GetLength();
				str = str.Mid(k, str.GetLength() - k);

				k = str.Find(L",");
				if (k > 0) {
					CString tmp = str.Left(k);
					int dur = 0;
					if (swscanf_s(tmp, L"%dx", &dur) == 1) {
						k++;
						str = str.Mid(k, str.GetLength() - k);
					}
				}
				pli->m_label = str.Trim();
			} else if (str.Find(L"#EXT-X-STREAM-INF:") == 0) {
				int k = str.Find(L"#EXT-X-STREAM-INF:") + CString(L"#EXT-X-STREAM-INF:").GetLength();
				str = str.Mid(k, str.GetLength() - k);
				pli->m_label = str.Trim();
			}
		} else {
			CString fullPath = MakePath(CombinePath(base, str));
			if (GetFileExt(fullPath).MakeLower() == L".m3u") {
				SAFE_DELETE(pli);

				ParseM3UPlayList(fullPath);
				continue;
			}

			pli->m_fns.push_back(MakePath(CombinePath(base, str)));
			m_pl.AddTail(*pli);

			SAFE_DELETE(pli);
		}
	}

	SAFE_DELETE(pli);

	return (m_pl.GetCount() > c);
}

bool CPlayerPlaylistBar::ParseCUEPlayList(CString fn)
{
	CString Title, Performer;
	std::list<CUETrack> CUETrackList;
	if (!ParseCUESheetFile(fn, CUETrackList, Title, Performer)) {
		return false;
	}

	std::list<CString> fileNames;
	for (const auto& cueTrack : CUETrackList) {
		if (std::find(fileNames.cbegin(), fileNames.cend(), cueTrack.m_fn) == fileNames.cend()) {
			fileNames.push_back(cueTrack.m_fn);
		}
	}

	CPath base(fn);
	base.RemoveFileSpec();

	INT_PTR c = m_pl.GetCount();

	for (const auto& fName : fileNames){
		CString fullPath = MakePath(CombinePath(base, fName));
		BOOL bExists = TRUE;
		if (!::PathFileExistsW(fullPath)) {
			CString ext = GetFileExt(fullPath);
			bExists = FALSE;

			CString filter;
			CAtlArray<CString> mask;
			AfxGetAppSettings().m_Formats.GetAudioFilter(filter, mask);
			std::list<CString> sl;
			Explode(mask[0], sl, L';');

			for (CString _mask : sl) {
				_mask.Delete(0, 1);

				CString newPath = fullPath;
				newPath.Replace(ext, _mask);

				if (::PathFileExistsW(newPath)) {
					fullPath = newPath;
					bExists = TRUE;
					break;
				}
			}
		}

		if (bExists) {
			CPlaylistItem pli;
			MakeCUETitle(pli.m_label, Title, Performer);

			CFileItem fi = fullPath;
			BOOL bFirst = TRUE;
			for (const auto& cueTrack : CUETrackList) {
				if (cueTrack.m_fn == fName) {
					CString cueTrackTitle;
					MakeCUETitle(cueTrackTitle, cueTrack.m_Title, cueTrack.m_Performer, cueTrack.m_trackNum);
					fi.AddChapter(Chapters{cueTrackTitle, cueTrack.m_rt});

					if (bFirst && fileNames.size() > 1) {
						MakeCUETitle(pli.m_label, cueTrack.m_Title, cueTrack.m_Performer);
					}
					bFirst = FALSE;
				}
			}

			fi.SetTitle(pli.m_label);

			ChaptersList chaplist;
			fi.GetChapters(chaplist);
			BOOL bTrustedChap = FALSE;
			for (size_t i = 0; i < chaplist.size(); i++) {
				if (chaplist[i].rt > 0) {
					bTrustedChap = TRUE;
					break;
				}
			}
			if (!bTrustedChap) {
				fi.ClearChapter();
			}

			pli.m_fns.push_front(fi);

			m_pl.AddTail(pli);
		}
	}

	return (m_pl.GetCount() > c);
}

void CPlayerPlaylistBar::Refresh()
{
	SetupList();
	ResizeListColumn();
}

bool CPlayerPlaylistBar::Empty()
{
	bool bWasPlaying = m_pl.RemoveAll();
	m_list.DeleteAllItems();
	SavePlaylist();

	return bWasPlaying;
}

void CPlayerPlaylistBar::Open(CString fn)
{
	std::list<CString> fns;
	fns.push_front(fn);

	m_bSingleElement = true;
	Open(fns, false);
	m_bSingleElement = false;
}

void CPlayerPlaylistBar::Open(std::list<CString>& fns, bool fMulti, CSubtitleItemList* subs/* = nullptr*/, bool bCheck/* = true*/)
{
	m_nSelected_idx = INT_MAX;

	ResolveLinkFiles(fns);
	Empty();
	Append(fns, fMulti, subs, bCheck);
}

void CPlayerPlaylistBar::Append(CString fn)
{
	std::list<CString> fns;
	fns.push_front(fn);
	Append(fns, false);
}

void CPlayerPlaylistBar::Append(std::list<CString>& fns, bool fMulti, CSubtitleItemList* subs/* = nullptr*/, bool bCheck/* = true*/)
{
	INT_PTR idx = -1;

	POSITION pos = m_pl.GetHeadPosition();
	while (pos) {
		idx++;
		m_pl.GetNext(pos);
	}

	if (fMulti) {
		ASSERT(subs == nullptr || subs->size() == 0);

		for (const auto& fn : fns) {
			ParsePlayList(fn, nullptr, bCheck);
		}
	} else {
		ParsePlayList(fns, subs, bCheck);
	}

	Refresh();
	EnsureVisible(m_pl.FindIndex((m_nSelected_idx != INT_MAX ? m_nSelected_idx : idx) + 1));
	SavePlaylist();

	UpdateList();
}

void CPlayerPlaylistBar::Append(CFileItemList& fis)
{
	INT_PTR idx = -1;

	POSITION pos = m_pl.GetHeadPosition();
	while (pos) {
		idx++;
		m_pl.GetNext(pos);
	}

	for (const auto& fi : fis) {
		CPlaylistItem pli;
		pli.m_fns.push_front(fi.GetName());
		pli.m_label = fi.GetTitle();
		m_pl.AddTail(pli);
	}

	Refresh();
	EnsureVisible(m_pl.FindIndex(idx + 1));
	SavePlaylist();

	UpdateList();
}

bool CPlayerPlaylistBar::Replace(CString filename, std::list<CString>& fns)
{
	if (filename.IsEmpty()) {
		return false;
	}

	CPlaylistItem pli;
	for (CString fn : fns) {
		if (!fn.Trim().IsEmpty()) {
			pli.m_fns.push_back(MakePath(fn));
		}
	}
	pli.AutoLoadFiles();

	POSITION pos = m_pl.GetHeadPosition();
	while (pos) {
		CPlaylistItem& pli2 = m_pl.GetAt(pos);
		if (pli2.FindFile(filename)) {
			m_pl.SetAt(pos, pli);
			m_pl.SetPos(pos);
			EnsureVisible(pos, false);
			return true;
		}
		m_pl.GetNext(pos);
	}
	return false;
}

void CPlayerPlaylistBar::Open(CStringW vdn, CStringW adn, int vinput, int vchannel, int ainput)
{
	Empty();
	Append(vdn, adn, vinput, vchannel, ainput);
}

void CPlayerPlaylistBar::Append(CStringW vdn, CStringW adn, int vinput, int vchannel, int ainput)
{
	CPlaylistItem pli;
	pli.m_type = CPlaylistItem::device;
	pli.m_fns.push_back(vdn);
	pli.m_fns.push_back(adn);
	pli.m_vinput = vinput;
	pli.m_vchannel = vchannel;
	pli.m_ainput = ainput;
	std::list<CStringW> sl;
	CStringW vfn = GetFriendlyName(vdn);
	CStringW afn = GetFriendlyName(adn);
	if (!vfn.IsEmpty()) {
		sl.push_back(vfn);
	}
	if (!afn.IsEmpty()) {
		sl.push_back(afn);
	}
	CStringW label = Implode(sl, '|');
	label.Replace(L"|", L" - ");
	pli.m_label = CString(label);
	m_pl.AddTail(pli);

	Refresh();
	EnsureVisible(m_pl.GetTailPosition());
	SavePlaylist();
}

void CPlayerPlaylistBar::SetupList()
{
	m_list.DeleteAllItems();

	POSITION pos = m_pl.GetHeadPosition();
	for (int i = 0; pos; i++) {
		CPlaylistItem& pli = m_pl.GetAt(pos);
		m_list.SetItemData(m_list.InsertItem(i, pli.GetLabel()), (DWORD_PTR)pos);
		m_list.SetItemText(i, COL_TIME, pli.GetLabel(1));
		m_pl.GetNext(pos);
	}
}

void CPlayerPlaylistBar::UpdateList()
{
	POSITION pos = m_pl.GetHeadPosition();
	for (int i = 0, j = m_list.GetItemCount(); pos && i < j; i++) {
		CPlaylistItem& pli = m_pl.GetAt(pos);
		m_list.SetItemData(i, (DWORD_PTR)pos);
		m_list.SetItemText(i, COL_NAME, pli.GetLabel(0));
		m_list.SetItemText(i, COL_TIME, pli.GetLabel(1));
		m_pl.GetNext(pos);
	}
}

void CPlayerPlaylistBar::EnsureVisible(POSITION pos, bool bMatchPos)
{
	int i = FindItem(pos);
	if (i < 0) {
		return;
	}

	m_list.SetItemState(-1, 0, LVIS_SELECTED);
	m_list.EnsureVisible(i - 1 + bMatchPos, TRUE);
	m_list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);

	m_list.Invalidate();
}

int CPlayerPlaylistBar::FindItem(POSITION pos)
{
	for (int i = 0; i < m_list.GetItemCount(); i++) {
		if ((POSITION)m_list.GetItemData(i) == pos) {
			return i;
		}
	}

	return -1;
}

POSITION CPlayerPlaylistBar::FindPos(int i)
{
	return i < 0 ? nullptr : (POSITION)m_list.GetItemData(i);
}

int CPlayerPlaylistBar::GetCount()
{
	return m_pl.GetCount();
}

int CPlayerPlaylistBar::GetSelIdx()
{
	return FindItem(m_pl.GetPos());
}

void CPlayerPlaylistBar::SetSelIdx(int i, bool bUpdatePos/* = false*/)
{
	POSITION pos = FindPos(i);
	m_pl.SetPos(pos);
	if (bUpdatePos) {
		EnsureVisible(pos, false);
	}
}

bool CPlayerPlaylistBar::IsAtEnd()
{
	POSITION pos = m_pl.GetPos(), tail = m_pl.GetTailPosition();
	bool isAtEnd = (pos && pos == tail);

	if (!isAtEnd && pos) {
		isAtEnd = m_pl.GetNextWrap(pos).m_fInvalid;
		while (isAtEnd && pos && pos != tail) {
			isAtEnd = m_pl.GetNextWrap(pos).m_fInvalid;
		}
	}

	return isAtEnd;
}

bool CPlayerPlaylistBar::GetCur(CPlaylistItem& pli)
{
	if (!m_pl.GetPos()) {
		return false;
	}
	pli = m_pl.GetAt(m_pl.GetPos());
	return true;
}

CPlaylistItem* CPlayerPlaylistBar::GetCur()
{
	return !m_pl.GetPos() ? nullptr : &m_pl.GetAt(m_pl.GetPos());
}

CString CPlayerPlaylistBar::GetCurFileName()
{
	CString fn;
	CPlaylistItem* pli = GetCur();
	if (pli && !pli->m_fns.empty()) {
		fn = pli->m_fns.front().GetName();
	}
	return fn;
}

bool CPlayerPlaylistBar::SetNext()
{
	POSITION pos = m_pl.GetPos(), org = pos;
	while (m_pl.GetNextWrap(pos).m_fInvalid && pos != org) {
		;
	}
	UpdateList();
	m_pl.SetPos(pos);
	EnsureVisible(pos);

	return (pos != org);
}

bool CPlayerPlaylistBar::SetPrev()
{
	POSITION pos = m_pl.GetPos(), org = pos;
	while (m_pl.GetPrevWrap(pos).m_fInvalid && pos != org) {
		;
	}
	m_pl.SetPos(pos);
	EnsureVisible(pos);

	return (pos != org);
}

void CPlayerPlaylistBar::SetFirstSelected()
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	if (pos) {
		pos = FindPos(m_list.GetNextSelectedItem(pos));
	} else {
		pos = m_pl.GetTailPosition();
		POSITION org = pos;
		while (m_pl.GetNextWrap(pos).m_fInvalid && pos != org) {
			;
		}
	}
	UpdateList();
	m_pl.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetFirst()
{
	POSITION pos = m_pl.GetTailPosition(), org = pos;
	while (m_pl.GetNextWrap(pos).m_fInvalid && pos != org) {
		;
	}
	UpdateList();
	m_pl.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetLast()
{
	POSITION pos = m_pl.GetHeadPosition(), org = pos;
	while (m_pl.GetPrevWrap(pos).m_fInvalid && pos != org) {
		;
	}
	m_pl.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetCurValid(bool fValid)
{
	POSITION pos = m_pl.GetPos();
	if (pos) {
		m_pl.GetAt(pos).m_fInvalid = !fValid;
	}
	UpdateList();
}

void CPlayerPlaylistBar::SetCurLabel(CString label)
{
	POSITION pos = m_pl.GetPos();
	if (pos) {
		m_pl.GetAt(pos).m_label = label;
	}
	UpdateList();
}

void CPlayerPlaylistBar::SetCurTime(REFERENCE_TIME rt)
{
	POSITION pos = m_pl.GetPos();
	if (pos) {
		CPlaylistItem& pli = m_pl.GetAt(pos);
		pli.m_duration = rt;
		m_list.SetItemText(FindItem(pos), COL_TIME, pli.GetLabel(1));
	}
	UpdateList();
}

OpenMediaData* CPlayerPlaylistBar::GetCurOMD(REFERENCE_TIME rtStart)
{
	CPlaylistItem* pli = GetCur();
	if (pli == nullptr) {
		return nullptr;
	}

	pli->AutoLoadFiles();

	CString fn = pli->m_fns.front().GetName().MakeLower();

	if (fn.Find(L"video_ts.ifo") >= 0
			|| fn.Find(L".ratdvd") >= 0) {
		if (OpenDVDData* p = DNew OpenDVDData()) {
			p->path = pli->m_fns.front().GetName();
			p->subs = pli->m_subs;
			return p;
		}
	}

	if (pli->m_type == CPlaylistItem::device) {
		if (OpenDeviceData* p = DNew OpenDeviceData()) {
			auto it = pli->m_fns.begin();
			for (int i = 0; i < _countof(p->DisplayName) && it != pli->m_fns.end(); ++i, ++it) {
				p->DisplayName[i] = (*it).GetName();
			}

			p->vinput = pli->m_vinput;
			p->vchannel = pli->m_vchannel;
			p->ainput = pli->m_ainput;
			return p;
		}
	} else {
		if (OpenFileData* p = DNew OpenFileData()) {
			p->fns = pli->m_fns;
			p->subs = pli->m_subs;
			p->rtStart = rtStart;
			return p;
		}
	}

	return nullptr;
}

bool CPlayerPlaylistBar::SelectFileInPlaylist(CString filename)
{
	if (filename.IsEmpty()) {
		return false;
	}
	POSITION pos = m_pl.GetHeadPosition();
	while (pos) {
		CPlaylistItem& pli = m_pl.GetAt(pos);
		if (pli.FindFile(filename)) {
			m_pl.SetPos(pos);
			EnsureVisible(pos, false);
			return true;
		}
		m_pl.GetNext(pos);
	}
	return false;
}

void CPlayerPlaylistBar::LoadPlaylist(CString filename)
{
	CString base;

	if (AfxGetMyApp()->GetAppSavePath(base)) {
		base.Append(L"default.mpcpl");

		if (::PathFileExistsW(base)) {
			if (AfxGetAppSettings().bRememberPlaylistItems) {
				ParseMPCPlayList(base);
				Refresh();
				if (m_nSelected_idx != INT_MAX) {
					SetSelIdx(m_nSelected_idx + 1, true);
				} else {
					SelectFileInPlaylist(filename);
				}
			} else {
				::DeleteFileW(base);
			}
		}
	}
}

void CPlayerPlaylistBar::SavePlaylist()
{
	CString base;

	if (AfxGetMyApp()->GetAppSavePath(base)) {
		CString file = base + L"default.mpcpl";

		if (AfxGetAppSettings().bRememberPlaylistItems) {
			// Only create this folder when needed
			if (!::PathFileExistsW(base)) {
				::CreateDirectoryW(base, nullptr);
			}

			SaveMPCPlayList(file, CTextFile::UTF8, false);
		} else if (::PathFileExistsW(file)) {
			::DeleteFileW(file);
		}
	}
}

BEGIN_MESSAGE_MAP(CPlayerPlaylistBar, CSizingControlBarG)
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_KEYDOWN, IDC_PLAYLIST, OnLvnKeyDown)
	ON_NOTIFY(NM_DBLCLK, IDC_PLAYLIST, OnNMDblclkList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PLAYLIST, OnCustomdrawList)
	ON_WM_MEASUREITEM()
	ON_WM_DRAWITEM()
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlayPlay)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_PLAYLIST, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_PLAYLIST, OnLvnEndlabeleditList)
END_MESSAGE_MAP()

// CPlayerPlaylistBar message handlers

void CPlayerPlaylistBar::ResizeListColumn()
{
	if (::IsWindow(m_list.m_hWnd)) {
		CRect r;
		GetClientRect(r);
		r.DeflateRect(2, 2);
		m_list.SetRedraw(FALSE);
		m_list.MoveWindow(r);
		m_list.GetClientRect(r);
		m_list.SetColumnWidth(COL_NAME, r.Width() - m_nTimeColWidth);
		m_list.SetRedraw(TRUE);
	}
}

void CPlayerPlaylistBar::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	__super::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_HIDEWINDOW) {
		m_bVisible = false;
		GetParentFrame()->SetFocus();
	} else if (lpwndpos->flags & SWP_SHOWWINDOW) {
		m_bVisible = true;
	}
}

void CPlayerPlaylistBar::OnSize(UINT nType, int cx, int cy)
{
	CSizingControlBarG::OnSize(nType, cx, cy);

	ResizeListColumn();
}

void CPlayerPlaylistBar::OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	*pResult = FALSE;

	std::vector<int> items;
	items.reserve(m_list.GetSelectedCount());
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos) {
		items.push_back(m_list.GetNextSelectedItem(pos));
	}

	if (pLVKeyDown->wVKey == VK_DELETE && items.size() > 0) {
		for (int i = (int)items.size() - 1; i >= 0; --i) {
			if (m_pl.RemoveAt(FindPos(items[i]))) {
				m_pMainFrame->SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			}
			m_list.DeleteItem(items[i]);
		}

		m_list.SetItemState(-1, 0, LVIS_SELECTED);
		m_list.SetItemState(
			std::max(std::min(items.front(), m_list.GetItemCount() - 1), 0),
			LVIS_SELECTED, LVIS_SELECTED);

		ResizeListColumn();

		*pResult = TRUE;
	} else if (pLVKeyDown->wVKey == VK_SPACE && items.size() == 1) {
		m_pl.SetPos(FindPos(items.front()));
		m_pMainFrame->OpenCurPlaylistItem();
		AfxGetMainWnd()->SetFocus();

		*pResult = TRUE;
	}
}

void CPlayerPlaylistBar::OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;

	if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0) {
		POSITION pos = FindPos(lpnmlv->iItem);
		// If the file is already playing, don't try to restore a previously saved position
		if (m_pMainFrame->GetPlaybackMode() == PM_FILE && pos == m_pl.GetPos()) {
			const CPlaylistItem& pli = m_pl.GetAt(pos);
			AfxGetAppSettings().RemoveFile(pli.m_fns.front());
		} else {
			m_pl.SetPos(pos);
		}

		m_list.Invalidate();
		m_pMainFrame->OpenCurPlaylistItem();
	}

	AfxGetMainWnd()->SetFocus();

	*pResult = 0;
}

void CPlayerPlaylistBar::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	*pResult = CDRF_DODEFAULT;
	CAppSettings& s = AfxGetAppSettings();

	int R, G, B;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage) {
		if (SysVersion::IsWin7orLater()) { // under WinXP cause the hang
			ResizeListColumn();
		}

		if (s.bUseDarkTheme) {
			ThemeRGB(30, 35, 40, R, G, B);
		}

		CRect r;
		GetClientRect(&r);
		FillRect(pLVCD->nmcd.hdc, &r, CBrush(s.bUseDarkTheme ? RGB(R, G, B) : RGB(255, 255, 255)));

		*pResult = CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW;

	}/* else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage) {

		pLVCD->nmcd.uItemState &= ~CDIS_SELECTED;
		pLVCD->nmcd.uItemState &= ~CDIS_FOCUS;

		if (!s.bUseDarkTheme) {
			pLVCD->clrTextBk = m_list.GetItemState(pLVCD->nmcd.dwItemSpec, LVIS_SELECTED) ? 0xf1dacc : CLR_DEFAULT;
		}

		*pResult = CDRF_NOTIFYPOSTPAINT;

	} else if (CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage) {

		if (m_list.GetItemState(pLVCD->nmcd.dwItemSpec, LVIS_SELECTED)) {

			int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
			CRect r;
			m_list.GetItemRect(nItem, &r, LVIR_BOUNDS);

			if (!s.bUseDarkTheme) {
				FrameRect(pLVCD->nmcd.hdc, &r, CBrush(0xc56a31));
			}
		}

		*pResult = CDRF_SKIPDEFAULT;
	}*/
}

void CPlayerPlaylistBar::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	__super::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
	if (m_itemHeight == 0) {
		m_itemHeight = lpMeasureItemStruct->itemHeight;
	}
	lpMeasureItemStruct->itemHeight = m_pMainFrame->ScaleY(m_itemHeight);
}

void CPlayerPlaylistBar::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl != IDC_PLAYLIST) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	int R, G, B;

	int nItem = lpDrawItemStruct->itemID;
	CRect rcItem = lpDrawItemStruct->rcItem;
	POSITION pos = FindPos(nItem);
	if (!pos) {
		ASSERT(FALSE);
		return;
	}

	bool fSelected = pos == m_pl.GetPos();
	CPlaylistItem& pli = m_pl.GetAt(pos);

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	if (!!m_list.GetItemState(nItem, LVIS_SELECTED)) {
		if (s.bUseDarkTheme) {
			ThemeRGB(15, 20, 25, R, G, B);
		}
		FillRect(pDC->m_hDC, rcItem, CBrush(s.bUseDarkTheme ? RGB(R, G, B) : 0x00f1dacc));
		FrameRect(pDC->m_hDC, rcItem, CBrush(s.bUseDarkTheme ? RGB(R, G, B) : 0xc56a31));
	} else {
		if (s.bUseDarkTheme) {
			ThemeRGB(30, 35, 40, R, G, B);
		}
		FillRect(pDC->m_hDC, rcItem, CBrush(s.bUseDarkTheme ? RGB(R, G, B) : RGB(255, 255, 255)));
	}

	COLORREF textcolor = fSelected ? 0xff : 0;

	if (s.bUseDarkTheme) {
		ThemeRGB(135, 140, 145, R, G, B);
		textcolor = fSelected ? s.clrFaceABGR : RGB(R, G, B);
	}

	if (pli.m_fInvalid) {
		textcolor |= 0xA0A0A0;
	}

	CString time = !pli.m_fInvalid ? m_list.GetItemText(nItem, COL_TIME) : L"Invalid";
	CSize timesize(0, 0);
	CPoint timept(rcItem.right, 0);
	if (time.GetLength() > 0) {
		timesize = pDC->GetTextExtent(time);
		if ((3+timesize.cx + 3) < rcItem.Width() / 2) {
			timept = CPoint(rcItem.right - (3 + timesize.cx + 3), (rcItem.top + rcItem.bottom - timesize.cy) / 2);

			pDC->SetTextColor(textcolor);
			pDC->TextOut(timept.x, timept.y, time);
		}
	}

	CString fmt, file;
	fmt.Format(L"%%0%dd. %%s", (int)log10(0.1 + m_pl.GetCount()) + 1);
	file.Format(fmt, nItem+1, m_list.GetItemText(nItem, COL_NAME));
	CSize filesize = pDC->GetTextExtent(file);
	while (3 + filesize.cx + 6 > timept.x && file.GetLength() > 3) {
		file = file.Left(file.GetLength() - 4) + L"...";
		filesize = pDC->GetTextExtent(file);
	}

	if (file.GetLength() > 3) {
		pDC->SetTextColor(textcolor);
		pDC->TextOut(rcItem.left + 3, (rcItem.top + rcItem.bottom - filesize.cy) / 2, file);
	}
}

BOOL CPlayerPlaylistBar::OnPlayPlay(UINT nID)
{
	m_list.Invalidate();
	return FALSE;
}

void CPlayerPlaylistBar::DropFiles(std::list<CString>& slFiles)
{
	SetForegroundWindow();
	m_list.SetFocus();

	m_pMainFrame->ParseDirs(slFiles);

	Append(slFiles, true);
}

void CPlayerPlaylistBar::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	ModifyStyle(WS_EX_ACCEPTFILES, 0);

	m_nDragIndex = ((LPNMLISTVIEW)pNMHDR)->iItem;

	UINT nSelected = m_list.GetSelectedCount();
	m_DragIndexes.clear();
	m_DragIndexes.reserve(nSelected);

	int nItem = -1;
	bool valid = false;
	for (UINT i = 0; i < nSelected; i++) {
		nItem = m_list.GetNextItem(nItem, LVNI_SELECTED);
		m_DragIndexes.push_back(nItem);
		if (nItem == m_nDragIndex) {
			valid = true;
		}
	}

	if (!valid) {
		ASSERT(0);
		m_DragIndexes.resize(1);
		m_DragIndexes.front() = m_nDragIndex;
	}

	CPoint p(0, 0);
	m_pDragImage = m_list.CreateDragImageEx(&p);

	CPoint p2 = ((LPNMLISTVIEW)pNMHDR)->ptAction;

	m_pDragImage->BeginDrag(0, p2 - p);
	m_pDragImage->DragEnter(GetDesktopWindow(), ((LPNMLISTVIEW)pNMHDR)->ptAction);

	m_bDragging = TRUE;
	m_nDropIndex = -1;

	SetCapture();
}

void CPlayerPlaylistBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDragging) {
		m_ptDropPoint = point;
		ClientToScreen(&m_ptDropPoint);

		m_pDragImage->DragMove(m_ptDropPoint);
		m_pDragImage->DragShowNolock(FALSE);

		WindowFromPoint(m_ptDropPoint)->ScreenToClient(&m_ptDropPoint);

		m_pDragImage->DragShowNolock(TRUE);

		{
			int iOverItem = m_list.HitTest(m_ptDropPoint);
			int iTopItem = m_list.GetTopIndex();
			int iBottomItem = m_list.GetBottomIndex();

			if (iOverItem == iTopItem && iTopItem != 0) { // top of list
				SetTimer(1, 100, nullptr);
			} else {
				KillTimer(1);
			}

			if (iOverItem >= iBottomItem && iBottomItem != (m_list.GetItemCount() - 1)) { // bottom of list
				SetTimer(2, 100, nullptr);
			} else {
				KillTimer(2);
			}
		}
	}

	__super::OnMouseMove(nFlags, point);
}

void CPlayerPlaylistBar::OnTimer(UINT_PTR nIDEvent)
{
	int iTopItem = m_list.GetTopIndex();
	int iBottomItem = iTopItem + m_list.GetCountPerPage() - 1;

	if (m_bDragging) {
		m_pDragImage->DragShowNolock(FALSE);

		if (nIDEvent == 1) {
			m_list.EnsureVisible(iTopItem - 1, false);
			m_list.UpdateWindow();
			if (m_list.GetTopIndex() == 0) {
				KillTimer(1);
			}
		} else if (nIDEvent == 2) {
			m_list.EnsureVisible(iBottomItem + 1, false);
			m_list.UpdateWindow();
			if (m_list.GetBottomIndex() == (m_list.GetItemCount() - 1)) {
				KillTimer(2);
			}
		}

		m_pDragImage->DragShowNolock(TRUE);
	}

	__super::OnTimer(nIDEvent);
}

void CPlayerPlaylistBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging) {
		::ReleaseCapture();

		m_bDragging = FALSE;
		m_pDragImage->DragLeave(GetDesktopWindow());
		m_pDragImage->EndDrag();

		delete m_pDragImage;
		m_pDragImage = nullptr;

		KillTimer(1);
		KillTimer(2);

		CPoint pt(point);
		ClientToScreen(&pt);

		if (WindowFromPoint(pt) == &m_list) {
			DropItemOnList();
		}
	}

	ModifyStyle(0, WS_EX_ACCEPTFILES);

	__super::OnLButtonUp(nFlags, point);
}

void CPlayerPlaylistBar::DropItemOnList()
{
	m_ptDropPoint.y += 10;
	m_nDropIndex = m_list.HitTest(CPoint(10, m_ptDropPoint.y));

	if (m_nDropIndex == m_nDragIndex || m_DragIndexes.empty()) {
		return;
	}
	if (m_nDropIndex < 0) {
		m_nDropIndex = m_list.GetItemCount();
	}

	WCHAR szLabel[MAX_PATH];
	LV_ITEM lvi = {};
	lvi.stateMask = LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED;
	lvi.pszText = szLabel;
	lvi.cchTextMax = MAX_PATH;

	const int nColumnCount = ((CHeaderCtrl*)m_list.GetDlgItem(0))->GetItemCount();

	int dropIdx = m_nDropIndex;
	// copy dragged items to new positions
	for (const auto& dragIdx : m_DragIndexes) {
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
		lvi.iItem = dragIdx;
		lvi.iSubItem = 0;
		m_list.GetItem(&lvi);

		lvi.iItem = dropIdx;
		m_list.InsertItem(&lvi);

		// update m_DragIndexes
		for (auto& idx : m_DragIndexes) {
			if (idx >= dropIdx) {
				idx += 1;
			}
		}

		// copy data from other columns
		for (int col = 1; col < nColumnCount; col++) {
			lvi.mask = LVIF_TEXT;
			wcscpy_s(lvi.pszText, MAX_PATH, (LPCTSTR)(m_list.GetItemText(dragIdx, col)));
			lvi.iSubItem = col;
			m_list.SetItem(&lvi);
		}

		dropIdx++;
	}

	// delete dragged items in old positions
	for (int i = (int)m_DragIndexes.size()-1; i >= 0; --i) {
		m_list.DeleteItem(m_DragIndexes[i]);
	}

	std::list<CPlaylistItem> tmp;
	UINT id = (UINT)-1;
	for (int i = 0; i < m_list.GetItemCount(); i++) {
		POSITION pos = (POSITION)m_list.GetItemData(i);
		CPlaylistItem& pli = m_pl.GetAt(pos);
		tmp.push_back(pli);
		if (pos == m_pl.GetPos()) {
			id = pli.m_id;
		}
	}
	m_pl.RemoveAll();
	auto it = tmp.begin();
	for (int i = 0; it != tmp.end(); i++) {
		CPlaylistItem& pli = *it++;
		m_pl.AddTail(pli);
		if (pli.m_id == id) {
			m_pl.SetPos(m_pl.GetTailPosition());
		}
		m_list.SetItemData(i, (DWORD_PTR)m_pl.GetTailPosition());
	}

	ResizeListColumn();
}

BOOL CPlayerPlaylistBar::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	if ((HWND)pTTT->lParam != m_list.m_hWnd) {
		return FALSE;
	}

	int row = ((pNMHDR->idFrom-1) >> 10) & 0x3fffff;
	int col = (pNMHDR->idFrom-1) & 0x3ff;

	if (row < 0 || row >= m_pl.GetCount()) {
		return FALSE;
	}

	CPlaylistItem& pli = m_pl.GetAt(FindPos(row));

	static CString strTipText; // static string
	strTipText.Empty();

	if (col == COL_NAME) {
		for (const auto& fi : pli.m_fns) {
			strTipText += L"\n" + fi.GetName();
		}
		strTipText.Trim();

		if (pli.m_type == CPlaylistItem::device) {
			if (pli.m_vinput >= 0) {
				strTipText.AppendFormat(L"\nVideo Input %d", pli.m_vinput);
			}
			if (pli.m_vchannel >= 0) {
				strTipText.AppendFormat(L"\nVideo Channel %d", pli.m_vchannel);
			}
			if (pli.m_ainput >= 0) {
				strTipText.AppendFormat(L"\nAudio Input %d", pli.m_ainput);
			}
		}

		::SendMessageW(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);
	} else if (col == COL_TIME) {
		return FALSE;
	}

	pTTT->lpszText = (LPWSTR)(LPCWSTR)strTipText;

	*pResult = 0;

	return TRUE;
}

void CPlayerPlaylistBar::OnContextMenu(CWnd* /*pWnd*/, CPoint p)
{
	LVHITTESTINFO lvhti;
	lvhti.pt = p;
	m_list.ScreenToClient(&lvhti.pt);
	m_list.SubItemHitTest(&lvhti);

	POSITION pos = FindPos(lvhti.iItem);
	//bool fSelected = (pos == m_pl.GetPos());
	bool fOnItem = !!(lvhti.flags&LVHT_ONITEM);

	CMenu m;
	m.CreatePopupMenu();

	enum {
		M_OPEN=1, M_ADD, M_REMOVE, M_CLEAR, M_CLIPBOARD, M_SAVEAS,
		M_SORTBYNAME, M_SORTBYPATH, M_RANDOMIZE, M_SORTBYID,
		M_SHUFFLE, M_HIDEFULLSCREEN
	};

	CAppSettings& s = AfxGetAppSettings();

	m.AppendMenu(MF_STRING | (!fOnItem ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_OPEN, ResStr(IDS_PLAYLIST_OPEN));
	m.AppendMenu(MF_STRING | MF_ENABLED, M_ADD, ResStr(IDS_PLAYLIST_ADD));
	m.AppendMenu(MF_STRING | (/*fSelected||*/!fOnItem ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_REMOVE, ResStr(IDS_PLAYLIST_REMOVE));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | (!m_pl.GetCount() ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_CLEAR, ResStr(IDS_PLAYLIST_CLEAR));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | (!fOnItem ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_CLIPBOARD, ResStr(IDS_PLAYLIST_COPYTOCLIPBOARD));
	m.AppendMenu(MF_STRING | (!m_pl.GetCount() ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_SAVEAS, ResStr(IDS_PLAYLIST_SAVEAS));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | (!m_pl.GetCount() ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_SORTBYNAME, ResStr(IDS_PLAYLIST_SORTBYLABEL));
	m.AppendMenu(MF_STRING | (!m_pl.GetCount() ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_SORTBYPATH, ResStr(IDS_PLAYLIST_SORTBYPATH));
	m.AppendMenu(MF_STRING | (!m_pl.GetCount() ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_RANDOMIZE, ResStr(IDS_PLAYLIST_RANDOMIZE));
	m.AppendMenu(MF_STRING | (!m_pl.GetCount() ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED), M_SORTBYID, ResStr(IDS_PLAYLIST_RESTORE));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | MF_ENABLED | (s.bShufflePlaylistItems ? MF_CHECKED : MF_UNCHECKED), M_SHUFFLE, ResStr(IDS_PLAYLIST_SHUFFLE));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | MF_ENABLED | (s.bHidePlaylistFullScreen ? MF_CHECKED : MF_UNCHECKED), M_HIDEFULLSCREEN, ResStr(IDS_PLAYLIST_HIDEFS));

	int nID = (int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this);
	switch (nID) {
		case M_OPEN:
			m_pl.SetPos(pos);
			m_list.Invalidate();
			m_pMainFrame->OpenCurPlaylistItem();
			break;
		case M_ADD:
			{
				if (m_pMainFrame->GetPlaybackMode() == PM_CAPTURE) {
					m_pMainFrame->AddCurDevToPlaylist();
					m_pl.SetPos(m_pl.GetTailPosition());
				} else {
					CString filter;
					CAtlArray<CString> mask;
					s.m_Formats.GetFilter(filter, mask);

					DWORD dwFlags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_ENABLEINCLUDENOTIFY | OFN_NOCHANGEDIR;
					if (!s.bKeepHistory) {
						dwFlags |= OFN_DONTADDTORECENT;
					}

					COpenFileDlg fd(mask, true, nullptr, nullptr, dwFlags, filter, this);
					if (fd.DoModal() != IDOK) {
						return;
					}

					std::list<CString> fns;

					POSITION pos = fd.GetStartPosition();
					while (pos) {
						fns.push_back(fd.GetNextPathName(pos));
					}

					Append(fns, fns.size() > 1, nullptr);
				}
			}
			break;
		case M_REMOVE:
			{
				std::vector<int> items;
				items.reserve(m_list.GetSelectedCount());
				POSITION pos = m_list.GetFirstSelectedItemPosition();
				while (pos) {
					items.push_back(m_list.GetNextSelectedItem(pos));
				}

				for (int i = (int)items.size() - 1; i >= 0; --i) {
					if (m_pl.RemoveAt(FindPos(items[i]))) {
						m_pMainFrame->SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					}
					m_list.DeleteItem(items[i]);
				}

				m_list.SetItemState(-1, 0, LVIS_SELECTED);
				m_list.SetItemState(
					std::max(std::min(items.front(), m_list.GetItemCount() - 1), 0),
					LVIS_SELECTED, LVIS_SELECTED);

				ResizeListColumn();

				SavePlaylist();
			}
			break;
		case M_CLEAR:
			if (Empty()) {
				m_pMainFrame->SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			}
			break;
		case M_SORTBYID:
			m_pl.SortById();
			SetupList();
			SavePlaylist();
			break;
		case M_SORTBYNAME:
			m_pl.SortByName();
			SetupList();
			SavePlaylist();
			break;
		case M_SORTBYPATH:
			m_pl.SortByPath();
			SetupList();
			SavePlaylist();
			break;
		case M_RANDOMIZE:
			m_pl.Randomize();
			SetupList();
			SavePlaylist();
			break;
		case M_CLIPBOARD:
			if (OpenClipboard() && EmptyClipboard()) {
				CString str;

				std::vector<int> items;
				items.reserve(m_list.GetSelectedCount());
				POSITION pos = m_list.GetFirstSelectedItemPosition();
				while (pos) {
					items.push_back(m_list.GetNextSelectedItem(pos));
				}

				for (const auto& item : items) {
					CPlaylistItem &pli = m_pl.GetAt(FindPos(item));
					for (const auto& fi : pli.m_fns) {
						str += L"\r\n" + fi.GetName();
					}
				}

				str.Trim();

				if (HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (str.GetLength()+1)*sizeof(WCHAR))) {
					if (WCHAR* s = (WCHAR*)GlobalLock(h)) {
						wcscpy_s(s, str.GetLength() + 1, str);
						GlobalUnlock(h);
						SetClipboardData(CF_UNICODETEXT, h);
					}
				}
				CloseClipboard();
			}
			break;
		case M_SAVEAS: {
			CSaveTextFileDialog fd(
				CTextFile::UTF8, nullptr, nullptr,
				L"MPC-BE playlist (*.mpcpl)|*.mpcpl|Playlist (*.pls)|*.pls|Winamp playlist (*.m3u)|*.m3u|Windows Media playlist (*.asx)|*.asx||",
				this);

			fd.m_ofn.lpstrInitialDir = s.strLastSavedPlaylistDir;
			if (fd.DoModal() != IDOK) {
				break;
			}

			CTextFile::enc encoding = (CTextFile::enc)fd.GetEncoding();
			if (encoding == CTextFile::ASCII) {
				encoding = CTextFile::ANSI;
			}

			int idx = fd.m_pOFN->nFilterIndex;

			CPath path(fd.GetPathName());
			s.strLastSavedPlaylistDir = AddSlash(GetFolderOnly(path));

			switch (idx) {
				case 1:
					path.AddExtension(L".mpcpl");
					break;
				case 2:
					path.AddExtension(L".pls");
					break;
				case 3:
					path.AddExtension(L".m3u");
					break;
				case 4:
					path.AddExtension(L".asx");
					break;
				default:
					break;
			}

			bool fRemovePath = true;

			CPath p(path);
			p.RemoveFileSpec();
			CString base = (LPCTSTR)p;

			pos = m_pl.GetHeadPosition();
			while (pos && fRemovePath) {
				CPlaylistItem& pli = m_pl.GetNext(pos);

				if (pli.m_type != CPlaylistItem::file) {
					fRemovePath = false;
				} else {
					auto it_f = pli.m_fns.begin();
					while (it_f != pli.m_fns.end() && fRemovePath) {
						CString fn = *it_f++;

						CPath p(fn);
						p.RemoveFileSpec();
						if (base != (LPCTSTR)p) {
							fRemovePath = false;
						}
					}

					auto it_s = pli.m_subs.begin();
					while (it_s != pli.m_subs.end() && fRemovePath) {
						CString fn = *it_s++;

						CPath p(fn);
						p.RemoveFileSpec();
						if (base != (LPCTSTR)p) {
							fRemovePath = false;
						}
					}
				}
			}

			if (idx == 1) {
				SaveMPCPlayList(path, encoding, fRemovePath);
				break;
			}

			CTextFile f;
			if (!f.Save(path, encoding)) {
				break;
			}

			if (idx == 2) {
				f.WriteString(L"[playlist]\n");
			} else if (idx == 3) {
				f.WriteString(L"#EXTM3U\n");
			} else if (idx == 4) {
				f.WriteString(L"<ASX version = \"3.0\">\n");
			}

			pos = m_pl.GetHeadPosition();
			CString str;
			int i;
			for (i = 0; pos; i++) {
				CPlaylistItem& pli = m_pl.GetNext(pos);

				if (pli.m_type != CPlaylistItem::file) {
					continue;
				}

				CString fn = pli.m_fns.front();

				/*
							if (fRemovePath)
							{
								CPath p(path);
								p.StripPath();
								fn = (LPCTSTR)p;
							}
				*/

				if (idx == 3 && !pli.m_label.IsEmpty()) { // M3U
					str.Format(L"#EXTINF:%s\n", pli.m_label);
					f.WriteString(str);
				}

				switch (idx) {
					case 2:
						str.Format(L"File%d=%s\n", i+1, fn);
						break;
					case 3:
						str.Format(L"%s\n", fn);
						break;
					case 4:
						str.Format(L"<Entry><Ref href = \"%s\"/></Entry>\n", fn);
						break;
					default:
						break;
				}
				f.WriteString(str);
			}

			if (idx == 2) {
				str.Format(L"NumberOfEntries=%d\n", i);
				f.WriteString(str);
				f.WriteString(L"Version=2\n");
			} else if (idx == 4) {
				f.WriteString(L"</ASX>\n");
			}
		}
		break;
		case M_SHUFFLE:
			s.bShufflePlaylistItems = !s.bShufflePlaylistItems;
			break;
		case M_HIDEFULLSCREEN:
			s.bHidePlaylistFullScreen = !s.bHidePlaylistFullScreen;
			break;
		default:
			break;
	}
}

void CPlayerPlaylistBar::OnLvnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (pDispInfo->item.iItem >= 0 && pDispInfo->item.pszText) {
		CPlaylistItem& pli = m_pl.GetAt((POSITION)m_list.GetItemData(pDispInfo->item.iItem));
		pli.m_label = pDispInfo->item.pszText;
		m_list.SetItemText(pDispInfo->item.iItem, 0, pDispInfo->item.pszText);
	}

	*pResult = 0;
}
