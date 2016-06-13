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
#include <atlutil.h>
#include <atlpath.h>
#include "MainFrm.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/Filehandle.h"
#include "SaveTextFileDialog.h"
#include "PlayerPlaylistBar.h"
#include "OpenDlg.h"

static CString MakePath(CString path)
{
	if (path.Find(L"://") >= 0 || YoutubeParser::CheckURL(path)) { // skip URLs
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
static bool ParseCUESheetFile(CString fn, CAtlList<CUETrack> &CUETrackList, CString& Title, CString& Performer)
{
	CTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn) || f.GetLength() > 32 * 1024) {
		return false;
	}

	Title.Empty();
	Performer.Empty();

	CString cueLine;

	CAtlArray<CString> sFilesArray;
	CAtlArray<CString> sTrackArray;
	while (f.ReadString(cueLine)) {
		CString cmd = GetCUECommand(cueLine);
		if (cmd == L"TRACK") {
			sTrackArray.Add(cueLine);
		} else if (cmd == L"FILE" && cueLine.Find(L" WAVE") > 0) {
			cueLine.Replace(L" WAVE", L"");
			cueLine.Trim(L" \"");
			sFilesArray.Add(cueLine);
		}
	};

	if (sTrackArray.IsEmpty() || sFilesArray.IsEmpty()) {
		return false;
	}

	BOOL bMultiple = sTrackArray.GetCount() == sFilesArray.GetCount();

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
				CString fName = bMultiple ? sFilesArray[idx++] : sFilesArray.GetCount() == 1 ? sFilesArray[0] : sFileName;
				CUETrackList.AddTail(CUETrack(fName, rt, trackNum, sTitle, sPerformer));
			}
			rt = _I64_MIN;
			sFileName = sFileName2;
			if (!Performer.IsEmpty()) {
				sPerformer = Performer;
			}

			TCHAR type[256] = { 0 };
			trackNum = 0;
			fAudioTrack = FALSE;
			if (2 == swscanf_s(cueLine, L"%d %s", &trackNum, type, _countof(type) - 1)) {
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
			int idx, mm, ss, ff;
			if (4 == swscanf_s(cueLine, L"%d %d:%d:%d", &idx, &mm, &ss, &ff) && fAudioTrack) {
				rt = MILLISECONDS_TO_100NS_UNITS((mm * 60 + ss) * 1000);
			}
		}
	}

	if (rt != _I64_MIN) {
		CString fName = bMultiple ? sFilesArray[idx] : sFilesArray.GetCount() == 1 ? sFilesArray[0] : sFileName2;
		CUETrackList.AddTail(CUETrack(fName, rt, trackNum, sTitle, sPerformer));
	}

	return CUETrackList.GetCount() > 0;
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
		m_fns.RemoveAll();
		m_fns.AddTailList(&pli.m_fns);
		m_subs.RemoveAll();
		m_subs.AddTailList(&pli.m_subs);
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

POSITION CPlaylistItem::FindFile(LPCTSTR path)
{
	POSITION pos = m_fns.GetHeadPosition();
	while (pos) {
		if (m_fns.GetAt(pos).GetName().CompareNoCase(path) == 0) {
			return pos;
		}
		m_fns.GetNext(pos);
	}
	return(NULL);
}

CString CPlaylistItem::GetLabel(int i)
{
	CString str;

	if (i == 0) {
		if (!m_label.IsEmpty()) {
			str = m_label;
		} else if (!m_fns.IsEmpty()) {
			str = GetFileOnly(m_fns.GetHead());
		}
	} else if (i == 1) {
		if (m_fInvalid) {
			return _T("Invalid");
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
static bool FindFileInList(CAtlList<T>& sl, CString fn)
{
	bool fFound = false;
	POSITION pos = sl.GetHeadPosition();
	while (pos && !fFound) {
		if (!CString(sl.GetNext(pos)).CompareNoCase(fn)) {
			fFound = true;
		}
	}
	return(fFound);
}

static void StringToPaths(const CString& curentdir, const CString& str, CAtlArray<CString>& paths)
{
	int pos = 0;
	do {
		CString s = str.Tokenize(_T(";"), pos);
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
			paths.Add(path);
			continue;
		}

		WIN32_FIND_DATA fd = { 0 };
		HANDLE hFind = FindFirstFile(path, &fd);
		if (hFind == INVALID_HANDLE_VALUE) {
			continue;
		} else {
			CPath parentdir = path + L"\\..";
			parentdir.Canonicalize();

			do {
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) {
					CString folder = parentdir + '\\' + fd.cFileName + '\\';

					size_t index = 0;
					size_t count = paths.GetCount();
					for (; index < count; index++) {
						if (folder.CompareNoCase(paths[index]) == 0) {
							break;
						}
					}
					if (index == count) {
						paths.Add(folder);
					}
				}
			} while (FindNextFile(hFind, &fd));
			FindClose(hFind);
		}
	} while (pos > 0);
}

void CPlaylistItem::AutoLoadFiles()
{
	if (m_fns.IsEmpty()) {
		return;
	}

	CString fn = m_fns.GetHead();
	if (fn.Find(_T("://")) >= 0) { // skip URLs
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
		CAtlArray<CString> paths;
		StringToPaths(curdir, s.strAudioPaths, paths);

		CAtlList<CString>* sl = &s.slAudioPathsAddons;
		POSITION pos = sl->GetHeadPosition();
		while (pos) {
			paths.Add(sl->GetNext(pos));
		}

		CMediaFormats& mf = s.m_Formats;
		if (!mf.FindAudioExt(ext)) {
			for (size_t i = 0; i < paths.GetCount(); i++) {
				WIN32_FIND_DATA fd = {0};

				HANDLE hFind;
				CAtlArray<CString> searchPattern;
				searchPattern.Add(paths[i] + name + _T("*.*"));
				if (!BDLabel.IsEmpty()) {
					searchPattern.Add(paths[i] + BDLabel + _T("*.*"));
				}
				for (size_t j = 0; j < searchPattern.GetCount(); j++) {
					hFind = FindFirstFile(searchPattern[j], &fd);

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
							CString fullpath = paths[i] + fd.cFileName;

							if (ext != ext2
									&& mf.FindAudioExt(ext2)
									&& !FindFileInList(m_fns, fullpath)
									&& s.GetFileEngine(fullpath) == DirectShow) {
								m_fns.AddTail(fullpath);
							}
						} while (FindNextFile(hFind, &fd));

						FindClose(hFind);
					}
				}
			}
		}
	}

	if (s.IsISRAutoLoadEnabled()) {
		CAtlArray<CString> paths;
		StringToPaths(curdir, s.strSubtitlePaths, paths);

		CAtlList<CString>* sl = &s.slSubtitlePathsAddons;
		POSITION pos = sl->GetHeadPosition();
		while (pos) {
			paths.Add(sl->GetNext(pos));
		}

		CAtlArray<Subtitle::SubFile> ret;
		Subtitle::GetSubFileNames(fn, paths, ret);

		for (size_t i = 0; i < ret.GetCount(); i++) {
			if (!FindFileInList(m_subs, ret[i].fn)) {
				m_subs.AddTail(ret[i].fn);
			}
		}

		if (!BDLabel.IsEmpty()) {
			ret.RemoveAll();
			Subtitle::GetSubFileNames(BDLabel, paths, ret);

			for (size_t i = 0; i < ret.GetCount(); i++) {
				if (!FindFileInList(m_subs, ret[i].fn)) {
					m_subs.AddTail(ret[i].fn);
				}
			}
		}
	}

	// cue-sheet file auto-load
	CString cuefn(fn);
	cuefn.Replace(ext, L"cue");
	if (::PathFileExists(cuefn)) {
		CString filter;
		CAtlArray<CString> mask;
		AfxGetAppSettings().m_Formats.GetAudioFilter(filter, mask);
		CAtlList<CString> sl;
		Explode(mask[0], sl, ';');

		BOOL bExists = false;
		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			CString _mask = sl.GetNext(pos);
			_mask.Delete(0, 2);
			_mask.MakeLower();
			if (_mask == ext) {
				bExists = TRUE;
				break;
			}
		}

		if (bExists) {
			CFileItem* fi = &m_fns.GetHead();
			if (!fi->GetChapterCount()) {

				CString Title, Performer;
				CAtlList<CUETrack> CUETrackList;
				if (ParseCUESheetFile(cuefn, CUETrackList, Title, Performer)) {
					CAtlList<CString> fileNames;
					POSITION pos = CUETrackList.GetHeadPosition();
					while (pos) {
						CUETrack cueTrack = CUETrackList.GetNext(pos);
						if (!fileNames.Find(cueTrack.m_fn)) {
							fileNames.AddTail(cueTrack.m_fn);
						}
					}
					if (fileNames.GetCount() == 1) {
						// support opening cue-sheet file with only a single file inside, even if its name differs from the current
						MakeCUETitle(m_label, Title, Performer);

						POSITION posCue = CUETrackList.GetHeadPosition();
						while (posCue) {
							CUETrack cueTrack = CUETrackList.GetNext(posCue);
							CString cueTrackTitle;
							MakeCUETitle(cueTrackTitle, cueTrack.m_Title, cueTrack.m_Performer, cueTrack.m_trackNum);
							fi->AddChapter(Chapters(cueTrackTitle, cueTrack.m_rt));
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
	: m_pos(NULL)
{
}

CPlaylist::~CPlaylist()
{
}

bool CPlaylist::RemoveAll()
{
	__super::RemoveAll();
	bool bWasPlaying = (m_pos != NULL);
	m_pos = NULL;
	return bWasPlaying;
}

bool CPlaylist::RemoveAt(POSITION pos)
{
	if (pos) {
		__super::RemoveAt(pos);
		if (m_pos == pos) {
			m_pos = NULL;
			return true;
		}
	}

	return false;
}

struct plsort_t {
	UINT n;
	POSITION pos;
};

static int compare(const void* arg1, const void* arg2)
{
	UINT a1 = ((plsort_t*)arg1)->n;
	UINT a2 = ((plsort_t*)arg2)->n;
	return a1 > a2 ? 1 : a1 < a2 ? -1 : 0;
}

struct plsort2_t {
	LPCTSTR str;
	POSITION pos;
};

int compare2(const void* arg1, const void* arg2)
{
	return StrCmpLogicalW(((plsort2_t*)arg1)->str, ((plsort2_t*)arg2)->str);
}

void CPlaylist::SortById()
{
	CAtlArray<plsort_t> a;
	a.SetCount(GetCount());
	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		a[i].n = GetAt(pos).m_id, a[i].pos = pos;
	}
	qsort(a.GetData(), a.GetCount(), sizeof(plsort_t), compare);
	for (size_t i = 0; i < a.GetCount(); i++) {
		AddTail(GetAt(a[i].pos));
		__super::RemoveAt(a[i].pos);
		if (m_pos == a[i].pos) {
			m_pos = GetTailPosition();
		}
	}
}

void CPlaylist::SortByName()
{
	CAtlArray<plsort2_t> a;
	a.SetCount(GetCount());
	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		CString fn = GetAt(pos).m_fns.GetHead();
		a[i].str = (LPCTSTR)fn + max(fn.ReverseFind('/'), fn.ReverseFind('\\')) + 1;
		a[i].pos = pos;
	}
	qsort(a.GetData(), a.GetCount(), sizeof(plsort2_t), compare2);
	for (size_t i = 0; i < a.GetCount(); i++) {
		AddTail(GetAt(a[i].pos));
		__super::RemoveAt(a[i].pos);
		if (m_pos == a[i].pos) {
			m_pos = GetTailPosition();
		}
	}
}

void CPlaylist::SortByPath()
{
	CAtlArray<plsort2_t> a;
	a.SetCount(GetCount());
	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		a[i].str = GetAt(pos).m_fns.GetHead(), a[i].pos = pos;
	}
	qsort(a.GetData(), a.GetCount(), sizeof(plsort2_t), compare2);
	for (size_t i = 0; i < a.GetCount(); i++) {
		AddTail(GetAt(a[i].pos));
		__super::RemoveAt(a[i].pos);
		if (m_pos == a[i].pos) {
			m_pos = GetTailPosition();
		}
	}
}

void CPlaylist::Randomize()
{
	CAtlArray<plsort_t> a;
	a.SetCount(GetCount());
	srand((unsigned int)time(NULL));
	POSITION pos = GetHeadPosition();
	for (int i = 0; pos; i++, GetNext(pos)) {
		a[i].n = rand(), a[i].pos = pos;
	}
	qsort(a.GetData(), a.GetCount(), sizeof(plsort_t), compare);
	CList<CPlaylistItem> pl;
	for (size_t i = 0; i < a.GetCount(); i++) {
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
	static CAtlArray<plsort_t> a;

	ASSERT(GetCount() > 2);
	// insert or remove items in playlist, or index out of bounds then recalculate
	if ((count != GetCount()) || (idx >= GetCount())) {
		a.RemoveAll();
		idx = 0;
		a.SetCount(count = GetCount());

		POSITION pos = GetHeadPosition();
		for (INT_PTR i = 0; pos; i++, GetNext(pos)) {
			a[i].pos = pos;    // initialize position array
		}

		//Use Fisher-Yates shuffle algorithm
		srand((unsigned)time(NULL));
		for (INT_PTR i=0; i<(count-1); i++) {
			INT_PTR r = i + (rand() % (count-i));
			POSITION temp = a[i].pos;
			a[i].pos = a[r].pos;
			a[r].pos = temp;
		}
	}

	return a[idx++].pos;
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

CPlayerPlaylistBar::CPlayerPlaylistBar()
	: m_list(0)
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
	if (!__super::Create(ResStr(IDS_PLAYLIST_CAPTION), pParentWnd, ID_VIEW_PLAYLIST, defDockBarID, _T("Playlist"))) {
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
	m_list.InsertColumn(COL_NAME, _T("Name"), LVCFMT_LEFT);
	m_list.InsertColumn(COL_TIME, _T("Time"), LVCFMT_RIGHT);

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
	lf.lfHeight = AfxGetMainFrame()->ScaleSystemToMonitorY(lf.lfHeight);

	m_font.DeleteObject();
	if (m_font.CreateFontIndirect(&lf)) {
		m_list.SetFont(&m_font);
	}

	CDC* pDC = m_list.GetDC();
	CFont* old = pDC->SelectObject(GetFont());
	m_nTimeColWidth = pDC->GetTextExtent(_T("000:00:00")).cx + AfxGetMainFrame()->ScaleX(5);
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
				CList<int> items;
				POSITION pos = m_list.GetFirstSelectedItemPosition();
				while (pos) {
					items.AddHead(m_list.GetNextSelectedItem(pos));
				}
				if (items.GetCount() == 1) {
					m_pl.SetPos(FindPos(items.GetHead()));
					CMainFrame*	pMainFrame = (CMainFrame*)AfxGetMainWnd();
					if (pMainFrame) {
						pMainFrame->OpenCurPlaylistItem();
						AfxGetMainWnd()->SetFocus();
					}

					return TRUE;
				}
			} else if (pMsg->wParam == 55 && GetKeyState(VK_CONTROL) < 0) {
				GetParentFrame()->ShowControlBar(this, FALSE, TRUE);

				return TRUE;
			}
		}

		if (pMsg->message == WM_CHAR) {
			TCHAR chr = static_cast<TCHAR>(pMsg->wParam);
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
	CString section = _T("ToolBars\\") + m_strSettingName;

	if (AfxGetApp()->GetProfileInt(section, _T("Visible"), FALSE)) {
		ShowWindow(SW_SHOW);
		m_bVisible = true;
	}

	__super::LoadState(pParent);
}

void CPlayerPlaylistBar::SaveState()
{
	__super::SaveState();

	CString section = _T("ToolBars\\") + m_strSettingName;

	AfxGetApp()->WriteProfileInt(section, _T("Visible"),
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
	CAtlList<CString> sl;
	sl.AddTail(fn);
	AddItem(sl, subs);
}

void CPlayerPlaylistBar::AddItem(CAtlList<CString>& fns, CSubtitleItemList* subs)
{
	CPlaylistItem pli;

	POSITION pos = fns.GetHeadPosition();
	while (pos) {
		CString fn = fns.GetNext(pos);
		if (!fn.Trim().IsEmpty()) {
			pli.m_fns.AddTail(MakePath(fn));
		}
	}

	if (subs) {
		POSITION pos = subs->GetHeadPosition();
		while (pos) {
			CString fn = subs->GetNext(pos);
			if (!fn.Trim().IsEmpty()) {
				pli.m_subs.AddTail(fn);
			}
		}
	}

	pli.AutoLoadFiles();
	if (pli.m_fns.GetCount() == 1) {
		YoutubeParser::YOUTUBE_FIELDS y_fields;
		if (YoutubeParser::Parse_URL(pli.m_fns.GetHead(), y_fields)) {
			pli.m_label    = y_fields.title;
			pli.m_duration = y_fields.duration;
		}
	}

	m_pl.AddTail(pli);
}

static bool SearchFiles(CString mask, CAtlList<CString>& sl)
{
	if (mask.Find(_T("://")) >= 0) {
		return false;
	}

	mask.Trim();
	sl.RemoveAll();

	CMediaFormats& mf = AfxGetAppSettings().m_Formats;

	WIN32_FILE_ATTRIBUTE_DATA fad;
	bool fFilterKnownExts = (GetFileAttributesEx(mask, GetFileExInfoStandard, &fad)
							 && (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
	if (fFilterKnownExts) {
		mask = CString(mask).TrimRight(_T("\\/")) + _T("\\*.*");
	}

	{
		CString dir = mask.Left(max(mask.ReverseFind('\\'), mask.ReverseFind('/'))+1);

		WIN32_FIND_DATA fd;
		HANDLE h = FindFirstFile(mask, &fd);
		if (h != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (CString(fd.cFileName).MakeUpper() == _T("VIDEO_TS")
						|| CString(fd.cFileName).MakeUpper() == _T("BDMV")) {
						SearchFiles(dir + fd.cFileName, sl);
					}
					continue;
				}

				CString fn = fd.cFileName;
				//CString ext = fn.Mid(fn.ReverseFind('.')+1).MakeLower();
				CString ext = fn.Mid(fn.ReverseFind('.')).MakeLower();
				CString path = dir + fd.cFileName;

				if (!fFilterKnownExts || mf.FindExt(ext)) {
					for (size_t i = 0; i < mf.GetCount(); i++) {
						CMediaFormatCategory& mfc = mf.GetAt(i);
						/* playlist files are skipped when playing the contents of an entire directory */
						if ((mfc.FindExt(ext)) && (mf[i].GetLabel().CompareNoCase(_T("pls")) != 0)) {
							sl.AddTail(path);
							break;
						}
					}
				}

			} while (FindNextFile(h, &fd));

			FindClose(h);

			if (sl.GetCount() == 0 && mask.Find(_T(":\\")) == 1) {
				if (CDROM_VideoCD == GetCDROMType(mask[0], sl)) {
					sl.RemoveAll();
				}
			}
		}
	}

	return(sl.GetCount() > 1
		   || sl.GetCount() == 1 && sl.GetHead().CompareNoCase(mask)
		   || sl.GetCount() == 0 && mask.FindOneOf(_T("?*")) >= 0);
}

void CPlayerPlaylistBar::ParsePlayList(CString fn, CSubtitleItemList* subs, bool bCheck/* = true*/)
{
	CAtlList<CString> sl;
	sl.AddTail(fn);
	ParsePlayList(sl, subs, bCheck);
}

void CPlayerPlaylistBar::ResolveLinkFiles(CAtlList<CString> &fns)
{
	// resolve .lnk files

	CComPtr<IShellLink> pSL;
	pSL.CoCreateInstance(CLSID_ShellLink);
	CComQIPtr<IPersistFile> pPF = pSL;

	if (pSL && pPF) {
		POSITION pos = fns.GetHeadPosition();
		while (pos) {
			CString& fn = fns.GetNext(pos);
			if (CPath(fn).GetExtension().MakeLower() == L".lnk") {
				TCHAR buff[MAX_PATH] = { 0 };
				if (SUCCEEDED(pPF->Load(fn, STGM_READ))
						&& SUCCEEDED(pSL->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI))
						&& SUCCEEDED(pSL->GetPath(buff, _countof(buff), NULL, 0))) {
					CString fnResolved(buff);
					if (!fnResolved.IsEmpty()) {
						fn = fnResolved;
					}
				}
			}
		}
	}
}

void CPlayerPlaylistBar::ParsePlayList(CAtlList<CString>& fns, CSubtitleItemList* subs, bool bCheck/* = true*/)
{
	if (fns.IsEmpty()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	ResolveLinkFiles(fns);

	if (bCheck && !YoutubeParser::CheckURL(fns.GetHead())) {
		CAtlList<CString> sl;
		if (SearchFiles(fns.GetHead(), sl)) {

			bool bDVD_BD = false;
			{
				POSITION pos = sl.GetHeadPosition();
				while (pos) {
					CString fn = sl.GetNext(pos);
					if (CString(fn).MakeUpper().Right(13) == _T("\\VIDEO_TS.IFO")
						|| CString(fn).MakeUpper().Right(11) == _T("\\INDEX.BDMV")) {
						fns.RemoveAll();
						fns.AddHead(fn);
						bDVD_BD = true;
						break;
					}
				}
			}

			if (!bDVD_BD) {
				if (sl.GetCount() > 1) {
					subs = NULL;
				}
				POSITION pos = sl.GetHeadPosition();
				while (pos) {
					ParsePlayList(sl.GetNext(pos), subs);
				}
				return;
			}
		}

		CAtlList<CString> redir;
		CString ct = GetContentType(fns.GetHead(), &redir);
		if (!redir.IsEmpty()) {
			POSITION pos = redir.GetHeadPosition();
			while (pos) {
				ParsePlayList(sl.GetNext(pos), subs);
			}
			return;
		}

		if (ct == L"application/x-mpc-playlist") {
			ParseMPCPlayList(fns.GetHead());
			return;
		} else if (ct == L"application/x-bdmv-playlist" && s.SrcFilters[SRC_MPEG]) {
			ParseBDMVPlayList(fns.GetHead());
			return;
		} else if (ct == L"audio/x-mpegurl" || ct == L"application/http-live-streaming-m3u") {
			ParseM3UPlayList(fns.GetHead());
			return;
		} else if (ct == L"application/x-cue-metadata") {
			if (ParseCUEPlayList(fns.GetHead())) {
				return;
			}
		}
	}

	AddItem(fns, subs);
}

static int s_int_comp(const void* i1, const void* i2)
{
	return (int)i1 - (int)i2;
}

static CString CombinePath(CPath p, CString fn)
{
	if (fn.Find(':') >= 0 || fn.Find(_T("\\")) == 0) {
		return fn;
	}

	p.Append(CPath(fn));
	CString out(p);
	if (out.Find(L"://")) {
		out.Replace('\\', '/');
	}
	return out;
}

bool CPlayerPlaylistBar::ParseBDMVPlayList(CString fn)
{
	CHdmvClipInfo	ClipInfo;
	CString			strPlaylistFile;
	CHdmvClipInfo::CPlaylist MainPlaylist;

	CPath Path(fn);
	Path.RemoveFileSpec();
	Path.RemoveFileSpec();

	if (SUCCEEDED(ClipInfo.FindMainMovie(Path + L"\\", strPlaylistFile, MainPlaylist, AfxGetMainFrame()->m_MPLSPlaylist))) {
		CAtlList<CString> strFiles;
		strFiles.AddHead(strPlaylistFile);
		Append(strFiles, MainPlaylist.GetCount() > 1, NULL);
	}

	return m_pl.GetCount() > 0;
}

bool CPlayerPlaylistBar::ParseMPCPlayList(CString fn)
{
	CString str;
	CAtlMap<int, CPlaylistItem> pli;
	CAtlArray<int> idx;

	CWebTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(fn) || !f.ReadString(str) || str != _T("MPCPLAYLIST") || f.GetLength() > MEGABYTE) {
		return false;
	}

	CPath base(fn);
	base.RemoveFileSpec();

	while (f.ReadString(str)) {
		CAtlList<CString> sl;
		Explode(str, sl, ',', 3);
		if (sl.GetCount() != 3) {
			continue;
		}

		if (int i = _ttoi(sl.RemoveHead())) {
			CString key = sl.RemoveHead();
			CString value = sl.RemoveHead();

			if (key == _T("type")) {
				pli[i].m_type = (CPlaylistItem::type_t)_ttol(value);
				idx.Add(i);
			} else if (key == _T("label")) {
				pli[i].m_label = value;
			} else if (key == _T("time")) {
				pli[i].m_duration = StringToReftime2(value);
			} else if (key == _T("filename")) {
				value = MakePath(CombinePath(base, value));
				pli[i].m_fns.AddTail(value);
			} else if (key == _T("subtitle")) {
				value = CombinePath(base, value);
				pli[i].m_subs.AddTail(value);
			} else if (key == _T("video")) {
				while (pli[i].m_fns.GetCount() < 2) {
					pli[i].m_fns.AddTail(_T(""));
				}
				pli[i].m_fns.GetHead() = value;
			} else if (key == _T("audio")) {
				while (pli[i].m_fns.GetCount() < 2) {
					pli[i].m_fns.AddTail(_T(""));
				}
				pli[i].m_fns.GetTail() = value;
			} else if (key == _T("vinput")) {
				pli[i].m_vinput = _ttol(value);
			} else if (key == _T("vchannel")) {
				pli[i].m_vchannel = _ttol(value);
			} else if (key == _T("ainput")) {
				pli[i].m_ainput = _ttol(value);
			} else if (key == _T("country")) {
				pli[i].m_country = _ttol(value);
			}
		}
	}

	qsort(idx.GetData(), idx.GetCount(), sizeof(int), s_int_comp);
	for (size_t i = 0; i < idx.GetCount(); i++) {
		m_pl.AddTail(pli[idx[i]]);
	}

	return pli.GetCount() > 0;
}

bool CPlayerPlaylistBar::SaveMPCPlayList(CString fn, CTextFile::enc e, bool fRemovePath)
{
	CTextFile f;
	if (!f.Save(fn, e)) {
		return false;
	}

	f.WriteString(_T("MPCPLAYLIST\n"));

	POSITION pos = m_pl.GetHeadPosition(), pos2;
	for (int i = 1; pos; i++) {
		CPlaylistItem& pli = m_pl.GetNext(pos);

		CString idx;
		idx.Format(_T("%d"), i);

		CString str;
		str.Format(_T("%d,type,%d"), i, pli.m_type);
		f.WriteString(str + _T("\n"));

		if (!pli.m_label.IsEmpty()) {
			f.WriteString(idx + _T(",label,") + pli.m_label + _T("\n"));
		}

		if (pli.m_duration > 0) {
			f.WriteString(idx + _T(",time,") + pli.GetLabel(1) + _T("\n"));
		}

		if (pli.m_type == CPlaylistItem::file) {
			pos2 = pli.m_fns.GetHeadPosition();
			while (pos2) {
				CString fn = pli.m_fns.GetNext(pos2);
				if (fRemovePath) {
					CPath p(fn);
					p.StripPath();
					fn = (LPCTSTR)p;
				}
				f.WriteString(idx + _T(",filename,") + fn + _T("\n"));
			}

			pos2 = pli.m_subs.GetHeadPosition();
			while (pos2) {
				CString fn = pli.m_subs.GetNext(pos2);
				if (fRemovePath) {
					CPath p(fn);
					p.StripPath();
					fn = (LPCTSTR)p;
				}
				f.WriteString(idx + _T(",subtitle,") + fn + _T("\n"));
			}
		} else if (pli.m_type == CPlaylistItem::device && pli.m_fns.GetCount() == 2) {
			f.WriteString(idx + _T(",video,") + pli.m_fns.GetHead().GetName() + _T("\n"));
			f.WriteString(idx + _T(",audio,") + pli.m_fns.GetTail().GetName() + _T("\n"));
			str.Format(_T("%d,vinput,%d"), i, pli.m_vinput);
			f.WriteString(str + _T("\n"));
			str.Format(_T("%d,vchannel,%d"), i, pli.m_vchannel);
			f.WriteString(str + _T("\n"));
			str.Format(_T("%d,ainput,%d"), i, pli.m_ainput);
			f.WriteString(str + _T("\n"));
			str.Format(_T("%d,country,%d"), i, pli.m_country);
			f.WriteString(str + _T("\n"));
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

	CPlaylistItem *pli = NULL;

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

				k = str.Find(_T(","));
				if (k > 0) {
					CString tmp = str.Left(k);
					int dur = 0;
					if (_stscanf_s(tmp, _T("%dx"), &dur) == 1) {
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

			pli->m_fns.AddTail(MakePath(CombinePath(base, str)));
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
	CAtlList<CUETrack> CUETrackList;
	if (!ParseCUESheetFile(fn, CUETrackList, Title, Performer)) {
		return false;
	}

	CAtlList<CString> fileNames;
	POSITION pos = CUETrackList.GetHeadPosition();
	while (pos) {
		CUETrack cueTrack = CUETrackList.GetNext(pos);
		if (!fileNames.Find(cueTrack.m_fn)) {
			fileNames.AddTail(cueTrack.m_fn);
		}
	}

	CPath base(fn);
	base.RemoveFileSpec();

	INT_PTR c = m_pl.GetCount();

	pos = fileNames.GetHeadPosition();
	while (pos) {
		CString fName = fileNames.GetNext(pos);
		CString fullPath = MakePath(CombinePath(base, fName));
		BOOL bExists = TRUE;
		if (!::PathFileExists(fullPath)) {
			CString ext = GetFileExt(fullPath);
			bExists = FALSE;

			CString filter;
			CAtlArray<CString> mask;
			AfxGetAppSettings().m_Formats.GetAudioFilter(filter, mask);
			CAtlList<CString> sl;
			Explode(mask[0], sl, ';');

			POSITION pos = sl.GetHeadPosition();
			while (pos) {
				CString _mask = sl.GetNext(pos);
				_mask.Delete(0, 1);

				CString newPath = fullPath;
				newPath.Replace(ext, _mask);

				if (::PathFileExists(newPath)) {
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
			POSITION posCue = CUETrackList.GetHeadPosition();
			BOOL bFirst = TRUE;
			while (posCue) {
				CUETrack cueTrack = CUETrackList.GetNext(posCue);
				if (cueTrack.m_fn == fName) {
					CString cueTrackTitle;
					MakeCUETitle(cueTrackTitle, cueTrack.m_Title, cueTrack.m_Performer, cueTrack.m_trackNum);
					fi.AddChapter(Chapters(cueTrackTitle, cueTrack.m_rt));

					if (bFirst && fileNames.GetCount() > 1) {
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

			pli.m_fns.AddHead(fi);

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

void CPlayerPlaylistBar::Open(CAtlList<CString>& fns, bool fMulti, CSubtitleItemList* subs/* = NULL*/, bool bCheck/* = true*/)
{
	ResolveLinkFiles(fns);
	Empty();
	Append(fns, fMulti, subs, bCheck);
}

void CPlayerPlaylistBar::Append(CAtlList<CString>& fns, bool fMulti, CSubtitleItemList* subs/* = NULL*/, bool bCheck/* = true*/)
{
	INT_PTR idx = -1;

	POSITION pos = m_pl.GetHeadPosition();
	while (pos) {
		idx++;
		m_pl.GetNext(pos);
	}

	if (fMulti) {
		ASSERT(subs == NULL || subs->GetCount() == 0);

		POSITION pos = fns.GetHeadPosition();
		while (pos) {
			ParsePlayList(fns.GetNext(pos), NULL, bCheck);
		}
	} else {
		ParsePlayList(fns, subs, bCheck);
	}

	Refresh();
	EnsureVisible(m_pl.FindIndex(idx + 1));
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

	pos = fis.GetHeadPosition();
	while (pos) {
		const CFileItem& item = fis.GetNext(pos);

		CPlaylistItem pli;
		pli.m_fns.AddHead(item.GetName());
		pli.m_label = item.GetTitle();
		m_pl.AddTail(pli);
	}

	Refresh();
	EnsureVisible(m_pl.FindIndex(idx + 1));
	SavePlaylist();

	UpdateList();
}

bool CPlayerPlaylistBar::Replace(CString filename, CAtlList<CString>& fns)
{
	if (filename.IsEmpty()) {
		return false;
	}

	CPlaylistItem pli;
	POSITION pos = fns.GetHeadPosition();
	while (pos) {
		CString fn = fns.GetNext(pos);
		if (!fn.Trim().IsEmpty()) {
			pli.m_fns.AddTail(MakePath(fn));
		}
	}
	pli.AutoLoadFiles();

	pos = m_pl.GetHeadPosition();
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
	pli.m_fns.AddTail(CString(vdn));
	pli.m_fns.AddTail(CString(adn));
	pli.m_vinput = vinput;
	pli.m_vchannel = vchannel;
	pli.m_ainput = ainput;
	CAtlList<CStringW> sl;
	CStringW vfn = GetFriendlyName(vdn);
	CStringW afn = GetFriendlyName(adn);
	if (!vfn.IsEmpty()) {
		sl.AddTail(vfn);
	}
	if (!afn.IsEmpty()) {
		sl.AddTail(afn);
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
	for (int i = 0; i < m_list.GetItemCount(); i++)
		if ((POSITION)m_list.GetItemData(i) == pos) {
			return(i);
		}
	return(-1);
}

POSITION CPlayerPlaylistBar::FindPos(int i)
{
	if (i < 0) {
		return(NULL);
	}
	return((POSITION)m_list.GetItemData(i));
}

int CPlayerPlaylistBar::GetCount()
{
	return(m_pl.GetCount()); // TODO: n - .fInvalid
}

int CPlayerPlaylistBar::GetSelIdx()
{
	return(FindItem(m_pl.GetPos()));
}

void CPlayerPlaylistBar::SetSelIdx(int i, bool bUpdatePos/* = FALSE*/)
{
	POSITION pos = FindPos(i);
	m_pl.SetPos(pos);
	if (bUpdatePos) {
		EnsureVisible(pos);
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
	if (!m_pl.GetPos()) {
		return NULL;
	}
	return &m_pl.GetAt(m_pl.GetPos());
}

CString CPlayerPlaylistBar::GetCurFileName()
{
	CString fn;
	CPlaylistItem* pli = GetCur();
	if (pli && !pli->m_fns.IsEmpty()) {
		fn = pli->m_fns.GetHead().GetName();
	}
	return(fn);
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
	if (pli == NULL) {
		return NULL;
	}

	pli->AutoLoadFiles();

	CString fn = pli->m_fns.GetHead().GetName().MakeLower();

	if (fn.Find(_T("video_ts.ifo")) >= 0
			|| fn.Find(_T(".ratdvd")) >= 0) {
		if (OpenDVDData* p = DNew OpenDVDData()) {
			p->path = pli->m_fns.GetHead().GetName();
			p->subs.AddTailList(&pli->m_subs);
			return p;
		}
	}

	if (pli->m_type == CPlaylistItem::device) {
		if (OpenDeviceData* p = DNew OpenDeviceData()) {
			POSITION pos = pli->m_fns.GetHeadPosition();
			for (int i = 0; i < _countof(p->DisplayName) && pos; i++) {
				p->DisplayName[i] = pli->m_fns.GetNext(pos).GetName();
			}
			p->vinput = pli->m_vinput;
			p->vchannel = pli->m_vchannel;
			p->ainput = pli->m_ainput;
			return p;
		}
	} else {
		if (OpenFileData* p = DNew OpenFileData()) {
			p->fns.AddTailList(&pli->m_fns);
			p->subs.AddTailList(&pli->m_subs);
			p->rtStart = rtStart;
			return p;
		}
	}

	return NULL;
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

		if (::PathFileExists(base)) {
			if (AfxGetAppSettings().bRememberPlaylistItems) {
				ParseMPCPlayList(base);
				Refresh();
				SelectFileInPlaylist(filename);
			} else {
				::DeleteFile(base);
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
			if (!::PathFileExists(base)) {
				::CreateDirectory(base, NULL);
			}

			SaveMPCPlayList(file, CTextFile::UTF8, false);
		} else if (::PathFileExists(file)) {
			::DeleteFile(file);
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

	CList<int> items;
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos) {
		items.AddHead(m_list.GetNextSelectedItem(pos));
	}

	if (pLVKeyDown->wVKey == VK_DELETE && items.GetCount() > 0) {
		pos = items.GetHeadPosition();
		while (pos) {
			int i = items.GetNext(pos);
			if (m_pl.RemoveAt(FindPos(i))) {
				AfxGetMainFrame()->SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			}
			m_list.DeleteItem(i);
		}

		m_list.SetItemState(-1, 0, LVIS_SELECTED);
		m_list.SetItemState(
			max(min(items.GetTail(), m_list.GetItemCount()-1), 0),
			LVIS_SELECTED, LVIS_SELECTED);

		ResizeListColumn();

		*pResult = TRUE;
	} else if (pLVKeyDown->wVKey == VK_SPACE && items.GetCount() == 1) {
		m_pl.SetPos(FindPos(items.GetHead()));
		AfxGetMainFrame()->OpenCurPlaylistItem();
		AfxGetMainWnd()->SetFocus();

		*pResult = TRUE;
	}
}

void CPlayerPlaylistBar::OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;

	if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0) {
		CMainFrame* pMainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

		POSITION pos = FindPos(lpnmlv->iItem);
		// If the file is already playing, don't try to restore a previously saved position
		if (pMainFrame->GetPlaybackMode() == PM_FILE && pos == m_pl.GetPos()) {
			const CPlaylistItem& pli = m_pl.GetAt(pos);
			AfxGetAppSettings().RemoveFile(pli.m_fns.GetHead());
		} else {
			m_pl.SetPos(pos);
		}

		m_list.Invalidate();
		pMainFrame->OpenCurPlaylistItem();
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
		if (IsWin7orLater()) { // under WinXP cause the hang
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
	lpMeasureItemStruct->itemHeight = AfxGetMainFrame()->ScaleY(m_itemHeight);
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

	CString time = !pli.m_fInvalid ? m_list.GetItemText(nItem, COL_TIME) : _T("Invalid");
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
	fmt.Format(_T("%%0%dd. %%s"), (int)log10(0.1 + m_pl.GetCount()) + 1);
	file.Format(fmt, nItem+1, m_list.GetItemText(nItem, COL_NAME));
	CSize filesize = pDC->GetTextExtent(file);
	while (3 + filesize.cx + 6 > timept.x && file.GetLength() > 3) {
		file = file.Left(file.GetLength() - 4) + _T("...");
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

void CPlayerPlaylistBar::DropFiles(CAtlList<CString>& slFiles)
{
	SetForegroundWindow();
	m_list.SetFocus();

	AfxGetMainFrame()->ParseDirs(slFiles);

	Append(slFiles, true);

}

void CPlayerPlaylistBar::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	ModifyStyle(WS_EX_ACCEPTFILES, 0);

	m_nDragIndex = ((LPNMLISTVIEW)pNMHDR)->iItem;

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
				SetTimer(1, 100, NULL);
			} else {
				KillTimer(1);
			}

			if (iOverItem >= iBottomItem && iBottomItem != (m_list.GetItemCount() - 1)) { // bottom of list
				SetTimer(2, 100, NULL);
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
		m_pDragImage = NULL;

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

	TCHAR szLabel[_MAX_PATH];
	LV_ITEM lvi;
	ZeroMemory(&lvi, sizeof(LV_ITEM));
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.stateMask = LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED;
	lvi.pszText = szLabel;
	lvi.iItem = m_nDragIndex;
	lvi.cchTextMax = _MAX_PATH;
	m_list.GetItem(&lvi);

	if (m_nDropIndex < 0) {
		m_nDropIndex = m_list.GetItemCount();
	}
	lvi.iItem = m_nDropIndex;
	m_list.InsertItem(&lvi);

	CHeaderCtrl* pHeader = (CHeaderCtrl*)m_list.GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();
	lvi.mask = LVIF_TEXT;
	lvi.iItem = m_nDropIndex;
	//INDEX OF DRAGGED ITEM WILL CHANGE IF ITEM IS DROPPED ABOVE ITSELF
	if (m_nDropIndex < m_nDragIndex) {
		m_nDragIndex++;
	}
	for (int col=1; col < nColumnCount; col++) {
		_tcscpy_s(lvi.pszText, _MAX_PATH, (LPCTSTR)(m_list.GetItemText(m_nDragIndex, col)));
		lvi.iSubItem = col;
		m_list.SetItem(&lvi);
	}

	m_list.DeleteItem(m_nDragIndex);

	CList<CPlaylistItem> tmp;
	UINT id = (UINT)-1;
	for (int i = 0; i < m_list.GetItemCount(); i++) {
		POSITION pos = (POSITION)m_list.GetItemData(i);
		CPlaylistItem& pli = m_pl.GetAt(pos);
		tmp.AddTail(pli);
		if (pos == m_pl.GetPos()) {
			id = pli.m_id;
		}
	}
	m_pl.RemoveAll();
	POSITION pos = tmp.GetHeadPosition();
	for (int i = 0; pos; i++) {
		CPlaylistItem& pli = tmp.GetNext(pos);
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
		POSITION pos = pli.m_fns.GetHeadPosition();
		while (pos) {
			strTipText += _T("\n") + pli.m_fns.GetNext(pos).GetName();
		}
		strTipText.Trim();

		if (pli.m_type == CPlaylistItem::device) {
			if (pli.m_vinput >= 0) {
				strTipText.AppendFormat(_T("\nVideo Input %d"), pli.m_vinput);
			}
			if (pli.m_vchannel >= 0) {
				strTipText.AppendFormat(_T("\nVideo Channel %d"), pli.m_vchannel);
			}
			if (pli.m_ainput >= 0) {
				strTipText.AppendFormat(_T("\nAudio Input %d"), pli.m_ainput);
			}
		}

		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);
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
	auto pMainFrm = AfxGetMainFrame();

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
			pMainFrm->OpenCurPlaylistItem();
			break;
		case M_ADD:
			{
				if (pMainFrm->GetPlaybackMode() == PM_CAPTURE) {
					pMainFrm->AddCurDevToPlaylist();
					m_pl.SetPos(m_pl.GetTailPosition());
				} else {
					CString filter;
					CAtlArray<CString> mask;
					s.m_Formats.GetFilter(filter, mask);

					DWORD dwFlags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_ENABLEINCLUDENOTIFY | OFN_NOCHANGEDIR;
					if (!s.fKeepHistory) {
						dwFlags |= OFN_DONTADDTORECENT;
					}

					COpenFileDlg fd(mask, true, NULL, NULL, dwFlags, filter, this);
					if (fd.DoModal() != IDOK) {
						return;
					}

					CAtlList<CString> fns;

					POSITION pos = fd.GetStartPosition();
					while (pos) {
						fns.AddTail(fd.GetNextPathName(pos));
					}

					Append(fns, fns.GetCount() > 1, NULL);
				}
			}
			break;
		case M_REMOVE:
			{
				CList<int> items;
				POSITION pos = m_list.GetFirstSelectedItemPosition();
				while (pos) {
					items.AddHead(m_list.GetNextSelectedItem(pos));
				}

				pos = items.GetHeadPosition();
				while (pos) {
					int i = items.GetNext(pos);
					if (m_pl.RemoveAt(FindPos(i))) {
						pMainFrm->SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					}
					m_list.DeleteItem(i);
				}

				m_list.SetItemState(-1, 0, LVIS_SELECTED);
				m_list.SetItemState(
					max(min(items.GetTail(), m_list.GetItemCount()-1), 0),
					LVIS_SELECTED, LVIS_SELECTED);

				ResizeListColumn();

				SavePlaylist();
			}
			break;
		case M_CLEAR:
			if (Empty()) {
				pMainFrm->SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
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

				CList<int> items;
				POSITION pos = m_list.GetFirstSelectedItemPosition();
				while (pos) {
					items.AddHead(m_list.GetNextSelectedItem(pos));
				}

				pos = items.GetHeadPosition();
				while (pos) {
					int i = items.GetNext(pos);
					CPlaylistItem &pli = m_pl.GetAt(FindPos(i));
					POSITION pos2 = pli.m_fns.GetHeadPosition();
					while (pos2) {
						str += _T("\r\n") + pli.m_fns.GetNext(pos2).GetName();
					}
				}

				str.Trim();

				if (HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (str.GetLength()+1)*sizeof(TCHAR))) {
					if (TCHAR* s = (TCHAR*)GlobalLock(h)) {
						_tcscpy_s(s, str.GetLength() + 1, str);
						GlobalUnlock(h);
						SetClipboardData(CF_UNICODETEXT, h);
					}
				}
				CloseClipboard();
			}
			break;
		case M_SAVEAS: {
			CSaveTextFileDialog fd(
				CTextFile::UTF8, NULL, NULL,
				_T("MPC-BE playlist (*.mpcpl)|*.mpcpl|Playlist (*.pls)|*.pls|Winamp playlist (*.m3u)|*.m3u|Windows Media playlist (*.asx)|*.asx||"),
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
					path.AddExtension(_T(".mpcpl"));
					break;
				case 2:
					path.AddExtension(_T(".pls"));
					break;
				case 3:
					path.AddExtension(_T(".m3u"));
					break;
				case 4:
					path.AddExtension(_T(".asx"));
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
					POSITION pos;

					pos = pli.m_fns.GetHeadPosition();
					while (pos && fRemovePath) {
						CString fn = pli.m_fns.GetNext(pos);

						CPath p(fn);
						p.RemoveFileSpec();
						if (base != (LPCTSTR)p) {
							fRemovePath = false;
						}
					}

					pos = pli.m_subs.GetHeadPosition();
					while (pos && fRemovePath) {
						CString fn = pli.m_subs.GetNext(pos);

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
				f.WriteString(_T("[playlist]\n"));
			} else if (idx == 3) {
				f.WriteString(_T("#EXTM3U\n"));
			} else if (idx == 4) {
				f.WriteString(_T("<ASX version = \"3.0\">\n"));
			}

			pos = m_pl.GetHeadPosition();
			CString str;
			int i;
			for (i = 0; pos; i++) {
				CPlaylistItem& pli = m_pl.GetNext(pos);

				if (pli.m_type != CPlaylistItem::file) {
					continue;
				}

				CString fn = pli.m_fns.GetHead();

				/*
							if (fRemovePath)
							{
								CPath p(path);
								p.StripPath();
								fn = (LPCTSTR)p;
							}
				*/

				if (idx == 3 && !pli.m_label.IsEmpty()) { // M3U
					str.Format(_T("#EXTINF:%s\n"), pli.m_label);
					f.WriteString(str);
				}

				switch (idx) {
					case 2:
						str.Format(_T("File%d=%s\n"), i+1, fn);
						break;
					case 3:
						str.Format(_T("%s\n"), fn);
						break;
					case 4:
						str.Format(_T("<Entry><Ref href = \"%s\"/></Entry>\n"), fn);
						break;
					default:
						break;
				}
				f.WriteString(str);
			}

			if (idx == 2) {
				str.Format(_T("NumberOfEntries=%d\n"), i);
				f.WriteString(str);
				f.WriteString(_T("Version=2\n"));
			} else if (idx == 4) {
				f.WriteString(_T("</ASX>\n"));
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
