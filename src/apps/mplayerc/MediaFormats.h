/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

#include <atlcoll.h>

enum filetype_t {
	TVideo = 0,
	TAudio,
	TPlaylist
};

class CMediaFormatCategory
{
protected:
	CString m_label, m_description, m_specreqnote;
	CAtlList<CString> m_exts, m_backupexts;
	filetype_t	m_filetype;

public:
	CMediaFormatCategory();
	CMediaFormatCategory(
		CString label, CString description, CAtlList<CString>& exts, filetype_t filetype = TVideo,
		CString specreqnote = L"");
	CMediaFormatCategory(
		CString label, CString description, CString exts, filetype_t filetype = TVideo,
		CString specreqnote = L"");
	virtual ~CMediaFormatCategory();

	void UpdateData(bool fSave);

	CMediaFormatCategory(const CMediaFormatCategory& mfc);
	CMediaFormatCategory& operator = (const CMediaFormatCategory& mfc);

	void RestoreDefaultExts();
	void SetExts(CAtlList<CString>& exts);
	void SetExts(CString exts);

	bool FindExt(CString ext) {
		return m_exts.Find(ext.TrimLeft('.').MakeLower()) != nullptr;
	}

	CString GetLabel() const {
		return m_label;
	}

	CString GetDescription() const {
		return m_description;
	}
	CString GetFilter();
	CString GetExts();
	CString GetExtsWithPeriod();
	CString GetBackupExtsWithPeriod();
	CString GetSpecReqNote() const {
		return m_specreqnote;
	}
	filetype_t GetFileType() const {
		return m_filetype;
	}
};

class CMediaFormats : public CAtlArray<CMediaFormatCategory>
{
public:
	CMediaFormats();
	virtual ~CMediaFormats();

	void UpdateData(bool fSave);

	bool FindExt(CString ext);
	bool FindAudioExt(CString ext);
	CMediaFormatCategory* FindMediaByExt(CString ext);

	void GetFilter(CString& filter, CAtlArray<CString>& mask);
	void GetAudioFilter(CString& filter, CAtlArray<CString>& mask);
};
