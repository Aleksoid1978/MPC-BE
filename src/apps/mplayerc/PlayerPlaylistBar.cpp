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
#include <atlutil.h>
#include <atlpath.h>
#include <IntShCut.h>
#include <algorithm>
#include <random>
#include <regex>
#include "MainFrm.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/Filehandle.h"
#include "SaveTextFileDialog.h"
#include "PlayerPlaylistBar.h"
#include "OpenDlg.h"
#include "Content.h"
#include "PlaylistNameDlg.h"
#include <coolsb/coolscroll.h>
#include "./Controls/MenuEx.h"
#include "TorrentInfo.h"

static CString MakePath(CString path)
{
	if (::PathIsURLW(path) || Youtube::CheckURL(path)) { // skip URLs
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
	REFERENCE_TIME m_rt = _I64_MIN;
	UINT m_trackNum = 0;
	CString m_fn;
	CString m_Title;
	CString m_Performer;

	CUETrack(const CString& fn, const REFERENCE_TIME rt, const UINT trackNum, const CString& Title, const CString& Performer) {
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
		const CString cmd = GetCUECommand(cueLine);

		if (cmd == L"TRACK") {
			if (rt != _I64_MIN) {
				const CString fName = bMultiple ? sFilesArray[idx++] : sFilesArray.size() == 1 ? sFilesArray[0] : sFileName;
				CUETrackList.push_back({ fName, rt, trackNum, sTitle, sPerformer });
			}
			rt = _I64_MIN;
			sFileName = sFileName2;
			if (!Performer.IsEmpty()) {
				sPerformer = Performer;
			}

			WCHAR type[256] = { 0 };
			trackNum = 0;
			fAudioTrack = FALSE;
			if (2 == swscanf_s(cueLine, L"%u %s", &trackNum, type, _countof(type))) {
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
				rt = UNITS * (mm * 60LL + ss) + UNITS * ff / 75;
			}
		}
	}

	if (rt != _I64_MIN) {
		const CString fName = bMultiple ? sFilesArray[idx] : sFilesArray.size() == 1 ? sFilesArray[0] : sFileName2;
		CUETrackList.push_back({ fName, rt, trackNum, sTitle, sPerformer });
	}

	return CUETrackList.size() > 0;
}

CPlaylistItem::CPlaylistItem()
	: m_type(file)
	, m_bInvalid(false)
	, m_bDirectory(false)
	, m_duration(0)
	, m_vinput(-1)
	, m_vchannel(-1)
	, m_ainput(-1)
	, m_country(0)
{
	m_id = m_globalid++;
}

CPlaylistItem& CPlaylistItem::operator = (const CPlaylistItem& pli)
{
	if (this != &pli) {
		m_id = pli.m_id;
		m_label = pli.m_label;
		m_fns = pli.m_fns;
		m_subs = pli.m_subs;
		m_type = pli.m_type;
		m_bInvalid = pli.m_bInvalid;
		m_bDirectory = pli.m_bDirectory;
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

bool CPlaylistItem::FindFolder(LPCTSTR path) const
{
	for (const auto& fi : m_fns) {
		CString str = fi.GetName();
		str.TrimRight(L">");
		str.TrimRight(L"<");
		if (str.CompareNoCase(path) == 0) {
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
			const auto& fn = m_fns.front();
			CUrl url;
			if (::PathIsURLW(fn) && url.CrackUrl(fn)) {
				str = fn.GetName();
				if (url.GetUrlPathLength() > 1) {
					str = url.GetUrlPath(); str.TrimRight(L'/');
					if (const int pos = str.ReverseFind(L'/'); pos != -1) {
						str = str.Right(str.GetLength() - pos - 1);
					}
				}
				m_label = str.TrimRight(L'/');
			}
			else {
				str = GetFileOnly(fn);
			}
		}
	} else if (i == 1) {
		if (m_bInvalid) {
			return L"Invalid";
		}

		if (m_type == file) {
			const REFERENCE_TIME rt = m_duration;
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
		std::vector<CString> mask;
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
	while (pos) {
		const plsort_str_t item = { GetAt(pos).GetLabel(), pos };
		a.emplace_back(item);

		GetNext(pos);
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
	while (pos) {
		const plsort_str_t item = { GetAt(pos).m_fns.front(), pos };
		a.emplace_back(item);

		GetNext(pos);
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

void CPlaylist::ReverseSort()
{
	std::vector<plsort_str_t> a;
	a.reserve(GetCount());

	POSITION pos = GetHeadPosition();
	while (pos) {
		const plsort_str_t item = { GetAt(pos).m_fns.front(), pos };
		a.emplace_back(item);

		GetNext(pos);
	}

	for (const auto& item : a) {
		AddHead(GetAt(item.pos));
		__super::RemoveAt(item.pos);
		if (m_pos == item.pos) {
			m_pos = GetHeadPosition();
		}
	}
}

POSITION CPlaylist::GetPos() const
{
	return m_pos;
}

void CPlaylist::SetPos(POSITION pos)
{
	m_pos = pos;
}

POSITION CPlaylist::Shuffle()
{
	ASSERT(GetCount() > 2);

	static INT_PTR idx = 0;
	static INT_PTR count = 0;
	static std::vector<POSITION> a;
	const auto Count = m_pos ? GetCount() - 1 : GetCount();

	if (count != Count || idx >= Count) {
		// insert or remove items in playlist, or index out of bounds then recalculate
		a.clear();
		idx = 0;
		a.reserve(count = Count);

		for (POSITION pos = GetHeadPosition(); pos; GetNext(pos)) {
			if (pos != m_pos) {
				a.emplace_back(pos);
			}
		}

		std::shuffle(a.begin(), a.end(), std::default_random_engine((unsigned)GetTickCount()));
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

static unsigned GetNextId()
{
	static unsigned id = 1;
	return id++;
}

IMPLEMENT_DYNAMIC(CPlayerPlaylistBar, CPlayerBar)

CPlayerPlaylistBar::CPlayerPlaylistBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_list(0)
	, m_nTimeColWidth(0)
	, m_bDragging(FALSE)
	, m_bHiddenDueToFullscreen(false)
	, m_bVisible(false)
	, m_cntOffset(0)
	, m_iTFontSize(0)
	, m_nSearchBarHeight(20)
{
	CAppSettings& s = AfxGetAppSettings();
	m_bUseDarkTheme = s.bUseDarkTheme;

	TSetColor();
	TGetSettings();
}

CPlayerPlaylistBar::~CPlayerPlaylistBar()
{
	TEnsureVisible(m_nCurPlayListIndex); // save selected tab visible
	SavePlaylist();
	TSaveSettings();

	for (auto& pl : m_pls) {
		SAFE_DELETE(pl);
	}

	for (auto&[key, icon] : m_icons) {
		DestroyIcon(icon);
	}

	for (auto&[key, icon] : m_icons_large) {
		DestroyIcon(icon);
	}
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
		| LVS_REPORT | LVS_AUTOARRANGE | LVS_NOSORTHEADER,
		CRect(0, 0, 100, 100), this, IDC_PLAYLIST);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_list.InsertColumn(COL_NAME, L"Name", LVCFMT_LEFT);
	m_list.InsertColumn(COL_TIME, L"Time", LVCFMT_RIGHT);

	m_REdit.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP, CRect(10, 10, 100, 10), this, IDC_FINDINPLAYLIST);
	if (AfxGetAppSettings().bUseDarkTheme) {
		m_REdit.SetBkColor(m_crBND);
		m_REdit.SetTextColor(m_crTH);
	} else {
		m_REdit.SetBkColor(::GetSysColor(COLOR_WINDOW));
		m_REdit.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	}
	m_REdit.SetSel(0, 0);

	ScaleFontInternal();

	if (AfxGetAppSettings().bUseDarkTheme) {
		InitializeCoolSB(m_list.m_hWnd, ThemeRGB);
		if (SysVersion::IsWin8orLater()) {
			CoolSB_SetSize(m_list.m_hWnd, SB_VERT, m_pMainFrame->GetSystemMetricsDPI(SM_CYVSCROLL), m_pMainFrame->GetSystemMetricsDPI(SM_CXVSCROLL));
		}
	}

	m_bFixedFloat = TRUE;
	TCalcLayout();
	TCalcREdit();

	return TRUE;
}

void CPlayerPlaylistBar::ReloadTranslatableResources()
{
	SetWindowTextW(ResStr(IDS_PLAYLIST_CAPTION));

	if (!m_tabs.empty()) {
		m_tabs[0].name = ResStr(IDS_PLAYLIST_MAIN_NAME);
		TCalcLayout();
		TDrawBar();
	}
}

void CPlayerPlaylistBar::ScaleFontInternal()
{
	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
	VERIFY(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0));

	auto& lf = ncm.lfMessageFont;
	lf.lfHeight = m_pMainFrame->ScaleSystemToMonitorY(lf.lfHeight) * AfxGetAppSettings().iPlsFontPercent / 100;
	m_iTFontSize = abs(lf.lfHeight);

	m_font.DeleteObject();
	if (m_font.CreateFontIndirectW(&lf)) {
		m_list.SetFont(&m_font);
		m_REdit.SetFont(&m_font);
	}

	CDC* pDC = m_list.GetDC();
	CFont* old = pDC->SelectObject(GetFont());
	m_nTimeColWidth = pDC->GetTextExtent(L"000:00:00").cx + m_pMainFrame->ScaleX(5);
	pDC->SelectObject(old);
	m_list.ReleaseDC(pDC);

	m_list.SetColumnWidth(COL_TIME, m_nTimeColWidth);

	m_fakeImageList.DeleteImageList();
	m_fakeImageList.Create(1, std::abs(lf.lfHeight) + m_pMainFrame->ScaleY(4), ILC_COLOR4, 10, 10);
	m_list.SetImageList(&m_fakeImageList, LVSIL_SMALL);

	m_nSearchBarHeight = m_pMainFrame->ScaleY(20);
}

void CPlayerPlaylistBar::ScaleFont()
{
	ScaleFontInternal();
	ResizeListColumn();
	TCalcLayout();
	TCalcREdit();
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
	if (pMsg->message == WM_MOUSEWHEEL) {
		const POINT point = { GET_X_LPARAM(pMsg->lParam), GET_Y_LPARAM(pMsg->lParam) };
		CRect rect;
		GetWindowRect(&rect);
		if (!rect.PtInRect(point)) {
			m_pMainFrame->GetWindowRect(&rect);
			if (rect.PtInRect(point)) {
				m_pMainFrame->SendMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
				SetFocus();
				return TRUE;
			}
		}
	}

	if (pMsg->message == WM_MBUTTONDOWN || pMsg->message == WM_MBUTTONUP || pMsg->message == WM_MBUTTONDBLCLK) {
		m_pMainFrame->PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
		return TRUE;
	}

	if (IsWindow(pMsg->hwnd) && IsVisible() && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
		if (pMsg->hwnd == m_REdit.GetSafeHwnd()) {
			IsDialogMessageW(pMsg);

			if (curPlayList.GetCount() > 1
					&& (pMsg->message == WM_CHAR
						|| (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_F3 || pMsg->wParam == VK_DELETE || pMsg->wParam == VK_BACK)))) {
				CString text; m_REdit.GetWindowTextW(text);
				if (!text.IsEmpty()) {
					::CharLowerBuffW(text.GetBuffer(), text.GetLength());

					auto& playlist = curPlayList;
					POSITION pos = playlist.GetHeadPosition();
					if (curTab.type == EXPLORER) {
						playlist.GetNext(pos);
					}
					POSITION cur_pos = nullptr;
					if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_F3) {
						auto item = TGetFocusedElement();
						if (item == 0 && curTab.type == EXPLORER) {
							item++;
						}
						cur_pos = FindPos(item);
						pos = FindPos(item + 1);
						if (!pos) {
							pos = playlist.GetHeadPosition();
							if (curTab.type == EXPLORER) {
								playlist.GetNext(pos);
							}
						}
					}
					while (pos != cur_pos) {
						auto& pl = playlist.GetAt(pos);
						auto label = pl.GetLabel();
						::CharLowerBuffW(label.GetBuffer(), label.GetLength());
						if (label.Find(text) >= 0) {
							EnsureVisible(pos);
							break;
						}

						playlist.GetNext(pos);
						if (!pos && cur_pos) {
							pos = playlist.GetHeadPosition();
							if (curTab.type == EXPLORER) {
								playlist.GetNext(pos);
							}
						}
					}
				}
			}

			if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
				m_REdit.SetSel(0, -1);
				m_REdit.Clear();
				m_REdit.SetSel(0, 0);

				m_list.SetFocus();
			}

			return TRUE;
		}

		if (pMsg->message == WM_SYSKEYDOWN
				&& pMsg->wParam == VK_RETURN && (HIWORD(pMsg->lParam) & KF_ALTDOWN)) {
			m_pMainFrame->PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
			return TRUE;
		}

		if (pMsg->message == WM_KEYDOWN) {
			switch (pMsg->wParam) {
			case VK_ESCAPE:
				GetParentFrame()->ShowControlBar(this, FALSE, TRUE);
				return TRUE;
			case VK_RETURN:
				if (GetKeyState(VK_CONTROL) < 0) {
					m_pMainFrame->PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
					return TRUE;
				}
				if (m_list.GetSelectedCount() == 1) {
					if (TNavigate()) {
						break;// return FALSE;
					}
					const int item = m_list.GetNextItem(-1, LVNI_SELECTED);
					curPlayList.SetPos(FindPos(item));
					m_pMainFrame->OpenCurPlaylistItem();

					return TRUE;
				}
				break;
			case 'A':
				if (curTab.type == EXPLORER) {
					break;
				}
				if (GetKeyState(VK_CONTROL) < 0) {
					m_list.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
				}
				break;
			case 'I':
				if (curTab.type == EXPLORER) {
					break;
				}
				if (GetKeyState(VK_CONTROL) < 0) {
					for (int nItem = 0; nItem < m_list.GetItemCount(); nItem++) {
						m_list.SetItemState(nItem, ~m_list.GetItemState(nItem, LVIS_SELECTED), LVIS_SELECTED);
					}
				}
				break;
			case 'C':
				if (GetKeyState(VK_CONTROL) < 0) {
					CopyToClipboard();
				}
				break;
			case 'V':
				if (curTab.type == EXPLORER) {
					break;
				}
				if (GetKeyState(VK_CONTROL) < 0) {
					PasteFromClipboard();
				}
				break;
			case VK_BACK:
				if (curTab.type == EXPLORER) {
					auto path = curPlayList.GetHead().m_fns.front().GetName();
					if (path.Right(1) == L"<") {
						auto oldPath = path;
						oldPath.TrimRight(L"\\<");

						path.TrimRight(L"\\<");
						path = GetFolderOnly(path);
						if (path.Right(1) == L":") {
							path = (L".");
						}
						path = AddSlash(path);

						curPlayList.RemoveAll();
						TParseFolder(path);
						Refresh();

						TSelectFolder(oldPath);
					}
				}
				break;
			case VK_UP:
			case VK_DOWN:
			case VK_HOME:
			case VK_END:
			case VK_PRIOR:
			case VK_NEXT:
			case VK_DELETE:
			case VK_APPS: // "Menu key"
				break;
			default:
				m_pMainFrame->PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
				return TRUE;
			}
		}

		if (IsDialogMessageW(pMsg)) {
			return TRUE;
		}
	}

	return CSizingControlBarG::PreTranslateMessage(pMsg);
}

COLORREF CPlayerPlaylistBar::ColorThemeRGB(const int iR, const int iG, const int iB) const
{
	return ThemeRGB(iR, iG,iB);
}

int CPlayerPlaylistBar::ScaleX(const int x) const
{
	return m_pMainFrame->ScaleX(x);
}

int CPlayerPlaylistBar::ScaleY(const int y) const
{
	return m_pMainFrame->ScaleY(y);
}

void CPlayerPlaylistBar::LoadState(CFrameWnd *pParent)
{
	CString section = L"ToolBars\\" + m_strSettingName;

	bool visible = false;
	AfxGetProfile().ReadBool(section, L"Visible", visible);
	if (visible) {
		ShowWindow(SW_SHOW);
		m_bVisible = true;
	}

	__super::LoadState(pParent);
}

void CPlayerPlaylistBar::SaveState()
{
	__super::SaveState();

	CString section = L"ToolBars\\" + m_strSettingName;

	AfxGetProfile().WriteBool(section, L"Visible", IsWindowVisible() || (AfxGetAppSettings().bHidePlaylistFullScreen && m_bHiddenDueToFullscreen));
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

	curPlayList.AddTail(pli);
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
					for (auto& mfc : mf) {
						/* playlist files are skipped when playing the contents of an entire directory */
						if (mfc.FindExt(ext) && mfc.GetFileType() != TPlaylist) {
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

// Resolve .lnk and .url files.
void CPlayerPlaylistBar::ResolveLinkFiles(std::list<CString> &fns)
{
	for (auto& fn : fns) {
		const CString extension = GetFileExt(fn).MakeLower();

		// Regular shortcut file.
		if (extension == L".lnk") {
			CComPtr<IShellLinkW> pShellLink;
			if (SUCCEEDED(pShellLink.CoCreateInstance(CLSID_ShellLink))) {
				if (CComQIPtr<IPersistFile> pPersistFile = pShellLink) {
					WCHAR buffer[MAX_PATH] = {};
					if (SUCCEEDED(pPersistFile->Load(fn, STGM_READ))
							// Possible recontruction of path.
							&& SUCCEEDED(pShellLink->Resolve(nullptr, SLR_ANY_MATCH | SLR_NO_UI))
							// Retrieve path.
							&& SUCCEEDED(pShellLink->GetPath(buffer, _countof(buffer), nullptr, 0))
							// non-empty buffer
							&& wcslen(buffer)) {
						fn = buffer;
					}
				}
			}

		// Internet shortcut file.
		} else if (extension == L".url" || extension == L".website") {
			CComPtr<IUniformResourceLocatorW> pUniformResourceLocator;
			if (SUCCEEDED(pUniformResourceLocator.CoCreateInstance(CLSID_InternetShortcut))) {
				if (CComQIPtr<IPersistFile> pPersistFile = pUniformResourceLocator) {
					WCHAR* buffer;
					if (SUCCEEDED(pPersistFile->Load(fn, STGM_READ))
							// Retrieve URL (foreign-allocated).
							&& SUCCEEDED(pUniformResourceLocator->GetURL(&buffer))) {
						if (wcslen(buffer)) {
							fn = buffer;
						}

						// Free foreign-allocated memory.
						IMalloc* pMalloc;
						if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
							pMalloc->Free(buffer);
							pMalloc->Release();
						}
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
				if (m_pMainFrame->CheckBD(fn) || m_pMainFrame->IsDVDStartFile(fn) /*&& ::PathFileExistsW(fn)*/) {
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
				ParsePlayList(r, subs, !::PathIsURLW(r));
			}
			return;
		}

		if (m_pMainFrame->CheckBD(fn)) {
			AddItem(fns, subs);

			CString empty, label;
			m_pMainFrame->MakeBDLabel(fn, empty, &label);
			curPlayList.GetTail().m_label = label;
			return;
		} else if (m_pMainFrame->IsDVDStartFile(fn) /*&& ::PathFileExistsW(fn)*/) {
			AddItem(fns, subs);
			CString empty, label;
			m_pMainFrame->MakeDVDLabel(fn, empty, &label);
			curPlayList.GetTail().m_label = label;
			return;
		} else if (ct == L"application/x-mpc-playlist") {
			if (ParseMPCPlayList(fn)) {
				return;
			}
		} else if (ct == L"audio/x-mpegurl" || ct == L"application/http-live-streaming-m3u") {
			if (ParseM3UPlayList(fn)) {
				return;
			}
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

	if (fn.Find(L"/") != -1) {
		CString url;
		if (fn.Find(L"//") == 0) {
			url = L"http:" + fn;
		} else {
			url = L"http://" + fn;
		}
		CUrl cUrl;
		if (cUrl.CrackUrl(url) && cUrl.GetHostNameLength()) {
			return url;
		}
	}

	p.Append(CPath(fn));
	CString out(p);
	if (out.Find(L"://")) {
		out.Replace('\\', '/');
	}
	return out;
}

bool CPlayerPlaylistBar::ParseMPCPlayList(const CString& fn)
{
	CString str;
	std::map<int, CPlaylistItem> pli;
	std::vector<int> idx;
	int selected_idx = -1;

	CWebTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn) || !f.ReadString(str) || str != L"MPCPLAYLIST" || f.GetLength() > MEGABYTE) {
		if (curTab.type == EXPLORER) {
			TParseFolder(L".\\");
		}
		return false;
	}

	CPath base(fn);
	base.RemoveFileSpec();

	curPlayList.m_nSelectedAudioTrack = curPlayList.m_nSelectedSubtitleTrack = -1;

	while (f.ReadString(str)) {
		std::list<CString> sl;
		Explode(str, sl, L',', 3);
		if (sl.size() == 2) {
			auto it = sl.cbegin();
			const auto& key = (*it++);
			int value = -1;
			StrToInt32((*it), value);

			if (key == L"audio") {
				curPlayList.m_nSelectedAudioTrack = value;
			} else if (key == L"subtitles") {
				curPlayList.m_nSelectedSubtitleTrack = value;
			}

			continue;
		} else if (sl.size() != 3) {
			continue;
		}

		auto it = sl.cbegin();
		if (int i = _wtoi(*it++)) {
			const auto& key = (*it++);
			auto value = (*it);

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
				value = curTab.type == PLAYLIST ? MakePath(CombinePath(base, value)) : value;
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

	const BOOL bIsEmpty = curPlayList.IsEmpty();

	std::sort(idx.begin(), idx.end());

	for (size_t i = 0; i < idx.size(); i++) {
		curPlayList.AddTail(pli[idx[i]]);
	}

	if (curTab.type == EXPLORER) {
		CString selected_path;
		if (bIsEmpty && selected_idx >= 0 && selected_idx < curPlayList.GetCount()) {
			POSITION pos = curPlayList.FindIndex(selected_idx);
			if (pos) {
				selected_path = curPlayList.GetAt(pos).m_fns.front().GetName();
			}
		}
		selected_idx = 0;

		if (curPlayList.IsEmpty()) {
			TParseFolder(L".\\");
		}
		else {
			auto path = curPlayList.GetHead().m_fns.front().GetName();
			if (path.Right(1) == L"<") {
				path.TrimRight(L"<");
				curPlayList.RemoveAll();
				if (::PathFileExistsW(path)) {
					TParseFolder(path);
				}
				else {
					TParseFolder(L".\\");
				}
			}
			else {
				curPlayList.RemoveAll();
				TParseFolder(L".\\");
			}
		}

		if (!selected_path.IsEmpty()) {
			int idx = 0;
			POSITION pos = curPlayList.GetHeadPosition();
			while (pos) {
				CPlaylistItem& pli = curPlayList.GetAt(pos);
				if (pli.FindFile(selected_path)) {
					selected_idx = idx;
					break;
				}
				curPlayList.GetNext(pos);
				idx++;
			}
		}
	}

	if (bIsEmpty && selected_idx >= 0 && selected_idx < curPlayList.GetCount()) {
		curPlayList.m_nSelected_idx = selected_idx - 1;
		curPlayList.m_nFocused_idx = curPlayList.m_nSelected_idx + 1;
	}

	return pli.size() > 0;
}

bool CPlayerPlaylistBar::SaveMPCPlayList(const CString& fn, const CTextFile::enc e, const bool bRemovePath)
{
	CTextFile f;
	if (!f.Save(fn, e)) {
		return false;
	}

	f.WriteString(L"MPCPLAYLIST\n");

	CString fmt;
	fmt.Format(L"audio,%d\n", curPlayList.m_nSelectedAudioTrack);
	f.WriteString(fmt);
	fmt.Format(L"subtitles,%d\n", curPlayList.m_nSelectedSubtitleTrack);
	f.WriteString(fmt);

	POSITION cur_pos = curPlayList.GetPos();

	POSITION pos = curPlayList.GetHeadPosition();
	for (int i = 1; pos; i++) {
		bool selected = (cur_pos == pos) && (curPlayList.GetCount() > 1);
		CPlaylistItem& pli = curPlayList.GetNext(pos);

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
				if (bRemovePath) {
					CPath p(fn);
					p.StripPath();
					fn = (LPCTSTR)p;
				}
				f.WriteString(idx + L",filename," + fn + L"\n");
			}

			for (const auto& si : pli.m_subs) {
				CString fn = si.GetName();
				if (bRemovePath) {
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

	CFileItemList audio_fns;

	std::vector<CPlaylistItem> playlist;
	CPlaylistItem *pli = nullptr;

	INT_PTR c = curPlayList.GetCount();
	CString str;
	while (f.ReadString(str)) {
		str = str.Trim();

		if (str.IsEmpty() || (str.Left(7) == L"#EXTM3U")) {
			continue;
		}

		if (!pli) {
			pli = DNew CPlaylistItem;
		}

		if (str.Left(1) == L"#") {
			if (str.Left(8) == L"#EXTINF:") {
				int pos = 8;
				str = str.Mid(pos, str.GetLength() - pos);

				pos = str.Find(L",");
				if (pos > 0) {
					const auto tmp = str.Left(pos);
					int dur = 0;
					if (swscanf_s(tmp, L"%dx", &dur) == 1) {
						pos++;
						str = str.Mid(pos, str.GetLength() - pos);
					}
				}
				pli->m_label = str.Trim();
			} else if (str.Left(18) == L"#EXT-X-STREAM-INF:") {
				const int pos = 18;
				str = str.Mid(pos, str.GetLength() - pos);
				pli->m_label = str.Trim();
			} else if (str.Left(13) == L"#EXT-X-MEDIA:" && str.Find(L"TYPE=AUDIO") > 13) {
				int pos = 13;
				str = str.Mid(pos, str.GetLength() - pos);

				std::list<CString> params;
				Explode(str, params, L',');
				for (const auto& param : params) {
					pos = param.Find(L'=');
					if (pos > 0) {
						const auto key = param.Left(pos);
						auto value = param.Mid(pos + 1);
						if (key == L"URI") {
							value.Trim(L'\"');
							const auto path = MakePath(CombinePath(base, value));
							audio_fns.push_back(path);
							break;
						}
					}
				}
			}
		} else {
			const auto path = MakePath(CombinePath(base, str));
			if (!fn.CompareNoCase(path)) {
				SAFE_DELETE(pli);
				continue;
			}

			if (!::PathIsURLW(path)) {
				const auto ext = GetFileExt(path).MakeLower();
				if (ext == L".m3u" || ext == L".m3u8") {
					if (ParseM3UPlayList(path)) {
						SAFE_DELETE(pli);
						continue;
					}
				} else if (ext == L".mpcpl") {
					if (ParseMPCPlayList(path)) {
						SAFE_DELETE(pli);
						continue;
					}
				} else {
					std::list<CString> redir;
					const auto ct = Content::GetType(path, &redir);
					if (!redir.empty()) {
						SAFE_DELETE(pli);
						for (const auto& r : redir) {
							ParsePlayList(r, nullptr);
						}
						continue;
					}
				}
			}
			pli->m_fns.push_back(path);
			if (!audio_fns.empty()) {
				pli->m_fns.insert(pli->m_fns.end(), audio_fns.begin(), audio_fns.end());
			}
			playlist.push_back(*pli);

			SAFE_DELETE(pli);
		}
	}

	SAFE_DELETE(pli);

	bool bNeedParse = false;
	if (playlist.size() == 1) {
		const auto& pli = playlist.front();
		const auto& fn = pli.m_fns.front();
		const auto ext = GetFileExt(fn).MakeLower();

		if (ext == L".m3u" || ext == L".m3u8") {
			Content::Online::Clear(fn);
			std::list<CString> redir;
			const auto ct = Content::GetType(fn, &redir);
			if (ct == L"audio/x-mpegurl" || ct == L"application/http-live-streaming-m3u") {
				bNeedParse = ParseM3UPlayList(fn);
			}
		}
	}

	if (!bNeedParse) {
		for (const auto& pli : playlist) {
			const auto& fn = pli.m_fns.front();
			if (::PathIsURLW(fn) && m_pMainFrame->OpenYoutubePlaylist(fn, TRUE)) {
				continue;
			}
			curPlayList.AddTail(pli);
		}
	}

	return (curPlayList.GetCount() > c);
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

	INT_PTR c = curPlayList.GetCount();

	for (const auto& fName : fileNames){
		CString fullPath = MakePath(CombinePath(base, fName));
		BOOL bExists = TRUE;
		if (!::PathFileExistsW(fullPath)) {
			CString ext = GetFileExt(fullPath);
			bExists = FALSE;

			CString filter;
			std::vector<CString> mask;
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

			curPlayList.AddTail(pli);
		}
	}

	return (curPlayList.GetCount() > c);
}

void CPlayerPlaylistBar::Refresh()
{
	SetupList();
	ResizeListColumn();
}

bool CPlayerPlaylistBar::Empty()
{
	if (curTab.type == EXPLORER) {
		return false;
	}

	bool bWasPlaying = curPlayList.RemoveAll();
	m_list.DeleteAllItems();
	SavePlaylist();

	return bWasPlaying;
}

void CPlayerPlaylistBar::Remove(const std::vector<int>& items, const bool bDelete)
{
	if (!items.empty()) {
		if (bDelete && (MessageBoxW(ResStr(IDS_PLAYLIST_DELETE_QUESTION), nullptr, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDNO)) {
			return;
		}

		m_list.SetRedraw(FALSE);

		bool bWasPlaying = false;
		std::list<CString> fns;

		for (int i = (int)items.size() - 1; i >= 0; --i) {
			POSITION pos = FindPos(items[i]);

			if (bDelete) {
				const auto item = curPlayList.GetAt(pos);
				for (const auto& fn : item.m_fns) {
					if (::PathFileExistsW(fn)) {
						fns.emplace_back(fn);
					}
				}
				for (const auto& sub : item.m_subs) {
					if (::PathFileExistsW(sub)) {
						fns.emplace_back(sub);
					}
				}
			}

			if (curPlayList.RemoveAt(pos)) {
				if (m_pMainFrame->m_eMediaLoadState == MLS_LOADED) {
					bWasPlaying = true;
				}
				CloseMedia();
			}
			m_list.DeleteItem(items[i]);
		}

		m_list.SetItemState(-1, 0, LVIS_SELECTED);
		m_list.SetItemState(std::max(std::min(items.front(), m_list.GetItemCount() - 1), 0), LVIS_SELECTED, LVIS_SELECTED);

		ResizeListColumn();
		SavePlaylist();
		UpdateList();

		m_list.SetRedraw();

		if (bDelete && !fns.empty()) {
			fns.sort();
			fns.unique();

			CString filesToDelete;
			for (const auto& fn : fns) {
				filesToDelete.Append(fn);
				filesToDelete.AppendChar(0);
			}
			filesToDelete.AppendChar(0);

			SHFILEOPSTRUCTW shfoDelete = {};
			shfoDelete.hwnd = m_hWnd;
			shfoDelete.wFunc = FO_DELETE;
			shfoDelete.pFrom = filesToDelete;
			shfoDelete.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_ALLOWUNDO;
			SHFileOperationW(&shfoDelete);

			if (bWasPlaying) {
				m_list.Invalidate();
				m_pMainFrame->OpenCurPlaylistItem();
			}
		}
	}
}

void CPlayerPlaylistBar::Open(const CString& fn)
{
	std::list<CString> fns;
	fns.push_front(fn);

	m_bSingleElement = true;
	Open(fns, false);
	m_bSingleElement = false;
}

void CPlayerPlaylistBar::Open(std::list<CString>& fns, const bool bMulti, CSubtitleItemList* subs/* = nullptr*/, bool bCheck/* = true*/)
{
	curPlayList.m_nFocused_idx = TGetFocusedElement();
	m_nCurPlayListIndex = 0;

	m_pMainFrame->m_bRememberSelectedTracks = false;
	curPlayList.m_nSelectedAudioTrack = curPlayList.m_nSelectedSubtitleTrack = -1;

	TEnsureVisible(m_nCurPlayListIndex);
	TSelectTab();

	Empty();
	ResolveLinkFiles(fns);
	Append(fns, bMulti, subs, bCheck);
}

void CPlayerPlaylistBar::Append(const CString& fn)
{
	std::list<CString> fns;
	fns.push_front(fn);
	Append(fns, false);
}

static void CorrectPaths(std::list<CString>& fns)
{
	for (auto& fn : fns) {
		if (std::regex_match(fn.GetString(), std::wregex(LR"(magnet:\?xt=urn:btih:[0-9a-fA-F]+(?:&\S+|$))"))) {
			fn.Format(AfxGetAppSettings().strTorrServerAddress, fn.GetString());
		} else if (fn.Right(8) == L".torrent" && ::PathFileExistsW(fn)) {
			CTorrentInfo torrentInfo;
			if (torrentInfo.Read(fn)) {
				const auto magnet = torrentInfo.Magnet();
				if (!magnet.empty()) {
					fn.Format(AfxGetAppSettings().strTorrServerAddress, magnet.c_str());
				}
			}
		}
	}
}

void CPlayerPlaylistBar::Append(std::list<CString>& fns, const bool bMulti, CSubtitleItemList* subs/* = nullptr*/, bool bCheck/* = true*/)
{
	const INT_PTR idx = curPlayList.GetCount();

	CorrectPaths(fns);

	if (curTab.type == EXPLORER) {
		curPlayList.m_nFocused_idx = TGetFocusedElement();
		m_nCurPlayListIndex = 0;

		TEnsureVisible(m_nCurPlayListIndex);
		TSelectTab();
	}

	if (bMulti) {
		ASSERT(subs == nullptr || subs->size() == 0);

		for (const auto& fn : fns) {
			ParsePlayList(fn, nullptr, bCheck);
		}
	} else {
		ParsePlayList(fns, subs, bCheck);
	}

	Refresh();
	EnsureVisible(curPlayList.FindIndex(curPlayList.m_nSelected_idx != INT_MAX ? curPlayList.m_nSelected_idx + 1 : idx));
	if (!idx && AfxGetAppSettings().bShufflePlaylistItems && GetCount() > 2) {
		SetNext();
	}
	SavePlaylist();

	UpdateList();

	curPlayList.m_nSelected_idx = INT_MAX;
}

void CPlayerPlaylistBar::Append(const CFileItemList& fis)
{
	if (curTab.type == EXPLORER) {
		curPlayList.m_nFocused_idx = TGetFocusedElement();
		m_nCurPlayListIndex = 0;

		TEnsureVisible(m_nCurPlayListIndex);
		TSelectTab();
	}

	const INT_PTR idx = curPlayList.GetCount();

	for (const auto& fi : fis) {
		CPlaylistItem pli;
		pli.m_fns.push_front(fi.GetName());
		pli.m_label = fi.GetTitle();
		curPlayList.AddTail(pli);
	}

	Refresh();
	EnsureVisible(curPlayList.FindIndex(idx + 1));
	SavePlaylist();

	UpdateList();
}

void CPlayerPlaylistBar::Open(const CStringW& vdn, const CStringW& adn, const int vinput, const int vchannel, const int ainput)
{
	Empty();
	Append(vdn, adn, vinput, vchannel, ainput);
}

void CPlayerPlaylistBar::Append(const CStringW& vdn, const CStringW& adn, const int vinput, const int vchannel, const int ainput)
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
	curPlayList.AddTail(pli);

	Refresh();
	EnsureVisible(curPlayList.GetTailPosition());
	SavePlaylist();
}

void CPlayerPlaylistBar::SetupList()
{
	m_list.DeleteAllItems();

	POSITION pos = curPlayList.GetHeadPosition();
	for (int i = 0; pos; i++) {
		CPlaylistItem& pli = curPlayList.GetAt(pos);
		m_list.SetItemData(m_list.InsertItem(i, pli.GetLabel()), (DWORD_PTR)pos);
		m_list.SetItemText(i, COL_TIME, pli.GetLabel(1));
		curPlayList.GetNext(pos);
	}
}

void CPlayerPlaylistBar::UpdateList()
{
	POSITION pos = curPlayList.GetHeadPosition();
	for (int i = 0, j = m_list.GetItemCount(); pos && i < j; i++) {
		CPlaylistItem& pli = curPlayList.GetAt(pos);
		m_list.SetItemData(i, (DWORD_PTR)pos);
		m_list.SetItemText(i, COL_NAME, pli.GetLabel(0));
		m_list.SetItemText(i, COL_TIME, pli.GetLabel(1));
		curPlayList.GetNext(pos);
	}
}

void CPlayerPlaylistBar::EnsureVisible(POSITION pos, bool bMatchPos)
{
	if (pos == nullptr) {
		return;
	}

	int i = FindItem(pos);
	if (i < 0) {
		return;
	}

	m_list.SetItemState(-1, 0, LVIS_SELECTED);
	m_list.EnsureVisible(i - 1 + bMatchPos, TRUE);
	m_list.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

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
	return (i < 0 || i >= m_list.GetItemCount()) ? nullptr : (POSITION)m_list.GetItemData(i);
}

int CPlayerPlaylistBar::GetCount(const bool bOnlyFiles/* = false*/)
{
	if (bOnlyFiles && curTab.type == EXPLORER) {
		int count = 0;
		POSITION pos = curPlayList.GetHeadPosition();
		while (pos) {
			const auto& playlist = curPlayList.GetNext(pos);
			if (!playlist.m_bDirectory) {
				count++;
			}
		}

		return count;
	}

	return curPlayList.GetCount();
}

int CPlayerPlaylistBar::GetSelIdx()
{
	return FindItem(curPlayList.GetPos());
}

void CPlayerPlaylistBar::SetSelIdx(int i, bool bUpdatePos/* = false*/)
{
	POSITION pos = FindPos(i);
	curPlayList.SetPos(pos);
	if (bUpdatePos) {
		EnsureVisible(pos, false);
	}
}

bool CPlayerPlaylistBar::IsAtEnd()
{
	POSITION pos = curPlayList.GetPos(), tail = curPlayList.GetTailPosition();
	bool isAtEnd = (pos && pos == tail);

	if (!isAtEnd && pos) {
		isAtEnd = curPlayList.GetNextWrap(pos).m_bInvalid;
		while (isAtEnd && pos && pos != tail) {
			isAtEnd = curPlayList.GetNextWrap(pos).m_bInvalid;
		}
	}

	return isAtEnd;
}

bool CPlayerPlaylistBar::GetCur(CPlaylistItem& pli)
{
	if (!curPlayList.GetPos()) {
		return false;
	}
	pli = curPlayList.GetAt(curPlayList.GetPos());
	return true;
}

CPlaylistItem* CPlayerPlaylistBar::GetCur()
{
	return !curPlayList.GetPos() ? nullptr : &curPlayList.GetAt(curPlayList.GetPos());
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
	POSITION pos = curPlayList.GetPos(), org = pos;
	if (!pos) {
		org = pos = curPlayList.GetHeadPosition();
	}

	for (;;) {
		const auto& playlist = curPlayList.GetNextWrap(pos);
		if ((playlist.m_bInvalid || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
	UpdateList();
	curPlayList.SetPos(pos);
	EnsureVisible(pos);

	return (pos != org);
}

bool CPlayerPlaylistBar::SetPrev()
{
	POSITION pos = curPlayList.GetPos(), org = pos;
	if (!pos) {
		org = pos = curPlayList.GetHeadPosition();
	}

	for (;;) {
		const auto& playlist = curPlayList.GetPrevWrap(pos);
		if ((playlist.m_bInvalid || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
	UpdateList();
	curPlayList.SetPos(pos);
	EnsureVisible(pos);

	return (pos != org);
}

void CPlayerPlaylistBar::SetFirstSelected()
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	if (pos) {
		pos = FindPos(m_list.GetNextSelectedItem(pos));
	} else {
		pos = curPlayList.GetTailPosition();
		POSITION org = pos;
		for (;;) {
			const auto& playlist = curPlayList.GetNextWrap(pos);
			if ((playlist.m_bInvalid || playlist.m_bDirectory) && pos != org) {
				continue;
			}
			break;
		}
	}
	UpdateList();
	curPlayList.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetFirst()
{
	POSITION pos = curPlayList.GetTailPosition(), org = pos;
	for (;;) {
		const auto& playlist = curPlayList.GetNextWrap(pos);
		if ((playlist.m_bInvalid || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
	UpdateList();
	curPlayList.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetLast()
{
	POSITION pos = curPlayList.GetHeadPosition(), org = pos;
	for (;;) {
		const auto& playlist = curPlayList.GetPrevWrap(pos);
		if ((playlist.m_bInvalid || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
	UpdateList();
	curPlayList.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetCurValid(const bool bValid)
{
	POSITION pos = curPlayList.GetPos();
	if (pos) {
		curPlayList.GetAt(pos).m_bInvalid = !bValid;
	}
	UpdateList();
}

void CPlayerPlaylistBar::SetCurLabel(CString label)
{
	POSITION pos = curPlayList.GetPos();
	if (pos) {
		curPlayList.GetAt(pos).m_label = label;
	}
	UpdateList();
}

void CPlayerPlaylistBar::Randomize()
{
	curPlayList.Randomize();
	SetupList();
	SavePlaylist();
	EnsureVisible(curPlayList.GetPos(), true);
}

void CPlayerPlaylistBar::SetCurTime(REFERENCE_TIME rt)
{
	POSITION pos = curPlayList.GetPos();
	if (pos) {
		CPlaylistItem& pli = curPlayList.GetAt(pos);
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

	if (TGetPathType(fn) != FILE) {
		return nullptr;
	}

	m_nCurPlaybackListId = curTab.id;

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

bool CPlayerPlaylistBar::SelectFileInPlaylist(const CString& filename)
{
	if (filename.IsEmpty()) {
		return false;
	}
	POSITION pos = curPlayList.GetHeadPosition();
	while (pos) {
		CPlaylistItem& pli = curPlayList.GetAt(pos);
		if (pli.FindFile(filename)) {
			curPlayList.SetPos(pos);
			EnsureVisible(pos, false);
			return true;
		}
		curPlayList.GetNext(pos);
	}
	return false;
}

void CPlayerPlaylistBar::LoadPlaylist(const CString& filename)
{
	int curpl = m_nCurPlayListIndex;
	for (size_t i = 0; i < m_tabs.size(); i++) {
		m_nCurPlayListIndex = i;
		CString base;
		if (AfxGetMyApp()->GetAppSavePath(base)) {
			base.Append(m_tabs[i].fn);

			if (::PathFileExistsW(base)) {
				if (AfxGetAppSettings().bRememberPlaylistItems) {
					ParseMPCPlayList(base);
				}
				else {
					::DeleteFileW(base);
				}
			}
			else if (m_tabs[i].type == EXPLORER) {
				TParseFolder(L".\\");
			}
		}
	}
	m_nCurPlayListIndex = curpl;

	Refresh();

	if (curPlayList.m_nSelected_idx != INT_MAX) {
		SetSelIdx(curPlayList.m_nSelected_idx + 1, true);
		curPlayList.m_nFocused_idx = curPlayList.m_nSelected_idx;
		curPlayList.m_nSelected_idx = INT_MAX;
	}
	else {
		if (!SelectFileInPlaylist(filename)) {
			EnsureVisible(FindPos(0));
		}
	}

	TDrawBar();
}

void CPlayerPlaylistBar::SavePlaylist()
{
	CString base;
	if (AfxGetMyApp()->GetAppSavePath(base)) {
		CString file = base + curTab.fn;

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

	TSaveSettings();
}

BEGIN_MESSAGE_MAP(CPlayerPlaylistBar, CSizingControlBarG)
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_KEYDOWN, IDC_PLAYLIST, OnLvnKeyDown)
	ON_NOTIFY(NM_DBLCLK, IDC_PLAYLIST, OnNMDblclkList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PLAYLIST, OnCustomdrawList)
	ON_WM_DRAWITEM()
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlayPlay)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_PLAYLIST, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_PLAYLIST, OnLvnEndlabeleditList)
	ON_WM_MEASUREITEM()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

// CPlayerPlaylistBar message handlers

void CPlayerPlaylistBar::ResizeListColumn()
{
	if (::IsWindow(m_list.m_hWnd)) {
		const auto& s = AfxGetAppSettings();

		CRect r;
		GetClientRect(&r);
		r.DeflateRect(2, 2);
		r.top += (m_rcTPage.Height() + 2);
		if (s.bShowPlaylistSearchBar) {
			r.bottom -= m_nSearchBarHeight;
		}

		m_list.SetRedraw(FALSE);
		m_list.MoveWindow(r);
		m_list.GetClientRect(&r);
		m_list.SetColumnWidth(COL_NAME, r.Width() - m_nTimeColWidth);
		m_list.SetRedraw();

		if (s.bUseDarkTheme) {
			InitializeCoolSB(m_list.m_hWnd, ThemeRGB);
			SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
			m_list.GetScrollInfo(SB_VERT, &si);

			CoolSB_SetScrollInfo(m_list.m_hWnd, SB_VERT, &si, TRUE);
			CoolSB_SetStyle(m_list.m_hWnd, SB_VERT, CSBS_HOTTRACKED);
			if (SysVersion::IsWin8orLater()) {
				CoolSB_SetSize(m_list.m_hWnd, SB_VERT, m_pMainFrame->GetSystemMetricsDPI(SM_CYVSCROLL), m_pMainFrame->GetSystemMetricsDPI(SM_CXVSCROLL));
			}

			GetClientRect(&r);
			r.DeflateRect(2, 2);
			r.top += (m_rcTPage.Height() + 2);
			if (s.bShowPlaylistSearchBar) {
				r.bottom -= m_nSearchBarHeight;
			}

			m_list.SetRedraw(FALSE);
			m_list.MoveWindow(r);
			m_list.GetClientRect(&r);
			m_list.SetColumnWidth(COL_NAME, r.Width() - m_nTimeColWidth);
			m_list.SetRedraw();
		} else {
			UninitializeCoolSB(m_list.m_hWnd);
		}
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
	TCalcLayout();
	TCalcREdit();
}

void CPlayerPlaylistBar::OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	*pResult = FALSE;

	switch (pLVKeyDown->wVKey) {
	case VK_DELETE:
		if (curTab.type == PLAYLIST && m_list.GetSelectedCount() > 0) {
			std::vector<int> items;
			items.reserve(m_list.GetSelectedCount());
			POSITION pos = m_list.GetFirstSelectedItemPosition();
			while (pos) {
				items.push_back(m_list.GetNextSelectedItem(pos));
			}
			Remove(items, GetKeyState(VK_SHIFT) < 0);

			*pResult = TRUE;
		}
		break;
	}
}

void CPlayerPlaylistBar::OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;
	if (curTab.type == PLAYLIST) {
		if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0) {
			POSITION pos = FindPos(lpnmlv->iItem);
			// If the file is already playing, don't try to restore a previously saved position
			if (m_pMainFrame->GetPlaybackMode() == PM_FILE && pos == curPlayList.GetPos()) {
				const CPlaylistItem& pli = curPlayList.GetAt(pos);
				AfxGetAppSettings().RemoveFile(pli.m_fns.front());
			}
			else {
				curPlayList.SetPos(pos);
			}

			m_list.Invalidate();
			m_pMainFrame->OpenCurPlaylistItem();
		}
	}
	else if (curTab.type == EXPLORER) {
		if (TNavigate()) {
		}
		else if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0) {
			POSITION pos = FindPos(lpnmlv->iItem);
			// If the file is already playing, don't try to restore a previously saved position
			if (m_pMainFrame->GetPlaybackMode() == PM_FILE && pos == curPlayList.GetPos()) {
				const CPlaylistItem& pli = curPlayList.GetAt(pos);
				AfxGetAppSettings().RemoveFile(pli.m_fns.front());
			}
			else {
				curPlayList.SetPos(pos);
			}

			m_list.Invalidate();
			m_pMainFrame->OpenCurPlaylistItem();
		}
	}

	*pResult = 0;
}

void CPlayerPlaylistBar::OnMouseLeave()
{
	m_tab_idx = -1;
	m_button_idx = -1;
	TDrawBar();
}

void CPlayerPlaylistBar::OnPaint()
{
	CPaintDC dc(this);
	TDrawBar();
	TDrawSearchBar();
}

void CPlayerPlaylistBar::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage) {
		if (SysVersion::IsWin7orLater()) { // under WinXP cause the hang
			ResizeListColumn();
		}

		CRect r;
		GetClientRect(&r);
		FillRect(pLVCD->nmcd.hdc, &r, CBrush(AfxGetAppSettings().bUseDarkTheme ? ThemeRGB(10, 15, 20) : RGB(255, 255, 255)));

		*pResult = CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW;

	}
	/* else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage) {

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

void CPlayerPlaylistBar::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl != IDC_PLAYLIST) {
		if (!nIDCtl && lpDrawItemStruct->CtlType == ODT_MENU && AfxGetAppSettings().bUseDarkTheme && AfxGetAppSettings().bDarkMenu) {
			CMenuEx::DrawItem(lpDrawItemStruct);
			return;
		}

		__super::OnDrawItem(nIDCtl, lpDrawItemStruct);
		return;
	}

	const int nItem = lpDrawItemStruct->itemID;
	const CRect& rcItem = lpDrawItemStruct->rcItem;
	POSITION pos = FindPos(nItem);
	if (!pos) {
		ASSERT(FALSE);
		return;
	}

	const CAppSettings& s = AfxGetAppSettings();
	const auto bSelected = pos == curPlayList.GetPos();
	const CPlaylistItem& pli = curPlayList.GetAt(pos);

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	int R, G, B, R2, G2, B2;
	GRADIENT_RECT gr = { 0, 1 };
	if (!!m_list.GetItemState(nItem, LVIS_SELECTED)) {
		if (s.bUseDarkTheme) {
			ThemeRGB(60, 65, 70, R, G, B);
			ThemeRGB(50, 55, 60, R2, G2, B2);
			TRIVERTEX tv[2] = {
				{rcItem.left, rcItem.top, R * 256, G * 256, B * 256, 255 * 256},
				{rcItem.right, rcItem.bottom, R2 * 256, G2 * 256, B2 * 256, 255 * 256},
			};
			pDC->GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);
			pDC->Draw3dRect(rcItem, ThemeRGB(80, 85, 90), ThemeRGB(30, 35, 40));
		}
		else {
			FillRect(pDC->m_hDC, rcItem, CBrush(0x00f1dacc));
			FrameRect(pDC->m_hDC, rcItem, CBrush(0xc56a31));
		}
	} else {
		if (s.bUseDarkTheme) {
			ThemeRGB(10, 15, 20, R, G, B);
			ThemeRGB(10, 15, 20, R2, G2, B2);
			TRIVERTEX tvs[2] = {
				{rcItem.left, rcItem.top, R * 256, G * 256, B * 256, 255 * 256},
				{rcItem.right, rcItem.bottom, R2 * 256, G2 * 256, B2 * 256, 255 * 256},
			};
			pDC->GradientFill(tvs, 2, &gr, 1, GRADIENT_FILL_RECT_V);
		}
		else {
			FillRect(pDC->m_hDC, rcItem, CBrush(RGB(255, 255, 255)));
		}
	}

	COLORREF textcolor = bSelected ? 0xff : 0;

	if (s.bUseDarkTheme) {
		textcolor = bSelected ? s.clrFaceABGR : (!!m_list.GetItemState(nItem, LVIS_SELECTED) ? ThemeRGB(165, 170, 175) : ThemeRGB(135, 140, 145));
	}

	if (pli.m_bInvalid) {
		textcolor |= 0xA0A0A0;
	}

	CString time = !pli.m_bInvalid ? m_list.GetItemText(nItem, COL_TIME) : L"Invalid";
	CSize timesize(0, 0);
	CPoint timept(rcItem.right, 0);
	if (time.GetLength() > 0) {
		timesize = pDC->GetTextExtent(time);
		if ((3 + timesize.cx + 3) < rcItem.Width() / 2) {
			timept = CPoint(rcItem.right - (3 + timesize.cx + 3), (rcItem.top + rcItem.bottom - timesize.cy) / 2);

			pDC->SetTextColor(textcolor);
			pDC->TextOut(timept.x, timept.y, time);
		}
	}

	CString fmt, file;
	fmt.Format(L"%%0%dd. %%s", (int)log10(0.1 + curPlayList.GetCount()) + 1);
	file.Format(fmt, nItem + 1, m_list.GetItemText(nItem, COL_NAME));

	int offset = 0;
	if (curTab.type == EXPLORER) {
		file = m_list.GetItemText(nItem, COL_NAME);
		const int w = rcItem.Height() - 4;

		bool bfsFolder = false;
		if (file.Right(1) == L"<") {
			file = L"[..]";
			bfsFolder = true;
		}
		else if (file.Right(1) == L">"){
			file.TrimRight(L">");
			file = L"[" + file + L"]";
			bfsFolder = true;
		}

		HICON hIcon = nullptr;
		if (bfsFolder) { // draw Folder Icon
			hIcon = w > 24 ? m_icons_large[L"_folder_"] : m_icons[L"_folder_"];
		}
		else { // draw Files Icon
			CString path;
			if (POSITION pos = FindPos(nItem); pos) {
				const CPlaylistItem& pli = curPlayList.GetAt(pos);
				if (!pli.m_fns.empty()) {
					path = pli.m_fns.front().GetName();
				}
			}

			auto ext = GetFileExt(path).MakeLower();
			if (ext.IsEmpty() && path.Right(1) == L":") {
				ext = path;
			}

			hIcon = w > 24 ? m_icons_large[ext] : m_icons[ext];
		}

		DrawIconEx(pDC->m_hDC, rcItem.left + 2, rcItem.top + 2, hIcon, w, w, 0, nullptr, DI_NORMAL);

		offset = rcItem.Height();
	}

	CSize filesize = pDC->GetTextExtent(file);
	filesize.cx += offset;
	while (3 + filesize.cx + 6 > timept.x && file.GetLength() > 3) {
		file = file.Left(file.GetLength() - 4) + L"...";
		filesize = pDC->GetTextExtent(file);
		filesize.cx += offset;
	}

	if (file.GetLength() > 3 || curTab.type == EXPLORER) { // L"C:".GetLenght() < 3
		pDC->SetTextColor(textcolor);
		pDC->TextOutW(rcItem.left + 3 + offset, (rcItem.top + rcItem.bottom - filesize.cy) / 2, file);
	}
}

BOOL CPlayerPlaylistBar::OnPlayPlay(UINT nID)
{
	m_list.Invalidate();
	return FALSE;
}

void CPlayerPlaylistBar::DropFiles(std::list<CString>& slFiles)
{
	if (curTab.type == EXPLORER) {
		return;
	}

	SetForegroundWindow();
	m_list.SetFocus();

	m_pMainFrame->ParseDirs(slFiles);

	Append(slFiles, true);
}

void CPlayerPlaylistBar::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (curTab.type == EXPLORER) {
		return;
	}

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
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
	tme.hwndTrack = GetSafeHwnd();
	tme.dwFlags = TME_LEAVE;
	TrackMouseEvent(&tme);

	TDrawBar();

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

void CPlayerPlaylistBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CRect rcBar;
	GetClientRect(&rcBar);

	if (!rcBar.PtInRect(point)) {
		__super::OnLButtonDblClk(nFlags, point);
	}
}

void CPlayerPlaylistBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rcBar;
	GetClientRect(&rcBar);

	if (!rcBar.PtInRect(point)) {
		__super::OnLButtonDown(nFlags, point);
	}

	if (m_tab_buttons[MENU].r.PtInRect(point)) {
		TOnMenu();
		return;
	}
	else if (m_tab_buttons[RIGHT].bVisible && m_tab_buttons[RIGHT].r.PtInRect(point)) {
		TSetOffset();
		TCalcLayout();
		TDrawBar();
	}
	else if (m_tab_buttons[LEFT].bVisible && m_tab_buttons[LEFT].r.PtInRect(point)) {
		TSetOffset(true);
		TCalcLayout();
		TDrawBar();
	}
	else {
		for (size_t i = 0; i < m_tabs.size(); i++) {
			if (m_tabs[i].r.PtInRect(point)) {
				SavePlaylist();
				curPlayList.m_nFocused_idx = TGetFocusedElement();
				m_nCurPlayListIndex = i;
				TSelectTab();
				return;
			}
		}
	}
	m_list.SetFocus();
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
		CPlaylistItem& pli = curPlayList.GetAt(pos);
		tmp.push_back(pli);
		if (pos == curPlayList.GetPos()) {
			id = pli.m_id;
		}
	}
	curPlayList.RemoveAll();
	auto it = tmp.begin();
	for (int i = 0; it != tmp.end(); i++) {
		CPlaylistItem& pli = *it++;
		curPlayList.AddTail(pli);
		if (pli.m_id == id) {
			curPlayList.SetPos(curPlayList.GetTailPosition());
		}
		m_list.SetItemData(i, (DWORD_PTR)curPlayList.GetTailPosition());
	}

	ResizeListColumn();
}

BOOL CPlayerPlaylistBar::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTW* pTTT = (TOOLTIPTEXTW*)pNMHDR;

	if ((HWND)pTTT->lParam != m_list.m_hWnd || !AfxGetAppSettings().bShowPlaylistTooltip) {
		return FALSE;
	}

	const int row = ((pNMHDR->idFrom - 1) >> 10) & 0x3fffff;
	const int col = (pNMHDR->idFrom - 1) & 0x3ff;

	if (row < 0 || row >= curPlayList.GetCount() || col == COL_TIME) {
		return FALSE;
	}

	const CPlaylistItem& pli = curPlayList.GetAt(FindPos(row));

	static CString strTipText; // static string
	strTipText.Empty();

	for (const auto& fi : pli.m_fns) {
		strTipText += L"\n" + fi.GetName();
	}
	strTipText.Trim();
	if (pli.m_bDirectory) {
		strTipText.TrimRight(L"<>");
	}

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

	pTTT->lpszText = (LPWSTR)strTipText.GetString();

	*pResult = 0;

	return TRUE;
}

void CPlayerPlaylistBar::OnContextMenu(CWnd* /*pWnd*/, CPoint p)
{
	CRect rcBar;
	GetClientRect(&rcBar);
	rcBar.bottom = rcBar.top + m_rcTPage.Height() + 2;

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	if (rcBar.PtInRect(pt)) {
		TOnMenu(true);
		return;
	}
	//else if (WindowFromPoint(pt) == &m_list) {
	//
	//}

	if (p.x == -1 && p.y == -1 && m_list.GetSelectedCount()) {
		// hack for the menu invoked with the VK_APPS key
		POSITION pos = m_list.GetFirstSelectedItemPosition();
		int nItem = m_list.GetNextSelectedItem(pos);
		RECT r;
		if (m_list.GetItemRect(nItem, &r, LVIR_LABEL)) {
			p.x = r.left;
			p.y = r.bottom;
			m_list.ClientToScreen(&p);
		}
	}

	LVHITTESTINFO lvhti;
	lvhti.pt = p;
	m_list.ScreenToClient(&lvhti.pt);
	m_list.SubItemHitTest(&lvhti);

	POSITION pos = FindPos(lvhti.iItem);
	bool bMIEnable = false;
	CString sCurrentPath;
	if (pos) {
		CPlaylistItem& pli = curPlayList.GetAt(pos);
		if (!pli.m_fns.empty()) {
			sCurrentPath = pli.m_fns.front().GetName();
			bMIEnable = !::PathIsURLW(sCurrentPath) && sCurrentPath != L"pipe://stdin";

			if (bMIEnable && curTab.type == EXPLORER) {
				bMIEnable = TGetPathType(sCurrentPath) == FILE;
			}
		}
	}
	const bool bOnItem = !!(lvhti.flags & LVHT_ONITEM);

	CMenu m;
	m.CreatePopupMenu();

	enum operation {
		M_OPEN = 1,
		M_ADD,
		M_REMOVE,
		M_DELETE,
		M_CLEAR,
		M_TOCLIPBOARD,
		M_FROMCLIPBOARD,
		M_SAVEAS,
		M_SORTBYNAME,
		M_SORTBYPATH,
		M_SORTBYDATE,
		M_SORTBYDATECREATED,
		M_SORTBYSIZE,
		M_SORTREVERSE,
		M_RANDOMIZE,
		M_SORTBYID, // restore
		M_SHUFFLE,
		M_REFRESH,
		M_MEDIAINFO,
		M_SHOWTOOLTIP,
		M_SHOWSEARCHBAR,
		M_HIDEFULLSCREEN,
		M_NEXTONERROR
	};

	CAppSettings& s = AfxGetAppSettings();

	const bool bExplorer = curTab.type == EXPLORER;
	m.AppendMenu(MF_STRING | (bOnItem ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_OPEN, ResStr(IDS_PLAYLIST_OPEN) + L"\tEnter");
	if (!bExplorer) {
		m.AppendMenu(MF_STRING | MF_ENABLED, M_ADD, ResStr(IDS_PLAYLIST_ADD));
		m.AppendMenu(MF_STRING | (bOnItem ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_REMOVE, ResStr(IDS_PLAYLIST_REMOVE) + L"\tDelete");
		m.AppendMenu(MF_SEPARATOR);
		m.AppendMenu(MF_STRING | (bOnItem ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_DELETE, ResStr(IDS_PLAYLIST_DELETE) + L"\tShift+Delete");
		m.AppendMenu(MF_SEPARATOR);
		m.AppendMenu(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_CLEAR, ResStr(IDS_PLAYLIST_CLEAR));
		m.AppendMenu(MF_SEPARATOR);
	}
	m.AppendMenu(MF_STRING | (m_list.GetSelectedCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_TOCLIPBOARD, ResStr(IDS_PLAYLIST_COPYTOCLIPBOARD) + L"\tCltr+C");
	if (bExplorer) {
		const bool bReverse = !!(curTab.sort >> 8);
		const auto sort = (SORT)(curTab.sort & 0xF);

		m.AppendMenu(MF_SEPARATOR);
		CMenu submenu2;
		submenu2.CreatePopupMenu();
		submenu2.AppendMenu(MF_STRING | MF_ENABLED | (sort == SORT::NAME ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYNAME, ResStr(IDS_PLAYLIST_SORTBYNAME));
		submenu2.AppendMenu(MF_STRING | MF_ENABLED | (sort == SORT::DATE ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYDATE, ResStr(IDS_PLAYLIST_SORTBYDATE));
		submenu2.AppendMenu(MF_STRING | MF_ENABLED | (sort == SORT::DATE_CREATED ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYDATECREATED, ResStr(IDS_PLAYLIST_SORTBYDATECREATED));
		submenu2.AppendMenu(MF_STRING | MF_ENABLED | (sort == SORT::SIZE ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYSIZE, ResStr(IDS_PLAYLIST_SORTBYSIZE));
		submenu2.AppendMenu(MF_SEPARATOR);
		submenu2.AppendMenu(MF_STRING | MF_ENABLED | (bReverse ? MF_CHECKED : MF_UNCHECKED), M_SORTREVERSE, ResStr(IDS_PLAYLIST_SORTREVERSE));
		m.AppendMenu(MF_STRING | MF_POPUP | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), (UINT_PTR)submenu2.Detach(), ResStr(IDS_PLAYLIST_SORT));
		m.AppendMenu(MF_SEPARATOR);

		m.AppendMenu(MF_STRING | MF_ENABLED, M_REFRESH, ResStr(IDS_PLAYLIST_EXPLORER_REFRESH));
		m.AppendMenu(MF_SEPARATOR);
	}
	else {
		m.AppendMenu(MF_STRING | (::IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_FROMCLIPBOARD, ResStr(IDS_PLAYLIST_PASTEFROMCLIPBOARD) + L"\tCltr+V");
		m.AppendMenu(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_SAVEAS, ResStr(IDS_PLAYLIST_SAVEAS));
		m.AppendMenu(MF_SEPARATOR);
		CMenu submenu2;
		submenu2.CreatePopupMenu();
		submenu2.AppendMenu(MF_STRING | MF_ENABLED, M_SORTBYNAME, ResStr(IDS_PLAYLIST_SORTBYLABEL));
		submenu2.AppendMenu(MF_STRING | MF_ENABLED, M_SORTBYPATH, ResStr(IDS_PLAYLIST_SORTBYPATH));
		submenu2.AppendMenu(MF_STRING | MF_ENABLED, M_SORTREVERSE, ResStr(IDS_PLAYLIST_SORTREVERSE));
		m.AppendMenu(MF_STRING | MF_POPUP | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), (UINT_PTR)submenu2.Detach(), ResStr(IDS_PLAYLIST_SORT));
		m.AppendMenu(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_RANDOMIZE, ResStr(IDS_PLAYLIST_RANDOMIZE));
		m.AppendMenu(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_SORTBYID, ResStr(IDS_PLAYLIST_RESTORE));
		m.AppendMenu(MF_SEPARATOR);
		m.AppendMenu(MF_STRING | MF_ENABLED | (s.bShufflePlaylistItems ? MF_CHECKED : MF_UNCHECKED), M_SHUFFLE, ResStr(IDS_PLAYLIST_SHUFFLE));
		m.AppendMenu(MF_SEPARATOR);
	}
	m.AppendMenu(MF_STRING | (bOnItem && bMIEnable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_MEDIAINFO, L"MediaInfo");
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | MF_ENABLED | (s.bShowPlaylistTooltip ? MF_CHECKED : MF_UNCHECKED), M_SHOWTOOLTIP, ResStr(IDS_PLAYLIST_SHOWTOOLTIP));
	m.AppendMenu(MF_STRING | MF_ENABLED | (s.bShowPlaylistSearchBar ? MF_CHECKED : MF_UNCHECKED), M_SHOWSEARCHBAR, ResStr(IDS_PLAYLIST_SHOWSEARCHBAR));
	m.AppendMenu(MF_STRING | MF_ENABLED | (s.bHidePlaylistFullScreen ? MF_CHECKED : MF_UNCHECKED), M_HIDEFULLSCREEN, ResStr(IDS_PLAYLIST_HIDEFS));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | MF_ENABLED | (s.bPlaylistNextOnError ? MF_CHECKED : MF_UNCHECKED), M_NEXTONERROR, ResStr(IDS_PLAYLIST_NEXTONERROR));

	if (s.bUseDarkTheme && s.bDarkMenu) {
		MENUINFO MenuInfo = { 0 };
		MenuInfo.cbSize = sizeof(MenuInfo);
		MenuInfo.hbrBack = m_pMainFrame->m_hPopupMenuBrush;
		MenuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
		SetMenuInfo(m.GetSafeHmenu(), &MenuInfo);

		CMenuEx::ChangeStyle(&m);
	}

	int nID = (int)m.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RETURNCMD, p.x, p.y, this);

	switch (nID) {
		case M_OPEN:
			if (TNavigate()) {
				return;
			}
			curPlayList.SetPos(pos);
			m_list.Invalidate();
			m_pMainFrame->OpenCurPlaylistItem();
			break;
		case M_ADD:
			{
				if (bExplorer) {
					return;
				}
				if (m_pMainFrame->GetPlaybackMode() == PM_CAPTURE) {
					m_pMainFrame->AddCurDevToPlaylist();
					curPlayList.SetPos(curPlayList.GetTailPosition());
				} else {
					CString filter;
					std::vector<CString> mask;
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
		case M_DELETE:
			{
				if (bExplorer) {
					return;
				}
				std::vector<int> items;
				items.reserve(m_list.GetSelectedCount());
				POSITION pos = m_list.GetFirstSelectedItemPosition();
				while (pos) {
					items.push_back(m_list.GetNextSelectedItem(pos));
				}

				Remove(items, nID == M_DELETE);
			}
			break;
		case M_CLEAR:
			if (bExplorer) {
				return;
			}
			if (Empty()) {
				CloseMedia();
			}
			break;
		case M_SORTBYID:
			if (bExplorer) {
				return;
			}
			curPlayList.SortById();
			SetupList();
			SavePlaylist();
			EnsureVisible(curPlayList.GetPos(), true);
			break;
		case M_SORTBYNAME:
			if (bExplorer) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::NAME) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::NAME;
					TSaveSettings();
					TFillPlaylist();
				}
				return;
			}
			curPlayList.SortByName();
			SetupList();
			SavePlaylist();
			EnsureVisible(curPlayList.GetPos(), true);
			break;
		case M_SORTBYPATH:
			if (bExplorer) {
				return;
			}
			curPlayList.SortByPath();
			SetupList();
			SavePlaylist();
			EnsureVisible(curPlayList.GetPos(), true);
			break;
		case M_SORTBYDATE:
			if (bExplorer) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::DATE) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::DATE;
					TSaveSettings();
					TFillPlaylist();
				}
				return;
			}
			break;
		case M_SORTBYDATECREATED:
			if (bExplorer) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::DATE_CREATED) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::DATE_CREATED;
					TSaveSettings();
					TFillPlaylist();
				}
				return;
			}
			break;
		case M_SORTBYSIZE:
			if (bExplorer) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::SIZE) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::SIZE;
					TSaveSettings();
					TFillPlaylist();
				}
				return;
			}
			break;
		case M_SORTREVERSE:
			if (bExplorer) {
				const auto bReverse = curTab.sort >> 8;
				const auto sort = (SORT)(curTab.sort & 0xF);
				curTab.sort = (!bReverse << 8) | sort;
				TSaveSettings();
				TFillPlaylist();
				return;
			}
			curPlayList.ReverseSort();
			SetupList();
			SavePlaylist();
			EnsureVisible(curPlayList.GetPos(), true);
			break;
		case M_RANDOMIZE:
			if (bExplorer) {
				return;
			}
			Randomize();
			break;
		case M_TOCLIPBOARD:
			CopyToClipboard();
			break;
		case M_FROMCLIPBOARD:
			PasteFromClipboard();
			break;
		case M_SAVEAS: {
			if (bExplorer) {
				return;
			}
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

			pos = curPlayList.GetHeadPosition();
			while (pos && fRemovePath) {
				CPlaylistItem& pli = curPlayList.GetNext(pos);

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

			pos = curPlayList.GetHeadPosition();
			CString str;
			int i;
			for (i = 0; pos; i++) {
				CPlaylistItem& pli = curPlayList.GetNext(pos);

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
					str.Format(L"#EXTINF:%I64d,%s\n",
						pli.m_duration > 0 ? (pli.m_duration + UNITS/2) / UNITS : -1LL,
						pli.m_label.GetString());
					f.WriteString(str.GetString());
				}

				switch (idx) {
					case 2:
						str.Format(L"File%d=%s\n", i + 1, fn.GetString());
						break;
					case 3:
						str.Format(L"%s\n", fn.GetString());
						break;
					case 4:
						str.Format(L"<Entry><Ref href = \"%s\"/></Entry>\n", fn.GetString());
						break;
					default:
						break;
				}
				f.WriteString(str.GetString());
			}

			if (idx == 2) {
				str.Format(L"NumberOfEntries=%d\n", i);
				f.WriteString(str.GetString());
				f.WriteString(L"Version=2\n");
			} else if (idx == 4) {
				f.WriteString(L"</ASX>\n");
			}
		}
		break;
		case M_SHUFFLE:
			if (bExplorer) {
				return;
			}
			s.bShufflePlaylistItems = !s.bShufflePlaylistItems;
			break;
		case M_REFRESH:
			if (bExplorer) {
				CString selected_path;
				const auto focusedElement = TGetFocusedElement();
				POSITION pos = curPlayList.FindIndex(focusedElement);
				if (pos) {
					selected_path = curPlayList.GetAt(pos).m_fns.front().GetName();
				}

				auto path = curPlayList.GetHead().m_fns.front().GetName();
				if (path.Right(1) == L"<") {
					path.TrimRight(L"<");
					curPlayList.RemoveAll();
					if (::PathFileExistsW(path)) {
						TParseFolder(path);
					} else {
						TParseFolder(L".\\");
					}
				} else {
					curPlayList.RemoveAll();
					TParseFolder(L".\\");
				}

				Refresh();

				if (!SelectFileInPlaylist(selected_path)) {
					EnsureVisible(curPlayList.GetHeadPosition());
				}
			}
			break;
		case M_MEDIAINFO:
			{
				const auto nID = s.nLastFileInfoPage;
				s.nLastFileInfoPage = IDD_FILEMEDIAINFO;
				CPPageFileInfoSheet m_fileinfo(sCurrentPath, m_pMainFrame, m_pMainFrame, true);
				m_fileinfo.DoModal();
				s.nLastFileInfoPage = nID;
			}
			break;
		case M_SHOWTOOLTIP:
			s.bShowPlaylistTooltip = !s.bShowPlaylistTooltip;
			break;
		case M_SHOWSEARCHBAR:
			s.bShowPlaylistSearchBar = !s.bShowPlaylistSearchBar;
			ResizeListColumn();
			TCalcLayout();
			TCalcREdit();
			TDrawBar();
			TDrawSearchBar();
			break;
		case M_HIDEFULLSCREEN:
			s.bHidePlaylistFullScreen = !s.bHidePlaylistFullScreen;
			break;
		case M_NEXTONERROR:
			s.bPlaylistNextOnError = !s.bPlaylistNextOnError;
			break;
		default:
			break;
	}
}

void CPlayerPlaylistBar::OnLvnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (pDispInfo->item.iItem >= 0 && pDispInfo->item.pszText) {
		CPlaylistItem& pli = curPlayList.GetAt((POSITION)m_list.GetItemData(pDispInfo->item.iItem));
		pli.m_label = pDispInfo->item.pszText;
		m_list.SetItemText(pDispInfo->item.iItem, 0, pDispInfo->item.pszText);
	}

	*pResult = 0;
}

void CPlayerPlaylistBar::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (!nIDCtl && lpMeasureItemStruct->CtlType == ODT_MENU && AfxGetAppSettings().bUseDarkTheme && AfxGetAppSettings().bDarkMenu) {
		CMenuEx::MeasureItem(lpMeasureItemStruct);
		return;
	}

	__super::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CPlayerPlaylistBar::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);

	if (IsWindow(m_list.GetSafeHwnd())) {
		m_list.SetFocus();
	}
}

void CPlayerPlaylistBar::TCalcLayout()
{
	CClientDC dc(this);
	CDC mdc;
	mdc.CreateCompatibleDC(&dc);
	mdc.SelectObject(&m_font);

	CRect rcTabBar;
	GetClientRect(&rcTabBar);

	rcTabBar.bottom = rcTabBar.top + m_iTFontSize + m_pMainFrame->ScaleY(WIDTH_TABBUTTON / 2);
	rcTabBar.DeflateRect(2, 1, 2, 1);

	const int wSysButton = rcTabBar.Height();

	m_rcTPage = rcTabBar;
	m_rcTPage.right = rcTabBar.right - wSysButton;

	m_tab_buttons[RIGHT].bVisible = false;
	m_tab_buttons[LEFT].bVisible = false;

	int wTsSize = 0;
	for (size_t i = 0; i < m_tabs.size(); i++) {
		wTsSize += mdc.GetTextExtent(m_tabs[i].name).cx + WIDTH_TABBUTTON;
	}
	m_tab_buttons[MENU].r = rcTabBar;
	m_tab_buttons[MENU].r.left = rcTabBar.right - wSysButton;
	m_tab_buttons[MENU].r.right = m_tab_buttons[MENU].r.left + wSysButton;


	if (wTsSize > rcTabBar.right - wSysButton || m_cntOffset > 0) {
		m_rcTPage.left = rcTabBar.left + wSysButton;
		m_rcTPage.right = rcTabBar.right - wSysButton * 2 - 3;
		m_tab_buttons[LEFT].r = rcTabBar;
		m_tab_buttons[RIGHT].r = rcTabBar;
		m_tab_buttons[LEFT].bVisible = true;
		m_tab_buttons[RIGHT].bVisible = true;
		m_tab_buttons[RIGHT].r.left = rcTabBar.right - wSysButton * 2;
		m_tab_buttons[RIGHT].r.right = m_tab_buttons[RIGHT].r.left + wSysButton;
		m_tab_buttons[LEFT].r.left = rcTabBar.left;
		m_tab_buttons[LEFT].r.right = m_tab_buttons[LEFT].r.left + wSysButton;

	}
	else if (wTsSize <= rcTabBar.right - wSysButton) {
		m_rcTPage.left = rcTabBar.left;
	}

	for (size_t i = 0; i < m_tabs.size(); i++) {
		auto& tab = m_tabs[i];

		tab.r = rcTabBar;
		int wTab = mdc.GetTextExtent(tab.name).cx + WIDTH_TABBUTTON;
		if (i == 0) {
			tab.r.left = m_rcTPage.left - TGetOffset();
			tab.r.right = tab.r.left + wTab;
		}
		else {
			tab.r.left = m_tabs[i - 1].r.right;
			tab.r.right = tab.r.left + wTab;
		}
	}

	mdc.DeleteDC();

	CRect rcWindow, rcClient;
	GetWindowRect(&rcWindow);
	GetClientRect(&rcClient);
	const int width = wSysButton * (m_tabs.size() > 1 ? 3 : 1) + m_tabs[0].r.Width() + (rcWindow.Width() - rcClient.Width()) * 2 - 3;
	const CSize cz(width, rcTabBar.Height() * 3 + (AfxGetAppSettings().bShowPlaylistSearchBar ? m_nSearchBarHeight * 2 : 0));
	m_szMinFloat = m_szMinHorz = m_szMinVert = m_szFixedFloat = cz;
}

void CPlayerPlaylistBar::TCalcREdit()
{
	if (IsWindow(m_REdit.GetSafeHwnd()) && AfxGetAppSettings().bShowPlaylistSearchBar) {
		CRect rcTabBar;
		GetClientRect(&rcTabBar);

		CRect rc(rcTabBar);
		rc.DeflateRect(3, 1);
		rc.top = rc.bottom - m_nSearchBarHeight + 2;
		m_REdit.MoveWindow(rc.left, rc.top, rc.Width(), rc.Height());
	}
}

void CPlayerPlaylistBar::TIndexHighighted()
{
	CPoint p;
	GetCursorPos(&p);
	ScreenToClient(&p);

	m_tab_idx = -1;
	m_button_idx = -1;
	for (size_t i = 0; i < m_tabs.size(); i++) {
		if (m_tabs[i].r.PtInRect(p) && m_rcTPage.PtInRect(p)) {
			m_tab_idx = i;
			return;
		}
	}
	for (size_t i = 0; i < std::size(m_tab_buttons); i++) {
		if (m_tab_buttons[i].bVisible && m_tab_buttons[i].r.PtInRect(p)) {
			m_button_idx = i;
			return;
		}
	}
}

void CPlayerPlaylistBar::TDrawBar()
{
	CClientDC dc(this);
	if (IsWindowVisible()) {
		TIndexHighighted();

		CDC mdc;
		mdc.CreateCompatibleDC(&dc);
		mdc.SelectObject(&m_font);

		CRect rcTabBar;
		GetClientRect(&rcTabBar);
		rcTabBar.bottom = rcTabBar.top + m_rcTPage.Height() + 2;

		CBitmap bm;
		bm.CreateCompatibleBitmap(&dc, rcTabBar.Width(), rcTabBar.Height());
		CBitmap* pOldBm = mdc.SelectObject(&bm);
		mdc.SetBkMode(TRANSPARENT);

		// background Tab bar
		mdc.FillSolidRect(rcTabBar, m_crBkBar);

		for (size_t i = 0; i < std::size(m_tab_buttons); i++) {
			auto& button = m_tab_buttons[i];
			if (button.bVisible) {
				// Buttons
				mdc.Draw3dRect(button.r, m_crBNL, m_crBND);
				mdc.SetTextColor(m_crTN);
				mdc.DrawText(button.name, button.name.GetLength(), &button.r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				// Highlighted Buttons
				if (i == m_button_idx) {
					mdc.SetTextColor(m_crTH);
					mdc.FillSolidRect(button.r, m_crBH);
					mdc.Draw3dRect(button.r, m_crBHL, m_crBHD);
					mdc.DrawText(button.name, button.name.GetLength(), &button.r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}
			}
		}
		//HRGN hrgn;
		//GetClipRgn(mdc, hrgn);

		HRGN hRgnExclude = ::CreateRectRgnIndirect(&m_rcTPage);
		SelectClipRgn(mdc, hRgnExclude);
		for (size_t i = 0; i < m_tabs.size(); i++) {
			auto& tab = m_tabs[i];
			CRect rcText = tab.r;
			rcText.DeflateRect(2, 2, 2, 2);
			// tab
			mdc.SetTextColor(m_crTN);
			mdc.DrawText(tab.name.GetString(), tab.name.GetLength(), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			// higjlighted tab
			if (i == m_tab_idx && m_tab_idx != m_nCurPlayListIndex) {
				mdc.SetTextColor(m_crTH);
				mdc.FillSolidRect(tab.r, m_crBH);
				mdc.Draw3dRect(tab.r, m_crBHL, m_crBHL);
				mdc.DrawText(tab.name.GetString(), tab.name.GetLength(), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			// current activetab
			if (i == m_nCurPlayListIndex) {
				if (i == m_tab_idx) { // selected and highlighted
					mdc.SetTextColor(m_crTS);
					mdc.FillSolidRect(tab.r, m_crBSH);
					mdc.Draw3dRect(tab.r, m_crBSHL, m_crBSHL);
					mdc.DrawText(tab.name.GetString(), tab.name.GetLength(), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}
				else { // selected
					mdc.FillSolidRect(tab.r, m_crBS);
					mdc.SetTextColor(m_crTS);
					mdc.Draw3dRect(tab.r, m_crBSL, m_crBSL);
					mdc.DrawText(tab.name.GetString(), tab.name.GetLength(), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}
			}
		}
		//SelectClipRgn(mdc, hrgn);

		dc.BitBlt(0, 0, rcTabBar.Width(), rcTabBar.Height(), &mdc, 0, 0, SRCCOPY);

		mdc.SelectObject(pOldBm);
		bm.DeleteObject();
		mdc.DeleteDC();
	}
}

void CPlayerPlaylistBar::TDrawSearchBar()
 {
	CClientDC dc(this);
	if (IsWindowVisible() && AfxGetAppSettings().bShowPlaylistSearchBar) {
		CRect rc;
		GetClientRect(&rc);
		rc.DeflateRect(2, 0);
		rc.top = rc.bottom - m_nSearchBarHeight;

		CDC mdc;
		mdc.CreateCompatibleDC(&dc);
		CBitmap bm;
		bm.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());

		CBitmap* pOldBm = mdc.SelectObject(&bm);
		mdc.SetBkMode(TRANSPARENT);

		//mdc.FillSolidRect(0, 0, rc.Width(), rc.Height(), m_crBND);
		mdc.Draw3dRect(0, 0, rc.Width(), rc.Height(), m_crBNL, m_crBNL);
		dc.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(), &mdc, 0, 0, SRCCOPY);

		mdc.SelectObject(pOldBm);
		bm.DeleteObject();
		mdc.DeleteDC();
	}
}

void CPlayerPlaylistBar::TOnMenu(bool bUnderCursor)
{
	m_button_idx = -1;

	CPoint p;
	if (bUnderCursor) {
		GetCursorPos(&p);
	}
	else {
		p.x = m_tab_buttons[MENU].r.left;
		p.y = m_tab_buttons[MENU].r.bottom;
		ClientToScreen(&p);
	}

	CMenu menu;
	menu.CreatePopupMenu();

	CAppSettings& s = AfxGetAppSettings();

	UINT id = 1;
	CMenu submenu;
	for (size_t i = 0; i < m_tabs.size(); i++) {
		UINT flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
		if (i == m_nCurPlayListIndex) {
			flags |= MF_CHECKED | MFT_RADIOCHECK;
		}
		menu.AppendMenuW(flags, id++, m_tabs[i].name);
	}
	menu.AppendMenuW(MF_SEPARATOR);

	submenu.CreatePopupMenu();
	submenu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_ENABLED, id++, ResStr(IDS_PLAYLIST_ADD_PLAYLIST));
	submenu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_ENABLED, id++, ResStr(IDS_PLAYLIST_ADD_EXPLORER));
	menu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)submenu.Detach(), ResStr(IDS_PLAYLIST_ADD_NEW));
	menu.AppendMenuW(MF_BYPOSITION | MF_STRING | ((m_nCurPlayListIndex > 0) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED))
		, id++, ResStr(IDS_PLAYLIST_RENAME_CURRENT));
	menu.AppendMenuW(MF_BYPOSITION | MF_STRING | ((m_nCurPlayListIndex > 0) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED))
		, id++, ResStr(IDS_PLAYLIST_DELETE_CURRENT));

	if (s.bUseDarkTheme && s.bDarkMenu) {
		MENUINFO MenuInfo = { 0 };
		MenuInfo.cbSize = sizeof(MenuInfo);
		MenuInfo.hbrBack = m_pMainFrame->m_hPopupMenuBrush;
		MenuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
		SetMenuInfo(menu.GetSafeHmenu(), &MenuInfo);

		CMenuEx::ChangeStyle(&menu);
	}

	CString strDefName;
	CString strGetName;

	int nID = (int)menu.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RETURNCMD, p.x, p.y, this);
	int size = m_tabs.size();

	bool bNewExplorer = false;

	if (nID > 0 && nID <= size) {
		SavePlaylist();
		curPlayList.m_nFocused_idx = TGetFocusedElement();
		m_nCurPlayListIndex = nID - 1;
		TEnsureVisible(m_nCurPlayListIndex);
		TSelectTab();
	} else {
		int element = (nID - size);

		switch (element) {
			case 1: // ADD PLAYLIST TAB
				{
					size_t cnt = 1;
					for (size_t i = 0; i < m_tabs.size(); i++) {
						if (m_tabs[i].type == PLAYLIST && i > 0) {
							cnt++;
						}
					}

					strDefName.Format(L"%s %u", ResStr(IDS_PLAYLIST_NAME).GetString(), cnt);
					CPlaylistNameDlg dlg(strDefName);
					if (dlg.DoModal() != IDOK) {
						return;
					}
					strGetName = dlg.m_name;
					if (strGetName.IsEmpty()) {
						return;
					}

					tab_t tab;
					tab.type = PLAYLIST;
					tab.name = strGetName;
					tab.fn.Format(L"Playlist%u.mpcpl", cnt);
					tab.id = GetNextId();
					m_tabs.insert(m_tabs.begin() + m_nCurPlayListIndex + 1, tab);

					CPlaylist* pl = DNew CPlaylist;
					m_pls.insert(m_pls.begin() + m_nCurPlayListIndex + 1, pl);

					SavePlaylist(); // save current playlist
					m_nCurPlayListIndex++;
					SavePlaylist(); //save new playlist

					TSelectTab();
					TEnsureVisible(m_nCurPlayListIndex);
					TCalcLayout();
				}
				break;
			case 2: // ADD EXPLORER TAB
				{
					size_t cnt = 1;
					for (size_t i = 0; i < m_tabs.size(); i++) {
						if (m_tabs[i].type == EXPLORER) {
							cnt++;
						}
					}
					strDefName.Format(L"%s %u", ResStr(IDS_PLAYLIST_EXPLORER_NAME).GetString(), cnt);
					CPlaylistNameDlg dlg(strDefName);
					if (dlg.DoModal() != IDOK) {
						return;
					}
					strGetName = dlg.m_name;
					if (strGetName.IsEmpty()) {
						return;
					}

					tab_t tab;
					tab.type = EXPLORER;
					tab.name = strGetName;
					tab.fn.Format(L"Explorer%u.mpcpl", cnt);
					tab.id = GetNextId();
					m_tabs.insert(m_tabs.begin() + m_nCurPlayListIndex + 1, tab);

					CPlaylist* pl = DNew CPlaylist;
					m_pls.insert(m_pls.begin() + m_nCurPlayListIndex + 1, pl);

					SavePlaylist(); //save current playlist
					m_nCurPlayListIndex++;
					SavePlaylist(); //save new playlist

					TSelectTab();
					TEnsureVisible(m_nCurPlayListIndex);
					TCalcLayout();
					TParseFolder(L".\\");

					bNewExplorer = true;
				}
				break;
			case 3: // RENAME TAB
				{
					strDefName = curTab.name;
					CPlaylistNameDlg dlg(strDefName);
					if (dlg.DoModal() != IDOK) {
						return;
					}
					strGetName = dlg.m_name;
					if (strGetName.IsEmpty()) {
						return;
					}
					curTab.name = strGetName;
					SavePlaylist();
				}
				break;
			case 4: // DELETE TAB
				{
					if (Empty()) {
						CloseMedia();
					}

					CString base;
					if (AfxGetMyApp()->GetAppSavePath(base)) {
						base.Append(curTab.fn);

						if (::PathFileExistsW(base)) {
							::DeleteFileW(base);
						}
					}

					m_tabs.erase(m_tabs.begin() + m_nCurPlayListIndex);
					SAFE_DELETE(m_pls[m_nCurPlayListIndex]);
					m_pls.erase(m_pls.begin() + m_nCurPlayListIndex);

					if (m_tabs.size() <= m_nCurPlayListIndex) {
						m_nCurPlayListIndex--;
					}

					TSelectTab();
					TEnsureVisible(m_nCurPlayListIndex);
					TCalcLayout();
				}
				break;
			case 0:
			default:
				return;
		}
	}

	Refresh();
	TCalcLayout();
	TDrawBar();

	if (bNewExplorer) {
		EnsureVisible(FindPos(0));
	}
	else {
		EnsureVisible(FindPos(curPlayList.m_nFocused_idx));
	}
}

void CPlayerPlaylistBar::TDeleteAllPlaylists()
{
	m_pMainFrame->SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	for (size_t i = 0; i < m_tabs.size(); i++) {
		CString base;
		if (AfxGetMyApp()->GetAppSavePath(base)) {
			base.Append(m_tabs[i].fn);

			if (::PathFileExistsW(base)) {
				::DeleteFileW(base);
			}
		}
	}
}

void CPlayerPlaylistBar::TSaveAllPlaylists()
{
	const auto nCurPlayListIndex = m_nCurPlayListIndex;

	for (size_t i = 0; i < m_tabs.size(); i++) {
		m_nCurPlayListIndex = i;
		SavePlaylist();
	}

	m_nCurPlayListIndex = nCurPlayListIndex;
}

void CPlayerPlaylistBar::TSelectTab()
{
	TSaveSettings();
	Refresh();

	if (curPlayList.m_nSelected_idx != INT_MAX) {
		SetSelIdx(curPlayList.m_nSelected_idx + 1, true);
		curPlayList.m_nSelected_idx = INT_MAX;
	}

	TCalcLayout();
	EnsureVisible(FindPos(curPlayList.m_nFocused_idx));
	if (IsWindowVisible()) {
		TDrawBar();
		m_list.SetFocus();
	}
}

void CPlayerPlaylistBar::TParseFolder(const CString& path)
{
	CAppSettings& s = AfxGetAppSettings();
	CMediaFormats& mf = s.m_Formats;

	SHFILEINFOW shFileInfo = {};
	UINT uFlags = SHGFI_ICON | SHGFI_SMALLICON;
	UINT uFlagsLargeIcon = SHGFI_ICON | SHGFI_LARGEICON;

	if (path == L".\\") { // root
		int nPos = 0;
		CString strDrive = "?:\\";
		DWORD dwDriveList = ::GetLogicalDrives();

		while (dwDriveList) {
			if (dwDriveList & 1) {
				strDrive.SetAt(0, 0x41 + nPos);
				strDrive = strDrive.Left(2);

				CPlaylistItem pli;
				pli.m_bDirectory = true;
				pli.m_fns.push_front(strDrive);
				curPlayList.AddTail(pli);

				if (m_icons.find(strDrive) == m_icons.cend()) {
					SHGetFileInfoW(strDrive, 0, &shFileInfo, sizeof(SHFILEINFOW), uFlags);
					m_icons[strDrive] = shFileInfo.hIcon;

					SHGetFileInfoW(strDrive, 0, &shFileInfo, sizeof(SHFILEINFOW), uFlagsLargeIcon);
					m_icons_large[strDrive] = shFileInfo.hIcon;
				}
			}
			dwDriveList >>= 1;
			nPos++;
		}

		return;
	}

	CPlaylistItem pli;
	pli.m_bDirectory = true;
	pli.m_fns.push_front(RemoveSlash(path) + L"<"); // Parent folder mark;
	curPlayList.AddTail(pli);

	const CString folder(L"_folder_");
	if (m_icons.find(folder) == m_icons.cend()) {
		SHGetFileInfoW(folder, FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(SHFILEINFOW), uFlags | SHGFI_USEFILEATTRIBUTES);
		m_icons[folder] = shFileInfo.hIcon;

		SHGetFileInfoW(folder, FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(SHFILEINFOW), uFlagsLargeIcon | SHGFI_USEFILEATTRIBUTES);
		m_icons_large[folder] = shFileInfo.hIcon;
	}

	auto& directory = curTab.directory; directory.clear();
	auto& files = curTab.files; files.clear();

	const CString mask = AddSlash(path) + L"*.*";
	WIN32_FIND_DATAW FindFileData = {};
	HANDLE hFind = FindFirstFileW(mask, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			const CString fileName = FindFileData.cFileName;

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && fileName != L"." && fileName != L"..") {
					file_data_t file_data;
					file_data.name = AddSlash(path) + fileName;
					file_data.time_created.LowPart  = FindFileData.ftCreationTime.dwLowDateTime;
					file_data.time_created.HighPart = FindFileData.ftCreationTime.dwHighDateTime;
					file_data.time.LowPart  = FindFileData.ftLastWriteTime.dwLowDateTime;
					file_data.time.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;
					file_data.size.LowPart  = FindFileData.nFileSizeLow;
					file_data.size.HighPart = FindFileData.nFileSizeHigh;
					directory.push_back(file_data);
				}
			}
			else {
				const CString ext = GetFileExt(FindFileData.cFileName).MakeLower();
				auto mfc = mf.FindMediaByExt(ext);
				if (ext == L".iso" || (mfc && mfc->GetFileType() != TPlaylist)) {
					file_data_t file_data;
					file_data.name = AddSlash(path) + fileName;
					file_data.time_created.LowPart  = FindFileData.ftCreationTime.dwLowDateTime;
					file_data.time_created.HighPart = FindFileData.ftCreationTime.dwHighDateTime;
					file_data.time.LowPart  = FindFileData.ftLastWriteTime.dwLowDateTime;
					file_data.time.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;
					file_data.size.LowPart  = FindFileData.nFileSizeLow;
					file_data.size.HighPart = FindFileData.nFileSizeHigh;
					files.push_back(file_data);
				}
			}
		} while (::FindNextFileW(hFind, &FindFileData));
		::FindClose(hFind);
	}

	TFillPlaylist(true);
}

void CPlayerPlaylistBar::TFillPlaylist(const bool bFirst/* = false*/)
{
	auto& directory = curTab.directory;
	auto& files = curTab.files;

	if (directory.empty() && files.empty()) {
		return;
	}

	CString selected_path;
	if (!bFirst) {
		const auto focusedElement = TGetFocusedElement();
		POSITION pos = curPlayList.FindIndex(focusedElement);
		if (pos) {
			selected_path = curPlayList.GetAt(pos).m_fns.front().GetName();
		}

		CPlaylistItem pli = curPlayList.GetHead(); // parent
		curPlayList.RemoveAll();
		curPlayList.AddTail(pli);
	}

	const bool bReverse = !!(curTab.sort >> 8);
	const auto sort = (SORT)(curTab.sort & 0xF);

	if (sort == SORT::NAME) {
		std::sort(directory.begin(), directory.end(), [&](const file_data_t& a, const file_data_t& b) {
			return (!bReverse ? StrCmpLogicalW(a.name, b.name) < 0 : StrCmpLogicalW(a.name, b.name) > 0);
		});

		std::sort(files.begin(), files.end(), [&](const file_data_t& a, const file_data_t& b) {
			return (!bReverse ? StrCmpLogicalW(a.name, b.name) < 0 : StrCmpLogicalW(a.name, b.name) > 0);
		});
	}
	else if (sort == SORT::DATE) {
		std::sort(directory.begin(), directory.end(), [&](const file_data_t& a, const file_data_t& b) {
			return (!bReverse ? a.time.QuadPart < b.time.QuadPart : a.time.QuadPart > b.time.QuadPart);
		});

		std::sort(files.begin(), files.end(), [&](const file_data_t& a, const file_data_t& b) {
			return (!bReverse ? a.time.QuadPart < b.time.QuadPart : a.time.QuadPart > b.time.QuadPart);
		});
	}
	else if (sort == SORT::DATE_CREATED) {
		std::sort(directory.begin(), directory.end(), [&](const file_data_t& a, const file_data_t& b) {
			return (!bReverse ? a.time_created.QuadPart < b.time_created.QuadPart : a.time_created.QuadPart > b.time_created.QuadPart);
		});

		std::sort(files.begin(), files.end(), [&](const file_data_t& a, const file_data_t& b) {
			return (!bReverse ? a.time_created.QuadPart < b.time_created.QuadPart : a.time_created.QuadPart > b.time_created.QuadPart);
		});
	}
	else if (sort == SORT::SIZE) {
		std::sort(directory.begin(), directory.end(), [](const file_data_t& a, const file_data_t& b) {
			return (StrCmpLogicalW(a.name, b.name) < 0);
		});

		std::sort(files.begin(), files.end(), [&](const file_data_t& a, const file_data_t& b) {
			return (!bReverse ? a.size.QuadPart < b.size.QuadPart : a.size.QuadPart > b.size.QuadPart);
		});
	}

	for (const auto& dir : directory) {
		CPlaylistItem pli;
		pli.m_bDirectory = true;
		pli.m_fns.push_front(dir.name + L">"); // Folders mark
		curPlayList.AddTail(pli);
	}

	for (const auto& file : files) {
		CPlaylistItem pli;
		pli.m_fns.push_front(file.name);
		curPlayList.AddTail(pli);

		if (bFirst) {
			const auto ext = GetFileExt(file.name).MakeLower();
			if (m_icons.find(ext) == m_icons.cend()) {
				SHFILEINFOW shFileInfo = {};

				SHGetFileInfoW(file.name, 0, &shFileInfo, sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
				m_icons[ext] = shFileInfo.hIcon;

				SHGetFileInfoW(file.name, 0, &shFileInfo, sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_LARGEICON);
				m_icons_large[ext] = shFileInfo.hIcon;
			}
		}
	}

	if (!bFirst) {
		Refresh();

		if (!SelectFileInPlaylist(selected_path)) {
			EnsureVisible(curPlayList.GetHeadPosition());
		}
	}
}

int CPlayerPlaylistBar::TGetPathType(const CString& path) const
{
	if (path.IsEmpty()) {
		return -1;
	}

	const auto suffix = path.Right(1);
	if (suffix == L":") {
		return DRIVE;
	}
	else if (suffix == L"<") {
		return PARENT;
	}
	else if (suffix == L">") {
		return FOLDER;
	}

	return FILE;
}

void CPlayerPlaylistBar::TTokenizer(const CString& strFields, LPCWSTR strDelimiters, std::vector<CString>& arFields)
{
	arFields.clear();

	int curPos = 0;
	CString token = strFields.Tokenize(strDelimiters, curPos);
	while (!token.IsEmpty()) {
		arFields.push_back(token);
		token = strFields.Tokenize(strDelimiters, curPos);
	}
}

void CPlayerPlaylistBar::TSaveSettings()
{
	CString str;

	if (!m_tabs.empty()) {
		// the first stroke has only saved last active playlist index and tabs offset (first visible tab on tabbar)
		str.AppendFormat(L"%d;%d;|", m_nCurPlayListIndex, m_cntOffset);
		const auto last = m_tabs.size() - 1;
		for (size_t i = 0; i <= last; i++) {
			const auto& tab = m_tabs[i];
			CString s;
			str.AppendFormat(L"%d;%s;%u%s%s", tab.type, RemoveFileExt(tab.fn.GetString()).GetString(), tab.sort, i > 0 ? CString(L";" + tab.name).GetString() : L"", i < last ? L"|" : L"");
		}
	}

	AfxGetAppSettings().strTabs = str;
}

void CPlayerPlaylistBar::TGetSettings()
{
	std::vector<CString> arStrokes, arFields;
	// parse strokes
	TTokenizer(AfxGetAppSettings().strTabs, L"|", arStrokes);

	for (size_t i = 0; i < arStrokes.size(); i++) {
		// parse fields in stroke
		TTokenizer(arStrokes[i], L";", arFields);
		if (arFields.size() < 2) {
			continue;
		}

		if (i == 0) { // the first stroke has only saved last active playlist index and tabs offset (first visible tab on tabbar)
			m_nCurPlayListIndex = _wtoi(arFields[0]);
			m_cntOffset = _wtoi(arFields[1]);
			continue;
		}

		// add tab
		tab_t tab;
		tab.type = (PLAYLIST_TYPE)_wtoi(arFields[0]);
		tab.name = m_pls.empty() ? ResStr(IDS_PLAYLIST_MAIN_NAME) : arFields[1];
		tab.fn = arFields[1] + (L".mpcpl");
		tab.id = GetNextId();
		if (arFields.size() >= 3) {
			tab.sort = _wtoi(arFields[2]);
		}
		if (!m_pls.empty() && arFields.size() >= 4) {
			tab.name = arFields[3];
		}
		m_tabs.push_back(tab);

		// add playlist
		CPlaylist* pl = DNew CPlaylist;
		m_pls.push_back(pl);
	}

	// if we have no tabs settings, create "Main"
	if (m_tabs.size() == 0) {
		tab_t tab;
		tab.type = PLAYLIST;
		tab.name = ResStr(IDS_PLAYLIST_MAIN_NAME);
		tab.fn = L"Default.mpcpl";
		tab.id = GetNextId();
		m_tabs.push_back(tab);

		// add playlist
		CPlaylist* pl = DNew CPlaylist;
		m_pls.push_back(pl);
	}

	if (m_cntOffset >= m_tabs.size()) {
		m_cntOffset = 0;
	}

	if (m_nCurPlayListIndex >= m_tabs.size()) {
		m_nCurPlayListIndex = 0;
	}
}

void CPlayerPlaylistBar::TSetOffset(bool toRight)
{
	if (toRight) {
		for (size_t i = 1; i < m_tabs.size(); i++) {
			if (m_tabs[i].r.left == m_rcTPage.left) {
				if (m_cntOffset > 0) {
					m_cntOffset--;
				}
				return;
			}
		}
		return;
	}

	for (size_t i = 0; i < m_tabs.size() - 1; i++) {
		if (m_tabs[i].r.left == m_rcTPage.left) {
			m_cntOffset++;
			return;
		}
	}
}

void CPlayerPlaylistBar::TEnsureVisible(int idx)
{
	if (m_tabs[idx].r.left >= m_rcTPage.left && m_tabs[idx].r.right <= m_rcTPage.right) {
		return;
	}

	if (m_tabs[idx].r.left < m_rcTPage.left) {
		for (size_t i = idx; i < m_tabs.size(); i++) {
			if (m_cntOffset > 0) {
				m_cntOffset--;
			}
			if (m_tabs[i].r.left + m_tabs[i].r.Width() == m_rcTPage.left) {
				return;
			}
		}
	} else if (m_tabs[idx].r.left > m_rcTPage.left && m_tabs[idx].r.right > m_rcTPage.right) {
		int ftab = TGetFirstVisible();
		int tempOffset = 0;
		do {
			tempOffset += m_tabs[ftab].r.Width();
			ftab++;
			m_cntOffset++;
		} while (m_tabs[idx].r.right - tempOffset > m_rcTPage.right && m_tabs[idx].r.Width() <= m_rcTPage.Width());
	}
}

int CPlayerPlaylistBar::TGetFirstVisible()
{
	int idx = -1;
	for (size_t i = 0; i < m_tabs.size(); i++) {
		idx++;
		if (m_tabs[i].r.left == m_rcTPage.left) {
			return idx;
		}

	}
	return idx;
}

static COLORREF TColorBrightness(const int lSkale, const COLORREF color)
{
	const int R = std::clamp(GetRValue(color) + lSkale, 0, 255);
	const int G = std::clamp(GetGValue(color) + lSkale, 0, 255);
	const int B = std::clamp(GetBValue(color) + lSkale, 0, 255);

	return RGB(R, G, B);
}

void CPlayerPlaylistBar::TSetColor()
{
	if (AfxGetAppSettings().bUseDarkTheme) {
		m_crBkBar = ThemeRGB(45, 50, 55);          // background tab bar

		m_crBN = m_crBkBar;                        // backgroung normal
		m_crBNL = TColorBrightness(+15, m_crBN);   // backgroung normal lighten (for light edge)
		m_crBND = TColorBrightness(-15, m_crBN);   // backgroung normal darken (for dark edge)

		m_crBS = TColorBrightness(-15, m_crBN);    // backgroung selected background
		m_crBSL = TColorBrightness(+30, m_crBS);   // backgroung selected lighten (for light edge)
		m_crBSD = TColorBrightness(-15, m_crBS);   // backgroung selected darken (for dark edge)

		m_crBH = TColorBrightness(+20, m_crBN);    // backgroung highlighted background
		m_crBHL = TColorBrightness(+15, m_crBH);   // backgroung highlighted lighten (for light edge)
		m_crBHD = TColorBrightness(-30, m_crBH);   // backgroung highlighted darken (for dark edge)

		m_crBSH = TColorBrightness(-20, m_crBN);   // backgroung selected highlighted background
		m_crBSHL = TColorBrightness(+40, m_crBSL); // backgroung selected highlighted lighten (for light edge)
		m_crBSHD = TColorBrightness(-0, m_crBSH);  // backgroung selected highlighted darken (for dark edge)

		m_crTN = TColorBrightness(+60, m_crBN);    // text normal
		m_crTH = TColorBrightness(+80, m_crTN);    // text normal lighten
		m_crTS = AfxGetAppSettings().clrFaceABGR;  // text selected
	}
	else {
		m_crBkBar = GetSysColor(COLOR_BTNFACE);    // background tab bar

		m_crBN = m_crBkBar;                        // backgroung normal
		m_crBNL = TColorBrightness(+15, m_crBN);   // backgroung normal lighten (for light edge)
		m_crBND = TColorBrightness(-15, m_crBN);   // backgroung normal darken (for dark edge)

		m_crBS = 0x00f1dacc;                       // backgroung selected background
		m_crBSL = 0xc56a31;                        // backgroung selected lighten (for light edge)
		m_crBSD = TColorBrightness(-15, m_crBS);   // backgroung selected darken (for dark edge)

		m_crBH = 0x00f1dacc;                       // backgroung highlighted background
		m_crBHL = TColorBrightness(+15, m_crBH);   // backgroung highlighted lighten (for light edge)
		m_crBHD = TColorBrightness(-30, m_crBH);   // backgroung highlighted darken (for dark edge)

		m_crBSH = TColorBrightness(+20, m_crBS);   // backgroung selected highlighted background
		m_crBSHL =TColorBrightness(-20, m_crBSL);  // backgroung selected highlighted lighten (for light edge)
		m_crBSHD = TColorBrightness(-0, m_crBSH);  // backgroung selected highlighted darken (for dark edge)

		m_crTN = RGB(50, 50, 50);                  // text normal
		m_crTH = RGB(0, 0, 0);                     // text normal lighten
		m_crTS = 0xff;                             // text selected
	}

	if (IsWindow(m_REdit.GetSafeHwnd())) {
		if (AfxGetAppSettings().bUseDarkTheme) {
			m_REdit.SetBkColor(m_crBND);
			m_REdit.SetTextColor(m_crTH);
		} else {
			m_REdit.SetBkColor(::GetSysColor(COLOR_WINDOW));
			m_REdit.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		}
	}
}

int CPlayerPlaylistBar::TGetOffset()
{
	CClientDC dc(this);
	CDC mdc;
	mdc.CreateCompatibleDC(&dc);
	mdc.SelectObject(&m_font);

	int widthOffset = 0;
	for (size_t i = 0; i < m_cntOffset; i++) {
		widthOffset += mdc.GetTextExtent(m_tabs[i].name).cx + WIDTH_TABBUTTON;
	}

	mdc.DeleteDC();

	return widthOffset;
}

bool CPlayerPlaylistBar::TNavigate()
{
	if (curTab.type == EXPLORER) {
		m_list.SetFocus();

		int item = m_list.GetNextItem(-1, LVNI_SELECTED);
		if (item < 0) {
			item++;
		}

		POSITION pos = FindPos(item);
		if (pos) {
			const auto& pli = curPlayList.GetAt(pos);
			if (pli.m_bDirectory && !pli.m_fns.empty()) {
				CString path = pli.m_fns.front().GetName();
				if (!path.IsEmpty()) {
					auto oldPath = path;

					const auto type = TGetPathType(path);
					switch (type) {
						case DRIVE:
							break;
						case PARENT:
							path.TrimRight(L"\\<");
							path = GetFolderOnly(path);
							if (path.Right(1) == L":") {
								path = (L".");
							}
							oldPath.TrimRight(L"\\<");
							break;
						case FOLDER:
							path.TrimRight(L">");
							break;
						default:
							return false;
					}
					path = AddSlash(path);

					curPlayList.RemoveAll();
					m_list.DeleteAllItems();

					TParseFolder(path);
					Refresh();

					type == PARENT ? TSelectFolder(oldPath) : EnsureVisible(curPlayList.GetHeadPosition());

					return true;
				}
			}
		}
	}

	return false;
}

bool CPlayerPlaylistBar::TSelectFolder(CString path)
{
	if (path.IsEmpty()) {
		return false;
	}

	POSITION pos = curPlayList.GetHeadPosition();
	while (pos) {
		const auto& pli = curPlayList.GetAt(pos);
		if (pli.m_bDirectory && pli.FindFolder(path)) {
			curPlayList.SetPos(pos);
			EnsureVisible(pos, false);
			return true;
		}
		curPlayList.GetNext(pos);
	}
	return false;
}

int CPlayerPlaylistBar::TGetFocusedElement() const
{
	for (int i = 0; i < m_list.GetItemCount(); i++) {
		if (m_list.GetItemState(i, LVIS_FOCUSED)) {
			return i;
		}
	}

	return 0;
}

void CPlayerPlaylistBar::CloseMedia() const
{
	if (m_nCurPlaybackListId == curTab.id) {
		m_pMainFrame->SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	}
}

void CPlayerPlaylistBar::CopyToClipboard()
{
	if (OpenClipboard() && EmptyClipboard()) {
		CString str;

		std::vector<int> items;
		items.reserve(m_list.GetSelectedCount());
		POSITION pos = m_list.GetFirstSelectedItemPosition();
		while (pos) {
			items.push_back(m_list.GetNextSelectedItem(pos));
		}

		for (const auto& item : items) {
			CPlaylistItem &pli = curPlayList.GetAt(FindPos(item));
			for (const auto& fi : pli.m_fns) {
				str += L"\r\n" + fi.GetName();
			}
		}

		str.Trim();

		if (HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (str.GetLength() + 1) * sizeof(WCHAR))) {
			if (WCHAR * s = (WCHAR*)GlobalLock(h)) {
				wcscpy_s(s, str.GetLength() + 1, str);
				GlobalUnlock(h);
				SetClipboardData(CF_UNICODETEXT, h);
			}
		}
		CloseClipboard();
	}
}

void CPlayerPlaylistBar::PasteFromClipboard()
{
	std::list<CString> sl;
	if (m_pMainFrame->GetFromClipboard(sl)) {
		Append(sl, sl.size() > 1, nullptr);
	}
}