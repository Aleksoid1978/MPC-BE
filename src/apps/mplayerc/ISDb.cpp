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
#include <atlpath.h>
#include "ISDb.h"

bool mpc_filehash(LPCTSTR fn, filehash& fh)
{
	CFile f;
	CFileException fe;

	if (!f.Open(fn, CFile::modeRead|CFile::osSequentialScan|CFile::shareDenyNone, &fe)) {
		return false;
	}

	CPath p(fn);
	p.StripPath();
	fh.name = (LPCTSTR)p;

	fh.size = f.GetLength();

	fh.mpc_filehash = fh.size;

	for (UINT64 tmp = 0, i = 0; i < 65536/sizeof(tmp) && f.Read(&tmp, sizeof(tmp)); fh.mpc_filehash += tmp, i++) {
		;
	}

	f.Seek(std::max(0LL, (INT64)fh.size - 65536), CFile::begin);

	for (UINT64 tmp = 0, i = 0; i < 65536/sizeof(tmp) && f.Read(&tmp, sizeof(tmp)); fh.mpc_filehash += tmp, i++) {
		;
	}

	return true;
}

void mpc_filehash(CPlaylist& pl, std::list<filehash>& fhs)
{
	fhs.clear();

	POSITION pos = pl.GetHeadPosition();

	while (pos) {
		CString fn = pl.GetNext(pos).m_fns.front();

		if (AfxGetAppSettings().m_Formats.FindAudioExt(CPath(fn).GetExtension().MakeLower())) {
			continue;
		}

		filehash fh;

		if (!mpc_filehash(fn, fh)) {
			continue;
		}

		fhs.push_back(fh);
	}
}

CStringA makeargs(CPlaylist& pl)
{
	std::list<filehash> fhs;
	mpc_filehash(pl, fhs);

	std::list<CStringA> args;

	int i = 0;
	for (const auto& fh : fhs) {
		CStringA str;
		str.Format("name[%d]=%s&size[%d]=%016I64x&hash[%d]=%016I64x",
				   i, UrlEncode(CStringA(fh.name), true),
				   i, fh.size,
				   i, fh.mpc_filehash);

		args.push_back(str);
		i++;
	}

	return Implode(args, '&');
}

bool OpenUrl(CInternetSession& is, CString url, CStringA& str)
{
	str.Empty();

	try {
		CAutoPtr<CStdioFile> f(is.OpenURL(url, 1, INTERNET_FLAG_TRANSFER_BINARY|INTERNET_FLAG_EXISTING_CONNECT));

		char buff[1024];
		for (int len; (len = f->Read(buff, sizeof(buff))) > 0; str += CStringA(buff, len)) {
			;
		}

		f->Close();
	} catch (CInternetException* ie) {
		ie->Delete();

		return false;
	}

	return true;
}
