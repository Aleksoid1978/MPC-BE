/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include <IntShCut.h>
#include <random>
#include "MainFrm.h"
#include "DSUtil/SysVersion.h"
#include "DSUtil/Filehandle.h"
#include "DSUtil/std_helper.h"
#include "DSUtil/UrlParser.h"
#include "SaveTextFileDialog.h"
#include "PlayerPlaylistBar.h"
#include "FileDialogs.h"
#include "Content.h"
#include "PlaylistNameDlg.h"
#include <ExtLib/ui/coolsb/coolscroll.h>
#include "./Controls/MenuEx.h"
#include "TorrentInfo.h"

#define ID_PLSMENU_ADD_PLAYLIST    2001
#define ID_PLSMENU_ADD_EXPLORER    2002
#define ID_PLSMENU_RENAME_PLAYLIST 2003
#define ID_PLSMENU_DELETE_PLAYLIST 2004

#define ID_PLSMENU_POSITION_LEFT   2005
#define ID_PLSMENU_POSITION_TOP    2006
#define ID_PLSMENU_POSITION_RIGHT  2007
#define ID_PLSMENU_POSITION_BOTTOM 2008
#define ID_PLSMENU_POSITION_FLOAT  2009

WCHAR LastChar(const CStringW& str)
{
	int len = str.GetLength();
	return len > 0 ? str.GetAt(len - 1) : 0;
}

static CStringW MakePath(CStringW path)
{
	if (::PathIsURLW(path) || Youtube::CheckURL(path)) { // skip URLs
		if (path.Left(8).MakeLower() == L"file:///") {
			path.Delete(0, 8);
			path.Replace('/', '\\');
		}

		return path;
	}

	path.Replace('/', '\\');

	CStringW c = GetFullCannonFilePath(path);

	return c;
}

struct CUETrack {
	REFERENCE_TIME m_rt = _I64_MIN;
	UINT m_trackNum = 0;
	CString m_fn;
	CString m_Title;
	CString m_Performer;

	CUETrack(const CString& fn, const REFERENCE_TIME rt, const UINT trackNum, const CString& Title, const CString& Performer) {
		m_rt        = rt;
		m_trackNum  = trackNum;
		m_fn        = fn;
		m_Title     = Title;
		m_Performer = Performer;
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
			sTrackArray.emplace_back(cueLine);
		} else if (cmd == L"FILE") {
			int a = cueLine.Find('\"');
			int b = cueLine.Find('\"', a + 1);
			int len = b - 1 - a;
			if (len > 0) {
				CStringW fn = cueLine.Mid(a + 1, len);
				fn.Trim();
				sFilesArray.emplace_back(fn);
			}
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
				const CString& fName = bMultiple ? sFilesArray[idx++] : sFilesArray.size() == 1 ? sFilesArray[0] : sFileName;
				CUETrackList.emplace_back(fName, rt, trackNum, sTitle, sPerformer);
			}
			rt = _I64_MIN;
			sFileName = sFileName2;
			if (!Performer.IsEmpty()) {
				sPerformer = Performer;
			}

			WCHAR type[256] = {};
			trackNum = 0;
			fAudioTrack = FALSE;
			if (2 == swscanf_s(cueLine, L"%u %s", &trackNum, type, (unsigned)std::size(type))) {
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
		} else if (cmd == L"FILE") {
			int a = cueLine.Find('\"');
			int b = cueLine.Find('\"', a + 1);
			int len = b - 1 - a;
			if (len > 0) {
				CStringW fn = cueLine.Mid(a + 1, len);
				fn.Trim();
				sFileName2 = fn;
				if (sFileName.IsEmpty()) {
					sFileName = fn;
				}
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
		const CString& fName = bMultiple ? sFilesArray[idx] : sFilesArray.size() == 1 ? sFilesArray[0] : sFileName2;
		CUETrackList.emplace_back(fName, rt, trackNum, sTitle, sPerformer);
	}

	return CUETrackList.size() > 0;
}

//
// CPlaylistItem
//

CPlaylistItem::CPlaylistItem()
{
	m_id = m_globalid++;
}

CPlaylistItem& CPlaylistItem::operator = (const CPlaylistItem& pli)
{
	if (this != &pli) {
		m_id         = pli.m_id;
		m_label      = pli.m_label;
		m_fi         = pli.m_fi;
		m_auds       = pli.m_auds;
		m_subs       = pli.m_subs;
		m_type       = pli.m_type;
		m_bInvalid   = pli.m_bInvalid;
		m_bDirectory = pli.m_bDirectory;
		m_duration   = pli.m_duration;
		m_vinput     = pli.m_vinput;
		m_vchannel   = pli.m_vchannel;
		m_ainput     = pli.m_ainput;
		m_country    = pli.m_country;
	}
	return(*this);
}

bool CPlaylistItem::FindFile(LPCWSTR path)
{
	if (m_fi.GetPath().CompareNoCase(path) == 0) {
		return true;
	}

	for (const auto& fi : m_auds) {
		if (fi.GetPath().CompareNoCase(path) == 0) {
			return true;
		}
	}

	return false;
}

bool CPlaylistItem::FindFolder(LPCWSTR path) const
{
	CString str = m_fi.GetPath();
	str.TrimRight(L'>');
	str.TrimRight(L'<');
	if (str.CompareNoCase(path) == 0) {
		return true;
	}

	for (const auto& fi : m_auds) {
		str = fi.GetPath();
		str.TrimRight(L'>');
		str.TrimRight(L'<');
		if (str.CompareNoCase(path) == 0) {
			return true;
		}
	}

	return false;
}

CString CPlaylistItem::GetLabel(int i) const
{
	CString str;

	if (i == 0) {
		if (!m_label.IsEmpty()) {
			str = m_label;
		} else if (m_fi.Valid()) {
			const auto& fn = m_fi;
			CUrlParser urlParser;
			if (::PathIsURLW(fn) && urlParser.Parse(fn)) {
				str = fn.GetPath();
				if (urlParser.GetUrlPathLength() > 1) {
					str = urlParser.GetUrlPath(); str.TrimRight(L'/');
					if (const int pos = str.ReverseFind(L'/'); pos != -1) {
						str = str.Right(str.GetLength() - pos - 1);
					}
				}
				//m_label = str.TrimRight(L'/');
			}
			else {
				str = GetFileName(fn);
			}
		}
	} else if (i == 1) {
		if (m_bInvalid) {
			return ResStr(IDS_PLAYLIST_INVALID);
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
static bool FindFileInList(std::list<T>& sl, const CString& fn)
{
	for (const auto& item : sl) {
		if (CString(item).CompareNoCase(fn) == 0) {
			return true;
		}
	}

	return false;
}

static void StringToPaths(const CStringW& curentdir, const CStringW& str, std::vector<CStringW>& paths)
{
	int pos = 0;
	do {
		CStringW s = str.Tokenize(L";", pos);
		if (s.GetLength() == 0) {
			continue;
		}
		int bs = s.ReverseFind('\\');
		int a = s.Find('*');
		if (a >= 0 && a < bs) {
			continue; // the asterisk can only be in the last folder
		}

		CStringW path = GetCombineFilePath(curentdir, s);

		if (::PathIsRootW(path) && ::PathFileExistsW(path)) {
			paths.emplace_back(path);
			continue;
		}

		WIN32_FIND_DATAW fd = { 0 };
		HANDLE hFind = FindFirstFileW(path, &fd);
		if (hFind == INVALID_HANDLE_VALUE) {
			continue;
		} else {
			CStringW parentdir = GetFullCannonFilePath(path + L"\\..");
			AddSlash(parentdir);

			do {
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) {
					CString folder = parentdir + fd.cFileName + '\\';

					size_t index = 0;
					size_t count = paths.size();
					for (; index < count; index++) {
						if (folder.CompareNoCase(paths[index]) == 0) {
							break;
						}
					}
					if (index == count) {
						paths.emplace_back(folder);
					}
				}
			} while (FindNextFileW(hFind, &fd));
			FindClose(hFind);
		}
	} while (pos > 0);
}

void CPlaylistItem::AutoLoadFiles()
{
	if (!m_fi.Valid()) {
		return;
	}

	auto& fpath = m_fi.GetPath();
	if (fpath.Find(L"://") >= 0) { // skip URLs
		return;
	}

	auto curdir = GetFolderPath(fpath);
	auto name   = GetRemoveFileExt(GetFileName(fpath));
	auto ext    = GetFileExt(fpath);

	CStringW BDLabel, empty;
	AfxGetMainFrame()->MakeBDLabel(fpath, empty, &BDLabel);
	if (BDLabel.GetLength()) {
		FixFilename(BDLabel);
	}

	CAppSettings& s = AfxGetAppSettings();

	if (s.fAutoloadAudio) {
		std::vector<CStringW> paths;
		StringToPaths(curdir, s.strAudioPaths, paths);

		for (const auto& apa : s.slAudioPathsAddons) {
			paths.emplace_back(apa);
		}

		CMediaFormats& mf = s.m_Formats;
		if (!mf.FindAudioExt(ext)) {
			for (const auto& path : paths) {
				std::vector<CStringW> searchPattern;
				searchPattern.emplace_back(path + name + L".*"); // more general filename mask, clarification will follow
				if (BDLabel.GetLength()) {
					searchPattern.emplace_back(path + BDLabel + L".*");
				}

				WIN32_FIND_DATAW fd = {};
				HANDLE hFind;

				for (const auto& pattern : searchPattern) {
					hFind = FindFirstFileW(pattern, &fd);

					if (hFind != INVALID_HANDLE_VALUE) {
						do {
							if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
								continue;
							}
							CStringW fn(fd.cFileName);
							if (fn[name.GetLength()] != L'.') {
								continue;
							}
							const CStringW ext2 = GetFileExt(fn);
							if (ext.CompareNoCase(ext2) == 0 || !mf.FindAudioExt(ext2)) {
								continue;
							}
							fn = path + fn;
							if (!FindFileInList(m_auds, fn)) {
								m_auds.emplace_back(fn);
							}
						} while (FindNextFileW(hFind, &fd));

						FindClose(hFind);
					}
				}
			}
		}
	}

	if (s.IsISRAutoLoadEnabled()) {
		std::vector<CStringW> paths;
		StringToPaths(curdir, s.strSubtitlePaths, paths);

		for (const auto& spa : s.slSubtitlePathsAddons) {
			paths.emplace_back(spa);
		}

		std::vector<CStringW> ret;
		Subtitle::GetSubFileNames(fpath, paths, ret);

		for (const auto& sub_fn : ret) {
			if (!FindFileInList(m_subs, sub_fn)) {
				m_subs.emplace_back(sub_fn);
			}
		}

		if (BDLabel.GetLength()) {
			ret.clear();
			Subtitle::GetSubFileNames(BDLabel, paths, ret);

			for (const auto& sub_fn : ret) {
				if (!FindFileInList(m_subs, sub_fn)) {
					m_subs.emplace_back(sub_fn);
				}
			}
		}
	}

	// cue-sheet file auto-load
	const CString cuefn = GetRenameFileExt(fpath, L".cue");
	if (::PathFileExistsW(cuefn)) {
		CStringW filter;
		std::vector<CStringW> mask;
		s.m_Formats.GetAudioFilter(filter, mask);
		std::list<CStringW> sl;
		Explode(mask[0], sl, L';');

		bool bExists = false;
		for (auto _mask : sl) {
			_mask.Delete(0, 2);
			_mask.MakeLower();
			if (_mask == ext) {
				bExists = true;
				break;
			}
		}

		if (bExists) {
			CFileItem* fi = &m_fi;
			if (!fi->GetChapterCount()) {

				CStringW Title, Performer;
				std::list<CUETrack> CUETrackList;
				if (ParseCUESheetFile(cuefn, CUETrackList, Title, Performer)) {
					std::list<CStringW> fileNames;

					for (const auto& cueTrack : CUETrackList) {
						if (std::find(fileNames.cbegin(), fileNames.cend(), cueTrack.m_fn) == fileNames.cend()) {
							fileNames.emplace_back(cueTrack.m_fn);
						}
					}

					if (fileNames.size() == 1) {
						// support opening cue-sheet file with only a single file inside, even if its name differs from the current
						MakeCUETitle(m_label, Title, Performer);

						for (const auto& cueTrack : CUETrackList) {
							CStringW cueTrackTitle;
							MakeCUETitle(cueTrackTitle, cueTrack.m_Title, cueTrack.m_Performer, cueTrack.m_trackNum);
							fi->AddChapter(cueTrackTitle, cueTrack.m_rt);
						}
						fi->SetTitle(m_label);

						ChaptersList chaplist;
						fi->GetChapters(chaplist);
						bool bTrustedChap = false;
						for (size_t i = 0; i < chaplist.size(); i++) {
							if (chaplist[i].rt > 0) {
								bTrustedChap = true;
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

//
// CPlaylist
//

POSITION CPlaylist::Append(CPlaylistItem& item, const bool bParseDuration)
{
	if (bParseDuration && !item.m_duration && item.m_fi.Valid()) {
		const auto& fn = item.m_fi.GetPath();
		if (!::PathIsURLW(item.m_fi) && ::PathFileExistsW(item.m_fi)) {
			MediaInfo MI;
			MI.Option(L"ParseSpeed", L"0");
			if (MI.Open(fn.GetString())) {
				CString duration = MI.Get(Stream_General, 0, L"Duration", Info_Text, Info_Name).c_str();
				if (!duration.IsEmpty() && StrToInt64(duration.GetString(), item.m_duration)) {
					item.m_duration *= 10000LL;
				}
			}
		}
	}

	return AddTail(item);
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
		const plsort_str_t item = { GetAt(pos).m_fi, pos };
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
		const plsort_str_t item = { GetAt(pos).m_fi, pos };
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

void CPlaylist::CalcCountFiles()
{
	m_nFilesCount = 0;
	POSITION pos = GetHeadPosition();
	while (pos) {
		const auto& playlist = GetNext(pos);
		if (!playlist.m_bDirectory) {
			m_nFilesCount++;
		}
	}
}

INT_PTR CPlaylist::GetCountInternal()
{
	return m_nFilesCount != -1 ? m_nFilesCount : GetCount();
}

POSITION CPlaylist::Shuffle()
{
	const auto CountInternal = GetCountInternal();
	ASSERT(CountInternal > 2);

	static INT_PTR idx = 0;
	static INT_PTR count = 0;
	static std::vector<POSITION> a;
	const auto Count = m_pos ? CountInternal - 1 : CountInternal;

	if (count != Count || idx >= Count) {
		// insert or remove items in playlist, or index out of bounds then recalculate
		a.clear();
		idx = 0;
		a.reserve(count = Count);

		for (POSITION pos = GetHeadPosition(); pos; GetNext(pos)) {
			if (pos != m_pos && !GetAt(pos).m_bDirectory) {
				a.emplace_back(pos);
			}
		}

		std::shuffle(a.begin(), a.end(), std::default_random_engine((unsigned)GetTickCount64()));
	}

	return a[idx++];
}

CPlaylistItem& CPlaylist::GetNextWrap(POSITION& pos)
{
	if (AfxGetAppSettings().bShufflePlaylistItems && GetCountInternal() > 2) {
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

//
// CPlayerPlaylistBar
//

IMPLEMENT_DYNAMIC(CPlayerPlaylistBar, CPlayerBar)

CPlayerPlaylistBar::CPlayerPlaylistBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_list(0)
{
	CAppSettings& s = AfxGetAppSettings();
	m_bUseDarkTheme = s.bUseDarkTheme;

	SetColor();
	TGetSettings();
}

CPlayerPlaylistBar::~CPlayerPlaylistBar()
{
	TEnsureVisible(m_nCurPlayListIndex); // save selected tab visible
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
		0, //less margins//WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE,
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
	NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
	VERIFY(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0));

	auto& lf = ncm.lfMessageFont;
	lf.lfHeight = MulDiv(m_pMainFrame->ScaleSystemToMonitorY(lf.lfHeight), AfxGetAppSettings().iPlsFontPercent, 100);
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

	m_nSearchBarHeight = m_pMainFrame->ScaleY(MulDiv(20, AfxGetAppSettings().iPlsFontPercent, 100));
}

void CPlayerPlaylistBar::ScaleFont()
{
	ScaleFontInternal();
	ResizeListColumn();
	TCalcLayout();
	TCalcREdit();
}

bool CPlayerPlaylistBar::IsShuffle() const
{
	return AfxGetAppSettings().bShufflePlaylistItems && curPlayList.GetCountInternal() > 2;
}

void CPlayerPlaylistBar::SelectDefaultPlaylist()
{
	curPlayList.m_nFocused_idx = TGetFocusedElement();
	m_nCurPlayListIndex = 0;

	m_pMainFrame->m_bRememberSelectedTracks = false;
	curPlayList.m_nSelectedAudioTrack = curPlayList.m_nSelectedSubtitleTrack = -1;

	TEnsureVisible(m_nCurPlayListIndex);
	TSelectTab();
}

bool CPlayerPlaylistBar::CheckAudioInCurrent(const CString& fn)
{
	auto pli = GetCur();
	if (pli) {
		return FindFileInList(pli->m_auds, fn);
	}

	return false;
}

void CPlayerPlaylistBar::AddAudioToCurrent(const CString& fn)
{
	auto pli = GetCur();
	if (pli) {
		if (!FindFileInList(pli->m_auds, fn)) {
			pli->m_auds.emplace_back(fn);
		}
	}
}

void CPlayerPlaylistBar::AddSubtitleToCurrent(const CString& fn)
{
	auto pli = GetCur();
	if (pli) {
		if (!FindFileInList(pli->m_subs, fn)) {
			pli->m_subs.emplace_back(fn);
		}
	}
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

			auto& playlist = GetCurPlayList();
			auto& curTab = GetCurTab();

			if (playlist.GetCount() > 1
					&& (pMsg->message == WM_CHAR
						|| (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_F3 || pMsg->wParam == VK_DELETE || pMsg->wParam == VK_BACK)))) {
				CString text; m_REdit.GetWindowTextW(text);
				if (!text.IsEmpty()) {
					::CharLowerBuffW(text.GetBuffer(), text.GetLength());

					POSITION pos = playlist.GetHeadPosition();
					if (curTab.type == PL_EXPLORER) {
						playlist.GetNext(pos);
					}
					POSITION cur_pos = nullptr;
					if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_F3) {
						auto item = TGetFocusedElement();
						if (item == 0 && curTab.type == PL_EXPLORER) {
							item++;
						}
						cur_pos = FindPos(item);
						pos = FindPos(item + 1);
						if (!pos) {
							pos = playlist.GetHeadPosition();
							if (curTab.type == PL_EXPLORER) {
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
							if (curTab.type == PL_EXPLORER) {
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

		if (!m_bEdit) {
			if (pMsg->message == WM_SYSKEYDOWN
					&& (HIWORD(pMsg->lParam) & KF_ALTDOWN)) {
				m_pMainFrame->PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
				return TRUE;
			}

			if (pMsg->message == WM_KEYDOWN) {
				auto& curTab = GetCurTab();

				switch (pMsg->wParam) {
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
					if (curTab.type == PL_EXPLORER) {
						break;
					}
					if (GetKeyState(VK_CONTROL) < 0) {
						m_list.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
					}
					break;
				case 'I':
					if (curTab.type == PL_EXPLORER) {
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
					if (curTab.type == PL_EXPLORER) {
						break;
					}
					if (GetKeyState(VK_CONTROL) < 0) {
						PasteFromClipboard();
					}
					break;
				case VK_BACK:
					if (curTab.type == PL_EXPLORER) {
						auto path = curPlayList.GetHead().m_fi.GetPath();
						if (LastChar(path) == L'<') {
							auto oldPath = path;
							oldPath.TrimRight(L"\\<");

							path.TrimRight(L"\\<");
							RemoveFileSpec(path);
							if (LastChar(path) == L':') {
								path = L".";
							}
							AddSlash(path);

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
					if (GetKeyState(VK_CONTROL) < 0) {
						m_pMainFrame->PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
						return TRUE;
					}
				case VK_DELETE:
				case VK_APPS: // "Menu key"
					break;
				case VK_F2:
					if (m_list.GetSelectedCount() == 1) {
						const auto nItem = m_list.GetNextItem(-1, LVNI_SELECTED);
						m_list.EditLabel(nItem);
					}
					break;
				default:
					m_pMainFrame->PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
					return TRUE;
				}
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

	const CString section = L"ToolBars\\" + m_strSettingName;
	AfxGetProfile().WriteBool(section, L"Visible", IsWindowVisible() || m_bVisible || (AfxGetAppSettings().bHidePlaylistFullScreen && m_bHiddenDueToFullscreen));
}

bool CPlayerPlaylistBar::IsHiddenDueToFullscreen() const
{
	return m_bHiddenDueToFullscreen;
}

void CPlayerPlaylistBar::SetHiddenDueToFullscreen(bool bHiddenDueToFullscreen)
{
	m_bHiddenDueToFullscreen = bHiddenDueToFullscreen;
}

void CPlayerPlaylistBar::AddItem(std::list<CString>& fns, CSubtitleItemList* subs)
{
	CPlaylistItem pli;

	auto it = fns.cbegin();
	pli.m_fi = *it++;
	while (it != fns.cend()) {
		pli.m_auds.emplace_back(*it++);
	}

	if (subs) {
		for (CString sub : *subs) {
			if (!sub.Trim().IsEmpty()) {
				pli.m_subs.emplace_back(sub);
			}
		}
	}

	pli.AutoLoadFiles();
	if (pli.m_auds.empty()) {
		Youtube::YoutubeFields y_fields;
		if (Youtube::Parse_URL(pli.m_fi, y_fields)) {
			pli.m_label    = y_fields.title;
			pli.m_duration = y_fields.duration;
		}
	}

	curPlayList.Append(pli, AfxGetAppSettings().bPlaylistDetermineDuration);
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
	AddSlash(path);
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
							sl.emplace_back(path + fd.cFileName);
							break;
						}
					}
				}

			} while (FindNextFileW(h, &fd));

			FindClose(h);

			if (sl.size() == 0 && StartsWith(mask, L":\\", 1)) {
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
	sl.emplace_back(fn);
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
				if (CComQIPtr<IPersistFile> pPersistFile = pShellLink.p) {
					WCHAR buffer[MAX_PATH] = {};
					if (SUCCEEDED(pPersistFile->Load(fn, STGM_READ))
							// Possible recontruction of path.
							&& SUCCEEDED(pShellLink->Resolve(nullptr, SLR_ANY_MATCH | SLR_NO_UI))
							// Retrieve path.
							&& SUCCEEDED(pShellLink->GetPath(buffer, std::size(buffer), nullptr, 0))
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
				if (CComQIPtr<IPersistFile> pPersistFile = pUniformResourceLocator.p) {
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
				if (m_pMainFrame->IsBDStartFile(fn) || m_pMainFrame->IsDVDStartFile(fn)) {
					fns.clear();
					fns.emplace_front(fn);
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
			Content::Online::Disconnect(fn);
			for (const auto& r : redir) {
				ParsePlayList(r, subs, !::PathIsURLW(r));
			}
			return;
		}

		if (m_pMainFrame->IsBDStartFile(fn) || m_pMainFrame->IsBDPlsFile(fn)) {
			AddItem(fns, subs);
			CString empty, label;
			m_pMainFrame->MakeBDLabel(fn, empty, &label);
			curPlayList.GetTail().m_label = label;
			return;
		} else if (m_pMainFrame->IsDVDStartFile(fn)) {
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

static CString CombinePath(CStringW base, const CStringW& relative)
{
	if (StartsWith(relative, L":\\", 1) || StartsWith(relative, L"\\")) {
		return relative;
	}

	CUrlParser urlParser(relative.GetString());
	if (urlParser.IsValid()) {
		return relative;
	}

	if (urlParser.Parse(base)) {
		return CUrlParser::CombineUrl(base, relative);
	}

	CombineFilePath(base, relative);
	return base;
}

bool CPlayerPlaylistBar::ParseMPCPlayList(const CString& fn)
{
	Content::Online::Disconnect(fn);

	CString str;
	std::map<int, CPlaylistItem> pli;
	std::vector<int> idx;
	int selected_idx = -1;

	auto& PlayList = GetCurPlayList();
	auto& curTab = GetCurTab();

	CWebTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn) || f.GetLength() > 10 * MEGABYTE || !f.ReadString(str) || str != L"MPCPLAYLIST") {
		if (curTab.type == PL_EXPLORER) {
			TParseFolder(L".\\");
		}
		return false;
	}

	CStringW base = GetFolderPath(fn);

	PlayList.m_nSelectedAudioTrack = PlayList.m_nSelectedSubtitleTrack = -1;

	while (f.ReadString(str)) {
		std::list<CString> sl;
		Explode(str, sl, L',', 3);
		if (sl.size() == 2) {
			auto it = sl.cbegin();
			const auto& key = (*it++);
			int value = -1;
			StrToInt32((*it), value);

			if (key == L"audio") {
				PlayList.m_nSelectedAudioTrack = value;
			} else if (key == L"subtitles") {
				PlayList.m_nSelectedSubtitleTrack = value;
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
				idx.emplace_back(i);
			} else if (key == L"label") {
				pli[i].m_label = value;
			} else if (key == L"time") {
				pli[i].m_duration = StringToReftime2(value);
			} else if (key == L"selected") {
				if (value == L"1") {
					selected_idx = i - 1;
				}
			} else if (key == L"filename") {
				value = (curTab.type == PL_BASIC) ? MakePath(CombinePath(base, value)) : value;
				if (!pli[i].m_fi.Valid()) {
					pli[i].m_fi = value;
				} else {
					pli[i].m_auds.emplace_back(value);
				}
			} else if (key == L"subtitle") {
				value = CombinePath(base, value);
				pli[i].m_subs.emplace_back(value);
			/*
			} else if (key == L"video") {
				while (pli[i].m_fns.size() < 2) {
					pli[i].m_fns.emplace_back(L"");
				}
				pli[i].m_fns = value;
			} else if (key == L"audio") {
				while (pli[i].m_fns.size() < 2) {
					pli[i].m_fns.emplace_back(L"");
				}
				pli[i].m_fns.back() = value;
			*/
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

	const BOOL bIsEmpty = PlayList.IsEmpty();

	std::sort(idx.begin(), idx.end());

	const bool bParseDuration = AfxGetAppSettings().bPlaylistDetermineDuration && curTab.type == PL_BASIC;
	for (size_t i = 0; i < idx.size(); i++) {
		PlayList.Append(pli[idx[i]], bParseDuration);
	}

	if (curTab.type == PL_EXPLORER) {
		CString selected_path;
		if (bIsEmpty && selected_idx >= 0 && selected_idx < PlayList.GetCount()) {
			POSITION pos = PlayList.FindIndex(selected_idx);
			if (pos) {
				selected_path = PlayList.GetAt(pos).m_fi.GetPath();
			}
		}
		selected_idx = 0;

		if (PlayList.IsEmpty()) {
			TParseFolder(L".\\");
		}
		else {
			auto path = PlayList.GetHead().m_fi.GetPath();
			if (LastChar(path) == L'<') {
				path.TrimRight(L'<');
				PlayList.RemoveAll();
				if (::PathFileExistsW(path)) {
					TParseFolder(path);
				}
				else {
					TParseFolder(L".\\");
				}
			}
			else {
				PlayList.RemoveAll();
				TParseFolder(L".\\");
			}
		}

		if (!selected_path.IsEmpty()) {
			int idx = 0;
			POSITION pos = PlayList.GetHeadPosition();
			while (pos) {
				CPlaylistItem& pli = PlayList.GetAt(pos);
				if (pli.FindFile(selected_path)) {
					selected_idx = idx;
					break;
				}
				PlayList.GetNext(pos);
				idx++;
			}
		}
	}

	if (bIsEmpty && selected_idx >= 0 && selected_idx < PlayList.GetCount()) {
		PlayList.m_nSelected_idx = selected_idx - 1;
		PlayList.m_nFocused_idx = PlayList.m_nSelected_idx + 1;
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
			CString fn = pli.m_fi.GetPath();
			if (bRemovePath) {
				fn = GetFileName(fn);
			}
			f.WriteString(idx + L",filename," + fn + L"\n");

			for (const auto& ai : pli.m_auds) {
				fn = ai.GetPath();
				if (bRemovePath) {
					fn = GetFileName(fn);
				}
				f.WriteString(idx + L",filename," + fn + L"\n");
			}

			for (const auto& si : pli.m_subs) {
				fn = si.GetPath();
				if (bRemovePath) {
					fn = GetFileName(fn);
				}
				f.WriteString(idx + L",subtitle," + fn + L"\n");
			}
		} else if (pli.m_type == CPlaylistItem::device && pli.m_fi.Valid()) {
			f.WriteString(idx + L",video," + pli.m_fi.GetPath() + L"\n");
			if (pli.m_auds.size()) {
				f.WriteString(idx + L",audio," + pli.m_auds.front().GetPath() + L"\n");
			}
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
	Content::Online::Disconnect(fn);

	CWebTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn)) {
		return false;
	}

	CStringW base;
	if (f.GetRedirectURL().GetLength()) {
		base = f.GetRedirectURL();
	}
	else {
		base = fn;
	}

	if (fn.Find(L"://") > 0) {
		base.Truncate(base.ReverseFind('/'));
	} else {
		RemoveFileSpec(base);
	}

	std::map<CString, CAudioItemList> audio_fns;
	CString audioId;

	std::vector<CPlaylistItem> playlist;

	CStringW title;
	CStringW album;
	REFERENCE_TIME duration = 0;

	INT_PTR c = curPlayList.GetCount();
	CStringW str;
	while (f.ReadString(str)) {
		FastTrim(str);

		if (str.IsEmpty() || (StartsWith(str, L"#EXTM3U"))) {
			continue;
		}

		if (str.GetAt(0) == L'#') {
			auto DeleteLeft = [](const auto pos, auto& str) {
				str = str.Mid(pos, str.GetLength() - pos);
				str.TrimLeft();
			};

			if (StartsWith(str, L"#EXTINF:")) {
				DeleteLeft(8, str);

				int pos = str.Find(L',');
				if (pos > 0) {
					const auto tmp = str.Left(pos);
					int dur = 0;
					if (swscanf_s(tmp, L"%dx", &dur) == 1) {
						pos++;
						str = str.Mid(pos, str.GetLength() - pos);

						if (dur > 0) {
							duration = UNITS * dur;
						}
					}
				}
				title = str.TrimLeft();
			} else if (StartsWith(str, L"#EXTALB:")) {
				DeleteLeft(8, str);
				album = str;
			} else if (StartsWith(str, L"#EXT-X-STREAM-INF:")) {
				DeleteLeft(18, str);
				title = str;

				audioId = RegExpParse(str.GetString(), LR"(AUDIO=\"(\S*?)\")");
			} else if (StartsWith(str, L"#EXT-X-MEDIA:") && str.Find(L"TYPE=AUDIO") >= 13) {
				const auto id = RegExpParse(str.GetString(), LR"(GROUP-ID=\"(\S*?)\")");
				if (!id.IsEmpty()) {
					const auto url = RegExpParse(str.GetString(), LR"(URI=\"(.*?)\")");
					if (!url.IsEmpty()) {
						auto& audio_items = audio_fns[id];
						audio_items.emplace_back(MakePath(CombinePath(base, url)));
					}
				}
			}
		} else {
			const auto path = MakePath(CombinePath(base, str));
			if (!fn.CompareNoCase(path)) {
				continue;
			}

			if (!::PathIsURLW(path)) {
				const auto ext = GetFileExt(path).MakeLower();
				if (ext == L".m3u" || ext == L".m3u8") {
					if (ParseM3UPlayList(path)) {
						continue;
					}
				} else if (ext == L".mpcpl") {
					if (ParseMPCPlayList(path)) {
						continue;
					}
				} else {
					std::list<CString> redir;
					const auto ct = Content::GetType(path, &redir);
					if (!redir.empty()) {
						for (const auto& r : redir) {
							ParsePlayList(r, nullptr);
						}
						continue;
					}
				}
			}

			CPlaylistItem pli;

			pli.m_fi = path;
			if (!audioId.IsEmpty()) {
				const auto it = audio_fns.find(audioId);
				if (it != audio_fns.cend()) {
					const auto& audio_items = it->second;
					pli.m_auds.insert(pli.m_auds.end(), audio_items.begin(), audio_items.end());
				}
				audioId.Empty();
			}

			if (title.GetLength()) {
				if (album.GetLength()) {
					pli.m_label = album + L" - " + title;
				} else {
					pli.m_label = title;
				}
			} else if (album.GetLength()) {
				pli.m_label = album;
			}

			pli.m_duration = duration;

			playlist.emplace_back(pli);

			title.Empty();
			album.Empty();
			duration = 0;
		}
	}

	bool bNeedParse = false;
	if (playlist.size() == 1) {
		const auto& pli = playlist.front();
		const auto& fn = pli.m_fi;
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
		for (auto& pli : playlist) {
			const auto& fn = pli.m_fi;
			if (::PathIsURLW(fn) && m_pMainFrame->OpenYoutubePlaylist(fn, TRUE)) {
				continue;
			}
			curPlayList.Append(pli, AfxGetAppSettings().bPlaylistDetermineDuration);
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
			fileNames.emplace_back(cueTrack.m_fn);
		}
	}

	CStringW base = GetFolderPath(fn);
	AddSlash(base);

	INT_PTR c = curPlayList.GetCount();

	for (const auto& fName : fileNames){
		CString fullPath = MakePath(base + fName);
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
					fi.AddChapter(cueTrackTitle, cueTrack.m_rt);

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

			pli.m_fi = fi;

			curPlayList.Append(pli, AfxGetAppSettings().bPlaylistDetermineDuration);
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
	if (GetCurTab().type == PL_EXPLORER) {
		return false;
	}

	bool bWasPlaying = curPlayList.RemoveAll();
	m_list.DeleteAllItems();
	SavePlaylist();

	return bWasPlaying;
}

void CPlayerPlaylistBar::RemoveMissingFiles()
{
	if (GetCurTab().type == PL_BASIC) {
		auto& PlayList = GetCurPlayList();

		int n = 0;

		POSITION pos = PlayList.GetHeadPosition();
		while (pos) {
			POSITION cur = pos;
			CPlaylistItem& pli = PlayList.GetNext(pos);
			if (pli.m_type == CPlaylistItem::file) {
				LPCWSTR path = pli.m_fi;
				if (!::PathIsURLW(path) && !::PathFileExistsW(path)) {
					PlayList.RemoveAt(cur);
					n++;
				}
			}
		}

		if (n) {
			SetupList();
			SavePlaylist();
		}
	}
}

void CPlayerPlaylistBar::Remove(const std::vector<int>& items, const bool bDelete)
{
	if (!items.empty()) {
		if (bDelete && (MessageBoxW(ResStr(IDS_PLAYLIST_DELETE_QUESTION), nullptr, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDNO)) {
			SetFocus();
			return;
		}

		m_list.SetRedraw(FALSE);

		bool bWasPlaying = false;
		std::list<CString> fns;

		for (int i = (int)items.size() - 1; i >= 0; --i) {
			POSITION pos = FindPos(items[i]);

			if (bDelete) {
				const auto item = curPlayList.GetAt(pos);
				if (item.m_fi.Valid()) {
					CStringW ext = item.m_fi.GetExt().MakeLower();
					if (ext != L".ifo" && ext != L".bdmv" && ext != L".mpls") {
						if (::PathFileExistsW(item.m_fi)) {
							fns.emplace_back(item.m_fi);
						}
						for (const auto& aud : item.m_auds) {
							if (::PathFileExistsW(aud)) {
								fns.emplace_back(aud);
							}
						}
						for (const auto& sub : item.m_subs) {
							if (::PathFileExistsW(sub)) {
								fns.emplace_back(sub);
							}
						}
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

			for (const auto& fn : fns) {
				FileOperationDelete(fn);
			}

			if (bWasPlaying) {
				m_list.Invalidate();
				m_pMainFrame->OpenCurPlaylistItem();
			}
		}

		SetFocus();
	}
}

void CPlayerPlaylistBar::Open(const CString& fn)
{
	std::list<CString> fns;
	fns.emplace_front(fn);

	m_bSingleElement = true;
	Open(fns, false);
	m_bSingleElement = false;
}

void CPlayerPlaylistBar::Open(std::list<CString>& fns, const bool bMulti, CSubtitleItemList* subs/* = nullptr*/, bool bCheck/* = true*/)
{
	SelectDefaultPlaylist();

	Empty();
	ResolveLinkFiles(fns);
	Append(fns, bMulti, subs, bCheck);
}

void CPlayerPlaylistBar::Append(const CString& fn)
{
	std::list<CString> fns;
	fns.emplace_front(fn);
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

	if (GetCurTab().type == PL_EXPLORER) {
		curPlayList.m_nFocused_idx = TGetFocusedElement();
		m_nCurPlayListIndex = 0;

		TEnsureVisible(m_nCurPlayListIndex);
		TSelectTab();
	}

	for (auto& fn : fns) {
		if (::PathIsURLW(fn)) {
			fn = UrlDecode(fn);
		}
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
	if (GetCurTab().type == PL_EXPLORER) {
		curPlayList.m_nFocused_idx = TGetFocusedElement();
		m_nCurPlayListIndex = 0;

		TEnsureVisible(m_nCurPlayListIndex);
		TSelectTab();
	}

	INT_PTR idx = curPlayList.GetCount();
	if (idx != 0) {
		idx++;
	}

	for (const auto& fi : fis) {
		CPlaylistItem pli;
		pli.m_fi = fi.GetPath();
		pli.m_label = fi.GetTitle();
		pli.m_duration = fi.GetDuration();
		curPlayList.Append(pli, AfxGetAppSettings().bPlaylistDetermineDuration);
	}

	Refresh();
	EnsureVisible(curPlayList.FindIndex(idx));
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
	pli.m_fi       = vdn;
	pli.m_auds.emplace_back(adn);
	pli.m_vinput   = vinput;
	pli.m_vchannel = vchannel;
	pli.m_ainput   = ainput;
	std::list<CStringW> sl;
	CStringW vfn = GetFriendlyName(vdn);
	CStringW afn = GetFriendlyName(adn);
	if (!vfn.IsEmpty()) {
		sl.emplace_back(vfn);
	}
	if (!afn.IsEmpty()) {
		sl.emplace_back(afn);
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

	auto& PlayList = GetCurPlayList();

	POSITION pos = PlayList.GetHeadPosition();
	for (int i = 0; pos; i++) {
		DWORD_PTR dwData = (DWORD_PTR)pos;
		CPlaylistItem& pli = PlayList.GetNext(pos);
		m_list.SetItemData(m_list.InsertItem(i, pli.GetLabel(0)), dwData);
		m_list.SetItemText(i, COL_TIME, pli.GetLabel(1));
	}
}

void CPlayerPlaylistBar::UpdateList()
{
	auto& PlayList = GetCurPlayList();

	POSITION pos = PlayList.GetHeadPosition();
	for (int i = 0, len = m_list.GetItemCount(); pos && i < len; i++) {
		DWORD_PTR dwData = (DWORD_PTR)pos;
		CPlaylistItem& pli = PlayList.GetNext(pos);
		m_list.SetItemData(i, dwData);
		m_list.SetItemText(i, COL_NAME, pli.GetLabel(0));
		m_list.SetItemText(i, COL_TIME, pli.GetLabel(1));
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
	if (bOnlyFiles && GetCurTab().type == PL_EXPLORER) {
		return curPlayList.m_nFilesCount;
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
		isAtEnd = curPlayList.GetNextWrap(pos).MustBeSkipped();
		while (isAtEnd && pos && pos != tail) {
			isAtEnd = curPlayList.GetNextWrap(pos).MustBeSkipped();
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
	if (pli && pli->m_fi.Valid()) {
		fn = pli->m_fi.GetPath();
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
		if ((playlist.MustBeSkipped() || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
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
		if ((playlist.MustBeSkipped() || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
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
			if ((playlist.MustBeSkipped() || playlist.m_bDirectory) && pos != org) {
				continue;
			}
			break;
		}
	}
	curPlayList.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetFirst()
{
	POSITION pos = curPlayList.GetTailPosition(), org = pos;
	for (;;) {
		const auto& playlist = curPlayList.GetNextWrap(pos);
		if ((playlist.MustBeSkipped() || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
	curPlayList.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetLast()
{
	POSITION pos = curPlayList.GetHeadPosition(), org = pos;
	for (;;) {
		const auto& playlist = curPlayList.GetPrevWrap(pos);
		if ((playlist.MustBeSkipped() || playlist.m_bDirectory) && pos != org) {
			continue;
		}
		break;
	}
	curPlayList.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetCurValid(const bool bValid)
{
	for (size_t i = 0; i < m_tabs.size(); i++) {
		if (m_nCurPlaybackListId == m_tabs[i].id) {
			POSITION pos = m_pls[i]->GetPos();
			if (pos) {
				auto& pli = m_pls[i]->GetAt(pos);
				pli.m_bInvalid = !bValid;
				if (i == m_nCurPlayListIndex) {
					auto index = FindItem(pos);
					if (index >= 0) {
						m_list.SetItemText(index, COL_NAME, pli.GetLabel(0));
						m_list.SetItemText(index, COL_TIME, pli.GetLabel(1));
					}
				}
			}
			break;
		}
	}
}

void CPlayerPlaylistBar::SetCurLabel(const CString& label)
{
	for (size_t i = 0; i < m_tabs.size(); i++) {
		if (m_nCurPlaybackListId == m_tabs[i].id) {
			POSITION pos = m_pls[i]->GetPos();
			if (pos) {
				auto& pli = m_pls[i]->GetAt(pos);
				pli.m_label = label;
				if (i == m_nCurPlayListIndex) {
					auto index = FindItem(pos);
					if (index >= 0) {
						m_list.SetItemText(index, COL_NAME, pli.GetLabel(0));
					}
				}
			}
			break;
		}
	}
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
	for (size_t i = 0; i < m_tabs.size(); i++) {
		if (m_nCurPlaybackListId == m_tabs[i].id) {
			POSITION pos = m_pls[i]->GetPos();
			if (pos) {
				auto& pli = m_pls[i]->GetAt(pos);
				pli.m_duration = rt;
				if (i == m_nCurPlayListIndex) {
					auto index = FindItem(pos);
					if (index >= 0) {
						m_list.SetItemText(index, COL_TIME, pli.GetLabel(1));
					}
				}
			}
			break;
		}
	}
}

OpenMediaData* CPlayerPlaylistBar::GetCurOMD(REFERENCE_TIME rtStart)
{
	CPlaylistItem* pli = GetCur();
	if (pli == nullptr) {
		return nullptr;
	}

	pli->AutoLoadFiles();

	CStringW fn = pli->m_fi.GetPath();
	fn.MakeLower();

	if (TGetPathType(fn) != IT_FILE) {
		return nullptr;
	}

	m_nCurPlaybackListId = GetCurTab().id;

	if (fn.Find(L"video_ts.ifo") >= 0) {
		if (OpenDVDData* p = DNew OpenDVDData()) {
			p->path = pli->m_fi.GetPath();
			p->subs = pli->m_subs;
			return p;
		}
	}

	if (pli->m_type == CPlaylistItem::device) {
		if (OpenDeviceData* p = DNew OpenDeviceData()) {
			p->DisplayName[0] = pli->m_fi.GetPath();
			auto it = pli->m_auds.cbegin();
			for (unsigned i = 1; i < std::size(p->DisplayName) && it != pli->m_auds.cend(); ++i, ++it) {
				p->DisplayName[i] = (*it).GetPath();
			}

			p->vinput = pli->m_vinput;
			p->vchannel = pli->m_vchannel;
			p->ainput = pli->m_ainput;
			return p;
		}
	} else {
		if (OpenFileData* p = DNew OpenFileData()) {
			p->fi = pli->m_fi;
			p->auds = pli->m_auds;
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
	const CAppSettings& s = AfxGetAppSettings();

	CStringW base;
	if (AfxGetMyApp()->GetAppSavePath(base)) {
		int curpl = m_nCurPlayListIndex;
		for (size_t i = 0; i < m_tabs.size(); i++) {
			m_nCurPlayListIndex = i;
			const CStringW pl_path = base + m_tabs[i].mpcpl_fn;

			if (::PathFileExistsW(pl_path)) {
				if (m_nCurPlayListIndex > 0 || s.bRememberPlaylistItems) {
					ParseMPCPlayList(pl_path);
				}
				else {
					::DeleteFileW(pl_path);
				}
			}
			else if (m_tabs[i].type == PL_EXPLORER) {
				TParseFolder(L".\\");
			}
		}
		m_nCurPlayListIndex = curpl;
	}

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
	const CAppSettings& s = AfxGetAppSettings();

	CString base;
	if (AfxGetMyApp()->GetAppSavePath(base)) {
		CString file = base + GetCurTab().mpcpl_fn;

		if (m_nCurPlayListIndex > 0 || s.bRememberPlaylistItems) {
			// create this folder when needed only
			if (!::PathFileExistsW(base)) {
				::CreateDirectoryW(base, nullptr);
			}

			SaveMPCPlayList(file, CTextFile::UTF8, false);
		}
		else if (::PathFileExistsW(file)) {
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
	ON_WM_DRAWITEM()
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlayPlay)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_PLAYLIST, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_BEGINLABELEDITW, IDC_PLAYLIST, OnLvnBeginlabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDITW, IDC_PLAYLIST, OnLvnEndlabeleditList)
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
		m_pMainFrame->SetFocus();
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
		if (GetCurTab().type == PL_BASIC && m_list.GetSelectedCount() > 0) {
			std::vector<int> items;
			items.reserve(m_list.GetSelectedCount());
			POSITION pos = m_list.GetFirstSelectedItemPosition();
			while (pos) {
				items.emplace_back(m_list.GetNextSelectedItem(pos));
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
	auto& curTab = GetCurTab();

	if (curTab.type == PL_BASIC) {
		if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0) {
			POSITION pos = FindPos(lpnmlv->iItem);
			// If the file is already playing, don't try to restore a previously saved position
			if (m_pMainFrame->GetPlaybackMode() == PM_FILE && pos == curPlayList.GetPos()) {
				const CPlaylistItem& pli = curPlayList.GetAt(pos);
				//AfxGetAppSettings().RemoveFile(pli.m_fns);
			}
			else {
				curPlayList.SetPos(pos);
			}

			m_list.Invalidate();
			m_pMainFrame->OpenCurPlaylistItem();
		}
	}
	else if (curTab.type == PL_EXPLORER) {
		if (TNavigate()) {
		}
		else if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0) {
			POSITION pos = FindPos(lpnmlv->iItem);
			// If the file is already playing, don't try to restore a previously saved position
			if (m_pMainFrame->GetPlaybackMode() == PM_FILE && pos == curPlayList.GetPos()) {
				const CPlaylistItem& pli = curPlayList.GetAt(pos);
				//AfxGetAppSettings().RemoveFile(pli.m_fns);
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
		if (!m_bEdit) {
			ResizeListColumn();
		}

		CRect r;
		GetClientRect(&r);
		FillRect(pLVCD->nmcd.hdc, &r, CBrush(m_crBackground));

		*pResult = CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW;

	}
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
	CRect rcText(rcItem);

	POSITION pos = FindPos(nItem);
	if (!pos) {
		ASSERT(FALSE);
		return;
	}

	const CAppSettings& s = AfxGetAppSettings();
	const auto bSelected = pos == curPlayList.GetPos();
	const CPlaylistItem& pli = curPlayList.GetAt(pos);

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	int oldDC = pDC->SaveDC();

	if (!!m_list.GetItemState(nItem, LVIS_SELECTED)) {
		if (s.bUseDarkTheme) {
			GRADIENT_RECT gr = { 0, 1 };
			tvSelected[0].x = rcItem.left; tvSelected[0].y = rcItem.top;
			tvSelected[1].x = rcItem.right; tvSelected[1].y = rcItem.bottom;
			pDC->GradientFill(tvSelected, 2, &gr, 1, GRADIENT_FILL_RECT_V);
			pDC->Draw3dRect(rcItem, m_crAvtiveItem3dRectTopLeft, m_crAvtiveItem3dRectBottomRight);
		}
		else {
			FillRect(pDC->m_hDC, rcItem, CBrush(0x00F1DACC));
			pDC->SetBkColor(0x00F1DACC);
			FrameRect(pDC->m_hDC, rcItem, CBrush(0xC56A31));
		}
	} else {
		if (s.bUseDarkTheme) {
			GRADIENT_RECT gr = { 0, 1 };
			tvNormal[0].x = rcItem.left; tvNormal[0].y = rcItem.top;
			tvNormal[1].x = rcItem.right; tvNormal[1].y = rcItem.bottom;
			pDC->GradientFill(tvNormal, 2, &gr, 1, GRADIENT_FILL_RECT_V);
		}
		else {
			FillRect(pDC->m_hDC, rcItem, CBrush(m_crBackground));
			pDC->SetBkColor(m_crBackground);
		}
	}

	if (s.bUseDarkTheme && m_bDrawDragImage) {
		FillRect(pDC->m_hDC, rcItem, CBrush(m_crDragImage));
		pDC->SetBkColor(m_crDragImage);
	}

	COLORREF textcolor = bSelected ? 0xFF : 0;

	if (s.bUseDarkTheme) {
		textcolor = bSelected ? m_crActiveItem : (!!m_list.GetItemState(nItem, LVIS_SELECTED) ? m_crSelectedItem : m_crNormalItem);
	}

	if (pli.m_bInvalid) {
		textcolor |= 0xA0A0A0;
	}

	pDC->SetTextColor(textcolor);

	CString time = m_list.GetItemText(nItem, COL_TIME);
	if (time.GetLength()) {
		CSize timesize = pDC->GetTextExtent(time);
		if ((3 + timesize.cx + 3) < rcItem.Width() / 2) {
			CRect rc(rcItem);
			rc.left = rc.right - (3 + timesize.cx + 3);
			pDC->DrawTextW(time, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			rcText.right -= (timesize.cx + 6);
		}
	}

	CString fmt, file;
	fmt.Format(L"%%0%dd. %%s", (int)log10(0.1 + curPlayList.GetCount()) + 1);
	file.Format(fmt, nItem + 1, m_list.GetItemText(nItem, COL_NAME));

	int offset = 0;
	if (GetCurTab().type == PL_EXPLORER) {
		file = m_list.GetItemText(nItem, COL_NAME);
		const int w = rcItem.Height() - 4;

		bool bfsFolder = false;
		if (LastChar(file) == L'<') {
			file = L"[..]";
			bfsFolder = true;
		}
		else if (LastChar(file) == L'>'){
			file.TrimRight(L'>');
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
				if (pli.m_fi.Valid()) {
					path = pli.m_fi.GetPath();
				}
			}

			auto ext = GetFileExt(path).MakeLower();
			if (ext.IsEmpty() && LastChar(path) == L':') {
				ext = path;
			}

			hIcon = w > 24 ? m_icons_large[ext] : m_icons[ext];
		}

		DrawIconEx(pDC->m_hDC, rcItem.left + 2, rcItem.top + 2, hIcon, w, w, 0, nullptr, DI_NORMAL);

		offset = rcItem.Height();
	}

	rcText.left += (3 + offset);
	rcText.right -= 6;
	pDC->DrawTextW(file, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

	pDC->RestoreDC(oldDC);
}

BOOL CPlayerPlaylistBar::OnPlayPlay(UINT nID)
{
	m_list.Invalidate();
	return FALSE;
}

void CPlayerPlaylistBar::DropFiles(std::list<CString>& slFiles)
{
	if (GetCurTab().type == PL_EXPLORER) {
		return;
	}

	SetForegroundWindow();
	m_list.SetFocus();

	m_pMainFrame->ParseDirs(slFiles);

	Append(slFiles, true);
}

void CPlayerPlaylistBar::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (GetCurTab().type == PL_EXPLORER) {
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
		m_DragIndexes.emplace_back(nItem);
		if (nItem == m_nDragIndex) {
			valid = true;
		}
	}

	if (!valid) {
		ASSERT(0);
		m_DragIndexes.resize(1);
		m_DragIndexes.front() = m_nDragIndex;
	}

	const auto& s = AfxGetAppSettings();

	m_bDrawDragImage = true;
	CPoint p;
	m_pDragImage = m_list.CreateDragImageEx(&p);
	m_bDrawDragImage = false;

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

		const auto CWnd = WindowFromPoint(m_ptDropPoint);
		if (CWnd && CWnd->GetSafeHwnd() == m_list.GetSafeHwnd()) {
			CWnd->ScreenToClient(&m_ptDropPoint);

			m_pDragImage->DragShowNolock(TRUE);

			int iOverItem = m_list.HitTest(m_ptDropPoint);
			int iTopItem = m_list.GetTopIndex();
			int iBottomItem = m_list.GetBottomIndex();

			if (iOverItem == iTopItem && iTopItem != 0) { // top of list
				SetTimer(1, 100, nullptr);
			} else {
				KillTimer(1);
			}

			if ((iOverItem >= iBottomItem && iBottomItem != (m_list.GetItemCount() - 1))) { // bottom of list
				SetTimer(2, 100, nullptr);
			} else {
				KillTimer(2);
			}
		} else {
			m_pDragImage->DragShowNolock(TRUE);
			KillTimer(1);
			KillTimer(2);
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
	} else if (point.x < m_tab_buttons[MENU].r.left) {
		if (m_tab_buttons[RIGHT].bVisible && m_tab_buttons[RIGHT].r.PtInRect(point)) {
			TSetOffset();
			TCalcLayout();
			TDrawBar();
		} else if (m_tab_buttons[LEFT].bVisible && m_tab_buttons[LEFT].r.PtInRect(point)) {
			TSetOffset(true);
			TCalcLayout();
			TDrawBar();
		} else {
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
	LVITEMW lvi = {};
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
			wcscpy_s(lvi.pszText, MAX_PATH, (LPCWSTR)(m_list.GetItemText(dragIdx, col)));
			lvi.iSubItem = col;
			m_list.SetItem(&lvi);
		}

		dropIdx++;
	}

	// delete dragged items in old positions
	for (int i = (int)m_DragIndexes.size()-1; i >= 0; --i) {
		m_list.DeleteItem(m_DragIndexes[i]);
	}

	std::vector<CPlaylistItem> pli_tmp;
	pli_tmp.reserve(m_list.GetItemCount());

	UINT id = (UINT)-1;
	for (int i = 0; i < m_list.GetItemCount(); i++) {
		POSITION pos = (POSITION)m_list.GetItemData(i);
		CPlaylistItem& pli = curPlayList.GetAt(pos);
		pli_tmp.emplace_back(pli);
		if (pos == curPlayList.GetPos()) {
			id = pli.m_id;
		}
	}
	curPlayList.RemoveAll();

	for (size_t i = 0; i < pli_tmp.size(); i++) {
		CPlaylistItem& pli = pli_tmp[i];
		curPlayList.AddTail(pli);
		if (pli.m_id == id) {
			curPlayList.SetPos(curPlayList.GetTailPosition());
		}
		m_list.SetItemData((int)i, (DWORD_PTR)curPlayList.GetTailPosition());
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

	strTipText = pli.m_fi.GetPath();
	if (pli.m_bDirectory) {
		strTipText.TrimRight(L"<>");
	}
	for (const auto& fi : pli.m_auds) {
		strTipText += L"\n" + fi.GetPath();
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
	} else {
		if (PathIsURLW(strTipText)) {
			EllipsisURL(strTipText, 300);
		} else {
			EllipsisPath(strTipText, 300);
		}
	}

	::SendMessageW(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)800);

	static auto wndStyle = ::GetWindowLongPtrW(pNMHDR->hwndFrom, GWL_STYLE);
	if ((wndStyle & TTS_NOPREFIX) == 0) {
		wndStyle |= TTS_NOPREFIX;
		::SetWindowLongPtrW(pNMHDR->hwndFrom, GWL_STYLE, wndStyle);
	}

	pTTT->lpszText = (LPWSTR)strTipText.GetString();

	*pResult = 0;

	return TRUE;
}

void CPlayerPlaylistBar::OnContextMenu(CWnd* /*pWnd*/, CPoint p)
{
	if (bTMenuPopup) {
		return;
	}

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

	auto& curTab = GetCurTab();

	LVHITTESTINFO lvhti;
	lvhti.pt = p;
	m_list.ScreenToClient(&lvhti.pt);
	m_list.SubItemHitTest(&lvhti);

	POSITION pos = FindPos(lvhti.iItem);

	ItemType item_type = IT_NONE;
	CString sCurrentPath;

	if (pos) {
		CPlaylistItem& pli = curPlayList.GetAt(pos);
		if (pli.m_fi.Valid()) {
			sCurrentPath = pli.m_fi.GetPath();

			if (curTab.type == PL_EXPLORER) {
				item_type = (ItemType)TGetPathType(sCurrentPath);
			}
			else if (::PathIsURLW(sCurrentPath)) {
				item_type = IT_URL;
			}
			else if (sCurrentPath == L"pipe://stdin") {
				item_type = IT_PIPE;
			}
			else if (::PathFileExistsW(sCurrentPath)) {
				item_type = IT_FILE;
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
		M_REMOVEMISSINGFILES,
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
		M_OPENFOLDER,
		M_MEDIAINFO,
		M_SHOWTOOLTIP,
		M_SHOWSEARCHBAR,
		M_HIDEFULLSCREEN,
		M_NEXTONERROR,
		M_SKIPINVALID,
		M_DURATION
	};

	CAppSettings& s = AfxGetAppSettings();

	m.AppendMenuW(MF_STRING | (bOnItem ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_OPEN, ResStr(IDS_PLAYLIST_OPEN) + L"\tEnter");
	if (curTab.type == PL_BASIC) {
		m.AppendMenuW(MF_STRING | MF_ENABLED, M_ADD, ResStr(IDS_PLAYLIST_ADD));
		m.AppendMenuW(MF_STRING | (bOnItem ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_REMOVE, ResStr(IDS_PLAYLIST_REMOVE) + L"\tDelete");
		m.AppendMenuW(MF_SEPARATOR);
		m.AppendMenuW(MF_STRING | (bOnItem ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_DELETE, ResStr(IDS_PLAYLIST_DELETE) + L"\tShift+Delete");
		m.AppendMenuW(MF_SEPARATOR);
		m.AppendMenuW(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_CLEAR, ResStr(IDS_PLAYLIST_CLEAR));
		m.AppendMenuW(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_REMOVEMISSINGFILES, ResStr(IDS_PLAYLIST_REMOVEMISSINGFILES));
		m.AppendMenuW(MF_SEPARATOR);
	}
	m.AppendMenuW(MF_STRING | (m_list.GetSelectedCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_TOCLIPBOARD, ResStr(IDS_PLAYLIST_COPYTOCLIPBOARD) + L"\tCtrl+C");
	if (curTab.type == PL_EXPLORER) {
		const bool bReverse = !!(curTab.sort >> 8);
		const auto sort = (SORT)(curTab.sort & 0xF);

		m.AppendMenuW(MF_SEPARATOR);
		CMenu submenu2;
		submenu2.CreatePopupMenu();
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED | (sort == SORT::NAME ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYNAME, ResStr(IDS_PLAYLIST_SORTBYNAME));
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED | (sort == SORT::DATE ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYDATE, ResStr(IDS_PLAYLIST_SORTBYDATE));
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED | (sort == SORT::DATE_CREATED ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYDATECREATED, ResStr(IDS_PLAYLIST_SORTBYDATECREATED));
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED | (sort == SORT::SIZE ? (MFT_RADIOCHECK | MFS_CHECKED) : 0), M_SORTBYSIZE, ResStr(IDS_PLAYLIST_SORTBYSIZE));
		submenu2.AppendMenuW(MF_SEPARATOR);
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED | (bReverse ? MF_CHECKED : MF_UNCHECKED), M_SORTREVERSE, ResStr(IDS_PLAYLIST_SORTREVERSE));
		m.AppendMenuW(MF_STRING | MF_POPUP | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), (UINT_PTR)submenu2.Detach(), ResStr(IDS_PLAYLIST_SORT));
		m.AppendMenuW(MF_SEPARATOR);

		m.AppendMenuW(MF_STRING | MF_ENABLED, M_REFRESH, ResStr(IDS_PLAYLIST_EXPLORER_REFRESH));
		m.AppendMenuW(MF_SEPARATOR);
		m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bShufflePlaylistItems ? MF_CHECKED : MF_UNCHECKED), M_SHUFFLE, ResStr(IDS_PLAYLIST_SHUFFLE));
		m.AppendMenuW(MF_SEPARATOR);
	}
	else { // (curTab.type == PL_BASIC)
		m.AppendMenuW(MF_STRING | ((::IsClipboardFormatAvailable(CF_UNICODETEXT) || ::IsClipboardFormatAvailable(CF_HDROP)) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_FROMCLIPBOARD, ResStr(IDS_PLAYLIST_PASTEFROMCLIPBOARD) + L"\tCtrl+V");
		m.AppendMenuW(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_SAVEAS, ResStr(IDS_PLAYLIST_SAVEAS));
		m.AppendMenuW(MF_SEPARATOR);
		CMenu submenu2;
		submenu2.CreatePopupMenu();
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED, M_SORTBYNAME, ResStr(IDS_PLAYLIST_SORTBYLABEL));
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED, M_SORTBYPATH, ResStr(IDS_PLAYLIST_SORTBYPATH));
		submenu2.AppendMenuW(MF_STRING | MF_ENABLED, M_SORTREVERSE, ResStr(IDS_PLAYLIST_SORTREVERSE));
		m.AppendMenuW(MF_STRING | MF_POPUP | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), (UINT_PTR)submenu2.Detach(), ResStr(IDS_PLAYLIST_SORT));
		m.AppendMenuW(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_RANDOMIZE, ResStr(IDS_PLAYLIST_RANDOMIZE));
		m.AppendMenuW(MF_STRING | (curPlayList.GetCount() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_SORTBYID, ResStr(IDS_PLAYLIST_RESTORE));
		m.AppendMenuW(MF_SEPARATOR);
		m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bShufflePlaylistItems ? MF_CHECKED : MF_UNCHECKED), M_SHUFFLE, ResStr(IDS_PLAYLIST_SHUFFLE));
		m.AppendMenuW(MF_SEPARATOR);
	}
	m.AppendMenuW(MF_STRING | (bOnItem && item_type == IT_FILE ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), M_MEDIAINFO, L"MediaInfo");
	if (item_type == IT_FILE) {
		m.AppendMenuW(MF_STRING | MF_ENABLED, M_OPENFOLDER, ResStr(IDS_PLAYLIST_OPENFOLDER));
	}
	m.AppendMenuW(MF_SEPARATOR);
	m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bShowPlaylistTooltip ? MF_CHECKED : MF_UNCHECKED), M_SHOWTOOLTIP, ResStr(IDS_PLAYLIST_SHOWTOOLTIP));
	m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bShowPlaylistSearchBar ? MF_CHECKED : MF_UNCHECKED), M_SHOWSEARCHBAR, ResStr(IDS_PLAYLIST_SHOWSEARCHBAR));
	m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bHidePlaylistFullScreen ? MF_CHECKED : MF_UNCHECKED), M_HIDEFULLSCREEN, ResStr(IDS_PLAYLIST_HIDEFS));
	m.AppendMenuW(MF_SEPARATOR);
	m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bPlaylistNextOnError ? MF_CHECKED : MF_UNCHECKED), M_NEXTONERROR, ResStr(IDS_PLAYLIST_NEXTONERROR));
	m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bPlaylistSkipInvalid ? MF_CHECKED : MF_UNCHECKED), M_SKIPINVALID, ResStr(IDS_PLAYLIST_SKIPINVALID));

	if (curTab.type == PL_BASIC) {
		m.AppendMenuW(MF_SEPARATOR);
		m.AppendMenuW(MF_STRING | MF_ENABLED | (s.bPlaylistDetermineDuration ? MF_CHECKED : MF_UNCHECKED), M_DURATION, ResStr(IDS_PLAYLIST_DETERMINEDURATION));
	}

	m_pMainFrame->SetColorMenu(m);

	int nID = (int)m.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RETURNCMD, p.x, p.y, this);

	switch (nID) {
		case M_OPEN:
			if (!TNavigate()) {
				curPlayList.SetPos(pos);
				m_list.Invalidate();
				m_pMainFrame->OpenCurPlaylistItem();
			}
			break;
		case M_ADD:
			if (curTab.type == PL_BASIC) {
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

					COpenFileDialog fd(nullptr, nullptr, dwFlags, filter, this);
					if (fd.DoModal() == IDOK) {
						std::list<CString> fns;
						fd.GetFilePaths(fns);

						Append(fns, fns.size() > 1, nullptr);
					}
				}
			}
			break;
		case M_REMOVE:
		case M_DELETE:
			if (curTab.type == PL_BASIC) {
				std::vector<int> items;
				items.reserve(m_list.GetSelectedCount());
				POSITION pos = m_list.GetFirstSelectedItemPosition();
				while (pos) {
					items.emplace_back(m_list.GetNextSelectedItem(pos));
				}

				Remove(items, nID == M_DELETE);
			}
			break;
		case M_CLEAR:
			if (curTab.type == PL_BASIC) {
				if (Empty()) {
					CloseMedia();
				}
			}
			break;
		case M_REMOVEMISSINGFILES:
			RemoveMissingFiles();
			break;
		case M_SORTBYID:
			if (curTab.type == PL_BASIC) {
				curPlayList.SortById();
				SetupList();
				SavePlaylist();
				EnsureVisible(curPlayList.GetPos(), true);
			}
			break;
		case M_SORTBYNAME:
			if (curTab.type == PL_EXPLORER) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::NAME) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::NAME;
					TSaveSettings();
					TFillPlaylist();
				}
			}
			else { // (curTab.type == PL_BASIC)
				curPlayList.SortByName();
				SetupList();
				SavePlaylist();
				EnsureVisible(curPlayList.GetPos(), true);
			}
			break;
		case M_SORTBYPATH:
			if (curTab.type == PL_BASIC) {
				curPlayList.SortByPath();
				SetupList();
				SavePlaylist();
				EnsureVisible(curPlayList.GetPos(), true);
			}
			break;
		case M_SORTBYDATE:
			if (curTab.type == PL_EXPLORER) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::DATE) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::DATE;
					TSaveSettings();
					TFillPlaylist();
				}
			}
			break;
		case M_SORTBYDATECREATED:
			if (curTab.type == PL_EXPLORER) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::DATE_CREATED) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::DATE_CREATED;
					TSaveSettings();
					TFillPlaylist();
				}
			}
			break;
		case M_SORTBYSIZE:
			if (curTab.type == PL_EXPLORER) {
				const auto sort = (SORT)(curTab.sort & 0xF);
				if (sort != SORT::SIZE) {
					const auto bReverse = curTab.sort >> 8;
					curTab.sort = (bReverse << 8) | SORT::SIZE;
					TSaveSettings();
					TFillPlaylist();
				}
			}
			break;
		case M_SORTREVERSE:
			if (curTab.type == PL_EXPLORER) {
				const auto bReverse = curTab.sort >> 8;
				const auto sort = (SORT)(curTab.sort & 0xF);
				curTab.sort = (!bReverse << 8) | sort;
				TSaveSettings();
				TFillPlaylist();
			}
			else { // (curTab.type == PL_BASIC)
				curPlayList.ReverseSort();
				SetupList();
				SavePlaylist();
				EnsureVisible(curPlayList.GetPos(), true);
			}
			break;
		case M_RANDOMIZE:
			if (curTab.type == PL_BASIC) {
				Randomize();
			}
			break;
		case M_TOCLIPBOARD:
			CopyToClipboard();
			break;
		case M_FROMCLIPBOARD:
			PasteFromClipboard();
			break;
		case M_SAVEAS: {
			if (curTab.type == PL_BASIC) {
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

				CStringW path(fd.GetPathName());
				CStringW base = GetFolderPath(path);
				s.strLastSavedPlaylistDir = GetAddSlash(base);

				if (GetFileExt(path).IsEmpty()) {
					switch (idx) {
					case 1:
						path.Append(L".mpcpl");
						break;
					case 2:
						path.Append(L".pls");
						break;
					case 3:
						path.Append(L".m3u");
						break;
					case 4:
						path.Append(L".asx");
						break;
					default:
						break;
					}
				}

				bool fRemovePath = true;

				pos = curPlayList.GetHeadPosition();
				while (pos && fRemovePath) {
					CPlaylistItem& pli = curPlayList.GetNext(pos);

					if (pli.m_type != CPlaylistItem::file) {
						fRemovePath = false;
					} else {
						{
							CStringW fpath = GetFolderPath(pli.m_fi);
							if (base != fpath) {
								fRemovePath = false;
							}
						}
						auto it_a = pli.m_auds.begin();
						while (it_a != pli.m_auds.end() && fRemovePath) {
							CStringW apath = GetFolderPath(*it_a++);
							if (base != apath) {
								fRemovePath = false;
							}
						}
						auto it_s = pli.m_subs.begin();
						while (it_s != pli.m_subs.end() && fRemovePath) {
							CStringW spath = GetFolderPath(*it_s++);
							if (base != spath) {
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

					CString fn = pli.m_fi;

					//if (fRemovePath) {
					//	fn = GetFileOnly(fn);
					//}

					switch (idx) {
					case 2:
						str.Format(L"File%d=%s\n", i + 1, fn.GetString());
						if (!pli.m_label.IsEmpty()) {
							str.AppendFormat(L"Title%d=%s\n", i + 1, pli.m_label.GetString());
						}
						if (pli.m_duration > 0) {
							str.AppendFormat(L"Length%d=%I64d\n", i + 1, (pli.m_duration + UNITS / 2) / UNITS);
						}
						break;
					case 3:
						str.Empty();
						if (!pli.m_label.IsEmpty() || pli.m_duration > 0) {
							str.AppendFormat(L"#EXTINF:%I64d,%s\n",
								pli.m_duration > 0 ? (pli.m_duration + UNITS / 2) / UNITS : -1LL,
								pli.m_label.GetString());
						}
						str.AppendFormat(L"%s\n", fn.GetString());
						break;
					case 4:
						str = L"<Entry>";
						if (!pli.m_label.IsEmpty()) {
							str.AppendFormat(L"<title>%s</title>", pli.m_label.GetString());
						}
						str.AppendFormat(L"<Ref href = \"%s\"/>", fn.GetString());
						str.Append(L"</Entry>\n");
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
		}
		break;
		case M_SHUFFLE:
			s.bShufflePlaylistItems = !s.bShufflePlaylistItems;
			break;
		case M_REFRESH:
			if (curTab.type == PL_EXPLORER) {
				CString selected_path;
				const auto focusedElement = TGetFocusedElement();
				POSITION pos = curPlayList.FindIndex(focusedElement);
				if (pos) {
					selected_path = curPlayList.GetAt(pos).m_fi.GetPath();
				}

				auto path = curPlayList.GetHead().m_fi.GetPath();
				if (LastChar(path) == L'<') {
					path.TrimRight(L'<');
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
				std::list<CString> files = { sCurrentPath };
				CPPageFileInfoSheet fileInfo(files, m_pMainFrame, m_pMainFrame, true);
				fileInfo.DoModal();
				s.nLastFileInfoPage = nID;
			}
			break;
		case M_OPENFOLDER:
			ShellExecuteW(nullptr, nullptr, L"explorer.exe", L"/select,\"" + sCurrentPath + L"\"", nullptr, SW_SHOWNORMAL);
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
		case M_SKIPINVALID:
			s.bPlaylistSkipInvalid = !s.bPlaylistSkipInvalid;
			break;
		case M_DURATION:
			s.bPlaylistDetermineDuration = !s.bPlaylistDetermineDuration;
			break;
	}
}

void CPlayerPlaylistBar::OnLvnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto hwnd = (HWND)m_list.SendMessage(LVM_GETEDITCONTROL);
	if (::IsWindow(hwnd)) {
		m_bEdit = true;
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
	m_bEdit = false;

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
	const CSize cz(width + (SysVersion::IsWin8orLater() ? m_pMainFrame->GetSystemMetricsDPI(SM_CXVSCROLL) : SM_CXVSCROLL),
				   rcTabBar.Height() * 3 + (AfxGetAppSettings().bShowPlaylistSearchBar ? m_nSearchBarHeight * 2 : 0));
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
		DeleteObject(hRgnExclude);
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
	bTMenuPopup = true;

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

	for (size_t i = 0; i < m_tabs.size(); i++) {
		UINT flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
		if (i == m_nCurPlayListIndex) {
			flags |= MF_CHECKED | MFT_RADIOCHECK;
		}
		menu.AppendMenuW(flags, 1 + i, m_tabs[i].name);
	}
	menu.AppendMenuW(MF_SEPARATOR);

	CMenu addPlaylistSubMenu;
	addPlaylistSubMenu.CreatePopupMenu();
	addPlaylistSubMenu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_PLSMENU_ADD_PLAYLIST, ResStr(IDS_PLAYLIST_ADD_PLAYLIST));
	addPlaylistSubMenu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_PLSMENU_ADD_EXPLORER, ResStr(IDS_PLAYLIST_ADD_EXPLORER));
	menu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)addPlaylistSubMenu.Detach(), ResStr(IDS_PLAYLIST_ADD_NEW));
	menu.AppendMenuW(MF_BYPOSITION | MF_STRING | ((m_nCurPlayListIndex > 0) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED))
		, ID_PLSMENU_RENAME_PLAYLIST, ResStr(IDS_PLAYLIST_RENAME_CURRENT));
	menu.AppendMenuW(MF_BYPOSITION | MF_STRING | ((m_nCurPlayListIndex > 0) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED))
		, ID_PLSMENU_DELETE_PLAYLIST, ResStr(IDS_PLAYLIST_DELETE_CURRENT));

	auto dockBarID = GetParent()->GetDlgCtrlID();

	menu.AppendMenuW(MF_SEPARATOR);
	CMenu positionPlaylistSubMenu;
	positionPlaylistSubMenu.CreatePopupMenu();

	UINT flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
	if (dockBarID == AFX_IDW_DOCKBAR_LEFT) {
		flags |= MF_CHECKED | MFT_RADIOCHECK;
	}
	positionPlaylistSubMenu.AppendMenuW(flags, ID_PLSMENU_POSITION_LEFT, ResStr(IDS_PLAYLIST_POSITION_LEFT));

	flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
	if (dockBarID == AFX_IDW_DOCKBAR_TOP) {
		flags |= MF_CHECKED | MFT_RADIOCHECK;
	}
	positionPlaylistSubMenu.AppendMenuW(flags, ID_PLSMENU_POSITION_TOP, ResStr(IDS_PLAYLIST_POSITION_TOP));

	flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
	if (dockBarID == AFX_IDW_DOCKBAR_RIGHT) {
		flags |= MF_CHECKED | MFT_RADIOCHECK;
	}
	positionPlaylistSubMenu.AppendMenuW(flags, ID_PLSMENU_POSITION_RIGHT, ResStr(IDS_PLAYLIST_POSITION_RIGHT));

	flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
	if (dockBarID == AFX_IDW_DOCKBAR_BOTTOM) {
		flags |= MF_CHECKED | MFT_RADIOCHECK;
	}
	positionPlaylistSubMenu.AppendMenuW(flags, ID_PLSMENU_POSITION_BOTTOM, ResStr(IDS_PLAYLIST_POSITION_BOTTOM));

	flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
	if (dockBarID == AFX_IDW_DOCKBAR_FLOAT) {
		flags |= MF_CHECKED | MFT_RADIOCHECK;
	}
	positionPlaylistSubMenu.AppendMenuW(flags, ID_PLSMENU_POSITION_FLOAT, ResStr(IDS_PLAYLIST_POSITION_FLOAT));

	menu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)positionPlaylistSubMenu.Detach(), ResStr(IDS_PLAYLIST_POSITION));

	m_pMainFrame->SetColorMenu(menu);

	int nID = (int)menu.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RETURNCMD, p.x, p.y, this);
	int size = m_tabs.size();

	bTMenuPopup = false;

	bool bNewExplorer = false;

	if (nID > 0 && nID <= size) {
		SavePlaylist();
		curPlayList.m_nFocused_idx = TGetFocusedElement();
		m_nCurPlayListIndex = nID - 1;
		TEnsureVisible(m_nCurPlayListIndex);
		TSelectTab();
	} else {
		CString strDefName;
		CString strGetName;

		switch (nID) {
			case ID_PLSMENU_ADD_PLAYLIST:
				{
					size_t cnt = 1;
					for (size_t i = 0; i < m_tabs.size(); i++) {
						if (m_tabs[i].type == PL_BASIC && i > 0) {
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
					tab.type = PL_BASIC;
					tab.name = strGetName;
					tab.mpcpl_fn.Format(L"Playlist%u.mpcpl", cnt);
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
			case ID_PLSMENU_ADD_EXPLORER:
				{
					size_t cnt = 1;
					for (size_t i = 0; i < m_tabs.size(); i++) {
						if (m_tabs[i].type == PL_EXPLORER) {
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
					tab.type = PL_EXPLORER;
					tab.name = strGetName;
					tab.mpcpl_fn.Format(L"Explorer%u.mpcpl", cnt);
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
			case ID_PLSMENU_RENAME_PLAYLIST:
				{
					strDefName = GetCurTab().name;
					CPlaylistNameDlg dlg(strDefName);
					if (dlg.DoModal() != IDOK) {
						return;
					}
					strGetName = dlg.m_name;
					if (strGetName.IsEmpty()) {
						return;
					}
					GetCurTab().name = strGetName;
					SavePlaylist();
					TSaveSettings();
				}
				break;
			case ID_PLSMENU_DELETE_PLAYLIST:
				{
					if (Empty()) {
						CloseMedia();
					}

					CString base;
					if (AfxGetMyApp()->GetAppSavePath(base)) {
						base.Append(GetCurTab().mpcpl_fn);

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
			case ID_PLSMENU_POSITION_FLOAT:
				if (dockBarID != AFX_IDW_DOCKBAR_FLOAT) {
					auto pMainFrame = AfxGetMainFrame();

					CRect rectMain;
					pMainFrame->GetWindowRect(rectMain);
					CPoint p(rectMain.right, rectMain.top);

					CRect rectDesktop;
					GetDesktopWindow()->GetWindowRect(&rectDesktop);

					CRect rect;
					GetWindowRect(rect);

					if (p.x < rectDesktop.left) {
						p.x = rectDesktop.left;
					}

					if (p.y < rectDesktop.top) {
						p.y = rectDesktop.top;
					}

					if (p.x + rect.Width() > rectDesktop.right) {
						p.x = rectDesktop.left;
					}

					if (p.y + rect.Height() > rectDesktop.bottom) {
						p.y = rectDesktop.top;
					}

					pMainFrame->m_wndToolBar.m_pDockSite->FloatControlBar(this, p);
				}
				break;
			case ID_PLSMENU_POSITION_LEFT:
				if (dockBarID != AFX_IDW_DOCKBAR_LEFT) {
					AfxGetMainFrame()->m_wndToolBar.m_pDockSite->DockControlBar(this, AFX_IDW_DOCKBAR_LEFT);
				}
				break;
			case ID_PLSMENU_POSITION_TOP:
				if (dockBarID != AFX_IDW_DOCKBAR_TOP) {
					AfxGetMainFrame()->m_wndToolBar.m_pDockSite->DockControlBar(this, AFX_IDW_DOCKBAR_TOP);
				}
				break;
			case ID_PLSMENU_POSITION_RIGHT:
				if (dockBarID != AFX_IDW_DOCKBAR_RIGHT) {
					AfxGetMainFrame()->m_wndToolBar.m_pDockSite->DockControlBar(this, AFX_IDW_DOCKBAR_RIGHT);
				}
				break;
			case ID_PLSMENU_POSITION_BOTTOM:
				if (dockBarID != AFX_IDW_DOCKBAR_BOTTOM) {
					AfxGetMainFrame()->m_wndToolBar.m_pDockSite->DockControlBar(this, AFX_IDW_DOCKBAR_BOTTOM);
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

	CStringW base;
	if (AfxGetMyApp()->GetAppSavePath(base)) {
		for (size_t i = 0; i < m_tabs.size(); i++) {
			const CStringW pl_path = base + m_tabs[i].mpcpl_fn;
			::DeleteFileW(pl_path);
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
				pli.m_fi = strDrive;
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

		curPlayList.CalcCountFiles();

		return;
	}

	CPlaylistItem pli;
	pli.m_bDirectory = true;
	pli.m_fi = GetRemoveSlash(path) + L"<"; // Parent folder mark;
	curPlayList.AddTail(pli);

	const CString folder(L"_folder_");
	if (m_icons.find(folder) == m_icons.cend()) {
		SHGetFileInfoW(folder, FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(SHFILEINFOW), uFlags | SHGFI_USEFILEATTRIBUTES);
		m_icons[folder] = shFileInfo.hIcon;

		SHGetFileInfoW(folder, FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(SHFILEINFOW), uFlagsLargeIcon | SHGFI_USEFILEATTRIBUTES);
		m_icons_large[folder] = shFileInfo.hIcon;
	}

	auto& curTab = GetCurTab();
	auto& directory = curTab.directory; directory.clear();
	auto& files = curTab.files; files.clear();

	const CString mask = GetAddSlash(path) + L"*.*";
	WIN32_FIND_DATAW FindFileData = {};
	HANDLE hFind = FindFirstFileW(mask, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			const CString fileName = FindFileData.cFileName;

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && fileName != L"." && fileName != L"..") {
					file_data_t file_data;
					file_data.name = GetAddSlash(path) + fileName;
					file_data.time_created.LowPart  = FindFileData.ftCreationTime.dwLowDateTime;
					file_data.time_created.HighPart = FindFileData.ftCreationTime.dwHighDateTime;
					file_data.time.LowPart  = FindFileData.ftLastWriteTime.dwLowDateTime;
					file_data.time.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;
					file_data.size.LowPart  = FindFileData.nFileSizeLow;
					file_data.size.HighPart = FindFileData.nFileSizeHigh;
					directory.emplace_back(file_data);
				}
			}
			else {
				const CString ext = GetFileExt(FindFileData.cFileName).MakeLower();
				auto mfc = mf.FindMediaByExt(ext);
				if (ext == L".iso" || (mfc && mfc->GetFileType() != TPlaylist)) {
					file_data_t file_data;
					file_data.name = GetAddSlash(path) + fileName;
					file_data.time_created.LowPart  = FindFileData.ftCreationTime.dwLowDateTime;
					file_data.time_created.HighPart = FindFileData.ftCreationTime.dwHighDateTime;
					file_data.time.LowPart  = FindFileData.ftLastWriteTime.dwLowDateTime;
					file_data.time.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;
					file_data.size.LowPart  = FindFileData.nFileSizeLow;
					file_data.size.HighPart = FindFileData.nFileSizeHigh;
					files.emplace_back(file_data);
				}
			}
		} while (::FindNextFileW(hFind, &FindFileData));
		::FindClose(hFind);
	}

	TFillPlaylist(true);
}

void CPlayerPlaylistBar::TFillPlaylist(const bool bFirst/* = false*/)
{
	auto& curTab = GetCurTab();
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
			selected_path = curPlayList.GetAt(pos).m_fi.GetPath();
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
		pli.m_fi = dir.name + L">"; // Folders mark
		curPlayList.AddTail(pli);
	}

	for (const auto& file : files) {
		CPlaylistItem pli;
		pli.m_fi = file.name;
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

	curPlayList.CalcCountFiles();

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

	const auto suffix = LastChar(path);
	if (suffix == L':') {
		return IT_DRIVE;
	}
	else if (suffix == L'<') {
		return IT_PARENT;
	}
	else if (suffix == L'>') {
		return IT_FOLDER;
	}

	return IT_FILE;
}

void CPlayerPlaylistBar::TTokenizer(const CString& strFields, LPCWSTR strDelimiters, std::vector<CString>& arFields)
{
	arFields.clear();

	int curPos = 0;
	CString token = strFields.Tokenize(strDelimiters, curPos);
	while (!token.IsEmpty()) {
		arFields.emplace_back(token);
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
			str.AppendFormat(L"%d;%s;%u%s%s", tab.type, GetRemoveFileExt(tab.mpcpl_fn.GetString()).GetString(), tab.sort, i > 0 ? CString(L";" + tab.name).GetString() : L"", i < last ? L"|" : L"");
		}
	}

	AfxGetAppSettings().strTabs = str;
	AfxGetAppSettings().SavePlaylistTabSetting();
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
		tab.mpcpl_fn = arFields[1] + (L".mpcpl");
		tab.id = GetNextId();
		if (arFields.size() >= 3) {
			tab.sort = _wtoi(arFields[2]);
		}
		if (!m_pls.empty() && arFields.size() >= 4) {
			tab.name = arFields[3];
		}
		m_tabs.emplace_back(tab);

		// add playlist
		CPlaylist* pl = DNew CPlaylist;
		m_pls.emplace_back(pl);
	}

	// if we have no tabs settings, create "Main"
	if (m_tabs.size() == 0) {
		tab_t tab;
		tab.type = PL_BASIC;
		tab.name = ResStr(IDS_PLAYLIST_MAIN_NAME);
		tab.mpcpl_fn = L"Default.mpcpl";
		tab.id = GetNextId();
		m_tabs.emplace_back(tab);

		// add playlist
		CPlaylist* pl = DNew CPlaylist;
		m_pls.emplace_back(pl);
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

static COLORREF ColorBrightness(const int lSkale, const COLORREF color)
{
	const int R = std::clamp(GetRValue(color) + lSkale, 0, 255);
	const int G = std::clamp(GetGValue(color) + lSkale, 0, 255);
	const int B = std::clamp(GetBValue(color) + lSkale, 0, 255);

	return RGB(R, G, B);
}

void CPlayerPlaylistBar::SetColor()
{
	if (AfxGetAppSettings().bUseDarkTheme) {
		m_crBkBar = ThemeRGB(45, 50, 55);           // background tab bar

		m_crBN = m_crBkBar;                         // backgroung normal
		m_crBNL = ColorBrightness(+15, m_crBN);     // backgroung normal lighten (for light edge)
		m_crBND = ColorBrightness(-15, m_crBN);     // backgroung normal darken (for dark edge)

		m_crBS = ColorBrightness(-15, m_crBN);      // backgroung selected background
		m_crBSL = ColorBrightness(+30, m_crBS);     // backgroung selected lighten (for light edge)
		m_crBSD = ColorBrightness(-15, m_crBS);     // backgroung selected darken (for dark edge)

		m_crBH = ColorBrightness(+20, m_crBN);      // backgroung highlighted background
		m_crBHL = ColorBrightness(+15, m_crBH);     // backgroung highlighted lighten (for light edge)
		m_crBHD = ColorBrightness(-30, m_crBH);     // backgroung highlighted darken (for dark edge)

		m_crBSH = ColorBrightness(-20, m_crBN);     // backgroung selected highlighted background
		m_crBSHL = ColorBrightness(+40, m_crBSL);   // backgroung selected highlighted lighten (for light edge)
		m_crBSHD = ColorBrightness(-0, m_crBSH);    // backgroung selected highlighted darken (for dark edge)

		m_crTN = ColorBrightness(+60, m_crBN);      // text normal
		m_crTH = ColorBrightness(+80, m_crTN);      // text normal lighten
		m_crTS = ThemeRGB(0xFF, 0xFF, 0xFF);        // text selected

		m_crBackground = ThemeRGB(10, 15, 20);      // background
		m_crDragImage = m_crBackground;             // drag'n'drop image
		m_crActiveItem = m_crTS;                    // active item
		m_crSelectedItem = ThemeRGB(165, 170, 175); // selected item
		m_crNormalItem = ThemeRGB(135, 140, 145);   // normal item

		m_crAvtiveItem3dRectTopLeft = ThemeRGB(80, 85, 90);
		m_crAvtiveItem3dRectBottomRight = ThemeRGB(30, 35, 40);

		// gradients
		int R, G, B;
		ThemeRGB(60, 65, 70, R, G, B);
		tvSelected[0] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
		ThemeRGB(50, 55, 60, R, G, B);
		tvSelected[1] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };

		ThemeRGB(10, 15, 20, R, G, B);
		tvNormal[0] = tvNormal[1] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
	}
	else {
		m_crBkBar = GetSysColor(COLOR_BTNFACE);   // background tab bar

		m_crBN = m_crBkBar;                       // backgroung normal
		m_crBNL = ColorBrightness(+15, m_crBN);   // backgroung normal lighten (for light edge)
		m_crBND = ColorBrightness(-15, m_crBN);   // backgroung normal darken (for dark edge)

		m_crBS = 0x00f1dacc;                      // backgroung selected background
		m_crBSL = 0xc56a31;                       // backgroung selected lighten (for light edge)
		m_crBSD = ColorBrightness(-15, m_crBS);   // backgroung selected darken (for dark edge)

		m_crBH = 0x00f1dacc;                      // backgroung highlighted background
		m_crBHL = ColorBrightness(+15, m_crBH);   // backgroung highlighted lighten (for light edge)
		m_crBHD = ColorBrightness(-30, m_crBH);   // backgroung highlighted darken (for dark edge)

		m_crBSH = ColorBrightness(+20, m_crBS);   // backgroung selected highlighted background
		m_crBSHL =ColorBrightness(-20, m_crBSL);  // backgroung selected highlighted lighten (for light edge)
		m_crBSHD = ColorBrightness(-0, m_crBSH);  // backgroung selected highlighted darken (for dark edge)

		m_crTN = RGB(50, 50, 50);                 // text normal
		m_crTH = RGB(0, 0, 0);                    // text normal lighten
		m_crTS = 0xff;                            // text selected

		m_crBackground = RGB(255, 255, 255);      // background
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
	if (GetCurTab().type == PL_EXPLORER) {
		m_list.SetFocus();

		int item = m_list.GetNextItem(-1, LVNI_SELECTED);
		if (item < 0) {
			item++;
		}

		POSITION pos = FindPos(item);
		if (pos) {
			const auto& pli = curPlayList.GetAt(pos);
			if (pli.m_bDirectory && pli.m_fi.Valid()) {
				CString path = pli.m_fi.GetPath();
				if (!path.IsEmpty()) {
					auto oldPath = path;

					const auto type = TGetPathType(path);
					switch (type) {
						case IT_DRIVE:
							break;
						case IT_PARENT:
							path.TrimRight(L"\\<");
							RemoveFileSpec(path);
							if (LastChar(path) == L':') {
								path = L".";
							}
							oldPath.TrimRight(L"\\<");
							break;
						case IT_FOLDER:
							path.TrimRight(L'>');
							break;
						default:
							return false;
					}
					AddSlash(path);

					curPlayList.RemoveAll();
					m_list.DeleteAllItems();

					TParseFolder(path);
					Refresh();

					if (type == IT_PARENT) {
						TSelectFolder(oldPath);
					} else {
						EnsureVisible(curPlayList.GetHeadPosition());
					}

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
	if (m_nCurPlaybackListId == m_tabs[m_nCurPlayListIndex].id) {
		m_pMainFrame->SendMessageW(WM_COMMAND, ID_FILE_CLOSEPLAYLIST);
	}
}

void CPlayerPlaylistBar::CopyToClipboard()
{
	std::vector<int> items;
	items.reserve(m_list.GetSelectedCount());
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos) {
		items.emplace_back(m_list.GetNextSelectedItem(pos));
	}

	if (items.size()) {
		CString str;
		for (const auto& item : items) {
			CPlaylistItem& pli = curPlayList.GetAt(FindPos(item));
			str = pli.m_fi.GetPath();
			for (const auto& fi : pli.m_auds) {
				str += L"\r\n" + fi.GetPath();
			}
		}

		CopyStringToClipboard(this->m_hWnd, str);
	}
}

void CPlayerPlaylistBar::PasteFromClipboard()
{
	std::list<CString> sl;
	if (m_pMainFrame->GetFromClipboard(sl)) {
		if (!(sl.size() == 1 && m_pMainFrame->OpenYoutubePlaylist(sl.front(), TRUE))) {
			Append(sl, sl.size() > 1, nullptr);
		}
	}
}
