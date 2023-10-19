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
#include "DSUtil/FileHandle.h"

typedef std::vector<Chapters> ChaptersList;


class CFileItem
{
	CString m_fpath;
	CString m_title;
	ChaptersList m_chapters;

	REFERENCE_TIME m_duration = 0;

public:
	CFileItem() = default;
	~CFileItem() = default;

	CFileItem(const CString& fpath)
		: m_fpath(fpath)
	{}
	CFileItem(const CString& fpath, const CString& title, const REFERENCE_TIME duration = 0)
		: m_fpath(fpath)
		, m_title(title)
		, m_duration(duration)
	{}
	CFileItem(const WCHAR* fpath)
		: m_fpath(fpath)
	{}

	const CFileItem& operator = (const CFileItem& fi) {
		m_fpath = fi.m_fpath;
		m_chapters.assign(fi.m_chapters.begin(), fi.m_chapters.end());

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
		return GetFileExt(m_fpath);
	};

	bool Valid() const {
		return m_fpath.GetLength() > 0;
	};

	REFERENCE_TIME GetDuration() const {
		return m_duration;
	};

	// Title
	void SetTitle(const CString& title) {
		m_title = title;
	}

	CString GetTitle() const {
		return m_title;
	};

	// Chapters
	void AddChapter(const Chapters& chapter) {
		m_chapters.emplace_back(chapter);
	}

	void ClearChapter() {
		m_chapters.clear();
	}

	size_t GetChapterCount() {
		return m_chapters.size();
	}

	void GetChapters(ChaptersList& chaplist) {
		chaplist = m_chapters;
	}

	void Clear() {
		m_fpath.Empty();
		m_title.Empty();
		m_chapters.clear();
		m_duration = 0;
	}
};

typedef std::list<CFileItem> CFileItemList;


class CExtraFileItem
{
	CStringW m_fpath;
	CStringW m_title;
	CStringA m_lang;

public:
	CExtraFileItem() = default;
	CExtraFileItem(const CStringW& fpath, const CStringW& title = L"", const CStringA& lang = "")
		: m_fpath(fpath)
		, m_title(title)
		, m_lang(lang)
	{}
	CExtraFileItem(const WCHAR* fpath, const WCHAR* title = L"", const CHAR* lang = "")
		: m_fpath(fpath)
		, m_title(title)
		, m_lang(lang)
	{}

	const CExtraFileItem& operator = (const CStringW& fpath) {
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

	bool IsEmpty() const {
		return m_fpath.IsEmpty();
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

typedef std::list<CExtraFileItem> CAudioItemList;
typedef std::list<CExtraFileItem> CSubtitleItemList;
