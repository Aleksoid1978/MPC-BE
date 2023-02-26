/*
 * (C) 2023 see Authors.txt
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
#include <wincrypt.h>

CStringW BynaryToBase64W(const BYTE* bytes, DWORD bytesLen)
{
	CStringW base64;

	DWORD base64Len = 0;
	if (::CryptBinaryToStringW(bytes, bytesLen,
			CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
			nullptr, &base64Len)) {

		if (!::CryptBinaryToStringW(bytes, bytesLen,
				CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
				base64.GetBufferSetLength(base64Len), &base64Len)) {

			base64.Empty();
		}
	}

	return base64;
}


unsigned Base64ToBynary(CStringW base64, BYTE** ppBytes)
{
	ASSERT(*ppBytes == nullptr);

	DWORD dwLen = 0;
	if (::CryptStringToBinaryW(base64, base64.GetLength(),
		CRYPT_STRING_BASE64,
		nullptr, &dwLen,
		nullptr, nullptr)) {

		*ppBytes = new(std::nothrow) BYTE[dwLen];
		if (*ppBytes) {
			if (::CryptStringToBinaryW(base64, base64.GetLength(),
				CRYPT_STRING_BASE64,
				*ppBytes, &dwLen,
				nullptr, nullptr)) {

				return dwLen;
			}

			delete[] *ppBytes;
			*ppBytes = nullptr;
		}
	}

	return 0;
}

unsigned Base64ToBynary(CStringW base64, std::unique_ptr<BYTE[]>& pBytes)
{
	DWORD dwLen = 0;
	if (::CryptStringToBinaryW(base64, base64.GetLength(),
		CRYPT_STRING_BASE64,
		nullptr, &dwLen,
		nullptr, nullptr)) {

		pBytes.reset(new(std::nothrow) BYTE[dwLen]);
		if (pBytes && ::CryptStringToBinaryW(base64, base64.GetLength(),
			CRYPT_STRING_BASE64,
			pBytes.get(), &dwLen,
			nullptr, nullptr)) {

			return dwLen;
		}
	}

	return 0;
}

unsigned Base64ToBynary(CStringW base64, std::vector<BYTE>& bytes)
{
	DWORD dwLen = 0;
	if (::CryptStringToBinaryW(base64, base64.GetLength(),
			CRYPT_STRING_BASE64,
			nullptr, &dwLen,
			nullptr, nullptr)) {

		bytes.resize(dwLen);
		if (::CryptStringToBinaryW(base64, base64.GetLength(),
			CRYPT_STRING_BASE64,
			bytes.data(), &dwLen,
			nullptr, nullptr)) {

			return bytes.size();
		}
	}

	return 0;
}
