/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "MpegSplitterFile.h"
#include "../../../DSUtil/AudioParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

CMpegSplitterFile::CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, CHdmvClipInfo &ClipInfo, bool bIsBD, bool ForcedSub, int AC3CoreOnly, bool SubEmptyPin)
	: CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL | FM_FILE_VAR | FM_STREAM)
	, m_type(MPEG_TYPES::mpeg_invalid)
	, m_rate(0)
	, m_bPESPTSPresent(TRUE)
	, m_ClipInfo(ClipInfo)
	, m_bIsBD(bIsBD)
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
			for (trhdr h; cnt < limit && Read(h); cnt++) {
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
		for (pvahdr h; cnt < limit && Read(h); cnt++) {
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
				if (Read(h)) {
					cnt++;
					m_rate = int(h.bitrate/8);
				}
			} else if ((b & 0xe0) == 0xc0 // audio, 110xxxxx, mpeg1/2/3
					   || (b & 0xf0) == 0xe0 // video, 1110xxxx, mpeg1/2
					   //|| (b & 0xbd) == 0xbd // private stream 1, 0xbd, ac3/dts/lpcm/subpic
					   || b == 0xbd) { // private stream 1, 0xbd, ac3/dts/lpcm/subpic
				peshdr h;
				if (Read(h, b) && BitRead(24, true) == 0x000001) {
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

	if (IsRandomAccess()) {
		__int64 len = GetLength();
		len = min(len, 5 * MEGABYTE);

		SearchPrograms(0, len);

		len = GetLength();
		__int64 stop = min(10 * MEGABYTE, len);
		SearchStreams(0, stop);

		int num = min(20, (len - stop) / (512 * KILOBYTE));
		if (num > 0) {
			__int64 step = (len - stop) / num;
			for (int i = 0; i < num; i++) {
				stop += step;
				__int64 start = stop - 256 * KILOBYTE;
				SearchStreams(start, stop);
			}
		}
	}
	else if (IsStreaming()) {
		__int64 len = GetAvailable();
		int n = 0;
		while (len < 512 * KILOBYTE && ++n < 10) { // wait 512 KB but no more 1 seconds
			Sleep(100);
			len = GetAvailable();
		}
		len = min(len, 3 * MEGABYTE); // limit just in case
		DbgLog((LOG_TRACE, 3, L"CMpegSplitterFile::Init() : streaming - use %I64d bytes to search for tracks", len));

		SearchPrograms(0, len);
		SearchStreams(0, len);
	}
	else { // downloading
		__int64 len = GetLength();
		len = min(len, 512 * KILOBYTE);

		SearchPrograms(0, len);
		SearchStreams(0, len);
	}

	if (!m_bIsBD) {
		REFERENCE_TIME rtMin = _I64_MAX;
		__int64 posMin       = -1;
		__int64 posMax       = posMin;

		for (int type = stream_type::video; type <= stream_type::audio; type++) {
			const CStreamList& streams = m_streams[type];
			POSITION pos = streams.GetHeadPosition();
			while (pos) {
				const stream& s = streams.GetNext(pos);
				const SyncPoints& sps = m_SyncPoints[s];
				if (sps.GetCount() > 1 && sps[0].rt < rtMin) {
					rtMin = sps[0].rt;
					posMin = posMax = sps[0].fp;
				}
			}
		}

		if (rtMin >= 0) {
			m_posMin = posMin;

			const REFERENCE_TIME maxDelta      = 30 * 60 * 10000000i64; // 30 minutes
			const REFERENCE_TIME maxStartDelta = 10 * 10000000i64;      // 10 seconds
			m_rtMin = m_rtMax = rtMin;
			for (int type = stream_type::video; type <= stream_type::audio; type++) {
				const CStreamList& streams = m_streams[type];
				POSITION pos = streams.GetHeadPosition();
				while (pos) {
					const stream& s = streams.GetNext(pos);
					const SyncPoints& sps = m_SyncPoints[s];
					for (size_t i = 1; i < sps.GetCount(); i++) {
						if (sps[i].rt > m_rtMax && sps[i].fp > posMax
								&& ((sps[i].rt - m_rtMax) < maxDelta)) {
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

			int indicated_rate = m_rate;
			int detected_rate = int(m_rtMax > m_rtMin ? UNITS * (posMax - posMin) / (m_rtMax - m_rtMin) : 0);

			m_rate = detected_rate ? detected_rate : m_rate;
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
				s.mt.majortype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.majortype		: MEDIATYPE_Video;
				s.mt.subtype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.subtype		: MEDIASUBTYPE_DVD_SUBPICTURE;
				s.mt.formattype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.formattype	: FORMAT_None;
				m_streams[stream_type::subpic].AddTail(s);
			}
		}
	}

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
				if (!Read(h, b) || !h.len) {
					continue;
				}

				__int64 pos3 = GetPos();
				nextPos = pos3 + h.len;

				if (h.fpts && AddStream(0, b, h.id_ext, h.len, FALSE) == TrackNum) {
					if (rtLimit != _I64_MAX && (h.pts - m_rtMin) > rtLimit) {
						Seek(pos);
						return INVALID_TIME;
					}

					rt		= h.pts;
					rtpos	= pos2;

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
			if (!Read(h)) {
				continue;
			}

			__int64 packet_pos = GetPos();
			nextPos = h.next;

			if (h.payloadstart && ISVALIDPID(h.pid) && h.pid == TrackNum) {
				if (NextMpegStartCode(b, 4)) {
					peshdr h2;
					if (Read(h2, b) && h2.fpts) { // pes packet
						if (rtLimit != _I64_MAX && (h2.pts - m_rtMin) > rtLimit) {
							Seek(pos);
							return INVALID_TIME;
						}

						rt		= h2.pts;
						rtpos	= h.hdrpos;

						BOOL bKeyFrame = TRUE;
						if (bKeyFrameOnly) {
							if (!h.randomaccess) {
								bKeyFrame = FALSE;

								int nBytes = h.bytes - (GetPos() - packet_pos);
								if (nBytes > 0) {
									CAtlArray<BYTE> p;
									p.SetCount((size_t)nBytes);
									ByteRead(p.GetData(), nBytes);

									// collect solid data packet
									{
										Seek(h.next);
										while (GetRemaining()) {
											__int64 start_pos_2 = GetPos();

											trhdr trhdr_2;
											if (!Read(trhdr_2)) {
												continue;
											}

											__int64 packet_pos_2 = GetPos();

											if (trhdr_2.payload && trhdr_2.pid == TrackNum) {
												if (trhdr_2.payloadstart) {
													if (NextMpegStartCode(b, 4)) {
														peshdr peshdr_2;
														if (Read(peshdr_2, b) && peshdr_2.fpts) {
															bKeyFrame = CheckKeyFrame(p, codec);
															nextPos = h.next = start_pos_2;
															break;
														}
													} else {
														Seek(packet_pos_2);
													}
												}

												if (trhdr_2.bytes > (GetPos() - packet_pos_2)) {
													__int64 nBytes_2 = trhdr_2.bytes - (GetPos() - packet_pos_2);
													size_t pSize = p.GetCount();
													p.SetCount(pSize + (size_t)nBytes_2);
													ByteRead(p.GetData() + pSize, nBytes_2);
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
			}

			Seek(h.next);
		} else if (m_type == MPEG_TYPES::mpeg_pva) {
			pvahdr h;
			if (!Read(h)) {
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
	if (m_type != MPEG_TYPES::mpeg_ts) {
		return;
	}

	Seek(start);

	while (GetPos() < stop) {
		trhdr h;
		if (!Read(h)) {
			continue;
		}

		ReadPrograms(h);
		Seek(h.next);
	}
}

void CMpegSplitterFile::SearchStreams(__int64 start, __int64 stop)
{
	avch.RemoveAll();
	hevch.RemoveAll();
	seqh.RemoveAll();

	Seek(start);

	for (;;) {
		if (GetPos() >= stop) {
			break;
		}

		BYTE b = 0;
		if (m_type == MPEG_TYPES::mpeg_ps) {
			if (!NextMpegStartCode(b)) {
				continue;
			}

			if (b == 0xba) { // program stream header
				pshdr h;
				if (!Read(h)) {
					continue;
				}
				m_rate = int(h.bitrate/8);
			} else if (b == 0xbc) { // program stream map
				UpdatePSM();
			} else if (b == 0xbb) { // program stream system header
				pssyshdr h;
				if (!Read(h)) {
					continue;
				}
			} else if ((b >= 0xbd && b < 0xf0) || (b == 0xfd)) { // pes packet
				const __int64 packet_start_pos = GetPos() - 4;

				peshdr h;
				if (!Read(h, b) || !h.len) {
					continue;
				}

				if (h.type == mpeg2 && h.scrambling) {
					Seek(GetPos() + h.len);
					continue;
				}

				__int64 pos = GetPos();
				DWORD TrackNum = AddStream(0, b, h.id_ext, h.len);

				if (h.fpts) {
					SyncPoints& sps = m_SyncPoints[TrackNum];
					sps.Add({ h.pts, packet_start_pos });
				}

				Seek(pos + h.len);
			}
		} else if (m_type == MPEG_TYPES::mpeg_ts) {
			trhdr h;
			if (!Read(h)) {
				continue;
			}

			__int64 pos = GetPos();

			if (h.payload && ISVALIDPID(h.pid)) {
				b = 0;
				peshdr h2;
				if (h.payloadstart) {
					if (NextMpegStartCode(b, 4)) {
						if (Read(h2, b)) { // pes packet
							if (h2.type == mpeg2 && h2.scrambling) {
								Seek(h.next);
								continue;
							}
						}
					} else {
						Seek(pos);
					}
				}

				DWORD TrackNum = AddStream(h.pid, b, 0, DWORD(h.bytes - (GetPos() - pos)));

				if (h2.fpts) {
					SyncPoints& sps = m_SyncPoints[TrackNum];
					sps.Add({ h2.pts, h.hdrpos });
				}
			}

			Seek(h.next);
		} else if (m_type == MPEG_TYPES::mpeg_pva) {
			pvahdr h;
			if (!Read(h)) {
				continue;
			}

			__int64 pos = GetPos();

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

#define MPEG_AUDIO					(1ULL << 0)
#define AAC_AUDIO					(1ULL << 1)
#define AC3_AUDIO					(1ULL << 2)
#define DTS_AUDIO					(1ULL << 3)
#define LPCM_AUDIO					(1ULL << 4)
#define MPEG2_VIDEO					(1ULL << 5)
#define H264_VIDEO					(1ULL << 6)
#define VC1_VIDEO					(1ULL << 7)
#define DIRAC_VIDEO					(1ULL << 8)
#define HEVC_VIDEO					(1ULL << 9)
#define PGS_SUB						(1ULL << 10)
#define DVB_SUB						(1ULL << 11)

#define PES_STREAM_TYPE_ANY			(MPEG_AUDIO | AAC_AUDIO | AC3_AUDIO | DTS_AUDIO/* | LPCM_AUDIO */| MPEG2_VIDEO | H264_VIDEO | DIRAC_VIDEO | HEVC_VIDEO/* | PGS_SUB*/ | DVB_SUB)

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
	{ SECONDARY_AUDIO_DTS_HD,				DTS_AUDIO	},
	{ PES_PRIVATE,							DTS_AUDIO	},
	// LPCM Audio
	{ AUDIO_STREAM_LPCM,					LPCM_AUDIO	},
	// MPEG2 Video
	{ VIDEO_STREAM_MPEG2,					MPEG2_VIDEO	},
	{ VIDEO_STREAM_MPEG2_ADDITIONAL_VIEW,	MPEG2_VIDEO	},
	// H.264/AVC1 Video
	{ VIDEO_STREAM_H264,					H264_VIDEO	},
	{ VIDEO_STREAM_H264_ADDITIONAL_VIEW,	H264_VIDEO	},
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
	{ PES_PRIVATE,							DVB_SUB		}
};

DWORD CMpegSplitterFile::AddStream(WORD pid, BYTE pesid, BYTE ps1id, DWORD len, BOOL bAddStream/* = TRUE*/)
{
	if (pid) {
		if (pesid) {
			m_pid2pes[pid] = pesid;
		} else {
			m_pid2pes.Lookup(pid, pesid);
		}
	}

	stream s;
	s.pid	= pid;
	s.pesid	= pesid;
	s.ps1id	= ps1id;

	if (m_bOpeningCompleted && m_type == MPEG_TYPES::mpeg_ts) {
		if (m_ClipInfo.IsHdmv()) {
			return s;
		}

		for (int type = stream_type::video; type < stream_type::unknown; type++) {
			if (m_streams[type].Find(s)) {
				return s;
			}
		}
	}

	const __int64 start		= GetPos();
	enum stream_type type	= stream_type::unknown;

	ULONGLONG stream_type = PES_STREAM_TYPE_ANY;
	PES_STREAM_TYPE pes_stream_type = INVALID;
	if (GetStreamType(s.pid ? s.pid : s.pesid, pes_stream_type)) {
		stream_type = 0ULL;

		for (size_t i = 0; i < _countof(PES_types); i++) {
			if (PES_types[i].pes_stream_type == pes_stream_type) {
				stream_type |= PES_types[i].stream_type;
			}
		}
	}

	if (!m_streams[stream_type::video].Find(s) && !m_streams[stream_type::audio].Find(s)
			&& ((!pesid && pes_stream_type != INVALID)
				|| (pesid >= 0xe0 && pesid < 0xf0)
				|| pesid == 0xfe)) { // mpeg video/audio
		// MPEG2
		if (stream_type & MPEG2_VIDEO) {
			// Sequence/extension header can be split into multiple packets
			if (!m_streams[stream_type::video].Find(s)) {
				if (!seqh.Lookup(s)) {
					seqh[s].Init();
				}

				if (seqh[s].data.GetCount()) {
					if (seqh[s].data.GetCount() < 512) {
						size_t size = seqh[s].data.GetCount();
						seqh[s].data.SetCount(size + (size_t)len);
						ByteRead(seqh[s].data.GetData() + size, len);
					} else {
						seqhdr h;
						if (Read(h, seqh[s].data, &s.mt)) {
							s.codec = stream_codec::MPEG;
							type = stream_type::video;
						}
					}
				} else {
					CMediaType mt;
					if (Read(seqh[s], len, &mt)) {
						if (m_type == MPEG_TYPES::mpeg_ts) {
							Seek(start);
							seqh[s].data.SetCount((size_t)len);
							ByteRead(seqh[s].data.GetData(), len);
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
			if (!m_streams[stream_type::video].Find(s) && Read(h, len, avch[s], &s.mt)) {
				s.codec = stream_codec::H264;
				type = stream_type::video;
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
			aachdr h = { 0 };

			if (!m_streams[stream_type::audio].Find(s)) {
				if (Read(h, len, &s.mt)) {
					if (m_aacValid[s].IsValid()) {
						type = stream_type::audio;
					} else {
						m_aacValid[s].Handle(h);
					}
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
			latm_aachdr h = { 0 };

			if (Read(h, len, &s.mt)) {
				if (m_aaclatmValid[s].IsValid()) {
					type = stream_type::audio;
				} else {
					m_aaclatmValid[s].Handle(h);
				}
			}
		}

		// AAC
		if (type == stream_type::unknown && (stream_type & AAC_AUDIO)) {
			Seek(start);
			aachdr h = { 0 };

			if (Read(h, len, &s.mt)) {
				if (m_aacValid[s].IsValid()) {
					type = stream_type::audio;
				} else {
					m_aacValid[s].Handle(h);
				}
			}
		}

		// MPEG Audio
		if (type == stream_type::unknown && (stream_type & MPEG_AUDIO)) {
			Seek(start);
			mpahdr h = { 0 };

			if (Read(h, len, &s.mt, false, true)) {
				if (m_mpaValid[s].IsValid()) {
					type = stream_type::audio;
				} else {
					m_mpaValid[s].Handle(h);
				}
			}
		}

		// AC3
		if (type == stream_type::unknown && (stream_type & AC3_AUDIO)) {
			Seek(start);
			ac3hdr h = { 0 };

			if (Read(h, len, &s.mt)) {
				if (m_ac3Valid[s].IsValid()) {
					type = stream_type::audio;
				} else {
					m_ac3Valid[s].Handle(h);
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
					latm_aachdr h = { 0 };

					if (Read(h, len, &s.mt)) {
						if (m_aaclatmValid[s].IsValid()) {
							type = stream_type::audio;
						} else {
							m_aaclatmValid[s].Handle(h);
						}
					}
				}

				// AC3, E-AC3, TrueHD
				if (type == stream_type::unknown && (stream_type & AC3_AUDIO)) {
					ac3hdr h;
					if (Read(h, len, &s.mt, true, (m_AC3CoreOnly != 0))) {
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
					dvbsub h;
					if (Read(h, len, &s.mt)) {
						type = stream_type::subpic;
					}
				}

				// LPCM Audio
				if (type == stream_type::unknown && (stream_type & LPCM_AUDIO)) {
					Seek(start);
					hdmvlpcmhdr h;
					if (Read(h, &s.mt)) {
						type = stream_type::audio;
					}
				}

				// PGS subtitles
				if (type == stream_type::unknown && (stream_type & PGS_SUB)) {
					Seek(start);

					int iProgram;
					const CHdmvClipInfo::Stream *pClipInfo;
					const program* pProgram = FindProgram(s.pid, iProgram, pClipInfo);

					hdmvsubhdr h;
					if (!m_ClipInfo.IsHdmv() && Read(h, &s.mt, pClipInfo ? pClipInfo->m_LanguageCode : NULL)) {
						type = stream_type::subpic;
					}
				}
			} else if (!m_bOpeningCompleted && type == stream_type::unknown) {
				stream* source = (stream*)m_streams[stream_type::audio].FindStream(s);
				if (source) {
					if (!m_AC3CoreOnly && pes_stream_type == AUDIO_STREAM_AC3_TRUE_HD && source->mt.subtype == MEDIASUBTYPE_DOLBY_AC3) {
						Seek(start);
						ac3hdr h;
						if (Read(h, len, &s.mt, false, false) && s.mt.subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
							source->mts.push_back(s.mt);
							source->mts.push_back(source->mt);
							source->mt = s.mt;
						}
					} else if ((pes_stream_type == AUDIO_STREAM_DTS_HD || pes_stream_type == AUDIO_STREAM_DTS_HD_MASTER_AUDIO) && source->dts.bDTSCore && !source->dts.bDTSHD && source->mt.pbFormat) {
						Seek(start);
						if (BitRead(32, true) == FCC(DTSHD_SYNC_WORD)) {
							BYTE* buf = DNew BYTE[len];
							audioframe_t audioframe;
							if (ByteRead(buf, len) == S_OK && ParseDTSHDHeader(buf, len, &audioframe)) {
								WAVEFORMATEX* wfe = (WAVEFORMATEX*)source->mt.pbFormat;
								wfe->nSamplesPerSec		= audioframe.samplerate;
								wfe->nChannels			= audioframe.channels;
								if (audioframe.param1) {
									wfe->wBitsPerSample = audioframe.param1;
								}

								source->dts.bDTSHD	= true;
							}
							delete [] buf;
						}
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
			BYTE b		= (BYTE)BitRead(8, true);
			WORD w		= (WORD)BitRead(16, true);
			DWORD dw	= (DWORD)BitRead(32, true);

			if (b >= 0x80 && b < 0x88 || w == 0x0b77) { // ac3
				s.ps1id = (b >= 0x80 && b < 0x88) ? (BYTE)(BitRead(32) >> 24) : 0x80;

				ac3hdr h = { 0 };
				if (!m_streams[stream_type::audio].Find(s)) {
					if (Read(h, len, &s.mt)) {
						if (m_ac3Valid[s].IsValid()) {
							type = stream_type::audio;
						} else {
							m_ac3Valid[s].Handle(h);
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

				do {
					// DVD-Audio LPCM
					if (b == 0xa0) {
						BitRead(8); // Continuity Counter - counts from 0x00 to 0x1f and then wraps to 0x00.
						DWORD headersize = (DWORD)BitRead(16); // LPCM_header_length
						if (headersize >= 8 && headersize + 4 < len) {
							dvdalpcmhdr h;
							if (Read(h, len - 4, &s.mt)) {
								Seek(start + 4 + headersize);
								type = stream_type::audio;
								break;
							}
						}
					}
					// DVD-Audio MLP
					else if (b == 0xa1 && len > 10) {
						BitRead(8); // Continuity Counter: 0x00..0x1f or 0x20..0x3f or 0x40..0x5f or 0x80..0x9f
						BitRead(8); // some unknown data
						DWORD headersize = (DWORD)BitRead(8); // MLP_header_length (always equal 6?)
						BitRead(32); // some unknown data
						WORD unknown1 = (WORD)BitRead(16); // 0x0000 or 0x0400
						if (headersize == 6 && (unknown1 == 0x0000 || unknown1 == 0x0400)) { // Maybe it's MLP?
							// MLP header may be missing in the first package
							if (!m_streams[stream_type::audio].Find(s)) {
								mlphdr h;
								if (Read(h, len - 10, &s.mt, true)) {
									// This is exactly the MLP.
									type = stream_type::audio;
								}
							}

							Seek(start + 10);
							break;
						}
					}

					// DVD LPCM
					if (m_streams[stream_type::audio].Find(s)) {
						Seek(start + 7);
					} else {
						Seek(start + 4);
						lpcmhdr h;
						if (Read(h, &s.mt)) {
							type = stream_type::audio;
						}
					}
				} while (false);
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
				s.pid = (WORD)((BitRead(8) << 8) | BitRead(16)); // pid = 0xa000 | track id

				ps2audhdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (w == 0xff90) { // ps2-mpg ac3 or subtitles
				s.ps1id = (BYTE)BitRead(8);
				s.pid = (WORD)((BitRead(8) << 8) | BitRead(16)); // pid = 0x9000 | track id

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
	} else if (pesid == 0xbe) { // padding
	} else if (pesid == 0xbf) { // private stream 2
	}

	if (bAddStream && type != stream_type::unknown && !m_streams[type].Find(s)) {
		if (s.pid) {
			if (!s.lang_set) {
				CStringA lang_name;
				m_pPMT_Lang.Lookup(s.pid, lang_name);
				if (!lang_name.IsEmpty()) {
					memcpy(s.lang, lang_name, 4);
					s.lang_set = true;
				}
			}

			for (int t = stream_type::video; t < stream_type::unknown; t++) {
				if (m_streams[t].Find(s)) {
					return s;
				}
			}
		}

		if (type == stream_type::video) {
			REFERENCE_TIME rtAvgTimePerFrame = 1;
			ExtractAvgTimePerFrame(&s.mt, rtAvgTimePerFrame);

			if (rtAvgTimePerFrame < 50000) {
				__int64 _pos = GetPos();
				__int64 nextPos;
				REFERENCE_TIME rt_start = NextPTS(s, s.codec, nextPos);
				if (rt_start != INVALID_TIME) {
					Seek(nextPos);
					REFERENCE_TIME rt_end = rt_start;
					int count = 0;
					while (count < 30) {
						REFERENCE_TIME rt = NextPTS(s, s.codec, nextPos);
						if (rt == INVALID_TIME) {
							break;
						}
						if (rt > rt_start) {
							rt_end = rt;
							count++;
						}

						Seek(nextPos);
					}

					if (count && (rt_start < rt_end)) {
						rtAvgTimePerFrame = (rt_end - rt_start) / (count - 1);
					}

					if (rtAvgTimePerFrame < 50000) {
						rtAvgTimePerFrame = 417083; // set 23.976 as default
					}

					if (rtAvgTimePerFrame) {
						if (s.mt.formattype == FORMAT_MPEG2_VIDEO) {
							((MPEG2VIDEOINFO*)s.mt.pbFormat)->hdr.AvgTimePerFrame	= rtAvgTimePerFrame;
						} else if (s.mt.formattype == FORMAT_VIDEOINFO2) {
							((VIDEOINFOHEADER2*)s.mt.pbFormat)->AvgTimePerFrame		= rtAvgTimePerFrame;
						} else if (s.mt.formattype == FORMAT_VideoInfo) {
							((VIDEOINFOHEADER*)s.mt.pbFormat)->AvgTimePerFrame		= rtAvgTimePerFrame;
						}
					}
				}

				Seek(_pos);
			}
		}

		if (m_bOpeningCompleted) {
			s.mts.push_back(s.mt);
		}

		if (!m_bOpeningCompleted || (m_bOpeningCompleted && m_streams[type].GetCount() > 0)) {
			m_streams[type].Insert(s, type);
		}
	}

	return s;
}

void CMpegSplitterFile::AddHdmvPGStream(WORD pid, const char* language_code)
{
	stream s;
	s.pid   = pid;
	s.pesid = 0xbd;

	hdmvsubhdr h;
	if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt, language_code)) {
		m_streams[stream_type::subpic].Insert(s, subpic);
	}
}

CAtlList<CMpegSplitterFile::stream>* CMpegSplitterFile::GetMasterStream()
{
	return
		!m_streams[stream_type::video].IsEmpty()  ? &m_streams[stream_type::video]  :
		!m_streams[stream_type::audio].IsEmpty()  ? &m_streams[stream_type::audio]  :
		!m_streams[stream_type::subpic].IsEmpty() ? &m_streams[stream_type::subpic] :
		NULL;
}

#define PAT_ID 0x00
#define PMT_ID 0x02

void CMpegSplitterFile::ReadPrograms(const trhdr& h)
{
	if (h.bytes <= 9) {
		return;
	}

	programData& ProgramData = m_ProgramData[h.pid];
	if (ProgramData.bFinished) {
		return;
	}

	if (h.payload && h.payloadstart) {
		trsechdr h2;
		if (Read(h2)) {
			switch (h2.table_id) {
				case PAT_ID :
					if (!m_programs.IsEmpty()) {
						return;
					}
					break;
				case PMT_ID :
				case 0xC8 :
				case 0xC9 :
				case 0xDA :
					if (m_programs.IsEmpty()) {
						return;
					}
					break;
				default:
					return;
			}

			ProgramData.table_id       = h2.table_id;
			ProgramData.section_length = h2.section_length;

			const size_t packet_len    = h.bytes - h2.hdr_size;
			const size_t max_len       = min(packet_len, ProgramData.section_length);

			ProgramData.pData.SetCount(max_len);
			ByteRead(ProgramData.pData.GetData(), max_len);
		}
	} else if (!ProgramData.pData.IsEmpty()) {
		const size_t data_len = ProgramData.pData.GetCount();
		const size_t max_len  = min((size_t)h.bytes, ProgramData.section_length - data_len);

		ProgramData.pData.SetCount(data_len + max_len);
		ByteRead(ProgramData.pData.GetData() + data_len, max_len);
	}

	if (ProgramData.IsFull()) {
		switch (ProgramData.table_id) {
			case PAT_ID :
				ReadPAT(ProgramData.pData);
				break;
			case PMT_ID :
				ReadPMT(ProgramData.pData, h.pid);
				break;
			case 0xC8 : // ATSC - Terrestrial Virtual Channel Table (TVCT)
			case 0xC9 : // ATSC - Cable Virtual Channel Table (CVCT) / Long-form Virtual Channel Table (L-VCT)
			case 0xDA : // ATSC - Satellite VCT (SVCT)
				ReadVCT(ProgramData.pData, ProgramData.table_id);
				break;
		}

		ProgramData.Finish();
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

void CMpegSplitterFile::ReadPMT(CAtlArray<BYTE>& pData, WORD pid)
{
	if (CPrograms::CPair* pPair = m_programs.Lookup(pid)) {
		memset(pPair->m_value.streams, 0, sizeof(pPair->m_value.streams));

		CGolombBuffer gb(pData.GetData(), pData.GetCount());
		int len = gb.GetSize();

		BYTE reserved1				= (BYTE)gb.BitRead(3);
		WORD PCR_PID				= (WORD)gb.BitRead(13);
		BYTE reserved2				= (BYTE)gb.BitRead(4);
		WORD program_info_length	= (WORD)gb.BitRead(12);
		UNREFERENCED_PARAMETER(reserved1);
		UNREFERENCED_PARAMETER(PCR_PID);
		UNREFERENCED_PARAMETER(reserved2);

		len -= (4 + program_info_length);
		if (len <= 0)
			return;

		if (program_info_length) {
			gb.SkipBytes(program_info_length);
		}

		for (int i = 0; i < _countof(pPair->m_value.streams) && len >= 5; i++) {
			BYTE stream_type	= (BYTE)gb.BitRead(8);
			BYTE nreserved1		= (BYTE)gb.BitRead(3);
			WORD pid			= (WORD)gb.BitRead(13);
			BYTE nreserved2		= (BYTE)gb.BitRead(4);
			WORD ES_info_length	= (WORD)gb.BitRead(12);
			UNREFERENCED_PARAMETER(nreserved1);
			UNREFERENCED_PARAMETER(nreserved2);

			if (!m_programs.FindProgram(pid)) {
				pPair->m_value.streams[i].pid	= pid;
				pPair->m_value.streams[i].type	= (PES_STREAM_TYPE)stream_type;
			}

			len -= (5 + ES_info_length);
			if (len < 0) {
				break;
			}

			BOOL DVB_SUBTITLE = FALSE;
			if (ES_info_length > 2) {
				int	info_length = ES_info_length;
				for (;;) {
					BYTE descriptor_tag		= (BYTE)gb.BitRead(8);
					BYTE descriptor_length	= (BYTE)gb.BitRead(8);
					info_length			   -= (2 + descriptor_length);
					if (info_length < 0)
						break;
					char ch[4] = { 0 };

					switch (descriptor_tag) {
						case 0x59: // Subtitling descriptor
							DVB_SUBTITLE = TRUE;
						case 0x0a: // ISO 639 language descriptor
						case 0x56: // Teletext descriptor
							gb.ReadBuffer((BYTE *)ch, 3);
							ch[3] = 0;
							for (BYTE i = 3; i < descriptor_length; i++) {
								gb.BitRead(8);
							}
							if (!(ch[0] == 'u' && ch[1] == 'n' && ch[2] == 'd')) {
								m_pPMT_Lang[pid] = CString(ch);
							}
							break;
						default:
							gb.SkipBytes(descriptor_length);
							break;
					}
					if (info_length <= 2) {
						if (info_length) {
							gb.SkipBytes(info_length);
						}
						break;
					}
				}
			} else if (ES_info_length) {
				gb.SkipBytes(ES_info_length);
			}

			if (m_ForcedSub) {
				if (stream_type == PRESENTATION_GRAPHICS_STREAM || DVB_SUBTITLE) {
					stream s;
					s.pid = pid;

					if (!m_streams[stream_type::subpic].Find(s)) {
						CStringA lang_name;
						m_pPMT_Lang.Lookup(s.pid, lang_name);
						if (!lang_name.IsEmpty()) {
							memcpy(s.lang, lang_name, 4);
							s.lang_set = true;
						}

						if (stream_type == PRESENTATION_GRAPHICS_STREAM) {
							CMpegSplitterFile::hdmvsubhdr hdr;
							if (Read(hdr, &s.mt, NULL)) {
								m_streams[stream_type::subpic].Insert(s, subpic);
							}
						} else {
							CMpegSplitterFile::dvbsub hdr;
							if (Read(hdr, 0, &s.mt, true)) {
								m_streams[stream_type::subpic].Insert(s, subpic);
							}
						}
					}
				}
			}
		}
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

		SHORT program_number = (SHORT)gb.BitRead(16);
		if (program_number > 0 && program_number < 0x2000) {
							
			POSITION pos = m_programs.GetStartPosition();
			while (pos) {
				CPrograms::CPair* pPair = m_programs.GetNext(pos);
				if (pPair->m_value.program_number == program_number) {
					pPair->m_value.name = short_name;
				}
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
		SHORT descriptors_size = (SHORT)gb.BitRead(10);
		if (descriptors_size) {
			gb.SkipBytes(descriptors_size);
		}
	}
}

const CMpegSplitterFile::program* CMpegSplitterFile::FindProgram(WORD pid, int &iStream, const CHdmvClipInfo::Stream* &pClipInfo)
{
	iStream   = -1;
	pClipInfo = NULL;

	if (m_type == MPEG_TYPES::mpeg_ts) {
		pClipInfo = m_ClipInfo.FindStream(pid);

		POSITION pos = m_programs.GetStartPosition();
		while (pos) {
			program* p = &m_programs.GetNextValue(pos);

			for (int i = 0; i < _countof(p->streams); i++) {
				if (p->streams[i].pid == pid) {
					iStream = i;
					return p;
				}
			}
		}
	}

	return NULL;
}

bool CMpegSplitterFile::GetStreamType(WORD pid, PES_STREAM_TYPE &stream_type)
{
	if (ISVALIDPID(pid)) {
		int iProgram;
		const CHdmvClipInfo::Stream *pClipInfo;
		const program* pProgram = FindProgram(pid, iProgram, pClipInfo);
		if (pProgram) {
			stream_type = pProgram->streams[iProgram].type;

			if (stream_type != INVALID) {
				return true;
			}
		}

		if (pid < _countof(m_psm) && m_psm[pid] != 0) {
			stream_type = m_psm[pid];
			return true;
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