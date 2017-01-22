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
#include "MP4SplitterFile.h"
#include "Ap4AsyncReaderStream.h"

CMP4SplitterFile::CMP4SplitterFile(IAsyncReader* pReader, HRESULT& hr)
	: CBaseSplitterFileEx(pReader, hr, FM_FILE | FM_FILE_DL | FM_STREAM)
	, m_pAp4File(NULL)
{
	if (FAILED(hr)) {
		return;
	}

	hr = Init();
}

CMP4SplitterFile::~CMP4SplitterFile()
{
	delete m_pAp4File;
}

AP4_Movie* CMP4SplitterFile::GetMovie()
{
	ASSERT(m_pAp4File);
	return m_pAp4File ? m_pAp4File->GetMovie() : NULL;
}

HRESULT CMP4SplitterFile::Init()
{
	Seek(0);

	SAFE_DELETE(m_pAp4File);

	AP4_ByteStream* stream = DNew AP4_AsyncReaderStream(this);

	m_pAp4File = DNew AP4_File(*stream, IsURL());

	AP4_Movie* movie = m_pAp4File->GetMovie();

	stream->Release();

	return movie ? S_OK : E_FAIL;
}
