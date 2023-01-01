/*
 * (C) 2022-2023 see Authors.txt
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
#include "DSUtil/Log.h"
#include "AESDecryptor.h"

#include <Windows.h>
#include <winternl.h>

#pragma comment(lib, "Bcrypt.lib")

CAESDecryptor::CAESDecryptor()
{
	auto ret = BCryptOpenAlgorithmProvider(&m_hAesAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
	DLogIf(!NT_SUCCESS(ret), L"AESDecryptor::AESDecryptor() : BCryptOpenAlgorithmProvider() failed. Error code: %ld", ret);
}

CAESDecryptor::~CAESDecryptor()
{
	if (m_hKey) {
		BCryptDestroyKey(m_hKey);
	}
	if (m_hAesAlg) {
		BCryptCloseAlgorithmProvider(m_hAesAlg, 0);
	}
}

bool CAESDecryptor::SetKey(const BYTE* key, size_t keySize, const BYTE* iv, size_t ivSize)
{
	if (!m_hAesAlg) {
		return false;
	}

	if (keySize != AESBLOCKSIZE) {
		return false;
	}
	if (ivSize != AESBLOCKSIZE) {
		return false;
	}

	ULONG uKeySize = 0;
	ULONG uSize = 0;
	auto ret = BCryptGetProperty(m_hAesAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&uKeySize), sizeof(ULONG), &uSize, 0);
	if (!NT_SUCCESS(ret)) {
		return false;
	}
	m_pKeyObject.reset((PUCHAR)HeapAlloc(GetProcessHeap(), 0, uKeySize));

	ret = BCryptGetProperty(m_hAesAlg, BCRYPT_BLOCK_LENGTH, reinterpret_cast<PUCHAR>(&m_BlockLen), sizeof(ULONG), &uSize, 0);
	if (!NT_SUCCESS(ret)) {
		return false;
	}
	if (m_BlockLen != AESBLOCKSIZE) {
		return false;
	}

	ret = BCryptSetProperty(m_hAesAlg, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_CBC)), sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
	if (!NT_SUCCESS(ret)) {
		return false;
	}

	ret = BCryptGenerateSymmetricKey(m_hAesAlg, &m_hKey, m_pKeyObject.get(), uKeySize, const_cast<PUCHAR>(key), keySize, 0);
	if (!NT_SUCCESS(ret)) {
		return false;
	}

	m_pIV.reset((PUCHAR)HeapAlloc(GetProcessHeap(), 0, m_BlockLen));
	memcpy(m_pIV.get(), iv, ivSize);

	m_bReadyDecrypt = true;
	return true;
}

bool CAESDecryptor::Decrypt(const BYTE* encryptedData, size_t encryptedSize, BYTE* decryptedData, size_t& decryptedSize, bool bPadding)
{
	if (!m_bReadyDecrypt) {
		return false;
	}

	decryptedSize = encryptedSize;
	auto ret = BCryptDecrypt(m_hKey,
							 const_cast<PUCHAR>(encryptedData),
							 encryptedSize,
							 nullptr,
							 m_pIV.get(),
							 m_BlockLen,
							 decryptedData,
							 decryptedSize,
							 reinterpret_cast<PULONG>(&decryptedSize),
							 bPadding ? BCRYPT_BLOCK_PADDING : 0);
	if (!NT_SUCCESS(ret)) {
		return false;
	}

	return true;
}
