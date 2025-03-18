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
#include "DSMSplitterFile.h"
#include "DSUtil/DSUtil.h"
#include <moreuuids.h>

CDSMSplitterFile::CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr, IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap)
	: CBaseSplitterFile(pReader, hr, FM_FILE | FM_FILE_DL)
	, m_rtFirst(0)
	, m_rtDuration(0)
{
	if (FAILED(hr)) {
		return;
	}

	hr = Init(res, chap);
}

HRESULT CDSMSplitterFile::Init(IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap)
{
	Seek(0);

	if (BitRead(DSMSW_SIZE<<3) != DSMSW || BitRead(5) != DSMP_FILEINFO) {
		return E_FAIL;
	}

	Seek(0);

	m_mts.clear();
	m_rtFirst = m_rtDuration = 0;
	m_fim.clear();
	m_sim.clear();
	res.ResRemoveAll();
	chap.ChapRemoveAll();

	dsmp_t type;
	UINT64 len;
	int limit = 65536;

	// examine the beginning of the file ...

	while (Sync(type, len, 0)) {
		__int64 pos = GetPos();

		if (type == DSMP_MEDIATYPE) {
			BYTE id;
			CMediaType mt;
			if (Read(len, id, mt)) {
				m_mts[id] = mt;
			}
		} else if (type == DSMP_SAMPLE) {
			CPacket p;
			if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
				m_rtFirst = p.rtStart;
				break;
			}
		} else if (type == DSMP_FILEINFO) {
			if ((BYTE)BitRead(8) > DSMF_VERSION) {
				return E_FAIL;
			}
			Read(len-1, m_fim);
		} else if (type == DSMP_STREAMINFO) {
			Read(len-1, m_sim[(BYTE)BitRead(8)]);
		} else if (type == DSMP_SYNCPOINTS) {
			Read(len, m_sps);
		} else if (type == DSMP_RESOURCE) {
			Read(len, res);
		} else if (type == DSMP_CHAPTERS) {
			Read(len, chap);
		}

		Seek(pos + len);
	}

	if (type != DSMP_SAMPLE) {
		return E_FAIL;
	}

	// ... and the end

	if (IsRandomAccess())
		for (int i = 1, j = (int)((GetLength()+limit/2)/limit); i <= j; i++) {
			__int64 seekpos = std::max(0LL, GetLength() - i*limit);
			Seek(seekpos);

			while (Sync(type, len, limit) && GetPos() < seekpos+limit) {
				__int64 pos = GetPos();

				if (type == DSMP_SAMPLE) {
					CPacket p;
					if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
						m_rtDuration = std::max(m_rtDuration, p.rtStop - m_rtFirst); // max isn't really needed, only for safety
						i = j;
					}
				} else if (type == DSMP_SYNCPOINTS) {
					Read(len, m_sps);
				} else if (type == DSMP_RESOURCE) {
					Read(len, res);
				} else if (type == DSMP_CHAPTERS) {
					Read(len, chap);
				}

				Seek(pos + len);
			}
		}

	if (m_rtFirst < 0) {
		m_rtDuration += m_rtFirst;
		m_rtFirst = 0;
	}

	return m_mts.size() > 0 ? S_OK : E_FAIL;
}

bool CDSMSplitterFile::Sync(dsmp_t& type, UINT64& len, __int64 limit)
{
	UINT64 pos;
	return Sync(pos, type, len, limit);
}

bool CDSMSplitterFile::Sync(UINT64& syncpos, dsmp_t& type, UINT64& len, __int64 limit)
{
	BitByteAlign();

	limit += DSMSW_SIZE;

	for (UINT64 id = 0; (id&((1ui64<<(DSMSW_SIZE<<3))-1)) != DSMSW; id = (id << 8) | (BYTE)BitRead(8)) {
		if (limit-- <= 0 || GetRemaining() <= 2) {
			return false;
		}
	}

	syncpos = GetPos() - (DSMSW_SIZE<<3);
	type = (dsmp_t)BitRead(5);
	len = BitRead(((int)BitRead(3)+1)<<3);

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, BYTE& id, CMediaType& mt)
{
	id = (BYTE)BitRead(8);
	ByteRead((BYTE*)&mt.majortype, sizeof(mt.majortype));
	ByteRead((BYTE*)&mt.subtype, sizeof(mt.subtype));
	mt.bFixedSizeSamples = (BOOL)BitRead(1);
	mt.bTemporalCompression = (BOOL)BitRead(1);
	mt.lSampleSize = (ULONG)BitRead(30);
	ByteRead((BYTE*)&mt.formattype, sizeof(mt.formattype));
	len -= 5 + sizeof(GUID)*3;
	ASSERT(len >= 0);
	if (len > 0) {
		mt.AllocFormatBuffer((LONG)len);
		ByteRead(mt.Format(), mt.FormatLength());
	} else {
		mt.ResetFormatBuffer();
	}
	return true;
}

bool CDSMSplitterFile::Read(__int64 len, CPacket* p, bool fData)
{
	if (!p) {
		return false;
	}

	p->TrackNumber = (DWORD)BitRead(8);
	p->bSyncPoint = (BOOL)BitRead(1);
	bool fSign = !!BitRead(1);
	int iTimeStamp = (int)BitRead(3);
	int iDuration = (int)BitRead(3);

	if (fSign && !iTimeStamp) {
		ASSERT(!iDuration);
		p->rtStart = INVALID_TIME;
		p->rtStop = INVALID_TIME + 1;
	} else {
		p->rtStart = (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		p->rtStop = p->rtStart + BitRead(iDuration<<3);
	}

	if (fData) {
		p->resize((INT_PTR)len - (2 + iTimeStamp + iDuration));
		ByteRead(p->data(), p->size());
	}

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, std::vector<SyncPoint>& sps)
{
	SyncPoint sp;
	sps.clear();

	while (len > 0) {
		bool fSign = !!BitRead(1);
		int iTimeStamp = (int)BitRead(3);
		int iFilePos = (int)BitRead(3);
		BitRead(1); // reserved

		sp.rt += (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		sp.fp += BitRead(iFilePos<<3);
		sps.push_back(sp);

		len -= 1 + iTimeStamp + iFilePos;
	}

	if (len != 0) {
		sps.clear();
		return false;
	}

	// TODO: sort sps

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, CStreamInfoMap& im)
{
	while (len >= 5) {
		CStringA key;
		ByteRead((BYTE*)key.GetBufferSetLength(4), 4);
		len -= 4;
		len -= Read(len, im[key]);
	}

	return len == 0;
}

bool CDSMSplitterFile::Read(__int64 len, IDSMResourceBagImpl& res)
{
	BYTE compression = (BYTE)BitRead(2);
	BYTE reserved = (BYTE)BitRead(6);
	UNREFERENCED_PARAMETER(reserved);
	len--;

	CDSMResource r;
	len -= Read(len, r.name);
	len -= Read(len, r.desc);
	len -= Read(len, r.mime);

	if (compression != 0) {
		return false;    // TODO
	}

	r.data.resize((size_t)len);
	ByteRead(r.data.data(), r.data.size());

	res += r;

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, IDSMChapterBagImpl& chap)
{
	CDSMChapter c(0, L"");

	while (len > 0) {
		bool fSign = !!BitRead(1);
		int iTimeStamp = (int)BitRead(3);
		BitRead(4); // reserved
		len--;

		c.rt += (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		len -= iTimeStamp;
		len -= Read(len, c.name);

		chap += c;
	}

	chap.ChapSort();

	return len == 0;
}

__int64 CDSMSplitterFile::Read(__int64 len, CStringW& str)
{
	char c;
	CStringA s;
	__int64 i = 0;
	while (i++ < len && (c = (char)BitRead(8)) != 0) {
		s += c;
	}
	str = UTF8ToWStr(s);
	return i;
}

__int64 CDSMSplitterFile::FindSyncPoint(REFERENCE_TIME rt)
{
	if (/*!m_sps.IsEmpty()*/ m_sps.size() > 1) {
		int i = range_bsearch(m_sps, m_rtFirst + rt);
		return i >= 0 ? m_sps[i].fp : 0;
	}

	if (m_rtDuration <= 0 || rt <= m_rtFirst) {
		return 0;
	}

	// ok, do the hard way then

	dsmp_t type;
	UINT64 syncpos, len;

	// 1. find some boundaries close to rt's position (minpos, maxpos)

	__int64 minpos = 0, maxpos = GetLength();

	for (int i = 0; i < 10 && (maxpos - minpos) >= 1024*1024; i++) {
		Seek((minpos + maxpos) / 2);

		while (GetPos() < maxpos) {
			if (!Sync(syncpos, type, len)) {
				continue;
			}

			__int64 pos = GetPos();

			if (type == DSMP_SAMPLE) {
				CPacket p;
				if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
					REFERENCE_TIME dt = (p.rtStart -= m_rtFirst) - rt;
					if (dt >= 0) {
						maxpos = std::max((__int64)syncpos - 65536, minpos);
					} else {
						minpos = syncpos;
					}
					break;
				}
			}

			Seek(pos + len);
		}
	}

	// 2. find the first packet just after rt (maxpos)

	Seek(minpos);

	while (GetRemaining()) {
		if (!Sync(syncpos, type, len)) {
			continue;
		}

		__int64 pos = GetPos();

		if (type == DSMP_SAMPLE) {
			CPacket p;
			if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
				REFERENCE_TIME dt = (p.rtStart -= m_rtFirst) - rt;
				if (dt >= 0) {
					maxpos = (__int64)syncpos;
					break;
				}
			}
		}

		Seek(pos + len);
	}

	// 3. iterate backwards from maxpos and find at least one syncpoint for every stream, except for subtitle streams

	std::map<BYTE,BYTE> ids;

	for (const auto&[id, mt] : m_mts) {
		if (mt.majortype != MEDIATYPE_Text && mt.majortype != MEDIATYPE_Subtitle) {
			ids[id] = 0;
		}
	}

	__int64 ret = maxpos;

	while (maxpos > 0 && ids.size()) {
		minpos = std::max(0LL, maxpos - 65536);

		Seek(minpos);

		while (Sync(syncpos, type, len) && GetPos() < maxpos) {
			UINT64 pos = GetPos();

			if (type == DSMP_SAMPLE) {
				CPacket p;
				if (Read(len, &p, false) && p.rtStart != INVALID_TIME && p.bSyncPoint) {
					auto it = ids.find((BYTE)p.TrackNumber);

					if (it != ids.end()) {
						ids.erase(it);
						ret = std::min(ret, (__int64)syncpos);
					}
				}
			}

			Seek(pos + len);
		}

		maxpos = minpos;
	}

	return ret;
}
