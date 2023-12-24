/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
		AfxGetProfile().WriteString(IDS_R_FILEFORMATS, m_label, GetExts());
	} else {
		CString exts = GetExts();
		AfxGetProfile().ReadString(IDS_R_FILEFORMATS, m_label, exts);
		SetExts(exts);
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
		ext.TrimLeft('.');

		if (ext.FindOneOf(L".\\/:*?\"<>|") >= 0) {
			m_exts.erase(cur);
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

CString CMediaFormatCategory::GetBackupExts() const
{
	return Implode(m_backupexts, ' ');
}

//
// CMediaFormats
//

void CMediaFormats::UpdateData(const bool bSave)
{
	if (bSave) {
		AfxGetProfile().DeleteSection(IDS_R_FILEFORMATS);
	} else {
		clear();
		reserve(55);

		// video files
		emplace_back(L"avi",         ResStr(IDS_MFMT_AVI),         L"avi divx");
		emplace_back(L"mpeg",        ResStr(IDS_MFMT_MPEG),        L"mpg mpeg mpe m1v m2v mpv2 mp2v pva evo m2p sfd");
		emplace_back(L"mpegts",      ResStr(IDS_MFMT_MPEGTS),      L"ts tp trp m2t m2ts mts rec ssif");
		emplace_back(L"dvdvideo",    ResStr(IDS_MFMT_DVDVIDEO),    L"vob ifo");
		emplace_back(L"mkv",         ResStr(IDS_MFMT_MKV),         L"mkv mk3d");
		emplace_back(L"webm",        ResStr(IDS_MFMT_WEBM),        L"webm");
		emplace_back(L"mp4",         ResStr(IDS_MFMT_MP4),         L"mp4 m4v mp4v mpv4 hdmov ismv");
		emplace_back(L"mov",         ResStr(IDS_MFMT_MOV),         L"mov");
		emplace_back(L"3gp",         ResStr(IDS_MFMT_3GP),         L"3gp 3gpp 3ga");
		emplace_back(L"3g2",         ResStr(IDS_MFMT_3G2),         L"3g2 3gp2");
		emplace_back(L"flv",         ResStr(IDS_MFMT_FLV),         L"flv f4v");
		emplace_back(L"ogm",         ResStr(IDS_MFMT_OGM),         L"ogm ogv");
		emplace_back(L"rm",          ResStr(IDS_MFMT_RM),          L"rm ram rmm rmvb");
		emplace_back(L"wmv",         ResStr(IDS_MFMT_WMV),         L"wmv wmp wm asf");
		//emplace_back(L"videocd",     ResStr(IDS_MFMT_VIDEOCD),     L"dat"); // "dat" extension is no longer supported
		emplace_back(L"bink",        ResStr(IDS_MFMT_BINK),        L"smk bik");
		emplace_back(L"flic",        ResStr(IDS_MFMT_FLIC),        L"fli flc flic");
		emplace_back(L"roq",         ResStr(IDS_MFMT_ROQ),         L"roq");
		emplace_back(L"dsm",         ResStr(IDS_MFMT_DSM),         L"dsm dsv dsa dss");
		emplace_back(L"rawvideo",    ResStr(IDS_MFMT_RAW_VIDEO),   L"y4m h264 264 vc1 h265 265 hm10 hevc obu");
		emplace_back(L"other",       ResStr(IDS_MFMT_OTHER),       L"amv wtv dvr-ms mxf ivf nut dav");
		// special files. used TScript to exclude automatic association
		emplace_back(L"swf",         ResStr(IDS_MFMT_SWF),         L"swf", TScript, L"ShockWave ActiveX control");
		emplace_back(L"avisynth",    ResStr(IDS_MFMT_AVISYNTH),    L"avs", TScript);
		emplace_back(L"vapoursynth", ResStr(IDS_MFMT_VAPOURSYNTH), L"vpy", TScript);
		// audio files
		emplace_back(L"ac3",         ResStr(IDS_MFMT_AC3),         L"ac3 ec3 eac3", TAudio);
		emplace_back(L"dts",         ResStr(IDS_MFMT_DTS),         L"dts dtshd dtsma", TAudio);
		emplace_back(L"aiff",        ResStr(IDS_MFMT_AIFF),        L"aif aifc aiff", TAudio);
		emplace_back(L"alac",        ResStr(IDS_MFMT_ALAC),        L"alac", TAudio);
		emplace_back(L"amr",         ResStr(IDS_MFMT_AMR),         L"amr awb", TAudio);
		emplace_back(L"ape",         ResStr(IDS_MFMT_APE),         L"ape apl", TAudio);
		emplace_back(L"au",          ResStr(IDS_MFMT_AU),          L"au snd", TAudio);
		emplace_back(L"audiocd",     ResStr(IDS_MFMT_CDA),         L"cda", TAudio);
		emplace_back(L"dvdaudio",    ResStr(IDS_MFMT_DVDAUDIO),    L"aob", TAudio);
		emplace_back(L"dsd",         ResStr(IDS_MFMT_DSD),         L"dsf dff", TAudio);
		emplace_back(L"flac",        ResStr(IDS_MFMT_FLAC),        L"flac", TAudio);
		emplace_back(L"m4a",         ResStr(IDS_MFMT_M4A),         L"m4a m4b aac", TAudio);
		emplace_back(L"midi",        ResStr(IDS_MFMT_MIDI),        L"mid midi rmi kar", TAudio);
		emplace_back(L"mka",         ResStr(IDS_MFMT_MKA),         L"mka", TAudio);
		emplace_back(L"weba",        ResStr(IDS_MFMT_WEBA),        L"weba", TAudio);
		emplace_back(L"mlp",         ResStr(IDS_MFMT_MLP),         L"mlp", TAudio);
		emplace_back(L"mp3",         ResStr(IDS_MFMT_MP3),         L"mp3", TAudio);
		emplace_back(L"mpa",         ResStr(IDS_MFMT_MPA),         L"mpa mp2 m1a m2a", TAudio);
		emplace_back(L"mpc",         ResStr(IDS_MFMT_MPC),         L"mpc", TAudio);
		emplace_back(L"ofr",         ResStr(IDS_MFMT_OFR),         L"ofr ofs", TAudio);
		emplace_back(L"ogg",         ResStr(IDS_MFMT_OGG),         L"ogg oga", TAudio);
		emplace_back(L"ra",          ResStr(IDS_MFMT_RA),          L"ra", TAudio);
		emplace_back(L"tak",         ResStr(IDS_MFMT_TAK),         L"tak", TAudio);
		emplace_back(L"tta",         ResStr(IDS_MFMT_TTA),         L"tta", TAudio);
		emplace_back(L"wav",         ResStr(IDS_MFMT_WAV),         L"wav w64", TAudio);
		emplace_back(L"wma",         ResStr(IDS_MFMT_WMA),         L"wma", TAudio);
		emplace_back(L"wavpack",     ResStr(IDS_MFMT_WAVPACK),     L"wv", TAudio);
		emplace_back(L"opus",        ResStr(IDS_MFMT_OPUS),        L"opus", TAudio);
		emplace_back(L"speex",       ResStr(IDS_MFMT_SPEEX),       L"spx", TAudio);
		// playlists
		emplace_back(L"pls",         ResStr(IDS_MFMT_PLS),         L"asx m3u m3u8 pls wpl mpcpl xspf", TPlaylist);
		emplace_back(L"bdpls",       ResStr(IDS_MFMT_BDPLS),       L"mpls bdmv", TVideo); // opens as a video file
		emplace_back(L"cue",         ResStr(IDS_MFMT_CUE),         L"cue", TPlaylist);
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
		const auto it = std::find_if(begin(), end(), [&ext](const CMediaFormatCategory& mfc){
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
