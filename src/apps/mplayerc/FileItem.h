/*
 * (C) 2023 see Authors.txt
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

#include "DSUtil/CUE.h"

typedef std::vector<Chapters> ChaptersList;


class CFileItem
{
	CString m_fpath;
	CString m_Title;
	ChaptersList m_ChaptersList;

	REFERENCE_TIME m_duration = 0;

public:
	CFileItem() = default;
	~CFileItem() = default;

	CFileItem(const CString& fpath)
		: m_fpath(fpath)
	{}
	CFileItem(const CString& fpath, const CString& title, const REFERENCE_TIME duration = 0)
		: m_fpath(fpath)
		, m_Title(title)
		, m_duration(duration)
	{}
	CFileItem(const WCHAR* fpath)
		: m_fpath(fpath)
	{}

	const CFileItem& operator = (const CFileItem& fi) {
		m_fpath = fi.m_fpath;
		m_ChaptersList.assign(fi.m_ChaptersList.begin(), fi.m_ChaptersList.end());

		return *this;
	}

	const CFileItem& operator = (const CString& str) {
		m_fpath = str;

		return *this;
	}

	operator CString() const {
		return m_fpath;
	}

	operator LPCWSTR() const {
		return m_fpath;
	}

	CString GetPath() const {
		return m_fpath;
	};

	CString GetExt() const {
		return m_fpath.Mid(m_fpath.ReverseFind(L'.') + 1).MakeLower();
	};

	REFERENCE_TIME GetDuration() const {
		return m_duration;
	};

	// Title
	void SetTitle(const CString& Title) {
		m_Title = Title;
	}

	CString GetTitle() const {
		return m_Title;
	};

	// Chapters
	void AddChapter(const Chapters& chap) {
		m_ChaptersList.emplace_back(chap);
	}

	void ClearChapter() {
		m_ChaptersList.clear();
	}

	size_t GetChapterCount() {
		return m_ChaptersList.size();
	}

	void GetChapters(ChaptersList& chaplist) {
		chaplist = m_ChaptersList;
	}
};

typedef std::list<CFileItem> CFileItemList;


class CSubtitleItem
{
	CStringW m_fpath;
	CStringW m_title;
	CStringA m_lang;

public:
	CSubtitleItem() = default;
	CSubtitleItem(const CStringW& fpath, const CStringW& title = L"", const CStringA& lang = "")
		: m_fpath(fpath)
		, m_title(title)
		, m_lang(lang)
	{}
	CSubtitleItem(const WCHAR* fpath, const WCHAR* title = L"", const CHAR* lang = "")
		: m_fpath(fpath)
		, m_title(title)
		, m_lang(lang)
	{}

	const CSubtitleItem& operator = (const CStringW& fpath) {
		m_fpath = fpath;

		return *this;
	}

	operator CStringW() const {
		return m_fpath;
	}

	operator LPCWSTR() const {
		return m_fpath;
	}

	void SetPath(const CStringW& fpath) {
		m_fpath = fpath;
	}

	CStringW GetPath() const {
		return m_fpath;
	};

	// Title
	void SetTitle(const CStringW& title) {
		m_title = title;
	}

	CStringW GetTitle() const {
		return m_title;
	};

	CStringA GetLang() const {
		return m_lang;
	};
};

typedef std::list<CSubtitleItem> CSubtitleItemList;
