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

#pragma once

#include "PlayerBar.h"
#include "PlayerListCtrl.h"
#include "../../DSUtil/CUE.h"
#include <vector>

typedef std::vector<Chapters> ChaptersList;
class CFileItem
{
	CString m_fn;
	CString m_Title;
	ChaptersList m_ChaptersList;

public:
	CFileItem() {};
	CFileItem(const CString& str) {
		m_fn = str;
	}
	CFileItem(const CString& strFn, const CString& strTitle) {
		m_fn = strFn;
		m_Title = strTitle;
	}
	CFileItem(const WCHAR* str) {
		m_fn = str;
	}

	const CFileItem& operator = (const CFileItem& fi) {
		m_fn = fi.m_fn;
		m_ChaptersList.assign(fi.m_ChaptersList.begin(), fi.m_ChaptersList.end());

		return *this;
	}

	const CFileItem& operator = (const CString& str) {
		m_fn = str;

		return *this;
	}

	operator CString() const {
		return m_fn;
	}

	operator LPCTSTR() const {
		return m_fn;
	}

	CString GetName() const {
		return m_fn;
	};

	// Title
	void SetTitle(CString Title) {
		m_Title = Title;
	}

	CString GetTitle() const {
		return m_Title;
	};

	// Chapters
	void AddChapter(Chapters chap) {
		m_ChaptersList.push_back(chap);
	}

	void ClearChapter() {
		m_ChaptersList.clear();
	}

	size_t GetChapterCount() {
		return m_ChaptersList.size();
	}

	void GetChapters(ChaptersList& chaplist) {
		chaplist.assign(m_ChaptersList.begin(), m_ChaptersList.end());
	}
};
typedef std::list<CFileItem> CFileItemList;

class CPlaylistItem
{
	static UINT m_globalid;

public:
	UINT m_id;
	CString m_label;

	CFileItemList m_fns;
	CSubtitleItemList m_subs;

	enum type_t {
		file,
		device
	} m_type;
	REFERENCE_TIME m_duration;
	int m_vinput, m_vchannel;
	int m_ainput;
	long m_country;

	bool m_fInvalid;

public:
	CPlaylistItem();
	virtual ~CPlaylistItem();

	CPlaylistItem(const CPlaylistItem& pli);
	CPlaylistItem& operator = (const CPlaylistItem& pli);

	bool FindFile(LPCTSTR path);
	void AutoLoadFiles();

	CString GetLabel(int i = 0);
};

class CPlaylist : public CList<CPlaylistItem>
{
protected:
	POSITION m_pos;

public:
	CPlaylist();
	virtual ~CPlaylist();

	bool RemoveAll();
	bool RemoveAt(POSITION pos);

	void SortByName();
	void SortByPath();
	void SortById();
	void Randomize();

	POSITION GetPos() const;
	void SetPos(POSITION pos);
	CPlaylistItem& GetNextWrap(POSITION& pos);
	CPlaylistItem& GetPrevWrap(POSITION& pos);

	POSITION Shuffle();
};

class OpenMediaData;
class CMainFrame;

class CPlayerPlaylistBar : public CPlayerBar
{
	DECLARE_DYNAMIC(CPlayerPlaylistBar)

private:
	enum {COL_NAME, COL_TIME};

	CMainFrame* m_pMainFrame;

	CImageList m_fakeImageList;
	CPlayerListCtrl m_list;

	int m_nTimeColWidth;
	void ResizeListColumn();

	void AddItem(CString fn, CSubtitleItemList* subs);
	void AddItem(std::list<CString>& fns, CSubtitleItemList* subs);
	void ParsePlayList(CString fn, CSubtitleItemList* subs, bool bCheck = true);
	void ParsePlayList(std::list<CString>& fns, CSubtitleItemList* subs, bool bCheck = true);
	void ResolveLinkFiles(std::list<CString> &fns);

	bool ParseMPCPlayList(CString fn);
	bool SaveMPCPlayList(CString fn, CTextFile::enc e, bool fRemovePath);

	bool ParseM3UPlayList(CString fn);
	bool ParseCUEPlayList(CString fn);

	void SetupList();
	void UpdateList();
	void EnsureVisible(POSITION pos, bool bMatchPos = true);
	int FindItem(POSITION pos);
	POSITION FindPos(int i);

	CImageList* m_pDragImage;
	BOOL m_bDragging;
	int m_nDragIndex;               // dragged item for which the mouse is dragged
	std::vector<int> m_DragIndexes; // all selected dragged items
	int m_nDropIndex;               // the position where dragged items will be moved

	CPoint m_ptDropPoint;

	void DropItemOnList();

	bool m_bHiddenDueToFullscreen;

	bool m_bVisible;

	int m_itemHeight = 0;

	CFont m_font;
	void ScaleFontInternal();

	int m_nSelected_idx = INT_MAX;

public:
	CPlayerPlaylistBar(CMainFrame* pMainFrame);
	virtual ~CPlayerPlaylistBar();

	BOOL Create(CWnd* pParentWnd, UINT defDockBarID);

	virtual void LoadState(CFrameWnd *pParent);
	virtual void SaveState();

	bool IsHiddenDueToFullscreen() const;
	void SetHiddenDueToFullscreen(bool bHidenDueToFullscreen);

	CPlaylist m_pl;

	int GetCount();
	int GetSelIdx();
	void SetSelIdx(int i, bool bUpdatePos = false);
	bool IsAtEnd();
	bool GetCur(CPlaylistItem& pli);
	CPlaylistItem* GetCur();
	CString GetCurFileName();
	bool SetNext();
	bool SetPrev();
	void SetFirstSelected();
	void SetFirst();
	void SetLast();
	void SetCurValid(bool fValid);
	void SetCurTime(REFERENCE_TIME rt);
	void SetCurLabel(CString label);

	void Refresh();
	bool Empty();

	void Open(CString fn);
	void Open(std::list<CString>& fns, bool fMulti, CSubtitleItemList* subs = nullptr, bool bCheck = true);
	void Append(CString fn);
	void Append(std::list<CString>& fns, bool fMulti, CSubtitleItemList* subs = nullptr, bool bCheck = true);
	void Append(CFileItemList& fis);
	bool Replace(CString filename, std::list<CString>& fns);

	void Open(CStringW vdn, CStringW adn, int vinput, int vchannel, int ainput);
	void Append(CStringW vdn, CStringW adn, int vinput, int vchannel, int ainput);

	OpenMediaData* GetCurOMD(REFERENCE_TIME rtStart = INVALID_TIME);

	void LoadPlaylist(CString filename);
	void SavePlaylist();

	bool SelectFileInPlaylist(CString filename);

	void DropFiles(std::list<CString>& slFiles);

	bool IsPlaylistVisible() const { return IsWindowVisible() || m_bVisible; }

	void ScaleFont();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnLvnKeydownList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult);
	void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg BOOL OnPlayPlay(UINT nID);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnLvnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);

	virtual void Invalidate() { m_list.Invalidate(); }
};
