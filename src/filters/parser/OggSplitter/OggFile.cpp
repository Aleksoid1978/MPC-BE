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
#include "OggFile.h"
#include <numeric>

COggFile::COggFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr, FM_FILE | FM_FILE_DL | FM_STREAM)
{
	if (FAILED(hr)) {
		return;
	}
	hr = Init();
}

HRESULT COggFile::Init()
{
	Seek(0);
	if (BitRead(32, true) != 'OggS') {
		return E_FAIL;
	}

	if (!Sync()) {
		return E_FAIL;
	}

	return S_OK;
}

bool COggFile::Sync(HANDLE hBreak)
{
	__int64 start = GetPos();

	DWORD dw;
	for (__int64 i = 0, j = hBreak ? GetLength() - start : MAX_PAGE_SIZE;
			i < j && S_OK == ByteRead((BYTE*)&dw, sizeof(dw))
			&& ((i&0xffff) || !hBreak || WaitForSingleObject(hBreak, 0) != WAIT_OBJECT_0);
			i++, Seek(start + i)) {
		if (dw == 'SggO') {
			Seek(start + i);
			return true;
		}
	}

	Seek(start);

	return false;
}

bool COggFile::Read(OggPageHeader& hdr, HANDLE hBreak)
{
	return Sync(hBreak) && S_OK == ByteRead((BYTE*)&hdr, sizeof(hdr)) && hdr.capture_pattern == 'SggO';
}

bool COggFile::Read(OggPage& page, const bool bFull, HANDLE hBreak)
{
	memset(&page.m_hdr, 0, sizeof(page.m_hdr));
	page.pos = 0;
	page.bComplete = false;
	page.m_lens.clear();
	page.clear();

	if (!Read(page.m_hdr, hBreak)|| !page.m_hdr.number_page_segments) {
		return false;
	}

	page.m_lens.resize(page.m_hdr.number_page_segments);
	if (S_OK != ByteRead(page.m_lens.data(), page.m_hdr.number_page_segments)) {
		return false;
	}

	page.pos = GetPos() - page.m_hdr.number_page_segments - sizeof(OggPageHeader);

	const size_t pagelen = std::accumulate(page.m_lens.cbegin(), page.m_lens.cend(), 0);
	if (bFull) {
		page.resize(pagelen);
		if (S_OK != ByteRead(page.data(), page.size())) {
			return false;
		}
	} else {
		Seek(GetPos() + pagelen);
	}

	page.bComplete = std::any_of(page.m_lens.cbegin(), page.m_lens.cend(), [](const BYTE len) {
		return len < 255;
	});

	return true;
}

bool COggFile::ReadPages(OggPage& page)
{
	bool bRet = Read(page);
	if (!page.bComplete) {
		for (;;) {
			OggPage page_next;
			bRet = Read(page_next);
			if (!bRet) {
				break;
			}
			page.insert(page.cend(), page_next.cbegin(), page_next.cend());
			page.m_lens.insert(page.m_lens.cend(), page_next.m_lens.cbegin(), page_next.m_lens.cend());
			if (page_next.bComplete) {
				break;
			}
		}
	}

	return bRet;
}
