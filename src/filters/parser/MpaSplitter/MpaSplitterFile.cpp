/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include "MpaSplitterFile.h"
#include "DSUtil/AudioParser.h"

#include <moreuuids.h>

CMpaSplitterFile::CMpaSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL | FM_STREAM)
	, m_mode(mode::none)
	, m_rtDuration(0)
	, m_startpos(0)
	, m_procsize(0)
	, m_coefficient(0.0)
	, m_bIsVBR(false)
{
	if (SUCCEEDED(hr)) {
		hr = Init();
	}
}

#define MPA_HEADER_SIZE  4
#define ADTS_HEADER_SIZE 9
#define AC4_HEADER_SIZE  7

#define MOVE_TO_MPA_START_CODE(b, e)      while(b <= e - MPA_HEADER_SIZE  && ((GETU16(b) & MPA_SYNCWORD) != MPA_SYNCWORD)) b++;
#define MOVE_TO_AAC_START_CODE(b, e)      while(b <= e - ADTS_HEADER_SIZE && ((GETU16(b) & AAC_ADTS_SYNCWORD) != AAC_ADTS_SYNCWORD)) b++;
#define MOVE_TO_AAC_LATM_START_CODE(b, e) while(b <= e - 7 && ((GETU16(b) & 0xE0FF) != 0xE056)) b++;
#define MOVE_TO_AC4_START_CODE(b, e)      while(b <= e - AC4_HEADER_SIZE  && !(GETU16(b) == 0x40AC || GETU16(b) == 0x41AC)) b++;

#define FRAMES_FLAG 0x0001

#define ID3v1_TAG_SIZE 128

HRESULT CMpaSplitterFile::Init()
{
	m_startpos = 0;
	__int64 endpos = GetLength();

	if (IsURL()) {
		SetCacheSize(16 * KILOBYTE);
	}

	Seek(0);
	BYTE data[16];
	if (ByteRead(data, sizeof(data)) != S_OK) {
		return E_FAIL;
	}

	const BYTE sign_mpegps[] = { 0x00, 0x00, 0x01, 0xBA }; // MPEG-PS (000001BA)
	const BYTE sign_riff[] =   { 'R', 'I', 'F', 'F' };     // RIFF files (AVI, WAV, AMV and other)
	const BYTE sign_mtv[] =    { 'A', 'M', 'V' };          // MTV files (.mtv)
	const BYTE sign_iff[] =    { 'F', 'O', 'R', 'M'};      // IFF files (ANIM)
	const BYTE sign_mpegts[] = { 0x47, 0x40, 0x00, 0x10 }; // MPEG-TS (?? ?? ?? ?? 10 00 40 47)
	const BYTE sign_wtv[] =    { 0xB7, 0xD8, 0x00, 0x20, 0x37, 0x49, 0xDA, 0x11,
		 0xA6, 0x4E, 0x00, 0x07, 0xE9, 0x5E, 0xAD, 0x8D }; // Windows Television (.wtv)

	// some files can be determined as Mpeg Audio
	if ((memcmp(data,   sign_mpegps, std::size(sign_mpegps)) == 0)||
		(memcmp(data,   sign_riff,   std::size(sign_riff)) == 0) ||
		(memcmp(data,   sign_mtv,    std::size(sign_mtv)) == 0) ||
		(memcmp(data,   sign_iff,    std::size(sign_iff)) == 0) ||
		(memcmp(data,   sign_mpegts, std::size(sign_mpegts)) == 0) ||
		(memcmp(data+4, sign_mpegts, std::size(sign_mpegts)) == 0) ||
		(memcmp(data,   sign_wtv,    std::size(sign_wtv)) == 0)) {
		return E_FAIL;
	}

	Seek(0);

	while (BitRead(24, true) == 'ID3') {
		BitRead(24);

		BYTE major = (BYTE)BitRead(8);
		BYTE revision = (BYTE)BitRead(8);
		UNREFERENCED_PARAMETER(revision);

		BYTE flags = (BYTE)BitRead(8);

		DWORD size = BitRead(32);
		size = hexdec2uint(size);

		m_startpos = GetPos() + size;

		// TODO: read extended header
		if (major <= 4) {
			auto buf = std::make_unique<BYTE[]>(size);
			if (ByteRead(buf.get(), size) == S_OK) {
				if (!m_pID3Tag) {
					m_pID3Tag = std::make_unique<CID3Tag>(major, flags);
				}

				m_pID3Tag->ReadTagsV2(buf.get(), size);
			}
		}

		Seek(m_startpos);
		for (int i = 0; i < (1 << 20) && m_startpos < endpos && !BitRead(8, true); i++) {
			m_startpos++;
			Seek(m_startpos);
		}
	}

	endpos = GetLength();
	if (IsRandomAccess()) {
		if (endpos > ID3v1_TAG_SIZE) {
			Seek(endpos - ID3v1_TAG_SIZE);

			if (BitRead(24) == 'TAG') {
				endpos -= ID3v1_TAG_SIZE;

				const size_t tag_size = ID3v1_TAG_SIZE - 3;
				auto buf = std::make_unique<BYTE[]>(tag_size);
				if (ByteRead(buf.get(), tag_size) == S_OK) {
					if (!m_pID3Tag) {
						m_pID3Tag = std::make_unique<CID3Tag>();
					}

					m_pID3Tag->ReadTagsV1(buf.get(), tag_size);
				}
			}
		}

		if (endpos > APE_TAG_FOOTER_BYTES) {
			BYTE buf[APE_TAG_FOOTER_BYTES] = {};

			Seek(endpos - APE_TAG_FOOTER_BYTES);
			if (ByteRead(buf, APE_TAG_FOOTER_BYTES) == S_OK) {
				m_pAPETag = std::make_unique<CAPETag>();
				if (m_pAPETag->ReadFooter(buf, APE_TAG_FOOTER_BYTES) && m_pAPETag->GetTagSize()) {
					const size_t tag_size = m_pAPETag->GetTagSize();
					Seek(endpos - tag_size);
					auto p = std::make_unique<BYTE[]>(tag_size);
					if (ByteRead(p.get(), tag_size) == S_OK) {
						m_pAPETag->ReadTags(p.get(), tag_size);
					}

					endpos -= tag_size;
				}
			}
		}
	}

	Seek(m_startpos);

	__int64 startpos = m_startpos;

	const __int64 limit = IsRandomAccess() && !IsURL() ? MEGABYTE : 64 * KILOBYTE;
	const __int64 size = IsRandomAccess() ? std::min(endpos - m_startpos, limit) : limit;
	std::unique_ptr<BYTE[]> buffer(new(std::nothrow) BYTE[size]);
	if (!buffer) {
		return E_FAIL;
	}

	int valid_cnt = 0;
	if (S_OK == ByteRead(buffer.get(), size)) {
		BYTE* start     = buffer.get();
		const BYTE* end = start + size;
		while (m_mode != mode::mpa) {
			MOVE_TO_MPA_START_CODE(start, end);
			if (start < end - MPA_HEADER_SIZE) {
				startpos = m_startpos + (start - buffer.get());
				int frame_size = ParseMPAHeader(start);
				if (frame_size == 0) {
					start++;
					continue;
				}
				if (start + frame_size + MPA_HEADER_SIZE > end) {
					break;
				}

				BYTE* start2 = start + frame_size;
				while (start2 + MPA_HEADER_SIZE <= end && valid_cnt < 10) {
					frame_size = ParseMPAHeader(start2);
					if (frame_size == 0) {
						valid_cnt = 0;
						m_mode = mode::none;
						start++;
						break;
					}

					valid_cnt++;
					m_mode = mode::mpa;
					if (start2 + frame_size >= end) {
						break;
					}

					start2 += frame_size;
				}
			} else {
				break;
			}
		}
		if (valid_cnt < 3) {
			m_mode = mode::none;
		}

		if (m_mode == mode::none) {
			BYTE* start     = buffer.get();
			const BYTE* end = start + size;
			while (m_mode != mode::mp4a) {
				MOVE_TO_AAC_START_CODE(start, end);
				if (start < end - ADTS_HEADER_SIZE) {
					startpos = m_startpos + (start - buffer.get());
					int frame_size = ParseADTSAACHeader(start);
					if (frame_size == 0) {
						start++;
						continue;
					}
					if (start + frame_size + ADTS_HEADER_SIZE > end) {
						break;
					}

					BYTE* start2 = start + frame_size;
					while (start2 + ADTS_HEADER_SIZE <= end && valid_cnt < 10) {
						frame_size = ParseADTSAACHeader(start2);
						if (frame_size == 0) {
							valid_cnt = 0;
							m_mode = mode::none;
							start++;
							break;
						}

						valid_cnt++;
						m_mode = mode::mp4a;
						if (start2 + frame_size >= end) {
							break;
						}

						start2 += frame_size;
					}
				} else {
					break;
				}
			}
			if (valid_cnt < 3) {
				m_mode = mode::none;
			}
		}

		if (m_mode == mode::none) {
			BYTE* start = buffer.get();
			const BYTE* end = start + size;
			while (m_mode != mode::latm) {
				MOVE_TO_AAC_LATM_START_CODE(start, end);
				if (start < end - 7) {
					startpos = m_startpos + (start - buffer.get());
					int frame_size = ParseAACLatmHeader(start, end - start);
					if (frame_size == 0) {
						start++;
						continue;
					}
					if (start + frame_size + 7 > end) {
						break;
					}

					BYTE* start2 = start + frame_size;
					while (start2 + 7 <= end && valid_cnt < 10) {
						frame_size = ParseAACLatmHeader(start2, end - start2);
						if (frame_size == 0) {
							valid_cnt = 0;
							m_mode = mode::none;
							start++;
							break;
						}

						valid_cnt++;
						m_mode = mode::latm;
						if (start2 + frame_size >= end) {
							break;
						}

						start2 += frame_size;
					}
				} else {
					break;
				}
			}
			if (valid_cnt < 3) {
				m_mode = mode::none;
			}
		}

		if (m_mode == mode::none) {
			BYTE* start = buffer.get();
			const BYTE* end = start + size;
			while (m_mode != mode::ac4) {
				MOVE_TO_AC4_START_CODE(start, end);
				if (start < end - AC4_HEADER_SIZE) {
					startpos = m_startpos + (start - buffer.get());
					int frame_size = ParseAC4Header(start, end - start);
					if (frame_size == 0) {
						start++;
						continue;
					}
					if (start + frame_size + AC4_HEADER_SIZE > end) {
						break;
					}

					BYTE* start2 = start + frame_size;
					while (start2 + AC4_HEADER_SIZE <= end && valid_cnt < 10) {
						frame_size = ParseAC4Header(start2, end - start2);
						if (frame_size == 0) {
							valid_cnt = 0;
							m_mode = mode::none;
							start++;
							break;
						}

						valid_cnt++;
						m_mode = mode::ac4;
						if (start2 + frame_size >= end) {
							break;
						}

						start2 += frame_size;
					}
				}
				else {
					break;
				}
			}
			if (valid_cnt < 3) {
				m_mode = mode::none;
			}
		}
	}

	if (m_mode == mode::none) {
		return E_FAIL;
	}

	m_startpos = startpos;
	m_endpos = IsStreaming() ? INT64_MAX : endpos;
	Seek(m_startpos);

	if (m_mode == mode::mpa) {
		Read(m_mpahdr, MPA_HEADER_SIZE, &m_mt, true);
	} else if (m_mode == mode::mp4a) {
		Read(m_aachdr, ADTS_HEADER_SIZE + 64, &m_mt, false);
	} else if (m_mode == mode::latm) {
		Read(m_latmhdr, 32, &m_mt);
	} else {
		Read(m_ac4hdr, 128, &m_mt);
		if (m_ac4hdr.objectCoding) {
			DLog(L"CMpaSplitterFile::Init() : AC4 stream with object coding is not supported in the decoder, ignore it.");
			return E_FAIL;
		}
	}

	if (m_mode == mode::mpa) {
		DWORD dwFrames = 0;		// total number of frames
		Seek(m_startpos + MPA_HEADER_SIZE + 32);
		if (BitRead(32, true) == 'Xing' || BitRead(32, true) == 'Info') {
			BitRead(32); // Skip ID tag
			DWORD dwFlags = (DWORD)BitRead(32);
			// extract total number of frames in file
			if (dwFlags & FRAMES_FLAG) {
				dwFrames = (DWORD)BitRead(32);
			}
		} else if (BitRead(32, true) == 'VBRI') {
			BitRead(32); // Skip ID tag
			// extract all fields from header (all mandatory)
			BitRead(16); // version
			BitRead(16); // delay
			BitRead(16); // quality
			BitRead(32); // bytes
			dwFrames = (DWORD)BitRead(32); // extract total number of frames in file
		}

		if (dwFrames) {
			bool l3ext = m_mpahdr.layer == 3 && !(m_mpahdr.version & 1);
			DWORD dwSamplesPerFrame = m_mpahdr.layer == 1 ? 384 : l3ext ? 576 : 1152;

			m_bIsVBR = true;
			m_rtDuration = 10000000i64 * (dwFrames * dwSamplesPerFrame / m_mpahdr.Samplerate);
		}
	}

	Seek(m_startpos);

	if (m_mode == mode::mpa) {
		m_coefficient = 10000000.0 * (GetLength() - m_startpos) * m_mpahdr.FrameSamples / m_mpahdr.Samplerate;
	} else if (m_mode == mode::mp4a) {
		m_coefficient = 10000000.0 * (GetLength() - m_startpos) * m_aachdr.FrameSamples / m_aachdr.Samplerate;
	} else if (m_mode == mode::latm) {
		m_coefficient = 10000000.0 * (GetLength() - m_startpos) * m_latmhdr.FrameSamples / m_latmhdr.samplerate;
	} else if (m_mode == mode::ac4) {
		m_coefficient = 10000000.0 * (GetLength() - m_startpos) * m_ac4hdr.FrameSamples / m_ac4hdr.samplerate;
	}

	int FrameSize;
	REFERENCE_TIME rtFrameDur, rtPrevDur = -1;
	clock_t start = clock();
	int i = 0;
	while (Sync(FrameSize, rtFrameDur) && (clock() - start) < CLOCKS_PER_SEC) {
		Seek(GetPos() + FrameSize);
		i = rtPrevDur == m_rtDuration ? i + 1 : 0;
		if (i == 10) {
			break;
		}
		rtPrevDur = m_rtDuration;
	}

	return S_OK;
}

bool CMpaSplitterFile::Sync(int limit/* = DEF_SYNC_SIZE*/)
{
	int FrameSize;
	REFERENCE_TIME rtDuration;
	return Sync(FrameSize, rtDuration, limit);
}

bool CMpaSplitterFile::Sync(int& FrameSize, REFERENCE_TIME& rtDuration, int limit/* = DEF_SYNC_SIZE*/, BOOL bExtraCheck/* = FALSE*/)
{
	__int64 endpos = std::min(m_endpos, GetPos() + limit);

	if (m_mode == mode::mpa) {
		while (GetPos() <= endpos - MPA_HEADER_SIZE) {
			mpahdr h;

			if (Read(h, (int)(endpos - GetPos()), nullptr, true, true)) {
				Seek(GetPos() - MPA_HEADER_SIZE);
				if (bExtraCheck) {
					__int64 pos = GetPos();
					if (pos + h.FrameSize + MPA_HEADER_SIZE < GetLength()) {
						Seek(pos + h.FrameSize);
						mpahdr h2;
						if (!Read(h2, MPA_HEADER_SIZE, nullptr, true)) {
							Seek(pos + 1);
							continue;
						}
					}
					Seek(pos);
				}
				AdjustDuration(h.FrameSize);

				FrameSize  = h.FrameSize;
				rtDuration = h.rtDuration;

				memcpy(&m_mpahdr, &h, sizeof(mpahdr));

				return true;
			} else {
				break;
			}
		}
	} else if (m_mode == mode::mp4a) {
		while (GetPos() <= endpos - 9) {
			aachdr h;

			if (Read(h, (int)(endpos - GetPos()))) {
				if (m_aachdr == h) {
					Seek(GetPos() - (h.fcrc ? 7 : 9));
					AdjustDuration(h.FrameSize);
					Seek(GetPos() + (h.fcrc ? 7 : 9));

					FrameSize  = h.FrameSize;
					rtDuration = h.rtDuration;

					return true;
				}
			} else {
				break;
			}
		}
	} else if (m_mode == mode::latm) {
		while (GetPos() <= endpos - 7) {
			latm_aachdr h;

			if (Read(h, (int)(endpos - GetPos()))) {
				if (m_latmhdr == h) {
					AdjustDuration(h.FrameSize);

					FrameSize  = h.FrameSize;
					rtDuration = h.rtDuration;

					return true;
				}
			} else {
				break;
			}
		}
	} else if (m_mode == mode::ac4) {
		while (GetPos() <= endpos - AC4_HEADER_SIZE) {
			ac4hdr h;

			auto pos = GetPos();
			if (Read(h, (int)(endpos - GetPos()), nullptr, true)) {
				if (m_ac4hdr == h) {
					AdjustDuration(h.FrameSize);

					FrameSize = h.FrameSize;
					rtDuration = h.rtDuration;

					return true;
				}
			}
			else {
				break;
			}
		}
	}

	return false;
}

void CMpaSplitterFile::AdjustDuration(int framesize)
{
	if (!framesize) {
		return;
	}

	if (!m_bIsVBR) {
		auto it = m_pos2fsize.find(GetPos());
		if (it == m_pos2fsize.end()) {
			m_procsize += framesize;
			m_pos2fsize.emplace(GetPos(), framesize);
			m_rtDuration = m_coefficient * m_pos2fsize.size() / m_procsize;
		}
	}
}
