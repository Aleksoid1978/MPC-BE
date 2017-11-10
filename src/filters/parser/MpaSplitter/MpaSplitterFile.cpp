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
#include "MpaSplitterFile.h"
#include "../../../DSUtil/AudioParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

CMpaSplitterFile::CMpaSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL | FM_STREAM)
	, m_mode(mode::none)
	, m_rtDuration(0)
	, m_startpos(0)
	, m_procsize(0)
	, m_coefficient(0.0)
	, m_bIsVBR(false)
	, m_pID3Tag(nullptr)
	, m_pAPETag(nullptr)
{
	if (SUCCEEDED(hr)) {
		hr = Init();
	}
}

CMpaSplitterFile::~CMpaSplitterFile()
{
	SAFE_DELETE(m_pID3Tag);
	SAFE_DELETE(m_pAPETag);
}

#define MPA_HEADER_SIZE  4
#define ADTS_HEADER_SIZE 9

#define MOVE_TO_MPA_START_CODE(b, e) while(b <= e - MPA_HEADER_SIZE  && ((GETWORD(b) & MPA_SYNCWORD) != MPA_SYNCWORD)) b++;
#define MOVE_TO_AAC_START_CODE(b, e) while(b <= e - ADTS_HEADER_SIZE && ((GETWORD(b) & AAC_ADTS_SYNCWORD) != AAC_ADTS_SYNCWORD)) b++;

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

	// some files can be determined as Mpeg Audio
	if ((BitRead(24, true) == 0x000001)		||	// MPEG-PS? (0x000001BA)
		(BitRead(32, true) == 'RIFF')		||	// RIFF files (AVI, WAV, AMV and other)
		(BitRead(24, true) == 'AMV')		||	// MTV files (.mtv)
		(BitRead(32, true) == 'FORM')		||	// IFF files (ANIM)
		(BitRead(32, true) == 0x47400010)	||	// ?
		((BitRead(64, true) & 0x00000000FFFFFFFF) == 0x47400010)) { // MPEG-TS
		return E_FAIL;
	}

	Seek(0);

	if (BitRead(24, true) == 'ID3') {
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

			BYTE* buf = DNew BYTE[size];
			ByteRead(buf, size);

			m_pID3Tag = DNew CID3Tag(major, flags);
			m_pID3Tag->ReadTagsV2(buf, size);
			delete [] buf;
		}

		Seek(m_startpos);
		for (int i = 0; i < (1 << 20) && m_startpos < endpos && BitRead(8, true) == 0; i++) {
			BitRead(8), m_startpos++;
		}
	}

	endpos = GetLength();
	if (IsRandomAccess()) {
		bool bID3TagPresent = false;
		if (endpos > ID3v1_TAG_SIZE) {
			Seek(endpos - ID3v1_TAG_SIZE);

			if (BitRead(24) == 'TAG') {
				bID3TagPresent = true;
				endpos -= ID3v1_TAG_SIZE;

				if (!m_pID3Tag) {
					m_pID3Tag = DNew CID3Tag();
				}

				size_t size = 128 - 3;
				BYTE* buf = DNew BYTE[size];
				ByteRead(buf, size);
				m_pID3Tag->ReadTagsV1(buf, size);

				delete [] buf;
			}
		}

		if (!bID3TagPresent && endpos > APE_TAG_FOOTER_BYTES) {
			BYTE buf[APE_TAG_FOOTER_BYTES] = {};

			Seek(endpos - APE_TAG_FOOTER_BYTES);
			if (ByteRead(buf, APE_TAG_FOOTER_BYTES) == S_OK) {
				m_pAPETag = DNew CAPETag;
				if (m_pAPETag->ReadFooter(buf, APE_TAG_FOOTER_BYTES) && m_pAPETag->GetTagSize()) {
					const size_t tag_size = m_pAPETag->GetTagSize();
					Seek(endpos - tag_size);
					BYTE *p = DNew BYTE[tag_size];
					if (ByteRead(p, tag_size) == S_OK) {
						m_pAPETag->ReadTags(p, tag_size);
					}
					delete [] p;

					endpos -= tag_size;
				}
			}
		}
	}

	Seek(m_startpos);

	__int64 startpos = 0;

	const __int64 limit = IsRandomAccess() ? MEGABYTE : 64 * KILOBYTE;
	const __int64 endDataPos = std::min(endpos / 2, limit + m_startpos);
	const __int64 size = endDataPos - m_startpos;
	BYTE* buffer = DNew BYTE[size];

	int valid_cnt = 0;
	if (S_OK == ByteRead(buffer, size)) {
		BYTE* start     = buffer;
		const BYTE* end = start + size;
		while (m_mode != mode::mpa) {
			MOVE_TO_MPA_START_CODE(start, end);
			if (start < end - MPA_HEADER_SIZE) {
				startpos = m_startpos + (start - buffer);
				int frame_size = ParseMPAHeader(start);
				if (frame_size == 0) {
					start++;
					continue;
				}
				if (start + frame_size >= end) {
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
			BYTE* start     = buffer;
			const BYTE* end = start + size;
			while (m_mode != mode::mp4a) {
				MOVE_TO_AAC_START_CODE(start, end);
				if (start < end - ADTS_HEADER_SIZE) {
					startpos = m_startpos + (start - buffer);
					int frame_size = ParseADTSAACHeader(start);
					if (frame_size == 0) {
						start++;
						continue;
					}
					if (start + frame_size >= end) {
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
	}

	delete [] buffer;

	if (m_mode == mode::none) {
		return E_FAIL;
	}

	m_startpos = startpos;
	Seek(m_startpos);

	if (m_mode == mode::mpa) {
		Read(m_mpahdr, MPA_HEADER_SIZE, &m_mt, true);
	} else {
		Read(m_aachdr, ADTS_HEADER_SIZE, &m_mt, false);
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
	__int64 endpos = std::min(GetLength(), GetPos() + limit);

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

				FrameSize	= h.FrameSize;
				rtDuration	= h.rtDuration;

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

					FrameSize	= h.FrameSize;
					rtDuration	= h.rtDuration;

					return true;
				}
			} else {
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
		int rValue;
		if (!m_pos2fsize.Lookup(GetPos(), rValue)) {
			m_procsize += framesize;
			m_pos2fsize.SetAt(GetPos(), framesize);
			m_rtDuration = m_coefficient * m_pos2fsize.GetCount() / m_procsize;
		}
	}
}
