/*
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
#include "DVBChannel.h"

CDVBChannel::CDVBChannel(void)
{
	m_ulFrequency		= 0;
	m_nPrefNumber		= 0;
	m_nOriginNumber		= 0;
	m_bEncrypted		= false;
	m_bNowNextFlag		= false;
	m_ulONID			= 0;
	m_ulTSID			= 0;
	m_ulSID				= 0;
	m_ulPMT				= 0;
	m_ulPCR				= 0;
	m_ulVideoPID		= 0;
	m_nVideoType		= DVB_MPV;
	m_nAudioCount		= 0;
	m_nDefaultAudio		= 0;
	m_nSubtitleCount	= 0;
}

CDVBChannel::~CDVBChannel(void)
{
}

void CDVBChannel::FromString(CString strValue)
{
	int		i = 0;
	int		nVersion;

	nVersion		= _wtol(strValue.Tokenize(L"|", i));
	m_strName		= strValue.Tokenize(L"|", i);
	m_ulFrequency	= _wtol(strValue.Tokenize(L"|", i));
	m_nPrefNumber	= _wtol(strValue.Tokenize(L"|", i));
	m_nOriginNumber	= _wtol(strValue.Tokenize(L"|", i));

	if (nVersion > FORMAT_VERSION_0) {
		m_bEncrypted	= !!_wtol(strValue.Tokenize(L"|", i));
	}

	if (nVersion > FORMAT_VERSION_1) {
		m_bNowNextFlag	= !!_wtol(strValue.Tokenize(L"|", i));
	}

	m_ulONID		= _wtol(strValue.Tokenize(L"|", i));
	m_ulTSID		= _wtol(strValue.Tokenize(L"|", i));
	m_ulSID			= _wtol(strValue.Tokenize(L"|", i));
	m_ulPMT			= _wtol(strValue.Tokenize(L"|", i));
	m_ulPCR			= _wtol(strValue.Tokenize(L"|", i));
	m_ulVideoPID	= _wtol(strValue.Tokenize(L"|", i));
	m_nVideoType	= (DVB_STREAM_TYPE)_wtol(strValue.Tokenize(L"|", i));
	m_nAudioCount	= _wtol(strValue.Tokenize(L"|", i));

	if (nVersion > FORMAT_VERSION_1) {
		m_nDefaultAudio	= _wtol(strValue.Tokenize(L"|", i));
	}

	m_nSubtitleCount = _wtol(strValue.Tokenize(L"|", i));

	for (int j=0; j<m_nAudioCount; j++) {
		m_Audios[j].PID			= _wtol(strValue.Tokenize(L"|", i));
		m_Audios[j].Type		= (DVB_STREAM_TYPE)_wtol(strValue.Tokenize(L"|", i));
		m_Audios[j].PesType		= (PES_STREAM_TYPE)_wtol(strValue.Tokenize(L"|", i));
		m_Audios[j].Language	= strValue.Tokenize(L"|", i);
	}

	for (int j=0; j<m_nSubtitleCount; j++) {
		m_Subtitles[j].PID		= _wtol(strValue.Tokenize(L"|", i));
		m_Subtitles[j].Type		= (DVB_STREAM_TYPE)_wtol(strValue.Tokenize(L"|", i));
		m_Subtitles[j].PesType	= (PES_STREAM_TYPE)_wtol(strValue.Tokenize(L"|", i));
		m_Subtitles[j].Language	= strValue.Tokenize(L"|", i);
	}
}

CString CDVBChannel::ToString()
{
	CString		strValue;

	strValue.AppendFormat (L"%d|%s|%lu|%d|%d|%d|%d|%lu|%lu|%lu|%lu|%lu|%lu|%d|%d|%d|%d",
						   FORMAT_VERSION_CURRENT,
						   m_strName,
						   m_ulFrequency,
						   m_nPrefNumber,
						   m_nOriginNumber,
						   m_bEncrypted,
						   m_bNowNextFlag,
						   m_ulONID,
						   m_ulTSID,
						   m_ulSID,
						   m_ulPMT,
						   m_ulPCR,
						   m_ulVideoPID,
						   m_nVideoType,
						   m_nAudioCount,
						   m_nDefaultAudio,
						   m_nSubtitleCount);

	for (int i=0; i<m_nAudioCount; i++) {
		strValue.AppendFormat (L"|%lu|%d|%d|%s", m_Audios[i].PID, m_Audios[i].Type, m_Audios[i].PesType, m_Audios[i].Language);
	}

	for (int i=0; i<m_nSubtitleCount; i++) {
		strValue.AppendFormat (L"|%lu|%d|%d|%s", m_Subtitles[i].PID, m_Subtitles[i].Type, m_Subtitles[i].PesType, m_Subtitles[i].Language);
	}

	return strValue;
}

void CDVBChannel::SetName(BYTE* Value)
{
	m_strName = CA2W((LPCSTR)Value);
}

void CDVBChannel::AddStreamInfo(ULONG ulPID, DVB_STREAM_TYPE nType, PES_STREAM_TYPE nPesType, LPCTSTR strLanguage)
{
	switch (nType) {
		case DVB_MPV :
		case DVB_H264 :
			m_ulVideoPID	= ulPID;
			m_nVideoType	= nType;
			break;
		case DVB_MPA :
		case DVB_AC3 :
		case DVB_EAC3 :
			if (m_nAudioCount < DVB_MAX_AUDIO) {
				m_Audios[m_nAudioCount].PID			= ulPID;
				m_Audios[m_nAudioCount].Type		= nType;
				m_Audios[m_nAudioCount].PesType		= nPesType;
				m_Audios[m_nAudioCount].Language	= strLanguage;
				m_nAudioCount++;
			}
			break;
		case DVB_SUBTITLE :
			if (m_nSubtitleCount < DVB_MAX_SUBTITLE) {
				m_Subtitles[m_nSubtitleCount].PID		= ulPID;
				m_Subtitles[m_nSubtitleCount].Type		= nType;
				m_Subtitles[m_nSubtitleCount].PesType	= nPesType;
				m_Subtitles[m_nSubtitleCount].Language	= strLanguage;
				m_nSubtitleCount++;
			}
			break;
	}
}
