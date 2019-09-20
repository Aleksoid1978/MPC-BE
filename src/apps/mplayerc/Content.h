/*
 * (C) 2016-2018 see Authors.txt
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

#pragma once

#include <afxadv.h>
#include <atlstr.h>

// TODO: make function
#define CorrectAceStream(path) if (path.Left(12) == L"acestream://") {path.Format(AfxGetAppSettings().strAceStreamAddress, path.Mid(12));}

namespace Content {
	namespace Online {
		const bool CheckConnect(const CString& fn);
		void Clear();
		void Clear(const CString& fn);
		void Disconnect(const CString& fn);
		void GetRaw(const CString& fn, std::vector<BYTE>& raw);
		void GetHeader(const CString& fn, CString& hdr);
	}
	const CString GetType(CString fn, std::list<CString>* redir = nullptr);
}
