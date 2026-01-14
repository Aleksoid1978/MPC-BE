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

#pragma once

#include "PlayerBar.h"
#include "PlayerListCtrl.h"
#include "controls/ColorEdit.h"
#include "FileItem.h"

class CPlaylistItem
{
	static inline UINT m_globalid = 0;

public:
	UINT m_id;
	CString m_label;
	bool m_autolabel = false;

	CFileItem         m_fi;
	CAudioItemList    m_auds;
	CSubtitleItemList m_subs;

	enum type_t {
		file,
		device
	} m_type = file;
	REFERENCE_TIME m_duration = 0;
	int m_vinput   = -1;
	int m_vchannel = -1;
	int m_ainput   = -1;
	long m_country = 0;

	bool m_bInvalid   = false;
	bool m_bDirectory = false;

public:
	CPlaylistItem();
	~CPlaylistItem() = default;

	CPlaylistItem& operator = (const CPlaylistItem& pli);

	bool FindFile(LPCWSTR path);
	bool FindFolder(LPCWSTR path) const;
	void AutoLoadFiles();

	CString GetLabel(int i = 0) const;

	const bool MustBeSkipped() const {
		if (m_bInvalid) {
			return AfxGetAppSettings().bPlaylistSkipInvalid;
		}

		return false;
	}
};

class CPlaylist : public CList<CPlaylistItem>
{
protected:
	POSITION m_pos = nullptr;

public:
	CPlaylist() = default;
	~CPlaylist() = default;

	POSITION Append(CPlaylistItem& item, const bool bParseDuration);

	bool RemoveAll();
	bool RemoveAt(POSITION pos);

	void SortByName();
	void SortByPath();
	void SortById();
	void Randomize();
	void ReverseSort();
	POSITION GetPos() const;
	void SetPos(POSITION pos);
	CPlaylistItem& GetNextWrap(POSITION& pos);
	CPlaylistItem& GetPrevWrap(POSITION& pos);

	void CalcCountFiles();
	INT_PTR GetCountInternal();
	POSITION Shuffle();

	INT_PTR m_nFilesCount = -1;

	int m_nSelected_idx = INT_MAX;
	int m_nFocused_idx = 0;

	int m_nSelectedAudioTrack = -1;
	int m_nSelectedSubtitleTrack = -1;
};

class OpenMediaData;
class CMainFrame;

class CPlayerPlaylistBar : public CPlayerBar
{
	DECLARE_DYNAMIC(CPlayerPlaylistBar)

private:
	enum {COL_NAME, COL_TIME};

	CMainFrame* m_pMainFrame = nullptr;

	CImageList m_fakeImageList;
	CPlayerListCtrl m_list;

	int m_nTimeColWidth = 0;
	void ResizeListColumn();

	void AddItem(std::list<CString>& fns, CSubtitleItemList* subs);
	void ParsePlayList(CString fn, CSubtitleItemList* subs, bool bCheck = true);
	void ParsePlayList(std::list<CString>& fns, CSubtitleItemList* subs, bool bCheck = true);
	void ResolveLinkFiles(std::list<CString> &fns);

	bool ParseMPCPlayList(const CString& fn);
	bool SaveMPCPlayList(const CString& fn, const UINT e, const bool bRemovePath);

	bool ParseM3UPlayList(CString fn);
	bool ParseCUEPlayList(CString fn);
	bool ParseASXPlayList(CString fn);

	void SetupList();
	void UpdateList();
	void EnsureVisible(POSITION pos, bool bMatchPos = true);
	int FindItem(POSITION pos);
	POSITION FindPos(int i);

	CImageList* m_pDragImage;
	BOOL m_bDragging = FALSE;
	int m_nDragIndex;               // dragged item for which the mouse is dragged
	std::vector<int> m_DragIndexes; // all selected dragged items
	int m_nDropIndex;               // the position where dragged items will be moved

	CPoint m_ptDropPoint;

	void DropItemOnList();

	bool m_bHiddenDueToFullscreen = false;

	bool m_bVisible = false;

	CFont m_font;
	void ScaleFontInternal();

	bool m_bSingleElement = false;

	int m_tab_idx = -1;
	int m_button_idx = -1;

#define WIDTH_TABBUTTON 20
	enum PLAYLIST_TYPE : int {
		PL_BASIC,
		PL_EXPLORER
	};

	enum ItemType : int {
		IT_NONE = -1,
		IT_PARENT = 0,
		IT_DRIVE,
		IT_FOLDER,
		IT_FILE,
		IT_URL,
		IT_PIPE,
	};

	enum SORT : BYTE {
		NAME,
		DATE,
		SIZE,
		DATE_CREATED,
	};

	struct file_data_t {
		CString name;
		ULARGE_INTEGER time;
		ULARGE_INTEGER time_created;
		ULARGE_INTEGER size;
	};

	struct tab_t {
		PLAYLIST_TYPE type = PL_BASIC; // playlist type
		CString name;                  // playlist label
		CString mpcpl_fn;              // playlist file name
		CRect r;                       // layout
		unsigned id = 0;               // unique id
		WORD sort = 0;                 // sorted flag

		std::vector<file_data_t> directory;
		std::vector<file_data_t> files;
	};
	std::vector<tab_t> m_tabs;
	inline tab_t& GetCurTab() {
		return m_tabs[m_nCurPlayListIndex];
	}

	enum {
		LEFT,
		RIGHT,
		MENU
	};
	struct tab_button_t {
		CString name;
		CRect r;
		bool bVisible = false;
	};
	tab_button_t m_tab_buttons[3] = {
		{L"<"},
		{L">"},
		{L"::", {}, true}
	};

	struct HICONDeleter {
		void operator()(HICON icon) const noexcept {
			if (icon) {
				DestroyIcon(icon);
			}
		}
	};
	using HICONPtr = std::unique_ptr<std::remove_pointer<HICON>::type, HICONDeleter>;

	std::map<CString, HICONPtr> m_icons;
	std::map<CString, HICONPtr> m_icons_large;

	void TDrawBar();
	void TDrawSearchBar();
	void TCalcLayout();
	void TCalcREdit();
	void TIndexHighighted();
	void TTokenizer(const CString& strFields, LPCWSTR strDelimiters, std::vector<CString>& arFields);
	void TParseFolder(const CString& path);
	void TFillPlaylist(const bool bFirst = false);
	void TGetSettings();
	void TSaveSettings();
	void TSelectTab();
	void TOnMenu(bool bUnderCurcor = false);
	void TSetOffset(bool toRight = false);
	void TEnsureVisible(int idx);

	bool bTMenuPopup = false;

	CRect m_rcTPage;
	size_t m_cntOffset = 0;

	int m_nSearchBarHeight = 20;
	CColorEdit m_REdit;

	int TGetFirstVisible();
	int TGetOffset();
	int TGetPathType(const CString& path) const;
	bool TNavigate();
	bool TSelectFolder(const CString& path);
	int TGetFocusedElement() const;
	int m_iTFontSize = 0;

	unsigned m_nCurPlaybackListId = 0;
	void CloseMedia() const;

	COLORREF m_crBkBar;

	COLORREF m_crBN;
	COLORREF m_crBNL;
	COLORREF m_crBND;

	COLORREF m_crBS;
	COLORREF m_crBSL;
	COLORREF m_crBSD;

	COLORREF m_crBH;
	COLORREF m_crBHL;
	COLORREF m_crBHD;

	COLORREF m_crBSH;
	COLORREF m_crBSHL;
	COLORREF m_crBSHD;

	COLORREF m_crTN;
	COLORREF m_crTH;
	COLORREF m_crTS;

	COLORREF m_crBackground;
	COLORREF m_crDragImage;
	COLORREF m_crActiveItem;
	COLORREF m_crSelectedItem;
	COLORREF m_crNormalItem;

	COLORREF m_crAvtiveItem3dRectTopLeft;
	COLORREF m_crAvtiveItem3dRectBottomRight;

	TRIVERTEX tvSelected[2];
	TRIVERTEX tvNormal[2];

public:
	CPlayerPlaylistBar(CMainFrame* pMainFrame);
	~CPlayerPlaylistBar();

	BOOL Create(CWnd* pParentWnd, UINT defDockBarID);

	virtual void ReloadTranslatableResources();

	virtual void LoadState(CFrameWnd *pParent);
	virtual void SaveState();

	bool IsHiddenDueToFullscreen() const;
	void SetHiddenDueToFullscreen(bool bHidenDueToFullscreen);

	std::vector<CPlaylist*> m_pls;
	size_t m_nCurPlayListIndex = 0;
	CPlaylist& GetCurPlayList() const {
		return *m_pls[m_nCurPlayListIndex];
	}
#define curPlayList GetCurPlayList()

	void SetColor();

	void TDeleteAllPlaylists();
	void TSaveAllPlaylists();

	int GetCount(const bool bOnlyFiles = false);
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
	void SetCurValid(const bool bValid);
	void SetCurTime(REFERENCE_TIME rt);
	void SetCurLabel(const CString& label);
	void Randomize();

	void Refresh();
	bool Empty();
	void RemoveMissingFiles();
	void Remove(const std::vector<int>& items, const bool bDelete);

	void Open(const CString& fn);
	void Open(std::list<CString>& fns, const bool bMulti, CSubtitleItemList* subs = nullptr, bool bCheck = true);
	void Append(const CString& fn);
	void Append(std::list<CString>& fns, const bool bMulti, CSubtitleItemList* subs = nullptr, bool bCheck = true);
	void Append(const CFileItemList& fis);

	void Open(const CStringW& vdn, const CStringW& adn, const int vinput, const int vchannel, const int ainput);
	void Append(const CStringW& vdn, const CStringW& adn, const int vinput, const int vchannel, const int ainput);

	OpenMediaData* GetCurOMD(REFERENCE_TIME rtStart = INVALID_TIME);

	void LoadPlaylist(const CString& filename);
	void SavePlaylist();

	bool SelectFileInPlaylist(const CString& filename);

	void DropFiles(std::list<CString>& slFiles);

	bool IsPlaylistVisible() const { return IsWindowVisible() || m_bVisible; }
	CSize GetMinSize() const { return m_szFixedFloat; }

	void ScaleFont();

	bool IsShuffle() const;

	void SelectDefaultPlaylist();

	bool CheckAudioInCurrent(const CString& fn);
	void AddAudioToCurrent(const CString& fn);
	void AddSubtitleToCurrent(const CString& fn);

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	virtual COLORREF ColorThemeRGB(const int iR, const int iG, const int iB) const;
	virtual int ScaleX(const int x) const;
	virtual int ScaleY(const int y) const;

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnLvnKeydownList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg BOOL OnPlayPlay(UINT nID);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnLvnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	virtual void Invalidate() { m_list.Invalidate(); }

	void PasteFromClipboard();
protected:
	void CopyToClipboard();

	bool m_bDrawDragImage = false;
	bool m_bEdit = false;
};
