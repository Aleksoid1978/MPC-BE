/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include "AviFile.h"
#include "DSUtil/AudioParser.h"

//
// CAviFile
//

CAviFile::CAviFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE)
{
	if (FAILED(hr)) {
		return;
	}

	ZeroMemory(&m_vprp, sizeof(m_vprp));
	m_isamv = false;
	hr = Init();
}

HRESULT CAviFile::Init()
{
	Seek(0);
	DWORD dw[3];
	if (S_OK != ReadAvi(dw) || dw[0] != FCC('RIFF') || (dw[2] != FCC('AVI ') && dw[2] != FCC('AVIX') && dw[2] != FCC('AMV '))) {
		return E_FAIL;
	}

	m_isamv = (dw[2] == FCC('AMV '));
	Seek(0);
	HRESULT hr = Parse(0, GetLength());
	UNREFERENCED_PARAMETER(hr);
	if (m_movis.empty()) { // FAILED(hr) is allowed as long as there was a movi chunk found
		return E_FAIL;
	}

	if (m_avih.dwStreams == 0 && m_strms.size() > 0) {
		m_avih.dwStreams = (DWORD)m_strms.size();
	}

	if (m_avih.dwStreams != m_strms.size()) {
		return E_FAIL;
	}

	for (DWORD i = 0; i < m_avih.dwStreams; ++i) {
		strm_t* s = m_strms[i].get();
		if (s->strh.fccType != FCC('auds')) {
			continue;
		}
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)s->strf.data();
		if (wfe->wFormatTag == WAVE_FORMAT_MPEGLAYER3 && wfe->nBlockAlign == 1152
				&& s->strh.dwScale == 1 && s->strh.dwRate != wfe->nSamplesPerSec) {
			// correcting encoder bugs...
			s->strh.dwScale = 1152;
			s->strh.dwRate = wfe->nSamplesPerSec;
		} else if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)s->strf.data();
			if (wfex->dwChannelMask == 0) {
				// correcting muxer bugs...
				wfex->dwChannelMask = GetDefChannelMask(wfe->nChannels);
			}
		}
	}

	if (!m_isamv && (FAILED(BuildIndex()))) {
		EmptyIndex();
	}

	return S_OK;
}

HRESULT CAviFile::BuildAMVIndex()
{
	strm_t::chunk NewChunk;
	DWORD ulType;
	DWORD ulSize;

	memset (&NewChunk, 0, sizeof(strm_t::chunk));
	while ((ReadAvi(ulType) == S_OK) && (ReadAvi(ulSize) == S_OK)) {
		switch (ulType) {
			case FCC('00dc'): // 01bw : JPeg
				NewChunk.size = ulSize;
				NewChunk.filepos = GetPos();
				NewChunk.orgsize = ulSize;
				NewChunk.fKeyFrame = true;
				m_strms[0]->cs.push_back(NewChunk);
				break;
			case FCC('01wb'): // 00dc : Audio
				NewChunk.size    = ulSize;
				NewChunk.orgsize = ulSize;
				NewChunk.fKeyFrame = true;
				NewChunk.filepos = GetPos();
				m_strms[1]->cs.push_back(NewChunk);
				break;
		}
		Seek(GetPos() + ulSize);
	}

	DLog(L"CAviFile::BuildAMVIndex() : Video packet : %Iu, Audio packet : %Iu", m_strms[0]->cs.size(), m_strms[1]->cs.size());
	return S_OK;
}

HRESULT CAviFile::Parse(DWORD parentid, __int64 end)
{
	HRESULT hr = S_OK;

	std::unique_ptr<strm_t> strm;

	while (S_OK == hr && GetPos() < end) {
		UINT64 pos = GetPos();

		DWORD id = 0, size;
		if (S_OK != ReadAvi(id) || id == 0) {
			return E_FAIL;
		}

		if (id == FCC('RIFF') || id == FCC('LIST')) {
			if (S_OK != ReadAvi(size) || S_OK != ReadAvi(id)) {
				return E_FAIL;
			}

			if (m_isamv) {
				size = (DWORD)(std::min(end, (__int64)DWORD_MAX) - GetPos() - 8); // No size set in AVM : guess end of file...
			}
			size += (size&1) + 8;

			DLog(L"CAviFile::Parse() : LIST '%c%c%c%c'",
					WCHAR((id>>0)&0xff),
					WCHAR((id>>8)&0xff),
					WCHAR((id>>16)&0xff),
					WCHAR((id>>24)&0xff));

			if (id == FCC('movi')) {
				m_movis.push_back(pos);
				if (m_isamv) {
					BuildAMVIndex();
				}
			} else {
				hr = Parse(id, pos + size);
			}
		} else {
			if (S_OK != ReadAvi(size)) {
				return E_FAIL;
			}

			DWORD cur_pos = GetPos();
			if (cur_pos < end) {
				size = (DWORD)std::min((__int64)size, end - cur_pos);
			} else if (cur_pos + size > end) {
				return E_FAIL; // broken chunk
			}

			DLog(L"CAviFile::Parse() : '%c%c%c%c'",
					WCHAR((id>>0)&0xff),
					WCHAR((id>>8)&0xff),
					WCHAR((id>>16)&0xff),
					WCHAR((id>>24)&0xff));

			if (parentid == FCC('INFO') && size > 0) {
				switch (id) {
					case FCC('IARL'): // Archival Location. Indicates where the subject of the file is archived.
					case FCC('IART'): // Artist. Lists the artist of the original subject of the file; for example, "Michaelangelo."
					case FCC('ICMS'): // Commissioned. Lists the name of the person or organization that commissioned the subject of the file; for example, "Pope Julian II."
					case FCC('ICMT'): // Comments. Provides general comments about the file or the subject of the file. If the comment is several sentences long, end each sentence with a period. Do not include new-line characters.
					case FCC('ICOP'): // Copyright. Records the copyright information for the file; for example, "Copyright Encyclopedia International 1991." If there are multiple copyrights, separate them by a semicolon followed by a space.
					case FCC('ICRD'): // Creation date. Specifies the date the subject of the file was created. List dates in year-month-day format, padding one-digit months and days with a zero on the left; for example, "1553-05-03" for May 3, 1553.
					case FCC('ICRP'): // Cropped. Describes whether an image has been cropped and, if so, how it was cropped; for example, "lower-right corner."
					case FCC('IDIM'): // Dimensions. Specifies the size of the original subject of the file; for example, "8.5 in h, 11 in w."
					case FCC('IDPI'): // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as "300."
					case FCC('IENG'): // Engineer. Stores the name of the engineer who worked on the file. If there are multiple engineers, separate the names by a semicolon and a blank; for example, "Smith, John; Adams, Joe."
					case FCC('IGNR'): // Genre. Describes the original work, such as "landscape," "portrait," "still life," etc.
					case FCC('IKEY'): // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon and a blank; for example, "Seattle; aerial view; scenery."
					case FCC('ILGT'): // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
					case FCC('IMED'): // Medium. Describes the original subject of the file, such as "computer image," "drawing," "lithograph," and so on.
					case FCC('INAM'): // Name. Stores the title of the subject of the file, such as "Seattle From Above."
					case FCC('IPLT'): // Palette Setting. Specifies the number of colors requested when digitizing an image, such as "256."
					case FCC('IPRD'): // Product. Specifies the name of the title the file was originally intended for, such as "Encyclopedia of Pacific Northwest Geography."
					case FCC('ISBJ'): // Subject. Describes the contents of the file, such as "Aerial view of Seattle."
					case FCC('ISFT'): // Software. Identifies the name of the software package used to create the file, such as "Microsoft WaveEdit."
					case FCC('ISHP'): // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
					case FCC('ISRC'): // Source. Identifies the name of the person or organization who supplied the original subject of the file; for example, "Trey Research."
					case FCC('ISRF'): // Source Form. Identifies the original form of the material that was digitized, such as "slide," "paper," "map," and so on. This is not necessarily the same as IMED.
					case FCC('ITCH'): { // Technician. Identifies the technician who digitized the subject file; for example, "Smith, John."
						CStringA str;
						if (S_OK != ByteRead((BYTE*)str.GetBufferSetLength(size), size)) {
							return E_FAIL;
						}
						m_info[id] = str;
						break;
					}
				}
			}

			switch (id) {
				case FCC('amvh'):
				case FCC('avih'):
					m_avih.fcc = id;
					m_avih.cb = size;
					if (S_OK != ReadAvi(m_avih, 8)) {
						return E_FAIL;
					}
					break;
				case FCC('strh'):
					if (!strm) {
						strm.reset(DNew strm_t());
					}
					strm->strh.fcc = FCC('strh');
					strm->strh.cb = size;
					if (S_OK != ReadAvi(strm->strh, 8)) {
						return E_FAIL;
					}
 					if (m_isamv) {
						// First alway video, second always audio
						strm->strh.fccType = m_strms.size() == 0 ? FCC('vids') : FCC('amva');
						strm->strh.dwRate  = m_avih.dwReserved[0]*1000; // dwReserved[0] = fps!
						strm->strh.dwScale = 1000;
					}
					break;
				case FCC('strn'):
					if (S_OK != ByteRead((BYTE*)strm->strn.GetBufferSetLength(size), size)) {
						return E_FAIL;
					}
					break;
				case FCC('strf'):
					if (!strm) {
						strm.reset(DNew strm_t());
					}
					strm->strf.resize(size);
					if (S_OK != ByteRead(strm->strf.data(), size)) {
						return E_FAIL;
					}
					if (strm->strh.fccType == FCC('auds')) {
						auto pwfe = reinterpret_cast<WAVEFORMATEX*>(strm->strf.data());
						if (!pwfe->nBlockAlign && strm->strh.dwScale) {
							pwfe->nBlockAlign = strm->strh.dwScale;
						}
					}
					if (m_isamv) {
						if (strm->strh.fccType == FCC('vids')) {
							strm->strf.resize(sizeof(BITMAPINFOHEADER));
							BITMAPINFOHEADER* pbmi = &((BITMAPINFO*)strm->strf.data())->bmiHeader;
							pbmi->biSize        = sizeof(BITMAPINFOHEADER);
							pbmi->biHeight      = m_avih.dwHeight;
							pbmi->biWidth       = m_avih.dwWidth;
							pbmi->biCompression = FCC('AMVV');
							pbmi->biPlanes      = 1;
							pbmi->biBitCount    = 24;
							pbmi->biSizeImage   = DIBSIZE(*pbmi);
						}
						m_strms.emplace_back(std::move(strm));
					}

					break;
				case FCC('indx'):
					if (!strm) {
						strm.reset(DNew strm_t());
					}
					ASSERT(strm->indx == nullptr);
					if (size < MAXDWORD-8) {
						AVISUPERINDEX* pSuperIndex;
						// Fix buffer overrun vulnerability : http://www.vulnhunt.com/advisories/CAL-20070912-1_Multiple_vendor_produce_handling_AVI_file_vulnerabilities.txt
						TRY {
							pSuperIndex = (AVISUPERINDEX*)DNew unsigned char [(size_t)(size + 8)];
						}
						CATCH (CMemoryException, e) {
							pSuperIndex = nullptr;
						}
						END_CATCH
						if (pSuperIndex) {
							strm->indx.reset(std::move(pSuperIndex));
							strm->indx->fcc = FCC('indx');
							strm->indx->cb = size;
							if (S_OK != ByteRead((BYTE*)strm->indx.get() + 8, size)) {
								return E_FAIL;
							}
							ASSERT(strm->indx->wLongsPerEntry == 4 && strm->indx->bIndexType == AVI_INDEX_OF_INDEXES);
						}
					}
					break;
				case FCC('dmlh'):
					if (S_OK != ReadAvi(m_dmlh)) {
						return E_FAIL;
					}
					break;
				case FCC('vprp'):
					if (S_OK != ReadAvi(m_vprp)) {
						return E_FAIL;
					}
					break;
				case FCC('idx1'):
					ASSERT(m_idx1 == nullptr);
					m_idx1.reset(DNew BYTE[size + 8]);
					{
						AVIOLDINDEX* idx = (AVIOLDINDEX*)m_idx1.get();
						idx->fcc = FCC('idx1');
						idx->cb = size;
					}
					if (S_OK != ByteRead(m_idx1.get() + 8, size)) {
						return E_FAIL;
					}
					break;
				default :
					DLog(L"CAviFile::Parse() : unknown tag '%c%c%c%c'",
							WCHAR((id>>0)&0xff),
							WCHAR((id>>8)&0xff),
							WCHAR((id>>16)&0xff),
							WCHAR((id>>24)&0xff));
					break;
			}

			size += (size&1) + 8;
		}

		Seek(pos + size);
	}

	if (strm) {
		m_strms.emplace_back(std::move(strm));
	}

	return hr;
}

REFERENCE_TIME CAviFile::GetTotalTime()
{
	REFERENCE_TIME total = 0;

	// first - try to get the length of the video track
	for (DWORD i = 0; i < m_avih.dwStreams; ++i) {
		strm_t* s = m_strms[i].get();
		if (s->strh.fccType == FCC('vids')) {
			ASSERT(s->strf.size() >= sizeof(BITMAPINFOHEADER));

			BITMAPINFOHEADER* pbmi = &((BITMAPINFO*)s->strf.data())->bmiHeader;
			if (pbmi->biCompression != FCC('DXSB') && pbmi->biCompression != FCC('DXSA')) { // skip XSUB subtitle
				REFERENCE_TIME t = s->GetRefTime((DWORD)s->cs.size(), s->totalsize);
				total = std::max(total, t);
			}
		}
	}

	if (!total) {
		// second - try to get the length of the audio track
		for (DWORD i = 0; i < m_avih.dwStreams; ++i) {
			strm_t* s = m_strms[i].get();
			if (s->strh.fccType == FCC('auds') || s->strh.fccType == FCC('amva')) {
				REFERENCE_TIME t = s->GetRefTime((DWORD)s->cs.size(), s->totalsize);
				total = std::max(total, t);
			}
		}
	}

	if (!total) {
		total = 10i64*m_avih.dwMicroSecPerFrame*m_avih.dwTotalFrames;
	}

	return total;
}

HRESULT CAviFile::BuildIndex()
{
	EmptyIndex();

	DWORD nSuperIndexes = 0;

	for (DWORD track = 0; track < m_avih.dwStreams; track++) {
		strm_t* s = m_strms[track].get();
		if (s->indx && s->indx->nEntriesInUse > 0) {
			nSuperIndexes++;
		}
	}

	if (nSuperIndexes == m_avih.dwStreams) {
		for (DWORD track = 0; track < m_avih.dwStreams; track++) {
			strm_t* s = m_strms[track].get();

			AVISUPERINDEX* idx = (AVISUPERINDEX*)s->indx.get();

			DWORD nEntriesInUse = 0;

			for (DWORD j = 0; j < idx->nEntriesInUse; ++j) {
				Seek(idx->aIndex[j].qwOffset);

				AVISTDINDEX stdidx;
				if (S_OK != ByteRead((BYTE*)&stdidx, FIELD_OFFSET(AVISTDINDEX, aIndex)) || (WORD)stdidx.fcc != 'xi') { // fcc = 'ix00', 'ix01', 'ix02',...
					EmptyIndex();
					return E_FAIL;
				}

				nEntriesInUse += stdidx.nEntriesInUse;
			}

			s->cs.resize(nEntriesInUse);

			DWORD frame = 0;
			UINT64 size = 0;

			for (DWORD j = 0; j < idx->nEntriesInUse; ++j) {
				Seek(idx->aIndex[j].qwOffset);

				std::unique_ptr<BYTE[]> pBuf(new(std::nothrow) BYTE[idx->aIndex[j].dwSize]);
				AVISTDINDEX* p = (AVISTDINDEX*)pBuf.get();

				if (!p || S_OK != ByteRead(pBuf.get(), idx->aIndex[j].dwSize) || p->qwBaseOffset >= (DWORDLONG)GetLength()) {
					EmptyIndex();
					return E_FAIL;
				}

				if (p->wLongsPerEntry == 6) {
					// Matrox's MPEG-2 stuff generates bIndexSubType=16 and wLongsPerEntry=6
					for (DWORD k = 0; k < p->nEntriesInUse * 3; k += 3) {
						s->cs[frame].filepos = p->qwBaseOffset + p->aIndex[k].dwOffset;
						s->cs[frame].fKeyFrame = true;
						s->cs[frame].fChunkHdr = false;
						s->cs[frame].size = s->cs[frame].orgsize = p->aIndex[k+1].dwOffset;

						++frame;
					}
				} else {
					for (DWORD k = 0; k < p->nEntriesInUse; ++k) {
						s->cs[frame].size      = size;
						s->cs[frame].filepos   = p->qwBaseOffset + p->aIndex[k].dwOffset;
						s->cs[frame].fKeyFrame = !(p->aIndex[k].dwSize&AVISTDINDEX_DELTAFRAME)
												 || s->strh.fccType == FCC('auds');
						s->cs[frame].fChunkHdr = false;
						s->cs[frame].orgsize   = p->aIndex[k].dwSize&AVISTDINDEX_SIZEMASK;

						if (m_idx1) {
							s->cs[frame].filepos  -= 8;
							s->cs[frame].fChunkHdr = true;
						}

						++frame;
						size += s->GetChunkSize(p->aIndex[k].dwSize&AVISTDINDEX_SIZEMASK);
					}
				}
			}

			s->totalsize = size;
		}
	} else if (AVIOLDINDEX* idx = (AVIOLDINDEX*)m_idx1.get()) {
		size_t len    = idx->cb / sizeof(idx->aIndex[0]);

		UINT64 offset = m_movis.front() + 8;

		//detect absolute chunk addressing (TODO: read AVI specification and make it better)
		if (idx->aIndex[0].dwOffset > offset) {
			DWORD id;
			Seek(offset + idx->aIndex[0].dwOffset);
			ReadAvi(id);
			if (id != idx->aIndex[0].dwChunkId) {
				DLog(L"WARNING: CAviFile::BuildIndex() detected absolute chunk addressing in \'idx1\'");
				offset = 0;
			}
		}

		for (DWORD track = 0; track < m_avih.dwStreams; track++) {
			strm_t* s = m_strms[track].get();

			// calculate the number of frames and set index size before using it
			size_t nFrames = 0;
			for (DWORD j = 0; j < len; ++j) {
				if (TRACKNUM(idx->aIndex[j].dwChunkId) == track) {
					nFrames++;
				}
			}
			s->cs.resize(nFrames);

			// read index
			size_t frame = 0;
			UINT64 size = 0;
			for (size_t i = 0; i < len; i++) {
				if (TRACKNUM(idx->aIndex[i].dwChunkId) == track) {
					s->cs[frame].size      = size;
					s->cs[frame].filepos   = offset + idx->aIndex[i].dwOffset;
					s->cs[frame].fKeyFrame = !!(idx->aIndex[i].dwFlags&AVIIF_KEYFRAME)
											 || s->strh.fccType == FCC('auds') // FIXME: some audio index is without any kf flag
											 || frame == 0; // grrr
					s->cs[frame].fChunkHdr = i + 1 == len || idx->aIndex[i].dwOffset != idx->aIndex[i + 1].dwOffset;
					s->cs[frame].orgsize   = idx->aIndex[i].dwSize;

					frame++;
					size += s->GetChunkSize(idx->aIndex[i].dwSize);
				}
			}

			s->totalsize = size;
		}
	}

	m_idx1.reset();
	for (DWORD track = 0; track < m_avih.dwStreams; track++) {
		m_strms[track]->indx.reset();
	}

	return S_OK;
}

void CAviFile::EmptyIndex(LONG TrackNum)
{
	if (TrackNum > -1) {
		if ((DWORD)TrackNum < m_avih.dwStreams) {
			strm_t* s = m_strms[TrackNum].get();
			s->cs.clear();
			s->totalsize = 0;
		}
		return;
	}

	for (DWORD track = 0; track < m_avih.dwStreams; track++) {
		strm_t* s = m_strms[track].get();
		s->cs.clear();
		s->totalsize = 0;
	}
}

bool CAviFile::IsInterleaved(bool fKeepInfo)
{
	if (m_strms.size() < 2) {
		return true;
	}
	/*
		if (m_avih.dwFlags&AVIF_ISINTERLEAVED) // not reliable, nandub can write f*cked up files and still sets it
			return true;
	*/
	for (DWORD i = 0; i < m_avih.dwStreams; ++i) {
		m_strms[i]->cs2.resize(m_strms[i]->cs.size());
	}

	std::vector<DWORD> curchunks(m_avih.dwStreams);
	std::vector<UINT64> cursizes(m_avih.dwStreams);

	int end = 0;

	for (;;) {
		UINT64 fpmin = _I64_MAX;

		DWORD n = (DWORD)-1;
		for (DWORD i = 0; i < m_avih.dwStreams; ++i) {
			DWORD curchunk = curchunks[i];
			std::vector<strm_t::chunk>& cs = m_strms[i]->cs;
			if (curchunk >= cs.size()) {
				continue;
			}
			UINT64 fp = cs[curchunk].filepos;
			if (fp < fpmin) {
				fpmin = fp;
				n = i;
			}
		}
		if (n == -1) {
			break;
		}

		strm_t* s = m_strms[n].get();
		DWORD& curchunk = curchunks[n];
		UINT64& cursize = cursizes[n];

		if (!s->IsRawSubtitleStream()) {
			strm_t::chunk2& cs2 = s->cs2[curchunk];
			cs2.t = (DWORD)(s->GetRefTime(curchunk, cursize)>>13); // for comparing later it is just as good as /10000 to get a near [ms] accuracy
			//cs2.t = (DWORD)(s->GetRefTime(curchunk, cursize)/10000);
			cs2.n = end++;
		}

		cursize = s->cs[curchunk].size;
		++curchunk;
	}

	std::fill(curchunks.begin(), curchunks.end(), 0);

	strm_t::chunk2 cs2last = {(DWORD)-1, 0};

	bool fInterleaved = true;

	while (fInterleaved) {
		strm_t::chunk2 cs2min = {LONG_MAX, LONG_MAX};

		DWORD n = (DWORD)-1;
		for (DWORD i = 0; i < m_avih.dwStreams; ++i) {
			DWORD curchunk = curchunks[i];
			if (curchunk >= m_strms[i]->cs2.size()) {
				continue;
			}
			strm_t::chunk2& cs2 = m_strms[i]->cs2[curchunk];
			if (cs2.t < cs2min.t) {
				cs2min = cs2;
				n = i;
			}
		}
		if (n == -1) {
			break;
		}

		++curchunks[n];

		if (cs2last.t != (DWORD)-1 && abs((int)cs2min.n - (int)cs2last.n) >= 1000) {
			fInterleaved = false;
		}

		cs2last = cs2min;
	}

	if (fInterleaved && !fKeepInfo) {
		// this is not needed anymore, let's save a little memory then
		for (DWORD i = 0; i < m_avih.dwStreams; ++i) {
			m_strms[i]->cs2.clear();
		}
	}

	return fInterleaved;
}

REFERENCE_TIME CAviFile::strm_t::GetRefTime(DWORD frame, UINT64 size)
{
	if (strh.dwRate == 0) {
		return 0;
	}
	if (strh.fccType == FCC('auds')) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)strf.data();
		if (wfe->nBlockAlign == 0) {
			return 0;
		}
		return (REFERENCE_TIME)ceil(10000000.0 * (size + strh.dwStart) * strh.dwScale / (unsigned __int64(strh.dwRate) * wfe->nBlockAlign));
		// need calculate in double, because the (10000000ui64 * size * strh.dwScale) can give overflow
		// "ceil" is necessary to compensate for framenumber->reftime->framenumber conversion
	}
	return (REFERENCE_TIME)ceil(10000000.0 * (frame + strh.dwStart) * strh.dwScale / strh.dwRate);
	// need calculate in double, because the (10000000ui64 * frame * strh.dwScale) can give overflow (verified in practice)
	// "ceil" is necessary to compensate for framenumber->reftime->framenumber conversion
}

DWORD CAviFile::strm_t::GetFrame(REFERENCE_TIME rt)
{
	DWORD frame;

	if (strh.dwScale == 0 || rt <= 0 || cs.size() == 0) {
		frame = 0;
	} else if (strh.fccType == FCC('auds')) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)strf.data();

		UINT64 size = (UINT64)(double(rt) * wfe->nBlockAlign * strh.dwRate / (strh.dwScale * 10000000.0));
		// need calculate in double, because the (rt * wfe->nBlockAlign * strh.dwRate) can give overflow
		frame = 1;
		for (; frame < cs.size(); ++frame) {
			if (cs[frame].size > size) {
				break;
			}
		}
		--frame;
	} else {
		frame = (DWORD)(double(rt) * strh.dwRate / (strh.dwScale * 10000000.0));
		// need calculate in double, because the (rt * strh.dwRate) can give overflow (verified in practice)
		if (frame >= cs.size()) {
			frame = (DWORD)cs.size() - 1;
		}
	}

	return frame;
}

DWORD CAviFile::strm_t::GetKeyFrame(REFERENCE_TIME rt)
{
	DWORD i = GetFrame(rt);
	for (; i > 0; i--) {
		if (cs[i].fKeyFrame) {
			break;
		}
	}
	return i;
}

DWORD CAviFile::strm_t::GetChunkSize(DWORD size)
{
	if (strh.fccType == FCC('auds')) {
		WORD nBlockAlign = ((WAVEFORMATEX*)strf.data())->nBlockAlign;
		size = nBlockAlign ? (size + (nBlockAlign - 1)) / nBlockAlign * nBlockAlign : 0; // round up for nando's vbr hack
	}

	return size;
}

bool CAviFile::strm_t::IsRawSubtitleStream()
{
	return (strn.Find("Subtitle") == 0 || (strh.fccType == FCC('txts') && cs.size() == 1));
}
