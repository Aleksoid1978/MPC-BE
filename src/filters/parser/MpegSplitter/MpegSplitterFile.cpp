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

#include "stdafx.h"
#include <MMReg.h>
#include <list>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include "MpegSplitterFile.h"
#include "../BaseSplitter/FrameDuration.h"
#include "../../../DSUtil/AudioParser.h"

CMpegSplitterFile::CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, CHdmvClipInfo &ClipInfo, bool bIsBD, bool ForcedSub, int AC3CoreOnly, bool SubEmptyPin)
	: CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL | FM_FILE_VAR | FM_STREAM)
	, m_type(MPEG_TYPES::mpeg_invalid)
	, m_rate(0)
	, m_bPESPTSPresent(TRUE)
	, m_bIsBD(bIsBD)
	, m_ClipInfo(ClipInfo)
	, m_ForcedSub(ForcedSub)
	, m_AC3CoreOnly(AC3CoreOnly)
	, m_SubEmptyPin(SubEmptyPin)
	, m_bOpeningCompleted(FALSE)
	, m_programs(&m_streams)
	, m_bIMKH_CCTV(FALSE)
	, m_rtMin(0)
	, m_rtMax(0)
	, m_posMin(0)
{
	memset(m_psm, 0, sizeof(m_psm));
	if (SUCCEEDED(hr)) {
		hr = Init(pAsyncReader);
	}
}

HRESULT CMpegSplitterFile::Init(IAsyncReader* pAsyncReader)
{
	if (m_ClipInfo.IsHdmv()) {
		if (CComQIPtr<ISyncReader> pReader = pAsyncReader) {
			// To support seamless BD playback, CMultiFiles should update m_rtPTSOffset variable each time when a new part is opened
			// use this code only if Blu-ray is detected
			pReader->SetPTSOffset(&m_rtPTSOffset);
		}
	}

	BYTE data[16];
	Seek(0);
	if (ByteRead(data, sizeof(data)) != S_OK) {
		return E_FAIL;
	}
	DWORD id = GETDWORD(data);
	DWORD id2 = GETDWORD(data + 4);

	if (id == FCC('RIFF') && GETDWORD(data+8) != FCC('CDXA') // AVI, WAVE, AMV and other
			|| id == 0xA3DF451A         // Matroska
			|| id2 == FCC('ftyp')       // MP4
			|| id2 == FCC('moov')       // MOV
			|| id2 == FCC('mdat')       // MOV
			|| id2 == FCC('wide')       // MOV
			|| id2 == FCC('skip')       // MOV
			|| id2 == FCC('free')       // MOV
			|| id2 == FCC('pnot')       // MOV
			|| id == FCC('Rar!')        // RAR
			|| id == 0x04034B50         // ZIP
			|| id == 0xAFBC7A37         // 7-Zip
			|| GETWORD(data) == 'ZM') { // EXE, DLL
		return E_FAIL;
	}

	Seek(0);
	m_bIMKH_CCTV = (BitRead(32, true) == 'IMKH');

	{
		byte b = 0;
		while (GetPos() < (65 * KILOBYTE) && (b = BitRead(8)) != 0x47 && GetRemaining());

		if (b == 0x47) {
			Seek(GetPos() - 1);
			int cnt = 0, limit = 4;
			for (trhdr h; cnt < limit && ReadTR(h); cnt++) {
				Seek(h.next);
			}
			if (cnt >= limit) {
				m_type = MPEG_TYPES::mpeg_ts;
			}
		}
	}

	if (m_type == MPEG_TYPES::mpeg_invalid) {
		Seek(0);
		int cnt = 0, limit = 4;
		for (pvahdr h; cnt < limit && ReadPVA(h); cnt++) {
			Seek(GetPos() + h.length);
		}
		if (cnt >= limit) {
			m_type = MPEG_TYPES::mpeg_pva;
		}
	}


	if (m_type == MPEG_TYPES::mpeg_invalid) {
		Seek(0);
		int cnt = 0, limit = 4;

		BYTE b;
		while (cnt < limit && GetPos() < (MEGABYTE / 2) && NextMpegStartCode(b)) {
			if (b == 0xba) {
				pshdr h;
				if (ReadPS(h)) {
					cnt++;
					m_rate = h.bitrate / 8;
				}
			} else if ((b & 0xe0) == 0xc0 // audio, 110xxxxx, mpeg1/2/3
					   || (b & 0xf0) == 0xe0 // video, 1110xxxx, mpeg1/2
					   || b == 0xbd) { // private stream 1, 0xbd, ac3/dts/lpcm/subpic
				peshdr h;
				if (ReadPES(h, b) && BitRead(24, true) == 0x000001) {
					cnt++;
				}
			}
		}

		if (cnt >= limit) {
			m_type = MPEG_TYPES::mpeg_ps;
		}
	}

	if (m_type == MPEG_TYPES::mpeg_invalid) {
		return E_FAIL;
	}

	if (m_ClipInfo.IsHdmv()) {
		CHdmvClipInfo::Streams& streams = m_ClipInfo.GetStreams();
		for (size_t i = 0; i < streams.GetCount(); i++) {
			CHdmvClipInfo::Stream& stream = streams[i];
			if (stream.m_Type == PRESENTATION_GRAPHICS_STREAM) {
				AddHdmvPGStream(stream.m_PID, stream.m_LanguageCode);
			}
		}
	}

	m_pmt_streams.clear();
	if (IsRandomAccess()) {
		const __int64 len = GetLength();

		__int64 stop = std::min(10LL * MEGABYTE, len);
		__int64 steps = 20;
		if (IsURL()) {
			stop = std::min(2LL * MEGABYTE, len);
			steps = 2;
		}

		SearchPrograms(0, stop);
		SearchStreams(0, stop);

		const int step_size = 512 * KILOBYTE;

		int num = std::min(steps, (len - stop) / step_size);
		if (num > 0) {
			__int64 step = (len - stop) / num;
			for (int i = 0; i < num; i++) {
				stop += step;
				const __int64 start = stop - std::min((__int64)step_size, step);
				SearchPrograms(start, stop);
				SearchStreams(start, stop);
			}
		}
	} else {
		__int64 stop = GetAvailable();
		const __int64 hi = IsStreaming() ? MEGABYTE : std::min((__int64)MEGABYTE, GetLength());
		const __int64 lo = IsStreaming() ? 64 * KILOBYTE : std::min(64LL * KILOBYTE, GetLength());
		stop = clamp(stop, lo, hi);
		SearchPrograms(0, stop);

		if (IsStreaming()) {
			for (const auto& pr : m_programs) {
				for (const auto& stream : pr.second.streams) {
					m_pmt_streams.emplace_back(stream);
				}
			}
		}

		stop = IsStreaming() ? 5 * MEGABYTE : std::min(10LL * MEGABYTE, GetLength());
		SearchStreams(0, stop, m_pmt_streams.empty() ? 2000 : 5000);
	}

	if (!m_bIsBD) {
		REFERENCE_TIME rtMin = _I64_MAX;
		__int64 posMin       = -1;
		__int64 posMax       = posMin;

		CMpegSplitterFile::CStreamList* pMasterStream = GetMasterStream();
		if (!pMasterStream) {
			ASSERT(0);
			return E_FAIL;
		}

		for (auto& it : m_SyncPoints) {
			auto& sps = it.second;
			std::list<REFERENCE_TIME> duplicates;
			for (size_t i = 0; i < sps.GetCount() - 1; i++) {
				for (size_t j = i + 1; j < sps.GetCount(); j++) {
					if (sps[i].rt == sps[j].rt) {
						duplicates.emplace_back(sps[i].rt);
					}
				}
			}

			if (!duplicates.empty()) {
				duplicates.sort();
				duplicates.unique();
				for (const auto& dup : duplicates) {
					for (size_t i = 0; i < sps.GetCount();) {
						if (sps[i].rt == dup) {
							sps.RemoveAt(i);
						} else {
							i++;
						}
					}
				}
			}
		}

		WORD main_program_number = WORD_MAX;
		if (m_type == MPEG_TYPES::mpeg_ts && m_programs.size() > 1) {
			// Remove empty programs
			for (auto it = m_programs.begin(); it != m_programs.end();) {
				auto& p = it->second;
				if (!p.streamCount(m_streams)) {
					m_programs.erase(it++);
				} else {
					it++;
				}
			}

			if (m_programs.size() > 1) {
				const DWORD pid = pMasterStream->GetHead();
				auto program = FindProgram(pid);
				if (program) {
					main_program_number = program->program_number;

					// deduplicate programs
					for (auto it = m_programs.begin(); it != m_programs.end();) {
						auto& p = it->second;
						if (p.program_number != main_program_number && p.streamFind(pid)) {
							m_programs.erase(it++);
						} else {
							it++;
						}
					}
				}
			}
		}

		auto verifyProgram = [&](const stream& s) {
			if (main_program_number != WORD_MAX) {
				auto program = FindProgram(s);
				if (!program || program->program_number != main_program_number) {
					return false;
				}
			}

			return true;
		};

		for (int type = stream_type::video; type < stream_type::unknown; type++) {
			if (type == stream_type::subpic) {
				continue;
			}
			const CStreamList& streams = m_streams[type];
			POSITION pos = streams.GetHeadPosition();
			while (pos) {
				const stream& s = streams.GetNext(pos);
				if (!verifyProgram(s)) {
					continue;
				}

				const SyncPoints& sps = m_SyncPoints[s];
				if (sps.GetCount()
						&& (sps[0].rt < rtMin || (sps[0].rt == rtMin && sps[0].fp < posMin))) {
					rtMin = sps[0].rt;
					posMin = posMax = sps[0].fp;
				}
			}
		}

		if (rtMin >= 0) {
			m_posMin = posMin;

			const REFERENCE_TIME maxStartDelta = 10 * 10000000i64; // 10 seconds
			m_rtMin = m_rtMax = rtMin;
			for (int type = stream_type::video; type < stream_type::unknown; type++) {
				if (type == stream_type::subpic) {
					continue;
				}
				const CStreamList& streams = m_streams[type];
				POSITION pos = streams.GetHeadPosition();
				while (pos) {
					const stream& s = streams.GetNext(pos);
					if (!verifyProgram(s)) {
						continue;
					}

					const SyncPoints& sps = m_SyncPoints[s];
					for (size_t i = 1; i < sps.GetCount(); i++) {
						if (sps[i].rt > m_rtMax && sps[i].fp > posMax) {
							m_rtMax	= sps[i].rt;
							posMax	= sps[i].fp;
						}
					}

					for (size_t i = 0; i < sps.GetCount() && sps[i].fp < m_posMin; i++) {
						if (llabs(rtMin - sps[i].rt) < maxStartDelta) {
							m_posMin = sps[i].fp;
						}
					}
				}
			}

			if (m_rtMax > m_rtMin) {
				m_rate = llMulDiv(posMax - posMin, UNITS, m_rtMax - m_rtMin, 0);
			}
		}
	}

	for (int type = stream_type::video; type < stream_type::unknown; type++) {
		const CStreamList& streams = m_streams[type];
		POSITION pos = streams.GetHeadPosition();
		while (pos) {
			const stream& s = streams.GetNext(pos);
			const SyncPoints& sps = m_SyncPoints[s];
			m_streamData[s].usePTS = sps.GetCount() > 1;
		}
	}

	m_bOpeningCompleted = TRUE;

	if (m_SubEmptyPin) {
		// Add fake Subtitle stream ...
		if (m_type == MPEG_TYPES::mpeg_ts) {
			if (m_streams[stream_type::video].GetCount()) {
				if (!m_ClipInfo.IsHdmv() && m_streams[stream_type::subpic].GetCount()) {
					stream s;
					s.pid = NO_SUBTITLE_PID;
					s.mt.majortype	= m_streams[stream_type::subpic].GetHead().mt.majortype;
					s.mt.subtype	= m_streams[stream_type::subpic].GetHead().mt.subtype;
					s.mt.formattype	= m_streams[stream_type::subpic].GetHead().mt.formattype;
					m_streams[stream_type::subpic].AddTail(s);
				} else {
					AddHdmvPGStream(NO_SUBTITLE_PID, "---");
				}
			}
		} else {
			if (m_streams[stream_type::video].GetCount()) {
				stream s;
				s.pid = NO_SUBTITLE_PID;
				s.mt.majortype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.majortype  : MEDIATYPE_Video;
				s.mt.subtype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.subtype    : MEDIASUBTYPE_DVD_SUBPICTURE;
				s.mt.formattype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.formattype : FORMAT_None;
				m_streams[stream_type::subpic].AddTail(s);
			}
		}
	}

	m_pmt_streams.clear();
	m_ProgramData.clear();
	avch.clear();
	hevch.clear();
	seqh.clear();

	Seek(m_posMin);

	return S_OK;
}

BOOL CMpegSplitterFile::CheckKeyFrame(CAtlArray<BYTE>& pData, stream_codec codec)
{
	if (codec == stream_codec::H264) {
		CH264Nalu Nalu;
		Nalu.SetBuffer(pData.GetData(), pData.GetCount());
		while (Nalu.ReadNext()) {
			NALU_TYPE nalu_type = Nalu.GetType();
			if (nalu_type == NALU_TYPE_IDR) {
				// IDR Nalu
				return TRUE;
			}
		}

		return FALSE;
	} else if (codec == stream_codec::MPEG) {
		BYTE id = 0;

		CGolombBuffer gb(pData.GetData(), pData.GetCount());
		while (!gb.IsEOF()) {
			if (!gb.NextMpegStartCode(id)) {
				break;
			}

			if (id == 0x00 && gb.RemainingSize()) {
				BYTE* picture = gb.GetBufferPos();
				BYTE pict_type = (picture[1] >> 3) & 7;
				if (pict_type == 1) {
					// Intra
					return TRUE;
				}
			}
		}

		return FALSE;
	}

	return TRUE;
}

REFERENCE_TIME CMpegSplitterFile::NextPTS(DWORD TrackNum, stream_codec codec, __int64& nextPos, BOOL bKeyFrameOnly/* = FALSE*/, REFERENCE_TIME rtLimit/* = _I64_MAX*/)
{
	REFERENCE_TIME rt	= INVALID_TIME;
	__int64 rtpos		= -1;
	__int64 pos			= GetPos();
	nextPos				= pos + 1;

	BYTE b;

	while (GetRemaining()) {
		if (m_type == MPEG_TYPES::mpeg_ps) {
			if (!NextMpegStartCode(b)) {
				ASSERT(0);
				break;
			}

			if ((b >= 0xbd && b < 0xf0) || (b == 0xfd)) {
				__int64 pos2 = GetPos() - 4;

				peshdr h;
				if (!ReadPES(h, b) || !h.len) {
					continue;
				}

				__int64 pos3 = GetPos();
				nextPos = pos3 + h.len;

				if (h.fpts && AddStream(0, b, h.id_ext, h.len, FALSE) == TrackNum) {
					if (rtLimit != _I64_MAX && (h.pts - m_rtMin) > rtLimit) {
						Seek(pos);
						return INVALID_TIME;
					}

					rt    = h.pts;
					rtpos = pos2;

					BOOL bKeyFrame = TRUE;
					if (bKeyFrameOnly) {
						bKeyFrame = FALSE;
						__int64 nBytes = h.len - (GetPos() - pos2);
						if (nBytes > 0) {
							CAtlArray<BYTE> p;
							p.SetCount((size_t)nBytes);
							ByteRead(p.GetData(), nBytes);

							bKeyFrame = CheckKeyFrame(p, codec);
						}
					}

					if (bKeyFrame) {
						break;
					}
				}

				Seek(pos3 + h.len);
			}
		} else if (m_type == MPEG_TYPES::mpeg_ts) {
			trhdr h;
			if (!ReadTR(h)) {
				continue;
			}

			__int64 packet_pos = GetPos();
			nextPos = h.next;

			if (h.pid == TrackNum && h.payloadstart && NextMpegStartCode(b, 4)) {
				peshdr h2;
				if (ReadPES(h2, b) && h2.fpts) {
					if (rtLimit != _I64_MAX && (h2.pts - m_rtMin) > rtLimit) {
						Seek(pos);
						return INVALID_TIME;
					}

					rt    = h2.pts;
					rtpos = h.hdrpos;

					BOOL bKeyFrame = TRUE;
					if (bKeyFrameOnly) {
						if (!h.randomaccess) {
							bKeyFrame = FALSE;

							if (h.bytes > (GetPos() - packet_pos)) {
								size_t size = h.bytes - (GetPos() - packet_pos);
								CAtlArray<BYTE> p;
								p.SetCount(size);
								ByteRead(p.GetData(), size);

								// collect solid data packet
								{
									Seek(h.next);
									while (GetRemaining()) {
										const __int64 start_pos_2 = GetPos();

										trhdr trhdr_2;
										if (!ReadTR(trhdr_2)) {
											continue;
										}

										if (trhdr_2.payload && trhdr_2.pid == TrackNum) {
											packet_pos = GetPos();
											peshdr peshdr_2;
											if (trhdr_2.payloadstart &&
													(!NextMpegStartCode(b, 4) || !ReadPES(peshdr_2, b))) {
												Seek(trhdr_2.next);
												continue;
											}
											if (peshdr_2.fpts) {
												bKeyFrame = CheckKeyFrame(p, codec);
												nextPos = h.next = start_pos_2;
												break;
											}

											if (trhdr_2.bytes > (GetPos() - packet_pos)) {
												size = trhdr_2.bytes - (GetPos() - packet_pos);
												const size_t old_size = p.GetCount();
												p.SetCount(old_size + size);
												ByteRead(p.GetData() + old_size, size);
											}
										}

										Seek(trhdr_2.next);
									}
								}
							}
						}
					}

					if (bKeyFrame) {
						break;
					}
				}
			}

			Seek(h.next);
		} else if (m_type == MPEG_TYPES::mpeg_pva) {
			pvahdr h;
			if (!ReadPVA(h)) {
				continue;
			}

			nextPos = GetPos() + h.length;

			if (h.fpts) {
				rt = h.pts;
				break;
			}
		}
	}

	if (rt != INVALID_TIME) {
		rt -= m_rtMin;
		if (rtpos >= 0) {
			pos = rtpos;
		}
	}

	Seek(pos);

	return rt;
}

void CMpegSplitterFile::SearchPrograms(__int64 start, __int64 stop)
{
	if (m_type != MPEG_TYPES::mpeg_ts || m_ClipInfo.IsHdmv()) {
		return;
	}

	m_ProgramData.clear();

	Seek(start);

	while (GetPos() < stop) {
		trhdr h;
		if (!ReadTR(h)) {
			continue;
		}

		ReadPrograms(h);
		Seek(h.next);
	}
}

void CMpegSplitterFile::SearchStreams(__int64 start, __int64 stop, DWORD msTimeOut/* = INFINITE*/)
{
	const ULONGLONG startTime = GetPerfCounter();

	avch.clear();
	hevch.clear();
	seqh.clear();

	Seek(start);

	for (;;) {
		if (!m_pmt_streams.empty()) {
			size_t streams_cnt = 0;
			for (const auto& stream : m_pmt_streams) {
				for (int type = stream_type::video; type < stream_type::subpic; type++) {
					if (m_streams[type].FindStream(stream.pid)) {
						streams_cnt++;
						break;
					}
				}
			}

			if (streams_cnt == m_pmt_streams.size()) {
				DLog(L"CMpegSplitterFile::SearchStreams() : all sreams from PMT is parsed, was used %I64d bytes, %llu ms", GetPos() - start, (GetPerfCounter() - startTime) / 10000ULL);
				break;
			}
		}

		if (msTimeOut != INFINITE) {
			const ULONGLONG endTime = GetPerfCounter();
			const DWORD deltaTime = (endTime - startTime) / 10000;
			if (deltaTime >= msTimeOut) {
				break;
			}
		}

		if (GetPos() >= stop
				|| (!IsStreaming() && GetPos() == GetLength())) {
			break;
		}

		BYTE b = 0;
		if (m_type == MPEG_TYPES::mpeg_ps) {
			if (!NextMpegStartCode(b)) {
				continue;
			}

			if (b == 0xba) { // program stream header
				pshdr h;
				if (!ReadPS(h)) {
					continue;
				}
				m_rate = int(h.bitrate/8);
			} else if (b == 0xbc) { // program stream map
				UpdatePSM();
			} else if (b == 0xbb) { // program stream system header
				pssyshdr h;
				if (!ReadPSS(h)) {
					continue;
				}
			} else if ((b >= 0xbd && b < 0xf0) || (b == 0xfd)) { // pes packet
				const __int64 packet_start_pos = GetPos() - 4;

				peshdr h;
				if (!ReadPES(h, b) || !h.len) {
					continue;
				}

				const __int64 pos = GetPos();
				const DWORD TrackNum = AddStream(0, b, h.id_ext, h.len);

				if (h.fpts) {
					SyncPoints& sps = m_SyncPoints[TrackNum];
					sps.Add({ h.pts, packet_start_pos });
				}

				Seek(pos + h.len);
			}
		} else if (m_type == MPEG_TYPES::mpeg_ts) {
			trhdr h;
			if (!ReadTR(h)) {
				continue;
			}

			if (h.payload && ISVALIDPID(h.pid)) {
				const __int64 pos = GetPos();
				peshdr h2;
				if (h.payloadstart
						&& (!NextMpegStartCode(b, 4) || !ReadPES(h2, b))) {
					Seek(h.next);
					continue;
				}

				if (h.bytes > (GetPos() - pos)) {
					const DWORD TrackNum = AddStream(h.pid, b, h2.id_ext, h.bytes - (GetPos() - pos));

					if (h2.fpts) {
						SyncPoints& sps = m_SyncPoints[TrackNum];
						sps.Add({ h2.pts, h.hdrpos });
					}
				}
			}

			Seek(h.next);
		} else if (m_type == MPEG_TYPES::mpeg_pva) {
			pvahdr h;
			if (!ReadPVA(h)) {
				continue;
			}

			const __int64 pos = GetPos();

			DWORD TrackNum = 0;
			if (h.streamid == 1) {
				TrackNum = AddStream(h.streamid, 0xe0, 0, h.length);
			} else if (h.streamid == 2) {
				TrackNum = AddStream(h.streamid, 0xc0, 0, h.length);
			}

			if (h.fpts && TrackNum) {
				SyncPoints& sps = m_SyncPoints[TrackNum];
				sps.Add({ h.pts, h.hdrpos });
			}

			Seek(pos + h.length);
		}
	}
}

#define MPEG_AUDIO        (1ULL << 0)
#define AAC_AUDIO         (1ULL << 1)
#define AC3_AUDIO         (1ULL << 2)
#define DTS_AUDIO         (1ULL << 3)
#define LPCM_AUDIO        (1ULL << 4)
#define MPEG2_VIDEO       (1ULL << 5)
#define H264_VIDEO        (1ULL << 6)
#define VC1_VIDEO         (1ULL << 7)
#define DIRAC_VIDEO       (1ULL << 8)
#define HEVC_VIDEO        (1ULL << 9)
#define PGS_SUB           (1ULL << 10)
#define DVB_SUB           (1ULL << 11)
#define TELETEXT_SUB      (1ULL << 12)
#define OPUS_AUDIO        (1ULL << 13)
#define DTS_EXPRESS_AUDIO (1ULL << 14)

#define PES_STREAM_TYPE_ANY (MPEG_AUDIO | AAC_AUDIO | AC3_AUDIO | DTS_AUDIO/* | LPCM_AUDIO */| MPEG2_VIDEO | H264_VIDEO | DIRAC_VIDEO | HEVC_VIDEO/* | PGS_SUB*/ | DVB_SUB | TELETEXT_SUB | OPUS_AUDIO | DTS_EXPRESS_AUDIO)

static const struct StreamType {
	PES_STREAM_TYPE pes_stream_type;
	ULONGLONG		stream_type;
} PES_types[] = {
	// MPEG Audio
	{ AUDIO_STREAM_MPEG1,					MPEG_AUDIO	},
	{ AUDIO_STREAM_MPEG2,					MPEG_AUDIO	},
	// AAC Audio
	{ AUDIO_STREAM_AAC,						AAC_AUDIO	},
	{ AUDIO_STREAM_AAC_LATM,				AAC_AUDIO	},
	// AC3/TrueHD Audio
	{ AUDIO_STREAM_AC3,						AC3_AUDIO	},
	{ AUDIO_STREAM_AC3_PLUS,				AC3_AUDIO	},
	{ AUDIO_STREAM_AC3_TRUE_HD,				AC3_AUDIO	},
	{ SECONDARY_AUDIO_AC3_PLUS,				AC3_AUDIO	},
	{ PES_PRIVATE,							AC3_AUDIO	},
	// DTS Audio
	{ AUDIO_STREAM_DTS,						DTS_AUDIO	},
	{ AUDIO_STREAM_DTS_HD,					DTS_AUDIO	},
	{ AUDIO_STREAM_DTS_HD_MASTER_AUDIO,		DTS_AUDIO	},
	{ PES_PRIVATE,							DTS_AUDIO	},
	// DTS Express
	{ SECONDARY_AUDIO_DTS_HD,				DTS_EXPRESS_AUDIO },
	// LPCM Audio
	{ AUDIO_STREAM_LPCM,					LPCM_AUDIO	},
	// Opus Audio
	{ PES_PRIVATE,							OPUS_AUDIO	},
	// MPEG2 Video
	{ VIDEO_STREAM_MPEG2,					MPEG2_VIDEO	},
	{ VIDEO_STREAM_MPEG2_ADDITIONAL_VIEW,	MPEG2_VIDEO	},
	// H.264/AVC1 Video
	{ VIDEO_STREAM_H264,					H264_VIDEO	},
	{ VIDEO_STREAM_H264_ADDITIONAL_VIEW,	H264_VIDEO	},
	{ VIDEO_STREAM_H264_MVC,				H264_VIDEO	},
	// VC-1 Video
	{ VIDEO_STREAM_VC1,						VC1_VIDEO	},
	// Dirac Video
	{ VIDEO_STREAM_DIRAC,					DIRAC_VIDEO	},
	// H.265/HEVC Video
	{ VIDEO_STREAM_HEVC,					HEVC_VIDEO	},
	{ PES_PRIVATE,							HEVC_VIDEO	},
	// PGS Subtitle
	{ PRESENTATION_GRAPHICS_STREAM,			PGS_SUB		},
	// DVB Subtitle
	{ PES_PRIVATE,							DVB_SUB		},
	// Teletext Subtitle
	{ PES_PRIVATE,							TELETEXT_SUB}
};

static const struct {
	DWORD                           codec_tag;
	CMpegSplitterFile::stream_codec codec;
	ULONGLONG                       stream_type;
} StreamDesc[] = {
	{ 'drac', CMpegSplitterFile::stream_codec::DIRAC, DIRAC_VIDEO },
	{ 'AC-3', CMpegSplitterFile::stream_codec::AC3,   AC3_AUDIO   },
	{ 'DTS1', CMpegSplitterFile::stream_codec::DTS,   DTS_AUDIO   },
	{ 'DTS2', CMpegSplitterFile::stream_codec::DTS,   DTS_AUDIO   },
	{ 'DTS3', CMpegSplitterFile::stream_codec::DTS,   DTS_AUDIO   },
	{ 'EAC3', CMpegSplitterFile::stream_codec::EAC3,  AC3_AUDIO   },
	{ 'HEVC', CMpegSplitterFile::stream_codec::HEVC,  HEVC_VIDEO  },
	{ 'VC-1', CMpegSplitterFile::stream_codec::VC1,   VC1_VIDEO   },
	{ 'Opus', CMpegSplitterFile::stream_codec::OPUS,  OPUS_AUDIO  }
};

DWORD CMpegSplitterFile::AddStream(WORD pid, BYTE pesid, BYTE ext_id, DWORD len, BOOL bAddStream/* = TRUE*/)
{
	stream s;
	s.pid   = pid;
	s.pesid = pesid;
	if (m_type != MPEG_TYPES::mpeg_ts) {
		s.ps1id = ext_id;
	}

	if (pesid == 0xbe || pesid == 0xbf) {
		// padding
		// private stream 2
		return s;
	}

	const __int64 start   = GetPos();
	enum stream_type type = stream_type::unknown;

	ULONGLONG stream_type = PES_STREAM_TYPE_ANY;
	PES_STREAM_TYPE pes_stream_type = INVALID;
	if (GetStreamType(s.pid ? s.pid : s.pesid, pes_stream_type)) {
		if (m_ClipInfo.IsHdmv() && pes_stream_type == PRESENTATION_GRAPHICS_STREAM) {
			return s;
		}

		stream_type = 0ULL;

		for (size_t i = 0; i < _countof(PES_types); i++) {
			if (PES_types[i].pes_stream_type == pes_stream_type) {
				stream_type |= PES_types[i].stream_type;
			}
		}
	}

	streamData& _streamData = m_streamData[s];
	if (_streamData.codec != stream_codec::NONE) {
		for (size_t i = 0; i < _countof(StreamDesc); i++) {
			if (StreamDesc[i].codec == _streamData.codec) {
				stream_type = StreamDesc[i].stream_type;
				break;
			}
		}
	}

	if (!m_streams[stream_type::video].Find(s) && !m_streams[stream_type::audio].Find(s)
			&& ((!pesid && pes_stream_type != INVALID)
				|| (pesid >= 0xe0 && pesid < 0xf0)
				|| (pesid >= 0xfa && pesid <= 0xfe)
				&& pesid != 0xfd)) { // mpeg video/audio
		// MPEG2
		if (stream_type & MPEG2_VIDEO) {
			// Sequence/extension header can be split into multiple packets
			if (!m_streams[stream_type::video].Find(s)) {
				seqhdr& h = seqh[s];
				if (h.data.GetCount()) {
					if (h.data.GetCount() < 512) {
						const size_t size = h.data.GetCount();
						h.data.SetCount(size + (size_t)len);
						ByteRead(h.data.GetData() + size, len);
					}
					if (h.data.GetCount() >= 512) {
						if (Read(h, h.data, &s.mt)) {
							s.codec = stream_codec::MPEG;
							type = stream_type::video;
						}
					}
				} else {
					CMediaType mt;
					if (Read(h, len, &mt)) {
						if (m_type == MPEG_TYPES::mpeg_ts) {
							Seek(start);
							h.data.SetCount((size_t)len);
							ByteRead(h.data.GetData(), len);
						} else {
							s.mt = mt;
							s.codec = stream_codec::MPEG;
							type = stream_type::video;
						}
					}
				}
			}
		}

		// AVC/H.264
		if (type == stream_type::unknown && (stream_type & H264_VIDEO)) {
			Seek(start);
			avchdr h;
			if (!m_streams[stream_type::video].Find(s) && !m_streams[stream_type::stereo].Find(s)
					&& Read(h, len, avch[s], &s.mt)) {
				if (s.mt.subtype == MEDIASUBTYPE_AMVC) {
					s.codec = stream_codec::MVC;
					type = stream_type::stereo;
				} else {
					s.codec = stream_codec::H264;
					type = stream_type::video;
				}
			}
		}

		// HEVC/H.265
		if (type == stream_type::unknown && (stream_type & HEVC_VIDEO)) {
			Seek(start);
			hevchdr h;
			if (!m_streams[stream_type::video].Find(s) && Read(h, len, hevch[s], &s.mt)) {
				s.codec = stream_codec::HEVC;
				type = stream_type::video;
			}
		}

		// AAC
		if (type == stream_type::unknown && (stream_type & AAC_AUDIO) && m_type == MPEG_TYPES::mpeg_ts) {
			Seek(start);
			aachdr h;
			if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt, false)) {
				m_aacValid[s].Handle(h);
				if (m_aacValid[s].IsValid()) {
					type = stream_type::audio;
				}
			}
		}
	}

	if (!m_streams[stream_type::audio].Find(s)
			&& ((!pesid && pes_stream_type != INVALID)
				|| pesid >= 0xc0 && pesid < 0xe0)) { // mpeg audio
		// AAC_LATM
		if (type == stream_type::unknown && (stream_type & AAC_AUDIO) && m_type == MPEG_TYPES::mpeg_ts) {
			Seek(start);
			latm_aachdr h;
			if (Read(h, len, &s.mt)) {
				m_aaclatmValid[s].Handle(h);
				if (m_aaclatmValid[s].IsValid()) {
					type = stream_type::audio;
				}
			}
		}

		// AAC
		if (type == stream_type::unknown && (stream_type & AAC_AUDIO)) {
			Seek(start);
			aachdr h;
			if (Read(h, len, &s.mt)) {
				m_aacValid[s].Handle(h);
				if (m_aacValid[s].IsValid()) {
					type = stream_type::audio;
				}
			}
		}

		// MPEG Audio
		if (type == stream_type::unknown && (stream_type & MPEG_AUDIO)) {
			Seek(start);
			mpahdr h;
			if (Read(h, len, &s.mt, false, true)) {
				m_mpaValid[s].Handle(h);
				if (m_mpaValid[s].IsValid()) {
					type = stream_type::audio;
				}
			}
		}

		// AC3
		if (type == stream_type::unknown && (stream_type & AC3_AUDIO)) {
			Seek(start);
			ac3hdr h;
			if (Read(h, len, &s.mt)) {
				m_ac3Valid[s].Handle(h);
				if (m_ac3Valid[s].IsValid()) {
					type = stream_type::audio;
				}
			}
		}

		// ADX ADPCM
		if (type == unknown && m_type == MPEG_TYPES::mpeg_ps) {
			Seek(start);
			adx_adpcm_hdr h;
			if (Read(h, len, &s.mt)) {
				type = audio;
			}
		}

		// A-law PCM
		if (type == unknown && m_type == MPEG_TYPES::mpeg_ps
				&& m_bIMKH_CCTV && pesid == 0xc0
				&& stream_type != MPEG_AUDIO && stream_type != AAC_AUDIO) {
			pcm_law_hdr h;
			if (Read(h, len, true, &s.mt)) {
				type = audio;
			}
		}
	}

	if (pesid == 0xbd || pesid == 0xfd) { // private stream 1
		if (s.pid) {
			if (!m_streams[stream_type::video].Find(s) && !m_streams[stream_type::audio].Find(s) && !m_streams[stream_type::subpic].Find(s)) {
				// AAC_LATM
				if (type == stream_type::unknown && (stream_type & AAC_AUDIO) && m_type == MPEG_TYPES::mpeg_ts) {
					Seek(start);
					latm_aachdr h;
					if (Read(h, len, &s.mt)) {
						m_aaclatmValid[s].Handle(h);
						if (m_aaclatmValid[s].IsValid()) {
							type = stream_type::audio;
						}
					}
				}

				// AC3, E-AC3, TrueHD
				if (type == stream_type::unknown && (stream_type & AC3_AUDIO)) {
					Seek(start);
					ac3hdr h;
					if (pes_stream_type == AUDIO_STREAM_AC3_TRUE_HD) {
						if (m_AC3CoreOnly) {
							if (ext_id == 0x76 && Read(h, len, &s.mt)) { // AC3 sub-stream
								type = stream_type::audio;
							}
						} else {
							if (ext_id == 0x72 && Read(h, len, &s.mt, false, false)) { // TrueHD core
								type = stream_type::audio;
							}
						}
					} else if (Read(h, len, &s.mt)) {
						type = stream_type::audio;
					}
				}

				// DTS, DTS HD, DTS HD MA
				if (type == stream_type::unknown && (stream_type & DTS_AUDIO)) {
					Seek(start);
					dtshdr h;
					if (Read(h, len, &s.mt, false)) {
						s.dts.bDTSCore = true;
						type = stream_type::audio;
					}
				}

				// DTS Express
				if (type == stream_type::unknown && (stream_type & DTS_EXPRESS_AUDIO)) {
					Seek(start);
					dtslbr_hdr h;
					if (Read(h, len, &s.mt)) {
						s.dts.bDTSHD = true;
						type = stream_type::audio;
					}
				}

				// OPUS
				if (type == stream_type::unknown && stream_type == OPUS_AUDIO) {
					Seek(start);
					opus_ts_hdr h;
					if (Read(h, len, _streamData.pmt.extraData, &s.mt)) {
						type = stream_type::audio;
					}
				}

				// VC1
				if (type == stream_type::unknown && (stream_type & VC1_VIDEO)) {
					Seek(start);
					vc1hdr h;
					if (Read(h, len, &s.mt)) {
						s.codec = stream_codec::VC1;
						type = stream_type::video;
					}
				}

				// DIRAC
				if (type == stream_type::unknown && (stream_type & DIRAC_VIDEO)) {
					Seek(start);
					dirachdr h;
					if (Read(h, len, &s.mt)) {
						type = stream_type::video;
					}
				}

				// DVB subtitles
				if (type == stream_type::unknown && (stream_type & DVB_SUB)) {
					Seek(start);
					dvbsubhdr h;
					if (Read(h, len, &s.mt, _streamData.pmt.lang)) {
						s.codec = stream_codec::DVB;
						type = stream_type::subpic;
					}
				}

				// Teletext subtitles
				if (type == stream_type::unknown && pesid == 0xbd && (stream_type & TELETEXT_SUB)) {
					Seek(start);
					teletextsubhdr h;
					if (Read(h, len, &s.mt, _streamData.pmt.lang)) {
						s.codec = stream_codec::TELETEXT;
						type = stream_type::subpic;
					}
				}

				// LPCM Audio
				if (type == stream_type::unknown && (stream_type & LPCM_AUDIO)) {
					Seek(start);
					if (ReadHDMVLPCMHdr(&s.mt)) {
						type = stream_type::audio;
					}
				}

				// PGS subtitles
				if (type == stream_type::unknown && (stream_type & PGS_SUB)) {
					Seek(start);

					int nProgram;
					const CHdmvClipInfo::Stream *pClipInfo;
					const program* pProgram = FindProgram(s.pid, &nProgram, &pClipInfo);

					hdmvsubhdr h;
					if (Read(h, &s.mt, pClipInfo ? pClipInfo->m_LanguageCode : _streamData.pmt.lang)) {
						s.codec = stream_codec::PGS;
						type = stream_type::subpic;
					}
				}
			} else if (!m_bOpeningCompleted && type == stream_type::unknown && (pes_stream_type == AUDIO_STREAM_DTS_HD || pes_stream_type == AUDIO_STREAM_DTS_HD_MASTER_AUDIO)) {
				stream* source = (stream*)m_streams[stream_type::audio].FindStream(s);
				if (source && source->dts.bDTSCore && !source->dts.bDTSHD && source->mt.pbFormat) {
					Seek(start);
					if (BitRead(32, true) == FCC(DTS_SYNCWORD_SUBSTREAM)) {
						BYTE* buf = DNew BYTE[len];
						audioframe_t aframe;
						if (ByteRead(buf, len) == S_OK && ParseDTSHDHeader(buf, len, &aframe)) {
							WAVEFORMATEX* wfe = (WAVEFORMATEX*)source->mt.pbFormat;
							wfe->nSamplesPerSec = aframe.samplerate;
							wfe->nChannels = aframe.channels;
							if (aframe.param1) {
								wfe->wBitsPerSample = aframe.param1;
							}
							if (aframe.param2 == DCA_PROFILE_HD_HRA) {
								wfe->nAvgBytesPerSec += CalcBitrate(aframe) / 8;
							} else {
								wfe->nAvgBytesPerSec = 0;
							}

							source->dts.bDTSHD	= true;
						}
						delete [] buf;
					}
				}
			}
		} else if (pesid == 0xfd) {
			vc1hdr h;
			if (!m_streams[stream_type::video].Find(s) && Read(h, len, &s.mt)) {
				s.codec = stream_codec::VC1;
				type = stream_type::video;
			}
		} else {
			BYTE b   = (BYTE)BitRead(8, true);
			WORD w   = (WORD)BitRead(16, true);
			DWORD dw = (DWORD)BitRead(32, true);

			if (b >= 0x80 && b < 0x88 || w == 0x0b77) { // ac3
				s.ps1id = (b >= 0x80 && b < 0x88) ? (BYTE)(BitRead(32) >> 24) : 0x80;

				ac3hdr h = { 0 };
				if (!m_streams[stream_type::audio].Find(s)) {
					if (Read(h, len, &s.mt)) {
						m_ac3Valid[s].Handle(h);
						if (m_ac3Valid[s].IsValid()) {
							type = stream_type::audio;
						}
					}
				}
			} else if (b >= 0x88 && b < 0x90 || dw == 0x7ffe8001) { // dts
				s.ps1id = (b >= 0x88 && b < 0x90) ? (BYTE)(BitRead(32) >> 24) : 0x88;

				dtshdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (b >= 0xa0 && b < 0xa8) { // lpcm
				s.ps1id = (BYTE)BitRead(8);

				if (auto stream = m_streams[stream_type::audio].FindStream(s)) {
					switch (stream->codec) {
						case stream_codec::DVDAudio: {
								BitRead(8);
								const WORD headersize = (WORD)BitRead(16);
								Seek(start + 4 + headersize);
							}
							break;
						case stream_codec::MLP:
							Seek(start + 10);
							break;
						case stream_codec::DVDLPCM:
							Seek(start + 7);
							break;
					}
				} else {
					do {
						// DVD-Audio LPCM
						if (b == 0xa0) {
							BitRead(8); // Continuity Counter - counts from 0x00 to 0x1f and then wraps to 0x00.
							const DWORD headersize = (DWORD)BitRead(16); // LPCM_header_length
							if (headersize >= 8 && headersize + 4 < len) {
								if (ReadDVDALPCMHdr(&s.mt)) {
									Seek(start + 4 + headersize);
									s.codec = stream_codec::DVDAudio;
									type = stream_type::audio;
									break;
								}
							}
						}
						// DVD-Audio MLP
						else if (b == 0xa1 && len > 10) {
							BitRead(8); // Continuity Counter: 0x00..0x1f or 0x20..0x3f or 0x40..0x5f or 0x80..0x9f
							BitRead(8); // some unknown data
							const BYTE headersize = (BYTE)BitRead(8); // MLP_header_length (always equal 6?)
							BitRead(32); // some unknown data
							BitRead(16); // some unknown data
							if (headersize == 6) { // MLP?
								// MLP header may be missing in the first package
								mlphdr h;
								if (Read(h, len - 10, &s.mt, true)) {
									// This is exactly the MLP.
									s.codec = stream_codec::MLP;
									type = stream_type::audio;
								}
								
								Seek(start + 10);
								break;
							}
						}

						// DVD LPCM
						Seek(start + 4);
						if (ReadDVDLPCMHdr(&s.mt)) {
							s.codec = stream_codec::DVDLPCM;
							type = stream_type::audio;
						}
					} while (false);
				}
			} else if (b >= 0x20 && b < 0x40) { // DVD subpic
				s.ps1id = (BYTE)BitRead(8);

				dvdspuhdr h;
				if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
					type = stream_type::subpic;
				}
			} else if (b >= 0x70 && b < 0x80) { // SVCD subpic
				s.ps1id = (BYTE)BitRead(8);

				svcdspuhdr h;
				if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
					type = stream_type::subpic;
				}
			} else if (b >= 0x00 && b < 0x10) { // CVD subpic
				s.ps1id = (BYTE)BitRead(8);

				cvdspuhdr h;
				if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
					type = stream_type::subpic;
				}
			} else if (w == 0xffa0 || w == 0xffa1) { // ps2-mpg audio
				s.ps1id = (BYTE)BitRead(8);
				s.pid   = (WORD)((BitRead(8) << 8) | BitRead(16)); // pid = 0xa000 | track id

				ps2audhdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (w == 0xff90) { // ps2-mpg ac3 or subtitles
				s.ps1id = (BYTE)BitRead(8);
				s.pid   = (WORD)((BitRead(8) << 8) | BitRead(16)); // pid = 0x9000 | track id

				w = (WORD)BitRead(16, true);

				if (w == 0x0b77) {
					ac3hdr h;
					if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt, false)) {
						type = stream_type::audio;
					}
				} else if (w == 0x0000) { // usually zero...
					ps2subhdr h;
					if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
						type = stream_type::subpic;
					}
				}
			} else if (b >= 0xc0 && b < 0xcf) { // dolby digital/dolby digital +
				s.ps1id = (BYTE)BitRead(8);
				// skip audio header - 3-byte
				BitRead(24);
				ac3hdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (b >= 0xb0 && b < 0xbf) { // truehd
				s.ps1id = (BYTE)BitRead(8);
				// TrueHD audio has a 4-byte header
				BitRead(32);
				ac3hdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt, false, false)) {
					type = stream_type::audio;
				}
			}
		}
	}

	if (bAddStream && type != stream_type::unknown && !m_streams[type].Find(s)) {
		if (s.pid) {
			if (!s.lang_set && _streamData.pmt.lang[0]) {
				strcpy_s(s.lang, _streamData.pmt.lang);
				s.lang_set = true;
			}

			for (int t = stream_type::video; t < stream_type::unknown; t++) {
				if (m_streams[t].Find(s)) {
					return s;
				}
			}
		}

		if (type == stream_type::video || type == stream_type::stereo) {
			REFERENCE_TIME rtAvgTimePerFrame = 1;
			ExtractAvgTimePerFrame(&s.mt, rtAvgTimePerFrame);

			if (rtAvgTimePerFrame < 50000) {
				const __int64 _pos = GetPos();
				Seek(m_posMin);

				__int64 nextPos;
				REFERENCE_TIME rt = NextPTS(s, s.codec, nextPos);
				if (rt != INVALID_TIME) {
					std::vector<REFERENCE_TIME> timecodes;
					timecodes.reserve(FrameDuration::DefaultFrameNum);

					Seek(nextPos);
					int count = 0;
					for (;;) {
						rt = NextPTS(s, s.codec, nextPos);
						if (rt == INVALID_TIME) {
							break;
						}

						timecodes.push_back((rt + 100) / 200); // get a real precision for time codes (need for some files)
						if (timecodes.size() >= FrameDuration::DefaultFrameNum) {
							break;
						}

						Seek(nextPos);
					}

					rtAvgTimePerFrame = FrameDuration::Calculate(timecodes, 200);

					if (s.mt.formattype == FORMAT_MPEG2_VIDEO) {
						((MPEG2VIDEOINFO*)s.mt.pbFormat)->hdr.AvgTimePerFrame = rtAvgTimePerFrame;
					} else if (s.mt.formattype == FORMAT_MPEGVideo) {
						((MPEG1VIDEOINFO*)s.mt.pbFormat)->hdr.AvgTimePerFrame = rtAvgTimePerFrame;
					} else if (s.mt.formattype == FORMAT_VIDEOINFO2) {
						((VIDEOINFOHEADER2*)s.mt.pbFormat)->AvgTimePerFrame   = rtAvgTimePerFrame;
					} else if (s.mt.formattype == FORMAT_VideoInfo) {
						((VIDEOINFOHEADER*)s.mt.pbFormat)->AvgTimePerFrame    = rtAvgTimePerFrame;
					}
				}

				Seek(_pos);
			}
		}

		if (m_bOpeningCompleted) {
			s.mts.push_back(s.mt);
		}

		if (!m_bOpeningCompleted || m_streams[type].GetCount() > 0) {
			BOOL bAdded = FALSE;
			if (_streamData.codec == stream_codec::TELETEXT && !_streamData.pmt.tlxPages.empty()) {
				for (auto& tlxPage : _streamData.pmt.tlxPages) {
					if (tlxPage.bSubtitle && tlxPage.page != 0x100) {
						s.tlxPage = tlxPage.page;
						strcpy_s(s.lang, tlxPage.lang);
						FreeMediaType(s.mt);
						s.mt.InitMediaType();

						teletextsubhdr hdr;
						Read(hdr, 0, &s.mt, tlxPage.lang, false);

						m_streams[type].Insert(s, type);
						bAdded = TRUE;
					}
				}
			}
			if (!bAdded) {
				m_streams[type].Insert(s, type);
			}
		}
	}

	return s;
}

void CMpegSplitterFile::AddHdmvPGStream(WORD pid, LPCSTR language_code)
{
	stream s;
	s.pid   = pid;
	s.pesid = 0xbd;

	hdmvsubhdr h;
	if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt, language_code)) {
		m_streams[stream_type::subpic].Insert(s, subpic);
	}
}

CMpegSplitterFile::CStreamList* CMpegSplitterFile::GetMasterStream()
{
	return
		!m_streams[stream_type::video].IsEmpty()  ? &m_streams[stream_type::video]  :
		!m_streams[stream_type::audio].IsEmpty()  ? &m_streams[stream_type::audio]  :
		!m_streams[stream_type::subpic].IsEmpty() ? &m_streams[stream_type::subpic] :
		nullptr;
}

void CMpegSplitterFile::ReadPrograms(const trhdr& h)
{
	if (h.bytes <= 9) {
		return;
	}

	auto it = m_ProgramData.find(h.pid);
	if (it != m_ProgramData.end() && it->second.bFinished) {
		return;
	}

	programData* ProgramData = (it != m_ProgramData.end()) ? &it->second : nullptr;
	if (h.payload && h.payloadstart) {
		psihdr h2;
		if (ReadPSI(h2)) {
			switch (h2.table_id) {
				case DVB_SI::SI_PAT:
					if (h.pid != MPEG2_PID::PID_PAT) {
						return;
					}
					break;
				case DVB_SI::SI_SDT:
				case 0x46:
					if (m_programs.empty() || h.pid != MPEG2_PID::PID_SDT) {
						return;
					}
					break;
				case 0xC8:
				case 0xC9:
				case 0xDA:
					if (m_programs.empty() || h.pid != MPEG2_PID::PID_VCT) {
						return;
					}
					break;
				case DVB_SI::SI_PMT:
					break;
				default:
					return;
			}

			if (h2.section_syntax_indicator == 1) {
				h2.section_length -= 4; // Reserving size of CRC32
			}

			const int packet_len = h.bytes - h2.hdr_size;
			const int max_len    = std::min(packet_len, h2.section_length);

			if (max_len > 0) {
				ProgramData = &m_ProgramData[h.pid];

				ProgramData->table_id       = h2.table_id;
				ProgramData->section_length = h2.section_length;

				ProgramData->pData.SetCount(max_len);
				ByteRead(ProgramData->pData.GetData(), max_len);
			}
		}
	} else if (ProgramData && !ProgramData->pData.IsEmpty()) {
		const size_t data_len = ProgramData->pData.GetCount();
		const size_t max_len  = std::min((size_t)h.bytes, ProgramData->section_length - data_len);

		ProgramData->pData.SetCount(data_len + max_len);
		ByteRead(ProgramData->pData.GetData() + data_len, max_len);
	}

	if (ProgramData && ProgramData->IsFull()) {
		switch (ProgramData->table_id) {
			case DVB_SI::SI_PAT:
				ReadPAT(ProgramData->pData);
				break;
			case DVB_SI::SI_PMT:
				ReadPMT(ProgramData->pData, h.pid);
				break;
			case DVB_SI::SI_SDT: // DVB - Service Description Table - actual_transport_stream
			case 0x46:           // DVB - Service Description Table - other_transport_stream
				ReadSDT(ProgramData->pData);
				break;
			case 0xC8: // ATSC - Terrestrial Virtual Channel Table (TVCT)
			case 0xC9: // ATSC - Cable Virtual Channel Table (CVCT) / Long-form Virtual Channel Table (L-VCT)
			case 0xDA: // ATSC - Satellite VCT (SVCT)
				ReadVCT(ProgramData->pData, ProgramData->table_id);
				break;
		}

		ProgramData->Finish();
	}
}

void CMpegSplitterFile::ReadPAT(CAtlArray<BYTE>& pData)
{
	CGolombBuffer gb(pData.GetData(), pData.GetCount());
	int len = gb.GetSize();

	for (int i = len/4; i > 0; i--) {
		WORD program_number = (WORD)gb.BitRead(16);
		BYTE reserved = (BYTE)gb.BitRead(3);
		WORD pid = (WORD)gb.BitRead(13);
		UNREFERENCED_PARAMETER(reserved);
		if (program_number != 0) {
			m_programs[pid].program_number = program_number;
		}
	}
}

static const char und[4] = "und";

// ISO 639 language descriptor
static void Descriptor_0A(CGolombBuffer& gb, LPSTR ISO_639_language_code)
{
	const char ch[4] = {};
	gb.ReadBuffer((BYTE *)ch, 3); // ISO 639 language code
	gb.BitRead(8);                // audio type

	if (ch[0] && strncmp(ch, und, 3)) {
		strcpy_s(ISO_639_language_code, 4, ch);
	}
}

// Teletext descriptor
static void Descriptor_56(CGolombBuffer& gb, int descriptor_length, LPSTR ISO_639_language_code, CMpegSplitterFile::teletextPages& tlxPages)
{
	tlxPages.clear();

	const int section_len = 5;
	int pos = 0;
	while (pos <= descriptor_length - section_len) {
		const char ch[4] = {};
		gb.ReadBuffer((BYTE *)ch, 3); // ISO 639 language code
		BYTE teletext_type            = gb.BitRead(5);
		BYTE teletext_magazine_number = gb.BitRead(3);
		BYTE teletext_page_number_1   = gb.BitRead(4);
		BYTE teletext_page_number_2   = gb.BitRead(4);

		if (!ISO_639_language_code[0] && ch[0] && strncmp(ch, und, 3)) {
			strcpy_s(ISO_639_language_code, 4, ch);
		}

		CMpegSplitterFile::teletextPage tlxPage;

		if (teletext_magazine_number == 0) teletext_magazine_number = 8;
		tlxPage.bSubtitle = (teletext_type == 0x02 || teletext_type == 0x05);
		tlxPage.page = (teletext_magazine_number << 8) | (teletext_page_number_1 << 4) | teletext_page_number_2;
		strcpy_s(tlxPage.lang, ch);

		if (std::find(tlxPages.begin(), tlxPages.end(), tlxPage) == tlxPages.end()) {
			tlxPages.emplace_back(tlxPage);
		}

		pos += section_len;
	}

#ifdef DEBUG_OR_LOG
	if (!tlxPages.empty()) {
		DLog(L"ReadPMT() : found %Iu teletext pages", tlxPages.size());
		for (auto& tlxPage : tlxPages) {
			DLog(L"    => 0x%03x - '%S', %s", tlxPage.page, tlxPage.lang, tlxPage.bSubtitle ? L"subtitle" : L"teletext");
		}
	}
#endif
}

// Subtitling descriptor
static void Descriptor_59(CGolombBuffer& gb, int descriptor_length, LPSTR ISO_639_language_code)
{
	const int section_len = 8;
	int pos = 0;
	while (pos <= descriptor_length - section_len) {
		const char ch[4] = {};
		gb.ReadBuffer((BYTE *)ch, 3); // ISO 639 language code
		gb.BitRead(8);                // subtitling type
		gb.BitRead(16);               // composition pageid
		gb.BitRead(16);               // ancillary page id

		if (!ISO_639_language_code[0] && ch[0] && strncmp(ch, und, 3)) {
			strcpy_s(ISO_639_language_code, 4, ch);
		}

		pos += section_len;
	}
}

static const BYTE opus_default_extradata[30] = {
	'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const BYTE opus_coupled_stream_cnt[9] = {
	1, 0, 1, 1, 2, 2, 2, 3, 3
};

static const BYTE opus_stream_cnt[9] = {
	1, 1, 1, 2, 2, 3, 4, 4, 5,
};

static const BYTE opus_channel_map[8][8] = {
	{ 0 },
	{ 0,1 },
	{ 0,2,1 },
	{ 0,1,2,3 },
	{ 0,4,1,2,3 },
	{ 0,4,1,2,3,5 },
	{ 0,4,1,2,3,5,6 },
	{ 0,6,1,2,3,4,5,7 },
};

struct MP4Descr {
	WORD active_es_id = 0;
	CAtlMap<WORD, CAtlArray<BYTE>> dec_config_descr;

	void Clear() {
		active_es_id = 0;
		dec_config_descr.RemoveAll();
	}
};

MP4Descr mp4_descr;

#define MP4ODescrTag           0x01
#define MP4IODescrTag          0x02
#define MP4ESDescrTag          0x03
#define MP4DecConfigDescrTag   0x04
#define MP4DecSpecificDescrTag 0x05
#define MP4SLDescrTag          0x06

static int mp4_read_descr_len(CGolombBuffer& gb, int& len)
{
	int descr_len = 0;
	int count = 4;
	while (count--) {
		const int c = gb.BitRead(8); len--;
		descr_len = (descr_len << 7) | (c & 0x7f);
		if (!(c & 0x80)) {
			break;
		}
	}

	return descr_len;
}

static void mp4_parse_es_descr(CGolombBuffer& gb, int& len, WORD& es_id)
{
	es_id = gb.BitRead(16); len -= 2;
	const int flags = gb.BitRead(8); len -= 1;
	if (flags & 0x80) { // streamDependenceFlag
		gb.BitRead(16); len -= 2;
	}
	if (flags & 0x40) { // URL_Flag
		const int _len = gb.BitRead(8); len -= 1;
		gb.SkipBytes(_len); len -= _len;
	}
	if (flags & 0x20) { // OCRstreamFlag
		gb.BitRead(16); len -= 2;
	}
}

static int mp4_read_descr(CGolombBuffer& gb, int& tag, int& len)
{
	tag = gb.BitRead(8); len--;
	return mp4_read_descr_len(gb, len);
}

static int parse_mp4_descr(CGolombBuffer& gb, int& len, int target_tag);

static int parse_mp4_descr_arr(CGolombBuffer& gb, int& len)
{
	while (len > 0) {
		const int ret = parse_mp4_descr(gb, len, 0);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int parse_MP4IODescrTag(CGolombBuffer& gb, int& len)
{
	gb.SkipBytes(7); len -= 7;
	return parse_mp4_descr_arr(gb, len);
}

static int parse_MP4ESDescrTag(CGolombBuffer& gb, int& len)
{
	WORD es_id = 0;
	int ret    = 0;
	mp4_parse_es_descr(gb, len, es_id);

	mp4_descr.active_es_id = es_id;

	if ((ret = parse_mp4_descr(gb, len, MP4DecConfigDescrTag)) < 0) {
		return ret;
	}
	if (len > 0) {
		ret = parse_mp4_descr(gb, len, MP4SLDescrTag);
	}

	return ret;
}

static int parse_MP4DecConfigDescrTag(CGolombBuffer& gb, int& len)
{
	CAtlArray<BYTE>& pData = mp4_descr.dec_config_descr[mp4_descr.active_es_id];
	pData.SetCount(len);
	gb.ReadBuffer(pData.GetData(), len);
	len = 0;

	return 0;
}

static int parse_MP4SLDescrTag(CGolombBuffer& gb, int& len)
{
	gb.SkipBytes(len);
	len = 0;

	return 0;
}

static int parse_mp4_descr(CGolombBuffer& gb, int& len, int target_tag)
{
	int tag = 0;
	int tag_len = mp4_read_descr(gb, tag, len);

	if (len < 0 || tag_len > len || tag_len <= 0) {
		return -1;
	}

	if (target_tag && target_tag != tag) {
		return -1;
	}

	len -= tag_len;

	int ret = 0;
	switch (tag) {
		case MP4IODescrTag:
			ret = parse_MP4IODescrTag(gb, tag_len);
			break;
		case MP4ODescrTag:
			//ret = parse_MP4ODescrTag(gb, tag_len);
			break;
		case MP4ESDescrTag:
			ret = parse_MP4ESDescrTag(gb, tag_len);
			break;
		case MP4DecConfigDescrTag:
			ret = parse_MP4DecConfigDescrTag(gb, tag_len);
			break;
		case MP4SLDescrTag:
			ret = parse_MP4SLDescrTag(gb, tag_len);
			break;
	}

	return ret;
}

static int mp4_read_dec_config_descr(CAtlArray<BYTE>& dec_config_descr, CMpegSplitterFile::stream& s)
{
	CGolombBuffer gb(dec_config_descr.GetData(), dec_config_descr.GetCount());

	const int object_type_id = gb.BitRead(8);
	if (object_type_id == 0x40 || object_type_id == 0x66 || object_type_id == 0x67 || object_type_id == 0x68) { // AAC codec
		gb.BitRead(8);  // stream type
		gb.BitRead(24); // buffer size db
		gb.BitRead(32); // max_rate
		gb.BitRead(32); // bit_rate

		int len = gb.RemainingSize();
		int tag = 0;
		int tag_len = mp4_read_descr(gb, tag, len);
		if (len && tag_len && tag_len <= len && tag == MP4DecSpecificDescrTag) {
			const BYTE* extra = gb.GetBufferPos();
			int samplingFrequency, channels;
			if (ReadAudioConfig(gb, samplingFrequency, channels)) {
				s.codec = CMpegSplitterFile::stream_codec::AAC_RAW;

				CMediaType* pmt = &s.mt;

				pmt->majortype       = MEDIATYPE_Audio;
				pmt->subtype         = MEDIASUBTYPE_RAW_AAC1;
				pmt->formattype      = FORMAT_WaveFormatEx;

				WAVEFORMATEX* wfe    = (WAVEFORMATEX*)DNew BYTE[sizeof(WAVEFORMATEX) + tag_len];
				memset(wfe, 0, sizeof(WAVEFORMATEX));
				wfe->wFormatTag      = WAVE_FORMAT_RAW_AAC1;
				wfe->nChannels       = channels;
				wfe->nSamplesPerSec  = samplingFrequency;
				wfe->nBlockAlign     = 1;
				wfe->nAvgBytesPerSec = 0;
				wfe->cbSize          = tag_len;
				if (tag_len) {
					memcpy((BYTE*)(wfe + 1), extra, tag_len);
				}

				pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

				delete [] wfe;

				return 0;
			}
		}
	}

	return -1;
}

void CMpegSplitterFile::ReadPMT(CAtlArray<BYTE>& pData, WORD pid)
{
	mp4_descr.Clear();

	auto& pr = m_programs[pid];

	CGolombBuffer gb(pData.GetData(), pData.GetCount());
	int len = gb.GetSize();

	BYTE reserved1           = (BYTE)gb.BitRead(3);
	WORD PCR_PID             = (WORD)gb.BitRead(13);
	BYTE reserved2           = (BYTE)gb.BitRead(4);
	WORD program_info_length = (WORD)gb.BitRead(12);
	UNREFERENCED_PARAMETER(reserved1);
	UNREFERENCED_PARAMETER(PCR_PID);
	UNREFERENCED_PARAMETER(reserved2);

	len -= (4 + program_info_length);
	if (len <= 0)
		return;

	if (program_info_length) {
		const int end_pos = gb.GetPos() + program_info_length;
		while (program_info_length >= 2) {
			int tag = gb.BitRead(8);
			int len = gb.BitRead(8);
			if (len > program_info_length - 2) {
				break;
			}
			program_info_length -= len + 2;

			if (tag == 0x1d) { // IOD descriptor
				gb.BitRead(8);
				gb.BitRead(8);
				len -= 2;

				parse_mp4_descr(gb, len, MP4IODescrTag);
			}

			gb.SkipBytes(len);
		}

		gb.Seek(end_pos);
	}

	while (len >= 5) {
		PES_STREAM_TYPE stream_type = (PES_STREAM_TYPE)gb.BitRead(8);
		BYTE nreserved1             = (BYTE)gb.BitRead(3);
		WORD pid                    = (WORD)gb.BitRead(13);
		BYTE nreserved2             = (BYTE)gb.BitRead(4);
		WORD ES_info_length         = (WORD)gb.BitRead(12);
		UNREFERENCED_PARAMETER(nreserved1);
		UNREFERENCED_PARAMETER(nreserved2);

		len -= 5;
		if (ES_info_length > len) {
			break;
		}
		len -= ES_info_length;

		if (stream_type != PES_STREAM_TYPE::INVALID && !pr.streamFind(pid)) {
			program::stream s;
			s.pid  = pid;
			s.type = stream_type;

			pr.streams.push_back(s);
		}

		streamData& _streamData = m_streamData[pid];
		stream s_aac_mp4;

		while (ES_info_length >= 2) {
			BYTE descriptor_tag    = (BYTE)gb.BitRead(8);
			BYTE descriptor_length = (BYTE)gb.BitRead(8);
			ES_info_length -= 2;

			if (!descriptor_length) {
				continue;
			}
			if (descriptor_length > ES_info_length) {
				break;
			}
			ES_info_length -= descriptor_length;

			char ISO_639_language_code[4] = {};
			teletextPages tlxPages;
			switch (descriptor_tag) {
				case 0x05: // registration descriptor
					{
						const DWORD codec_tag = gb.ReadDword(); descriptor_length -= 4;
						if (descriptor_length) {
							gb.SkipBytes(descriptor_length);
						}
						for (size_t i = 0; i < _countof(StreamDesc); i++) {
							if (codec_tag == StreamDesc[i].codec_tag) {
								_streamData.codec = StreamDesc[i].codec;
								break;
							}
						}
					}
					break;
				case 0x0a: // ISO 639 language descriptor
					Descriptor_0A(gb, ISO_639_language_code);
					break;
				case 0x1E: // SL descriptor
				case 0x1F: // FMC descriptor
					{
						WORD es_id = gb.BitRead(16); descriptor_length -= 2;
						if (descriptor_length) {
							gb.SkipBytes(descriptor_length);
						}
						if (es_id && !mp4_descr.dec_config_descr.IsEmpty()) {
							CAtlArray<BYTE>& dec_config_descr = mp4_descr.dec_config_descr[es_id];
							if (!dec_config_descr.IsEmpty()) {
								s_aac_mp4.pid = pid;
								if (!m_streams[stream_type::audio].Find(s_aac_mp4)) {
									if (mp4_read_dec_config_descr(dec_config_descr, s_aac_mp4) == 0) {
										_streamData.codec = stream_codec::AAC_RAW;
									}
								}
							}
						}
					}
					break;
				case 0x56: // Teletext descriptor
					Descriptor_56(gb, descriptor_length, ISO_639_language_code, tlxPages);
					_streamData.codec = stream_codec::TELETEXT;
					break;
				case 0x59: // Subtitling descriptor
					Descriptor_59(gb, descriptor_length, ISO_639_language_code);
					_streamData.codec = stream_codec::DVB;
					break;
				case 0x7f: // DVB extension descriptor
					{
						const BYTE ext_desc_tag = gb.BitRead(8); descriptor_length -= 1;
						if (ext_desc_tag == 0x80) {
							int channel_config_code = gb.BitRead(8); descriptor_length -= 1;
							if (channel_config_code >= 0 && channel_config_code <= 0x8) {
								CAtlArray<BYTE>& extradata = _streamData.pmt.extraData;
								if (extradata.GetCount() != sizeof(opus_default_extradata)) {
									extradata.SetCount(sizeof(opus_default_extradata));
									memcpy(extradata.GetData(), &opus_default_extradata, sizeof(opus_default_extradata));

									BYTE channels = 0;
									extradata[9]  = channels = channel_config_code ? channel_config_code : 2;
									extradata[18] = channel_config_code ? (channels > 2) : 255;
									extradata[19] = opus_stream_cnt[channel_config_code];
									extradata[20] = opus_coupled_stream_cnt[channel_config_code];
									if (channels >= 1) {
										memcpy(&extradata[21], opus_channel_map[channels - 1], channels);
									}

									_streamData.codec = stream_codec::OPUS;
								}
							}
						}
						if (descriptor_length) {
							gb.SkipBytes(descriptor_length);
						}
					}
					break;
				case 0xfd: // Data Component descriptor
					{
						const SHORT data_component_id = gb.ReadShort(); descriptor_length -= 2;
						if (descriptor_length) {
							gb.SkipBytes(descriptor_length);
						}
						if (data_component_id == 0x0008) {
							// ARIB Subtitle
							_streamData.codec = stream_codec::ARIB;
						}
					}
					break;
				default:
					gb.SkipBytes(descriptor_length);
					break;
			}

			if (ISO_639_language_code[0]) {
				strcpy_s(_streamData.pmt.lang, ISO_639_language_code);
			}
			if (!tlxPages.empty()) {
				_streamData.pmt.tlxPages = tlxPages;
			}
		}

		if (ES_info_length) {
			gb.SkipBytes(ES_info_length);
		}

		if (s_aac_mp4.codec == stream_codec::AAC_RAW) {
			if (_streamData.pmt.lang[0]) {
				strcpy_s(s_aac_mp4.lang, _streamData.pmt.lang);
				s_aac_mp4.lang_set = true;
			}
			m_streams[stream_type::audio].Insert(s_aac_mp4, stream_type::audio);
		}

		if (m_ForcedSub) {
			if (stream_type == PRESENTATION_GRAPHICS_STREAM
					|| _streamData.codec == stream_codec::DVB
					|| _streamData.codec == stream_codec::TELETEXT) {
				stream s;
				s.pid = pid;

				if (!m_streams[stream_type::subpic].Find(s)) {
					s.codec = _streamData.codec;
					if (_streamData.pmt.lang[0]) {
						strcpy_s(s.lang, _streamData.pmt.lang);
						s.lang_set = true;
					}

					if (stream_type == PRESENTATION_GRAPHICS_STREAM) {
						hdmvsubhdr hdr;
						if (Read(hdr, &s.mt, _streamData.pmt.lang)) {
							m_streams[stream_type::subpic].Insert(s, stream_type::subpic);
							s.codec = _streamData.codec = stream_codec::PGS;
						}
					} else if (_streamData.codec == stream_codec::DVB) {
						dvbsubhdr hdr;
						if (Read(hdr, 0, &s.mt, _streamData.pmt.lang, false)) {
							m_streams[stream_type::subpic].Insert(s, stream_type::subpic);
						}
					} else if (_streamData.codec == stream_codec::TELETEXT) {
						BOOL bAdded = FALSE;
						teletextsubhdr hdr;
						if (!_streamData.pmt.tlxPages.empty()) {
							for (auto& tlxPage : _streamData.pmt.tlxPages) {
								if (tlxPage.bSubtitle && tlxPage.page != 0x100) {
									s.tlxPage = tlxPage.page;
									strcpy_s(s.lang, tlxPage.lang);
									FreeMediaType(s.mt);
									s.mt.InitMediaType();

									Read(hdr, 0, &s.mt, _streamData.pmt.lang, false);
									m_streams[stream_type::subpic].Insert(s, stream_type::subpic);

									bAdded = TRUE;
								}
							}
						}
						if (!bAdded) {
							Read(hdr, 0, &s.mt, _streamData.pmt.lang, false);
							m_streams[stream_type::subpic].Insert(s, stream_type::subpic);
						}
					}
				}
			}
		}
	}

	mp4_descr.Clear();
}

CString ConvertDVBString(const BYTE* pBuffer, int nLength)
{
	static const UINT16 codepages[0x20] = {
		28591, // 00 - ISO 8859-1 Latin I
		28595, // 01 - ISO 8859-5 Cyrillic
		28596, // 02 - ISO 8859-6 Arabic
		28597, // 03 - ISO 8859-7 Greek
		28598, // 04 - ISO 8859-8 Hebrew
		28599, // 05 - ISO 8859-9 Latin 5
		28591, // 06 - ??? - ISO/IEC 8859-10 - Latin alphabet No. 6
		28591, // 07 - ??? - ISO/IEC 8859-11 - Latin/Thai (draft only)
		28591, // 08 - reserved
		28603, // 09 - ISO 8859-13 - Estonian
		28591, // 0a - ??? - ISO/IEC 8859-14 - Latin alphabet No. 8 (Celtic)
		28605, // 0b - ISO 8859-15 Latin 9
		28591, // 0c - reserved
		28591, // 0d - reserved
		28591, // 0e - reserved
		28591, // 0f - reserved
		0,      // 10 - See codepages10 array
		28591, // 11 - ??? - ISO/IEC 10646 - Basic Multilingual Plane (BMP)
		28591, // 12 - ??? - KSX1001-2004 - Korean Character Set
		20936, // 13 - Chinese Simplified (GB2312-80)
		950,   // 14 - Chinese Traditional (Big5)
		28591, // 15 - ??? - UTF-8 encoding of ISO/IEC 10646 - Basic Multilingual Plane (BMP)
		28591, // 16 - reserved
		28591, // 17 - reserved
		28591, // 18 - reserved
		28591, // 19 - reserved
		28591, // 1a - reserved
		28591, // 1b - reserved
		28591, // 1c - reserved
		28591, // 1d - reserved
		28591, // 1e - reserved
		28591  // 1f - TODO!
	};

	static const UINT16 codepages10[0x10] = {
		28591, // 00 - reserved
		28591, // 01 - ISO 8859-1 Western European
		28592, // 02 - ISO 8859-2 Central European
		28593, // 03 - ISO 8859-3 Latin 3
		28594, // 04 - ISO 8859-4 Baltic
		28595, // 05 - ISO 8859-5 Cyrillic
		28596, // 06 - ISO 8859-6 Arabic
		28597, // 07 - ISO 8859-7 Greek
		28598, // 08 - ISO 8859-8 Hebrew
		28599, // 09 - ISO 8859-9 Turkish
		28591, // 0a - ??? - ISO/IEC 8859-10
		28591, // 0b - ??? - ISO/IEC 8859-11
		28591, // 0c - ??? - ISO/IEC 8859-12
		28603, // 0d - ISO 8859-13 Estonian
		28591, // 0e - ??? - ISO/IEC 8859-14
		28605, // 0f - ISO 8859-15 Latin 9

		// 0x10 to 0xFF - reserved for future use
	};

	CString strResult;
	if (nLength > 0) {
		UINT cp = CP_ACP;

		if (pBuffer[0] == 0x10) {
			pBuffer++;
			nLength--;
			if (pBuffer[0] == 0x00) {
				cp = codepages10[pBuffer[1]];
			} else { // if (pBuffer[0] > 0x00)
				// reserved for future use, use default codepage
				cp = codepages[0];
			}
			pBuffer += 2;
			nLength -= 2;
		} else if (pBuffer[0] < 0x20) {
			cp = codepages[pBuffer[0]];
			pBuffer++;
			nLength--;
		} else { // No code page indication, use the default
			cp = codepages[0];
		}

		int nDestSize = MultiByteToWideChar(cp, MB_PRECOMPOSED, (LPCSTR)pBuffer, nLength, nullptr, 0);
		if (nDestSize > 0) {
			MultiByteToWideChar(cp, MB_PRECOMPOSED, (LPCSTR)pBuffer, nLength, strResult.GetBuffer(nLength), nDestSize);
			strResult.ReleaseBuffer(nDestSize);
		}
	}

	return strResult;
}

void CMpegSplitterFile::ReadSDT(CAtlArray<BYTE>& pData)
{
	if (pData.GetCount() < 3) {
		return;
	}

	CGolombBuffer gb(pData.GetData(), pData.GetCount());

	gb.BitRead(16); // original_network_id
	gb.BitRead(8);  // reserved_future_use

	while (gb.RemainingSize() >= 5) {
		WORD program_number = (WORD)gb.BitRead(16);
		gb.BitRead(6); // reserved_future_use
		gb.BitRead(1); // EIT_schedule_flag
		gb.BitRead(1); // EIT_present_following_flag
		gb.BitRead(3); // running_status
		gb.BitRead(1); // free_CA_mode
		int descriptors_size = (int)gb.BitRead(12);

		while (descriptors_size >= 2) {
			BYTE descriptor_tag = gb.ReadByte();
			BYTE descriptor_len = gb.ReadByte();
			descriptors_size -= 2;
			if (descriptor_len > descriptors_size) {
				return;
			}
			descriptors_size -= descriptor_len;

			switch (descriptor_tag) {
				case MPEG2_DESCRIPTOR::DT_SERVICE: {
						gb.ReadByte(); // service_type
						BYTE service_provider_name_length = gb.ReadByte();
						descriptor_len--;
						if (service_provider_name_length >= descriptor_len) {
							return;
						}
						gb.SkipBytes(service_provider_name_length);
						descriptor_len -= service_provider_name_length;

						BYTE service_name_length = gb.ReadByte();
						descriptor_len--;
						if (service_name_length > descriptor_len) {
							return;
						}

						BYTE pBuffer[256] = { 0 };
						gb.ReadBuffer(pBuffer, service_name_length);
						const CString service_name = ConvertDVBString(pBuffer, service_name_length);

						if (!service_name.IsEmpty()) {
							if (auto p = m_programs.FindProgram(program_number)) {
								p->name = service_name;
							}
						}
					}
					break;
				default:
					gb.SkipBytes(descriptor_len);
			}
		}
	}
}

// ATSC - service location
static void Descriptor_A1(CGolombBuffer& gb, int descriptor_length, CAtlMap<WORD, char[4]>& ISO_639_languages)
{
	BYTE reserved        = (BYTE)gb.BitRead(3);
	WORD PCR_PID         = (WORD)gb.BitRead(13);
	BYTE number_elements = (BYTE)gb.BitRead(8);
	UNREFERENCED_PARAMETER(reserved);
	UNREFERENCED_PARAMETER(PCR_PID);

	descriptor_length -= 3;

	for (BYTE cnt = 0; cnt < number_elements && descriptor_length >= 6; cnt++) {
		gb.BitRead(8);                   // stream_type
		gb.BitRead(3);                   // reserved

		WORD pid = (WORD)gb.BitRead(13); // elementary_PID
		const char ch[4] = {};
		gb.ReadBuffer((BYTE *)ch, 3);    // ISO 639 language code
		if (ch[0] && strncmp(ch, und, 3)) {
			strcpy_s(ISO_639_languages[pid], ch);
		}

		descriptor_length -= 6;
	}

	if (descriptor_length > 0) {
		gb.SkipBytes(descriptor_length);
	}
}

void CMpegSplitterFile::ReadVCT(CAtlArray<BYTE>& pData, BYTE table_id)
{
	CGolombBuffer gb(pData.GetData(), pData.GetCount());

	gb.BitRead(8); // protocol_version
	BYTE num_channels_in_section = (BYTE)gb.BitRead(8);
	for (BYTE cnt = 0; cnt < num_channels_in_section; cnt++) {
		// name stored in UTF16BE encoding
		int nLength = table_id == 0xDA ? 8 : 7;
		CString short_name;
		gb.ReadBuffer((BYTE*)short_name.GetBufferSetLength(nLength), nLength * 2);
		for (int i = 0, j = short_name.GetLength(); i < j; i++) {
			short_name.SetAt(i, (short_name[i] << 8) | (short_name[i] >> 8));
		}

		gb.BitRead(4);  // reserved
		gb.BitRead(10); // major_channel_number
		gb.BitRead(10); // minor_channel_number

		if (table_id == 0xDA) {
			gb.BitRead(6);  // modulation_mode
			gb.BitRead(32); // carrier_frequency
			gb.BitRead(32); // carrier_symbol_rate
			gb.BitRead(2);  // polarization
			gb.BitRead(8);  // FEC_Inner
		} else {
			gb.BitRead(8);  // modulation_mode
			gb.BitRead(32); // carrier_frequency
		}

		gb.BitRead(16); // channel_TSID

		WORD program_number = (WORD)gb.BitRead(16);
		if (program_number > 0 && program_number < 0x2000) {
			if (auto p = m_programs.FindProgram(program_number)) {
				p->name = short_name;
			}
		}

		gb.BitRead(2); // ETM_location
		gb.BitRead(1); // reserved/access_controlled
		gb.BitRead(1); // hidden

		gb.BitRead(2); // path_select + out_of_band/reserved

		gb.BitRead(1); // hide_guide
		gb.BitRead(3); // reserved
		gb.BitRead(6); // service_type

		gb.BitRead(16); // source_id

		if (table_id == 0xDA) {
			gb.BitRead(8); // feed_id
		}

		gb.BitRead(6); // reserved
		int descriptors_size = (int)gb.BitRead(10);
		if (descriptors_size) {
			while (descriptors_size > 0) {
				BYTE descriptor_tag    = (BYTE)gb.BitRead(8);
				BYTE descriptor_length = (BYTE)gb.BitRead(8);
				descriptors_size      -= (2 + descriptor_length);
				if (descriptors_size < 0) {
					break;
				}

				CAtlMap<WORD, char[4]> ISO_639_languages;

				switch (descriptor_tag) {
					case 0xA1: // ATSC - service location
						Descriptor_A1(gb, descriptor_length, ISO_639_languages);
						break;
					default:
						gb.SkipBytes(descriptor_length);
						break;
				}

				if (!ISO_639_languages.IsEmpty()) {
					POSITION pos = ISO_639_languages.GetStartPosition();
					while (pos) {
						CAtlMap<USHORT, char[4]>::CPair* pPair = ISO_639_languages.GetNext(pos);
						const WORD& pid = pPair->m_key;
						if (!m_streamData[pid].pmt.lang[0]) {
							strcpy_s(m_streamData[pid].pmt.lang, pPair->m_value);
						}
					}
				}
			}
		}
	}
}

const CMpegSplitterFile::program* CMpegSplitterFile::FindProgram(WORD pid, int* pStream/* = nullptr*/, const CHdmvClipInfo::Stream** ppClipInfo/* = nullptr*/)
{
	if (pStream) {
		*pStream = -1;
	}
	if (ppClipInfo) {
		*ppClipInfo = nullptr;
	}

	if (m_type == MPEG_TYPES::mpeg_ts) {
		if (ppClipInfo) {
			*ppClipInfo = m_ClipInfo.FindStream(pid);
		}

		for (auto it = m_programs.begin(); it != m_programs.end(); it++) {
			auto p = &it->second;
			if (p->streamFind(pid, pStream)) {
				return p;
			}
		}
	}

	return nullptr;
}

bool CMpegSplitterFile::GetStreamType(WORD pid, PES_STREAM_TYPE &stream_type)
{
	if (ISVALIDPID(pid)) {
		if (m_type == MPEG_TYPES::mpeg_ts) {
			int nStream;
			const CHdmvClipInfo::Stream *pClipInfo;
			const program* pProgram = FindProgram(pid, &nStream, &pClipInfo);
			if (pClipInfo) {
				stream_type = pClipInfo->m_Type;
			} else if (pProgram) {
				stream_type = pProgram->streams[nStream].type;
			}

			if (stream_type != INVALID) {
				return true;
			}
		} else if (m_type == MPEG_TYPES::mpeg_ps) {
			if (pid < _countof(m_psm) && m_psm[pid] != INVALID) {
				stream_type = m_psm[pid];
				return true;
			}
		}
	}

	return false;
}

void CMpegSplitterFile::UpdatePSM()
{
	WORD psm_length		= (WORD)BitRead(16);
	BitRead(8);
	BitRead(8);
	WORD ps_info_length	= (WORD)BitRead(16);
	Skip(ps_info_length);

	WORD es_map_length	= (WORD)BitRead(16);
	if (es_map_length <= _countof(m_psm) * 4) {
		while (es_map_length > 4) {
			BYTE type			= (BYTE)BitRead(8);
			BYTE es_id			= (BYTE)BitRead(8);
			WORD es_info_length	= (WORD)BitRead(16);

			m_psm[es_id]		= (PES_STREAM_TYPE)type;

			es_map_length		-= 4 + es_info_length;

			Skip(es_info_length);
		}

		BitRead(32);
	}
}

static UINT64 ReadPTS(CBaseSplitterFile* pFile)
{
	#define MARKER(f) if (f->BitRead(1) != 1) { DEBUG_ASSERT(FALSE); };

	UINT64 pts = pFile->BitRead(3) << 30;
	MARKER(pFile) // 32..30
	pts |= pFile->BitRead(15) << 15;
	MARKER(pFile) // 29..15
	pts |= pFile->BitRead(15);
	MARKER(pFile) // 14..0

	return pts;
}

#define MARKER if (BitRead(1) != 1) { DEBUG_ASSERT(FALSE); /*return false;*/} // TODO
bool CMpegSplitterFile::ReadPS(pshdr& h)
{
	memset(&h, 0, sizeof(h));

	BYTE b = (BYTE)BitRead(8, true);

	if ((b & 0xf1) == 0x21) {
		h.type = mpeg1;

		EXECUTE_ASSERT(BitRead(4) == 2);

		h.scr = ReadPTS(this);
		MARKER;
		h.bitrate = BitRead(22);
		MARKER;
	} else if ((b & 0xc4) == 0x44) {
		h.type = mpeg2;

		EXECUTE_ASSERT(BitRead(2) == 1);

		h.scr = ReadPTS(this);
		h.scr = (h.scr * 300 + BitRead(9)) * 10 / 27;
		MARKER;
		h.bitrate = BitRead(22);
		MARKER;
		MARKER;
		BitRead(5); // reserved
		UINT64 stuffing = BitRead(3);
		Skip(stuffing);
		/*
		while (stuffing-- > 0) {
			EXECUTE_ASSERT(BitRead(8) == 0xff);
		}
		*/
	} else {
		return false;
	}

	h.bitrate *= 400;

	return true;
}

bool CMpegSplitterFile::ReadPSS(pssyshdr& h)
{
	memset(&h, 0, sizeof(h));

	WORD len = (WORD)BitRead(16);
	MARKER;
	h.rate_bound = (DWORD)BitRead(22);
	MARKER;
	h.audio_bound = (BYTE)BitRead(6);
	h.fixed_rate = !!BitRead(1);
	h.csps = !!BitRead(1);
	h.sys_audio_loc_flag = !!BitRead(1);
	h.sys_video_loc_flag = !!BitRead(1);
	MARKER;
	h.video_bound = (BYTE)BitRead(5);

	EXECUTE_ASSERT((BitRead(8) & 0x7f) == 0x7f); // reserved (should be 0xff, but not in reality)

	for (len -= 6; len > 3; len -= 3) { // TODO: also store these, somewhere, if needed
		UINT64 stream_id = BitRead(8);
		UNREFERENCED_PARAMETER(stream_id);
		EXECUTE_ASSERT(BitRead(2) == 3);
		UINT64 p_std_buff_size_bound = (BitRead(1) ? 1024 : 128) * BitRead(13);
		UNREFERENCED_PARAMETER(p_std_buff_size_bound);
	}

	return true;
}

#define PTS(pts) (llMulDiv(pts, 10000, 90, 0) + m_rtPTSOffset)
bool CMpegSplitterFile::ReadPES(peshdr& h, BYTE code)
{
	memset(&h, 0, sizeof(h));

	if (!((code >= 0xbd && code < 0xf0) || (code >= 0xfa && code <= 0xfe))) { // 0xfd => blu-ray (.m2ts)
		return false;
	}

	h.len = (WORD)BitRead(16);

	if (code == 0xbe || code == 0xbf) {
		// padding
		// private stream 2
		return true;
	}

	// mpeg1 stuffing (ff ff .. , max 16x)
	for (int i = 0; i < 16 && BitRead(8, true) == 0xff; i++) {
		BitRead(8);
		if (h.len) {
			h.len--;
		}
	}

	h.type = (BYTE)BitRead(2, true) == mpeg2 ? mpeg2 : mpeg1;

	if (h.type == mpeg1) {
		BYTE b = (BYTE)BitRead(2);

		if (b == 1) {
			h.std_buff_size = (BitRead(1) ? 1024 : 128) * BitRead(13);
			if (h.len) {
				h.len -= 2;
			}
			b = (BYTE)BitRead(2);
		}

		if (b == 0) {
			h.fpts = (BYTE)BitRead(1);
			h.fdts = (BYTE)BitRead(1);
		}
	} else if (h.type == mpeg2) {
		EXECUTE_ASSERT(BitRead(2) == mpeg2);
		h.scrambling = (BYTE)BitRead(2);
		h.priority = (BYTE)BitRead(1);
		h.alignment = (BYTE)BitRead(1);
		h.copyright = (BYTE)BitRead(1);
		h.original = (BYTE)BitRead(1);
		h.fpts = (BYTE)BitRead(1);
		h.fdts = (BYTE)BitRead(1);
		h.escr = (BYTE)BitRead(1);
		h.esrate = (BYTE)BitRead(1);
		h.dsmtrickmode = (BYTE)BitRead(1);
		h.morecopyright = (BYTE)BitRead(1);
		h.crc = (BYTE)BitRead(1);
		h.extension = (BYTE)BitRead(1);
		h.hdrlen = (BYTE)BitRead(8);
	} else {
		ASSERT(FALSE);
		goto error;
	}

	if (h.fpts) {
		if (h.type == mpeg2) {
			if (h.fdts != ((BYTE)BitRead(4) & 0x01)) { DEBUG_ASSERT(FALSE); /*goto error;*/} // TODO
		}

		h.pts = PTS(ReadPTS(this));
	}

	if (h.fdts) {
		if (BitRead(4) != 0x01) { DEBUG_ASSERT(FALSE); /*goto error;*/} // TODO

		h.dts = PTS(ReadPTS(this));
	}

	if (h.type == mpeg1) {
		if (!h.fpts && !h.fdts && BitRead(4) != 0xf) { DEBUG_ASSERT(FALSE); /* goto error;*/} // TODO

		if (h.len) {
			h.len--;
			if (h.fpts) {
				h.len -= 4;
			}
			if (h.fdts) {
				h.len -= 5;
			}
		}
	}

	if (h.type == mpeg2) {
		if (h.len) {
			h.len -= 3 + h.hdrlen;
		}

		int left = h.hdrlen;
		if (h.fpts) {
			left -= 5;
		}
		if (h.fdts) {
			left -= 5;
		}

		if (h.extension) { /* PES extension */
			BYTE pes_ext = (BYTE)BitRead(8);
			left--;
			BYTE skip = (pes_ext >> 4) & 0xb;
			skip += skip & 0x9;
			if (pes_ext & 0x40 || skip > left) {
				DLog(L"peshdr read - pes_ext %X is invalid", pes_ext);
				pes_ext = skip = 0;
			}
			Skip(skip);
			left -= skip;
			if (pes_ext & 0x01) { /* PES extension 2 */
				BYTE ext2_len = (BYTE)BitRead(8);
				left--;
				if ((ext2_len & 0x7f) > 0) {
					BYTE id_ext = (BYTE)BitRead(8);
					if ((id_ext & 0x80) == 0) {
						h.id_ext = id_ext;
					}
					left--;
				}
			}
		}

		if (left < 0) {
			goto error;
		}

		Skip(left);

		if (h.scrambling) {
			goto error;
		}
	}

	return true;

error:
	memset(&h, 0, sizeof(h));
	return false;
}

bool CMpegSplitterFile::ReadPVA(pvahdr& h, bool fSync)
{
	memset(&h, 0, sizeof(h));

	BitByteAlign();

	if (fSync) {
		for (int i = 0; i < 65536; i++) {
			if ((BitRead(64, true) & 0xfffffc00ffe00000i64) == 0x4156000055000000i64) {
				break;
			}
			BitRead(8);
		}
	}

	if ((BitRead(64, true) & 0xfffffc00ffe00000i64) != 0x4156000055000000i64) {
		return false;
	}

	h.hdrpos = GetPos();

	h.sync = (WORD)BitRead(16);
	h.streamid = (BYTE)BitRead(8);
	h.counter = (BYTE)BitRead(8);
	h.res1 = (BYTE)BitRead(8);
	h.res2 = BitRead(3);
	h.fpts = BitRead(1);
	h.postbytes = BitRead(2);
	h.prebytes = BitRead(2);
	h.length = (WORD)BitRead(16);

	if (h.length > 6136) {
		return false;
	}

	__int64 pos = GetPos();

	if (h.streamid == 1 && h.fpts) {
		h.pts = PTS(BitRead(32));
	} else if (h.streamid == 2 && (h.fpts || (BitRead(32, true) & 0xffffffe0) == 0x000001c0)) {
		BYTE b;
		if (!NextMpegStartCode(b, 4)) {
			return false;
		}
		peshdr h2;
		if (!ReadPES(h2, b)) {
			return false;
		}
		// Maybe bug, code before: if (h.fpts = h2.fpts) h.pts = h2.pts;
		h.fpts = h2.fpts;
		if (h.fpts) {
			h.pts = h2.pts;
		}
	}

	BitRead(8 * h.prebytes);

	h.length -= (WORD)(GetPos() - pos);

	return true;
}

bool CMpegSplitterFile::ReadTR(trhdr& h, bool fSync)
{
	memset(&h, 0, sizeof(h));

	BitByteAlign();

	if (m_tslen == 0) {
		__int64 pos = GetPos();
		int count	= 0;

		for (int i = 0; i < 192; i++) {
			if (BitRead(8, true) == 0x47) {
				__int64 pos = GetPos();
				Seek(pos + 188);
				if (BitRead(8, true) == 0x47) {
					if (m_tslen != 188) {
						count = 0;
					}
					m_tslen = 188;    // TS stream
					if (count > 1) {
						break;
					}
					count++;
				} else {
					Seek(pos + 192);
					if (BitRead(8, true) == 0x47) {
						if (m_tslen != 192) {
							count = 0;
						}
						m_tslen = 192;    // M2TS stream
						if (count > 1) {
							break;
						}
						count++;
					}
				}
			} else {
				BitRead(8);
			}
		}

		Seek(pos);

		if (m_tslen == 0) {
			return false;
		}
	}

	if (fSync) {
		for (int i = 0; i < m_tslen; i++) {
			if (BitRead(8, true) == 0x47) {
				if (i == 0) {
					break;
				}
				Seek(GetPos() + m_tslen);
				if (BitRead(8, true) == 0x47) {
					Seek(GetPos() - m_tslen);
					break;
				}
			}

			BitRead(8);

			if (i == m_tslen - 1) {
				return false;
			}
		}
	}

	if (BitRead(8, true) != 0x47) {
		return false;
	}

	h.hdrpos = GetPos();

	h.next = GetPos() + m_tslen;

	h.sync = (BYTE)BitRead(8);
	h.error = BitRead(1);
	h.payloadstart = BitRead(1);
	h.transportpriority = BitRead(1);
	h.pid = BitRead(13);
	h.scrambling = BitRead(2);
	h.adapfield = BitRead(1);
	h.payload = BitRead(1);
	h.counter = BitRead(4);

	h.bytes = 188 - 4;

	if (h.adapfield) {
		h.length = (BYTE)BitRead(8);

		if (h.length > 0) {
			h.discontinuity = BitRead(1);
			h.randomaccess = BitRead(1);
			h.priority = BitRead(1);
			h.fPCR = BitRead(1);
			h.OPCR = BitRead(1);
			h.splicingpoint = BitRead(1);
			h.privatedata = BitRead(1);
			h.extension = BitRead(1);

			int i = 1;

			if (h.fPCR && h.length > 6) {
				h.PCR = BitRead(33);
				BitRead(6);
				UINT64 PCRExt = BitRead(9);
				h.PCR = (h.PCR * 300 + PCRExt) * 10 / 27;
				i += 6;
			}

			h.length = (BYTE)std::min((int)h.length, h.bytes-1);
			for (; i < h.length; i++) {
				BitRead(8);
			}
		}

		h.bytes -= h.length + 1;

		if (h.bytes < 0) {
			ASSERT(0);
			return false;
		}
	}

	return true;
}

bool CMpegSplitterFile::ReadPSI(psihdr& h)
{
	memset(&h, 0, sizeof(h));

	if (BitRead(24, true) == 0x000001) {
		return false;
	}

	BYTE pointer_field           = (BYTE)BitRead(8);
	h.hdr_size++;
	Skip(pointer_field);
	h.hdr_size += pointer_field;

	h.table_id                   = (BYTE)BitRead(8);
	h.section_syntax_indicator   = (BYTE)BitRead(1);
	h.private_bits               = (BYTE)BitRead(1);
	h.reserved1                  = (BYTE)BitRead(2);
	h.section_length_unused      = (BYTE)BitRead(2);
	h.section_length             = (int)BitRead(10);
	h.hdr_size += 3;
	if (h.section_syntax_indicator) {
		h.transport_stream_id    = (WORD)BitRead(16);
		h.reserved2              = (BYTE)BitRead(2);
		h.version_number         = (BYTE)BitRead(5);
		h.current_next_indicator = (BYTE)BitRead(1);
		h.section_number         = (BYTE)BitRead(8);
		h.last_section_number    = (BYTE)BitRead(8);

		h.section_length -= 5;
		h.hdr_size += 5;
	}

	return h.reserved1 == 0x03 && h.section_length_unused == 0x00 && h.reserved2 == 0x03 && ((h.table_id <= 0x06) ? (h.section_syntax_indicator == 1 && h.section_length > 4) : h.section_length > 0);
}
