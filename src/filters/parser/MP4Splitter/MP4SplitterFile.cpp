/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2019 see Authors.txt
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
#include "MP4SplitterFile.h"
#include "Ap4AsyncReaderStream.h"

CMP4SplitterFile::CMP4SplitterFile(IAsyncReader* pReader, HRESULT& hr)
	: CBaseSplitterFileEx(pReader, hr, FM_FILE | FM_FILE_DL | FM_STREAM)
	, m_pAp4File(nullptr)
{
	if (FAILED(hr)) {
		return;
	}

	hr = Init();
}

CMP4SplitterFile::~CMP4SplitterFile()
{
	SAFE_DELETE(m_pID3Tag);
	delete m_pAp4File;
}

AP4_Movie* CMP4SplitterFile::GetMovie()
{
	ASSERT(m_pAp4File);
	return m_pAp4File ? m_pAp4File->GetMovie() : nullptr;
}

HRESULT CMP4SplitterFile::Init()
{
	Seek(0);

	SAFE_DELETE(m_pAp4File);

	AP4_ByteStream* stream = DNew AP4_AsyncReaderStream(this);

	AP4_Offset pos = 0;
	if (BitRead(24) == 'ID3') {
		const BYTE major = (BYTE)BitRead(8);
		const BYTE revision = (BYTE)BitRead(8);
		UNREFERENCED_PARAMETER(revision);

		const BYTE flags = (BYTE)BitRead(8);

		DWORD size = BitRead(32);
		size = hexdec2uint(size);

		if (major <= 4) {
			BYTE* buf = DNew BYTE[size];
			if (SUCCEEDED(ByteRead(buf, size))) {
				m_pID3Tag = DNew CID3Tag(major, flags);
				m_pID3Tag->ReadTagsV2(buf, size);
			}
			delete[] buf;
		}

		pos = size + 10;
	}

	stream->Seek(pos);
	m_pAp4File = DNew AP4_File(*stream, IsURL());

	AP4_Movie* movie = m_pAp4File->GetMovie();

	stream->Release();

	return movie ? S_OK : E_FAIL;
}
