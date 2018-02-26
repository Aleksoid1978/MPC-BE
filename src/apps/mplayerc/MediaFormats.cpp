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
#include "MainFrm.h"
#include "MediaFormats.h"
#include "OpenImage.h"

//
// CMediaFormatCategory
//

CMediaFormatCategory::CMediaFormatCategory()
	: m_filetype(TVideo)
{
}

CMediaFormatCategory::CMediaFormatCategory(const CString& label, const CString& description, std::list<CString>& exts, const filetype_t& filetype, const CString& specreqnote)
	: m_label(label)
	, m_description(description)
	, m_specreqnote(specreqnote)
	, m_filetype(filetype)
	, m_exts(exts)
	, m_backupexts(exts)
{
}

CMediaFormatCategory::CMediaFormatCategory(const CString& label, const CString& description, const CString& exts, const filetype_t& filetype, const CString& specreqnote)
	: m_label(label)
	, m_description(description)
	, m_specreqnote(specreqnote)
	, m_filetype(filetype)
{
	ExplodeMin(exts, m_exts, ' ');

	for (auto& ext : m_exts) {
		ext.TrimLeft('.');
	}

	m_backupexts = m_exts;
}

CMediaFormatCategory::~CMediaFormatCategory()
{
}

void CMediaFormatCategory::UpdateData(const bool& bSave)
{
	if (bSave) {
		AfxGetMyApp()->WriteProfileString(IDS_R_FILEFORMATS, m_label, GetExts());
	} else {
		SetExts(AfxGetMyApp()->GetProfileString(IDS_R_FILEFORMATS, m_label, GetExts()));
	}
}

CMediaFormatCategory::CMediaFormatCategory(const CMediaFormatCategory& mfc)
{
	*this = mfc;
}

CMediaFormatCategory& CMediaFormatCategory::operator = (const CMediaFormatCategory& mfc)
{
	if (this != &mfc) {
		m_label       = mfc.m_label;
		m_description = mfc.m_description;
		m_specreqnote = mfc.m_specreqnote;
		m_filetype    = mfc.m_filetype;
		m_exts        = mfc.m_exts;
		m_backupexts  = mfc.m_backupexts;
	}

	return *this;
}

void CMediaFormatCategory::RestoreDefaultExts()
{
	m_exts = m_backupexts;
}

void CMediaFormatCategory::SetExts(std::list<CString>& exts)
{
	m_exts = exts;
}

void CMediaFormatCategory::SetExts(const CString& exts)
{
	m_exts.clear();
	ExplodeMin(exts, m_exts, ' ');

	for (auto it = m_exts.begin(); it != m_exts.end();) {
		auto cur = it++;
		CString& ext = *cur;

		if (ext[0] == '\\') {
			m_exts.erase(cur);
		} else {
			ext.TrimLeft('.');
		}
	}
}

CString CMediaFormatCategory::GetFilter() const
{
	CString filter;

	for (const auto& ext : m_exts) {
		filter += L"*." + ext + L";";
	}

	filter.TrimRight(';'); // cheap...
	return filter;
}

CString CMediaFormatCategory::GetExts() const
{
	return Implode(m_exts, ' ');
}

CString CMediaFormatCategory::GetExtsWithPeriod() const
{
	CString exts;

	for (const auto& ext : m_exts) {
		exts += L"." + ext + L" ";
	}

	exts.TrimRight(' ');
	return exts;
}

CString CMediaFormatCategory::GetBackupExtsWithPeriod() const
{
	CString exts;

	for (const auto& backupext : m_backupexts) {
		exts += L"." + backupext + L" ";
	}

	exts.TrimRight(' ');

	return exts;
}

//
// CMediaFormats
//

CMediaFormats::CMediaFormats()
{
}

CMediaFormats::~CMediaFormats()
{
}

void CMediaFormats::UpdateData(const bool& bSave)
{
	if (bSave) {
		AfxGetMyApp()->WriteProfileString(IDS_R_FILEFORMATS, nullptr, nullptr);
	} else {
		clear();
		reserve(51);

#define ADDFMT(f) emplace_back(CMediaFormatCategory##f)
		// video files
		ADDFMT((L"avi",         ResStr(IDS_MFMT_AVI),         L"avi divx"));
		ADDFMT((L"mpeg",        ResStr(IDS_MFMT_MPEG),        L"mpg mpeg mpe m1v m2v mpv2 mp2v pva evo m2p sfd"));
		ADDFMT((L"mpegts",      ResStr(IDS_MFMT_MPEGTS),      L"ts tp trp m2t m2ts mts rec ssif"));
		ADDFMT((L"dvdvideo",    ResStr(IDS_MFMT_DVDVIDEO),    L"vob ifo"));
		ADDFMT((L"mkv",         ResStr(IDS_MFMT_MKV),         L"mkv mk3d"));
		ADDFMT((L"webm",        ResStr(IDS_MFMT_WEBM),        L"webm"));
		ADDFMT((L"mp4",         ResStr(IDS_MFMT_MP4),         L"mp4 m4v mp4v mpv4 hdmov ismv"));
		ADDFMT((L"mov",         ResStr(IDS_MFMT_MOV),         L"mov"));
		ADDFMT((L"3gp",         ResStr(IDS_MFMT_3GP),         L"3gp 3gpp 3ga"));
		ADDFMT((L"3g2",         ResStr(IDS_MFMT_3G2),         L"3g2 3gp2"));
		ADDFMT((L"flv",         ResStr(IDS_MFMT_FLV),         L"flv f4v"));
		ADDFMT((L"ogm",         ResStr(IDS_MFMT_OGM),         L"ogm ogv"));
		ADDFMT((L"rm",          ResStr(IDS_MFMT_RM),          L"rm ram rmm rmvb"));
		ADDFMT((L"roq",         ResStr(IDS_MFMT_ROQ),         L"roq"));
		ADDFMT((L"wmv",         ResStr(IDS_MFMT_WMV),         L"wmv wmp wm asf"));
//		ADDFMT((L"videocd",     ResStr(IDS_MFMT_VIDEOCD),     L"dat")); // "dat" extension is no longer supported
		ADDFMT((L"bink",        ResStr(IDS_MFMT_BINK),        L"smk bik", TVideo, L"smackw32/binkw32.dll in dll path"));
		ADDFMT((L"flic",        ResStr(IDS_MFMT_FLIC),        L"fli flc flic"));
		ADDFMT((L"dsm",         ResStr(IDS_MFMT_DSM),         L"dsm dsv dsa dss"));
		ADDFMT((L"swf",         ResStr(IDS_MFMT_SWF),         L"swf", TVideo, L"ShockWave ActiveX control"));
		ADDFMT((L"other",       ResStr(IDS_MFMT_OTHER),       L"amv wtv dvr-ms mxf ivf"));
		ADDFMT((L"rawvideo",    ResStr(IDS_MFMT_RAW_VIDEO),   L"y4m h264 264 vc1 h265 265 hm10 hevc"));
		// audio files
		ADDFMT((L"ac3dts",      ResStr(IDS_MFMT_AC3),         L"ac3 dts dtshd", TAudio));
		ADDFMT((L"aiff",        ResStr(IDS_MFMT_AIFF),        L"aif aifc aiff", TAudio));
		ADDFMT((L"alac",        ResStr(IDS_MFMT_ALAC),        L"alac", TAudio));
		ADDFMT((L"amr",         ResStr(IDS_MFMT_AMR),         L"amr awb", TAudio));
		ADDFMT((L"ape",         ResStr(IDS_MFMT_APE),         L"ape apl", TAudio));
		ADDFMT((L"au",          ResStr(IDS_MFMT_AU),          L"au snd", TAudio));
		ADDFMT((L"audiocd",     ResStr(IDS_MFMT_CDA),         L"cda", TAudio));
		ADDFMT((L"dvdaudio",    ResStr(IDS_MFMT_DVDAUDIO),    L"aob", TAudio));
		ADDFMT((L"dsd",         ResStr(IDS_MFMT_DSD),         L"dsf dff", TAudio));
		ADDFMT((L"flac",        ResStr(IDS_MFMT_FLAC),        L"flac", TAudio));
		ADDFMT((L"m4a",         ResStr(IDS_MFMT_M4A),         L"m4a m4b aac", TAudio));
		ADDFMT((L"midi",        ResStr(IDS_MFMT_MIDI),        L"mid midi rmi", TAudio));
		ADDFMT((L"mka",         ResStr(IDS_MFMT_MKA),         L"mka", TAudio));
		ADDFMT((L"mlp",         ResStr(IDS_MFMT_MLP),         L"mlp", TAudio));
		ADDFMT((L"mp3",         ResStr(IDS_MFMT_MP3),         L"mp3", TAudio));
		ADDFMT((L"mpa",         ResStr(IDS_MFMT_MPA),         L"mpa mp2 m1a m2a", TAudio));
		ADDFMT((L"mpc",         ResStr(IDS_MFMT_MPC),         L"mpc", TAudio));
		ADDFMT((L"ofr",         ResStr(IDS_MFMT_OFR),         L"ofr ofs", TAudio));
		ADDFMT((L"ogg",         ResStr(IDS_MFMT_OGG),         L"ogg oga", TAudio));
		ADDFMT((L"ra",          ResStr(IDS_MFMT_RA),          L"ra", TAudio));
		ADDFMT((L"tak",         ResStr(IDS_MFMT_TAK),         L"tak", TAudio));
		ADDFMT((L"tta",         ResStr(IDS_MFMT_TTA),         L"tta", TAudio));
		ADDFMT((L"wav",         ResStr(IDS_MFMT_WAV),         L"wav w64", TAudio));
		ADDFMT((L"wma",         ResStr(IDS_MFMT_WMA),         L"wma", TAudio));
		ADDFMT((L"wavpack",     ResStr(IDS_MFMT_WAVPACK),     L"wv", TAudio));
		ADDFMT((L"opus",        ResStr(IDS_MFMT_OPUS),        L"opus", TAudio));
		ADDFMT((L"speex",       ResStr(IDS_MFMT_SPEEX),       L"spx", TAudio));
		// playlists
		ADDFMT((L"pls",         ResStr(IDS_MFMT_PLS),         L"asx m3u m3u8 pls wpl mpcpl xspf", TPlaylist));
		ADDFMT((L"bdpls",       ResStr(IDS_MFMT_BDPLS),       L"mpls bdmv", TVideo)); // opens as a video file
		ADDFMT((L"cue",         ResStr(IDS_MFMT_CUE),         L"cue", TPlaylist));
#undef ADDFMT
	}

	for (auto& mfc : (*this)) {
		mfc.UpdateData(bSave);
	}
}

bool CMediaFormats::FindExt(const CString& ext)
{
	return (FindMediaByExt(ext) != nullptr);
}

bool CMediaFormats::FindAudioExt(const CString& ext)
{
	CMediaFormatCategory* pmfc = FindMediaByExt(ext);
	return (pmfc && pmfc->GetFileType() == TAudio);
}

CMediaFormatCategory* CMediaFormats::FindMediaByExt(CString ext)
{
	ext.TrimLeft('.');

	if (!ext.IsEmpty()) {
		const auto it = std::find_if(begin(), end(), [ext](const CMediaFormatCategory& mfc){
			return mfc.FindExt(ext);
		});
		if (it != cend()) {
			return &(*it);
		}
	}

	return nullptr;
}

void CMediaFormats::GetFilter(CString& filter, std::vector<CString>& mask)
{
	CString strTemp;

	// Add All Media formats
	filter += ResStr(IDS_AG_MEDIAFILES);
	mask.emplace_back(L"");

	for (const auto& mfc : (*this)) {
		strTemp = mfc.GetFilter() + L";";
		mask[0] += strTemp;
		filter += strTemp;
	}
	mask[0].TrimRight(';');
	filter.TrimRight(';');
	filter += L"|";

	// Add Video formats
	filter += ResStr(IDS_AG_VIDEOFILES);
	mask.emplace_back(L"");

	for (const auto& mfc : (*this)) {
		if (mfc.GetFileType() == TVideo) {
			strTemp = mfc.GetFilter() + L";";
			mask[1] += strTemp;
			filter += strTemp;
		}
	}
	filter.TrimRight(';');
	filter += L"|";

	// Add Audio formats
	filter += ResStr(IDS_AG_AUDIOFILES);
	mask.emplace_back(L"");

	for (const auto& mfc : (*this)) {
		if (mfc.GetFileType() == TAudio) {
			strTemp = mfc.GetFilter() + L";";
			mask[1] += strTemp;
			filter += strTemp;
		}
	}
	filter.TrimRight(';');
	filter += L"|";

	for (const auto& mfc : (*this)) {
		filter += mfc.GetDescription() + L"|" + mfc.GetFilter() + L"|";
		mask.emplace_back(mfc.GetFilter());
	}

	filter += ResStr(IDS_AG_ALLFILES);
	mask.emplace_back(L"*.*");

	filter += L"|";
}

void CMediaFormats::GetAudioFilter(CString& filter, std::vector<CString>& mask)
{
	CString strTemp;
	filter += ResStr(IDS_AG_AUDIOFILES);
	mask.emplace_back(L"");

	for (const auto& mfc : (*this)) {
		if (mfc.GetFileType() == TAudio) {
			strTemp = mfc.GetFilter() + L";";
			mask[0] += strTemp;
			filter += strTemp;
		}
	}

	mask[0].TrimRight(';');
	filter.TrimRight(';');
	filter += L"|";

	for (const auto& mfc : (*this)) {
		if (mfc.GetFileType() == TAudio) {
			filter += mfc.GetDescription() + L"|" + mfc.GetFilter() + L"|";
			mask.emplace_back(mfc.GetFilter());
		}
	}

	filter += ResStr(IDS_AG_ALLFILES);
	mask.emplace_back(L"*.*");

	filter += L"|";
}
