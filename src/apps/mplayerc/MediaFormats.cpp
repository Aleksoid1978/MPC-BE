/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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

CMediaFormatCategory::CMediaFormatCategory(
	CString label, CString description, CAtlList<CString>& exts, filetype_t filetype,
	CString specreqnote)
{
	m_label			= label;
	m_description	= description;
	m_specreqnote	= specreqnote;
	m_filetype		= filetype;
	m_exts.AddTailList(&exts);
	m_backupexts.AddTailList(&m_exts);
}

CMediaFormatCategory::CMediaFormatCategory(
	CString label, CString description, CString exts, filetype_t filetype,
	CString specreqnote)
{
	m_label			= label;
	m_description	= description;
	m_specreqnote	= specreqnote;
	m_filetype		= filetype;
	ExplodeMin(exts, m_exts, ' ');
	POSITION pos = m_exts.GetHeadPosition();

	while (pos) {
		m_exts.GetNext(pos).TrimLeft('.');
	}

	m_backupexts.AddTailList(&m_exts);
}

CMediaFormatCategory::~CMediaFormatCategory()
{
}

void CMediaFormatCategory::UpdateData(bool fSave)
{
	if (fSave) {
		AfxGetApp()->WriteProfileString(_T("FileFormats"), m_label, GetExts());
	} else {
		SetExts(AfxGetApp()->GetProfileString(_T("FileFormats"), m_label, GetExts()));
	}
}

CMediaFormatCategory::CMediaFormatCategory(const CMediaFormatCategory& mfc)
{
	*this = mfc;
}

CMediaFormatCategory& CMediaFormatCategory::operator = (const CMediaFormatCategory& mfc)
{
	if (this != &mfc) {
		m_label			= mfc.m_label;
		m_description	= mfc.m_description;
		m_specreqnote	= mfc.m_specreqnote;
		m_filetype		= mfc.m_filetype;
		m_exts.RemoveAll();
		m_exts.AddTailList(&mfc.m_exts);
		m_backupexts.RemoveAll();
		m_backupexts.AddTailList(&mfc.m_backupexts);
	}

	return *this;
}

void CMediaFormatCategory::RestoreDefaultExts()
{
	m_exts.RemoveAll();
	m_exts.AddTailList(&m_backupexts);
}

void CMediaFormatCategory::SetExts(CAtlList<CString>& exts)
{
	m_exts.RemoveAll();
	m_exts.AddTailList(&exts);
}

void CMediaFormatCategory::SetExts(CString exts)
{
	m_exts.RemoveAll();
	ExplodeMin(exts, m_exts, ' ');
	POSITION pos = m_exts.GetHeadPosition();

	while (pos) {
		POSITION cur = pos;
		CString& ext = m_exts.GetNext(pos);

		if (ext[0] == '\\') {
			m_exts.RemoveAt(cur);
		} else {
			ext.TrimLeft('.');
		}
	}
}

CString CMediaFormatCategory::GetFilter()
{
	CString filter;
	POSITION pos = m_exts.GetHeadPosition();

	while (pos) {
		filter += _T("*.") + m_exts.GetNext(pos) + _T(";");
	}

	filter.TrimRight(_T(';')); // cheap...
	return(filter);
}

CString CMediaFormatCategory::GetExts()
{
	CString exts = Implode(m_exts, ' ');

	return(exts);
}

CString CMediaFormatCategory::GetExtsWithPeriod()
{
	CString exts;
	POSITION pos = m_exts.GetHeadPosition();

	while (pos) {
		exts += _T(".") + m_exts.GetNext(pos) + _T(" ");
	}

	exts.TrimRight(_T(' '));

	return(exts);
}

CString CMediaFormatCategory::GetBackupExtsWithPeriod()
{
	CString exts;
	POSITION pos = m_backupexts.GetHeadPosition();

	while (pos) {
		exts += _T(".") + m_backupexts.GetNext(pos) + _T(" ");
	}

	exts.TrimRight(_T(' '));

	return(exts);
}

//
// 	CMediaFormats
//

CMediaFormats::CMediaFormats()
{
}

CMediaFormats::~CMediaFormats()
{
}

void CMediaFormats::UpdateData(bool fSave)
{
	if (fSave) {
		AfxGetApp()->WriteProfileString(_T("FileFormats"), NULL, NULL);
	} else {
		RemoveAll();

#define ADDFMT(f) Add(CMediaFormatCategory##f)
		// video files
		ADDFMT((_T("avi"),         ResStr(IDS_MFMT_AVI),         _T("avi")));
		ADDFMT((_T("mpeg"),        ResStr(IDS_MFMT_MPEG),        _T("mpg mpeg mpe m1v m2v mpv2 mp2v pva evo m2p sfd")));
		ADDFMT((_T("mpegts"),      ResStr(IDS_MFMT_MPEGTS),      _T("ts tp trp m2t m2ts mts rec ssif")));
		ADDFMT((_T("dvdvideo"),    ResStr(IDS_MFMT_DVDVIDEO),    _T("vob ifo")));
		ADDFMT((_T("mkv"),         ResStr(IDS_MFMT_MKV),         _T("mkv")));
		ADDFMT((_T("webm"),        ResStr(IDS_MFMT_WEBM),        _T("webm")));
		ADDFMT((_T("mp4"),         ResStr(IDS_MFMT_MP4),         _T("mp4 m4v mp4v mpv4 hdmov ismv")));
		ADDFMT((_T("mov"),         ResStr(IDS_MFMT_MOV),         _T("mov")));
		ADDFMT((_T("3gp"),         ResStr(IDS_MFMT_3GP),         _T("3gp 3gpp 3ga")));
		ADDFMT((_T("3g2"),         ResStr(IDS_MFMT_3G2),         _T("3g2 3gp2")));
		ADDFMT((_T("flv"),         ResStr(IDS_MFMT_FLV),         _T("flv f4v")));
		ADDFMT((_T("ogm"),         ResStr(IDS_MFMT_OGM),         _T("ogm ogv")));
		ADDFMT((_T("rm"),          ResStr(IDS_MFMT_RM),          _T("rm ram rmm rmvb")));
		ADDFMT((_T("roq"),         ResStr(IDS_MFMT_ROQ),         _T("roq")));
		ADDFMT((_T("rt"),          ResStr(IDS_MFMT_RT),          _T("rt rp smil")));
		ADDFMT((_T("wmv"),         ResStr(IDS_MFMT_WMV),         _T("wmv wmp wm asf")));
//		ADDFMT((_T("videocd"),     ResStr(IDS_MFMT_VIDEOCD),     _T("dat")));
		ADDFMT((_T("ratdvd"),      ResStr(IDS_MFMT_RATDVD),      _T("ratdvd"), TVideo, _T("ratdvd media file")));
		ADDFMT((_T("bink"),        ResStr(IDS_MFMT_BINK),        _T("smk bik"), TVideo, _T("smackw32/binkw32.dll in dll path")));
		ADDFMT((_T("flic"),        ResStr(IDS_MFMT_FLIC),        _T("fli flc flic")));
		ADDFMT((_T("dsm"),         ResStr(IDS_MFMT_DSM),         _T("dsm dsv dsa dss")));
		ADDFMT((_T("ivf"),         ResStr(IDS_MFMT_IVF),         _T("ivf")));
		ADDFMT((_T("swf"),         ResStr(IDS_MFMT_SWF),         _T("swf"), TVideo, _T("ShockWave ActiveX control")));
		ADDFMT((_T("other"),       ResStr(IDS_MFMT_OTHER),       _T("divx amv wtv dvr-ms")));
		ADDFMT((_T("raw video"),   ResStr(IDS_MFMT_RAW_VIDEO),   _T("h264 264 vc1 h265 265 hm10 hevc")));
		// audio files
		ADDFMT((_T("ac3dts"),      ResStr(IDS_MFMT_AC3),         _T("ac3 dts dtshd"), TAudio));
		ADDFMT((_T("aiff"),        ResStr(IDS_MFMT_AIFF),        _T("aif aifc aiff"), TAudio));
		ADDFMT((_T("alac"),        ResStr(IDS_MFMT_ALAC),        _T("alac"), TAudio));
		ADDFMT((_T("amr"),         ResStr(IDS_MFMT_AMR),         _T("amr awb"), TAudio));
		ADDFMT((_T("ape"),         ResStr(IDS_MFMT_APE),         _T("ape apl"), TAudio));
		ADDFMT((_T("au"),          ResStr(IDS_MFMT_AU),          _T("au snd"), TAudio));
		ADDFMT((_T("audiocd"),     ResStr(IDS_MFMT_CDA),         _T("cda"), TAudio));
		ADDFMT((_T("dvdaudio"),    ResStr(IDS_MFMT_DVDAUDIO),    _T("aob"), TAudio));
		ADDFMT((_T("dsd"),         _T("DSD"),                    _T("dsf dff"), TAudio));
		ADDFMT((_T("flac"),        ResStr(IDS_MFMT_FLAC),        _T("flac"), TAudio));
		ADDFMT((_T("m4a"),         ResStr(IDS_MFMT_M4A),         _T("m4a m4b aac"), TAudio));
		ADDFMT((_T("midi"),        ResStr(IDS_MFMT_MIDI),        _T("mid midi rmi"), TAudio));
		ADDFMT((_T("mka"),         ResStr(IDS_MFMT_MKA),         _T("mka"), TAudio));
		ADDFMT((_T("mlp"),         ResStr(IDS_MFMT_MLP),         _T("mlp"), TAudio));
		ADDFMT((_T("mp3"),         ResStr(IDS_MFMT_MP3),         _T("mp3"), TAudio));
		ADDFMT((_T("mpa"),         ResStr(IDS_MFMT_MPA),         _T("mpa mp2 m1a m2a"), TAudio));
		ADDFMT((_T("mpc"),         ResStr(IDS_MFMT_MPC),         _T("mpc"), TAudio));
		ADDFMT((_T("ofr"),         ResStr(IDS_MFMT_OFR),         _T("ofr ofs"), TAudio));
		ADDFMT((_T("ogg"),         ResStr(IDS_MFMT_OGG),         _T("ogg oga"), TAudio));
		ADDFMT((_T("ra"),          ResStr(IDS_MFMT_RA),          _T("ra"), TAudio));
		ADDFMT((_T("tak"),         ResStr(IDS_MFMT_TAK),         _T("tak"), TAudio));
		ADDFMT((_T("tta"),         ResStr(IDS_MFMT_TTA),         _T("tta"), TAudio));
		ADDFMT((_T("wav"),         ResStr(IDS_MFMT_WAV),         _T("wav w64"), TAudio));
		ADDFMT((_T("wma"),         ResStr(IDS_MFMT_WMA),         _T("wma"), TAudio));
		ADDFMT((_T("wavpack"),     ResStr(IDS_MFMT_WAVPACK),     _T("wv"), TAudio));
		ADDFMT((_T("opus"),        ResStr(IDS_MFMT_OPUS),        _T("opus"), TAudio));
		ADDFMT((_T("speex"),       ResStr(IDS_MFMT_SPEEX),       _T("spx"), TAudio));
		// playlists
		ADDFMT((_T("pls"),         ResStr(IDS_MFMT_PLS),         _T("asx m3u m3u8 pls wvx wax wmx mpcpl xspf"), TPlaylist));
		ADDFMT((_T("bdpls"),       ResStr(IDS_MFMT_BDPLS),       _T("mpls bdmv"), TVideo)); // opens as a video file
		ADDFMT((_T("cue"),         ResStr(IDS_MFMT_CUE),         _T("cue"), TPlaylist));
#undef ADDFMT
	}

	for (size_t i = 0; i < GetCount(); i++) {
		GetAt(i).UpdateData(fSave);
	}
}

bool CMediaFormats::FindExt(CString ext)
{
	return (FindMediaByExt(ext) != NULL);
}

bool CMediaFormats::FindAudioExt(CString ext)
{
	CMediaFormatCategory* pmfc = FindMediaByExt(ext);
	return (pmfc && pmfc->GetFileType() == TAudio);
}

CMediaFormatCategory* CMediaFormats::FindMediaByExt(CString ext)
{
	ext.TrimLeft(_T('.'));

	if (!ext.IsEmpty()) {
		for (size_t i = 0; i < GetCount(); i++) {
			CMediaFormatCategory& mfc = GetAt(i);
			if (mfc.FindExt(ext)) {
				return &mfc;
			}
		}
	}

	return NULL;
}

void CMediaFormats::GetFilter(CString& filter, CAtlArray<CString>& mask)
{
	CString strTemp;

	// Add All Media formats
	filter += ResStr(IDS_AG_MEDIAFILES);
	mask.Add(_T(""));

	for (size_t i = 0; i < GetCount(); i++) {
		strTemp	= GetAt(i).GetFilter() + _T(";");
		mask[0]	+= strTemp;
		filter	+= strTemp;
	}
	mask[0].TrimRight(_T(';'));
	filter.TrimRight(_T(';'));
	filter += _T("|");

	// Add Video formats
	filter += ResStr(IDS_AG_VIDEOFILES);
	mask.Add(_T(""));

	for (size_t i = 0; i < GetCount(); i++) {
		if (GetAt(i).GetFileType() == TVideo) {
			strTemp = GetAt(i).GetFilter() + _T(";");
			mask[1] += strTemp;
			filter += strTemp;
		}
	}
	filter.TrimRight(_T(';'));
	filter += _T("|");

	// Add Audio formats
	filter += ResStr(IDS_AG_AUDIOFILES);
	mask.Add(_T(""));

	for (size_t i = 0; i < GetCount(); i++) {
		if (GetAt(i).GetFileType() == TAudio) {
			strTemp	= GetAt(i).GetFilter() + _T(";");
			mask[1]	+= strTemp;
			filter	+= strTemp;
		}
	}
	filter.TrimRight(_T(';'));
	filter += _T("|");

	for (size_t i = 0; i < GetCount(); i++) {
		CMediaFormatCategory& mfc	= GetAt(i);
		filter						+= mfc.GetDescription() + _T("|" + GetAt(i).GetFilter() + _T("|"));
		mask.Add(mfc.GetFilter());
	}

	filter += ResStr(IDS_AG_ALLFILES);
	mask.Add(_T("*.*"));

	filter += _T("|");
}

void CMediaFormats::GetAudioFilter(CString& filter, CAtlArray<CString>& mask)
{
	CString		strTemp;
	filter += ResStr(IDS_AG_AUDIOFILES);
	mask.Add(_T(""));

	for (size_t i = 0; i < GetCount(); i++) {
		CMediaFormatCategory& mfc = GetAt(i);
		if (mfc.GetFileType() == TAudio/* && mfc.GetEngineType() == DirectShow*/) {
			strTemp	= GetAt(i).GetFilter() + _T(";");
			mask[0]	+= strTemp;
			filter	+= strTemp;
		}
	}

	mask[0].TrimRight(_T(';'));
	filter.TrimRight(_T(';'));
	filter += _T("|");

	for (size_t i = 0; i < GetCount(); i++) {
		CMediaFormatCategory& mfc = GetAt(i);
		if (mfc.GetFileType() == TAudio/* && mfc.GetEngineType() == DirectShow*/) {
			filter += mfc.GetDescription() + _T("|") + GetAt(i).GetFilter() + _T("|");
			mask.Add(mfc.GetFilter());
		}
	}

	filter += ResStr(IDS_AG_ALLFILES);
	mask.Add(_T("*.*"));

	filter += _T("|");
}
