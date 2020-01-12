/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2020 see Authors.txt
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

enum filetype_t {
	TVideo = 0,
	TAudio,
	TPlaylist,
	TScript
};

class CMediaFormatCategory
{
protected:
	CString m_label, m_description, m_specreqnote;
	std::list<CString> m_exts, m_backupexts;
	filetype_t m_filetype;

public:
	CMediaFormatCategory();
	CMediaFormatCategory(
		const CString& label, const CString& description, std::list<CString>& exts, const filetype_t& filetype = TVideo,
		const CString& specreqnote = L"");
	CMediaFormatCategory(
		const CString& label, const CString& description, const CString& exts, const filetype_t& filetype = TVideo,
		const CString& specreqnote = L"");
	virtual ~CMediaFormatCategory();

	void UpdateData(const bool& bSave);

	CMediaFormatCategory(const CMediaFormatCategory& mfc);
	CMediaFormatCategory& operator = (const CMediaFormatCategory& mfc);

	void RestoreDefaultExts();
	void SetExts(std::list<CString>& exts);
	void SetExts(const CString& exts);

	bool FindExt(CString ext) const {
		return std::find(m_exts.cbegin(), m_exts.cend(), ext.TrimLeft('.').MakeLower()) != m_exts.cend();
	}

	CString GetLabel() const {
		return m_label;
	}

	CString GetDescription() const {
		return m_description;
	}
	CString GetFilter() const;
	CString GetExts() const;
	CString GetExtsWithPeriod() const;
	CString GetBackupExts() const;
	CString GetSpecReqNote() const {
		return m_specreqnote;
	}
	filetype_t GetFileType() const {
		return m_filetype;
	}
};

class CMediaFormats : public std::vector<CMediaFormatCategory>
{
public:
	CMediaFormats();
	virtual ~CMediaFormats();

	void UpdateData(const bool& bSave);

	bool FindExt(const CString& ext);
	bool FindAudioExt(const CString& ext);
	CMediaFormatCategory* FindMediaByExt(CString ext);

	void GetFilter(CString& filter, std::vector<CString>& mask);
	void GetAudioFilter(CString& filter, std::vector<CString>& mask);
};
