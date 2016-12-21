/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "Ifo.h"

#if 0 // stupid warning LNK4006 and LNK4088
inline uint16_t bswap_16(uint16_t value) { return (uint16_t)_byteswap_ushort((unsigned short)value); }
inline uint32_t bswap_32(uint32_t value) { return (uint32_t)_byteswap_ulong((unsigned long)value); }
inline uint64_t bswap_64(uint64_t value) { return (uint64_t)_byteswap_uint64((unsigned __int64)value); }
#else
inline uint16_t bswap_16(uint16_t value) {
	return (value >> 8) + (value << 8);
}

inline uint32_t bswap_32(uint32_t value) {
	return (value >> 24) + (value << 24) + ((value & 0xff00) << 8) + ((value & 0xff0000) >> 8);
}

inline uint64_t bswap_64(uint64_t value) {
	return	((value & 0xFF00000000000000) >> 56) +
			((value & 0x00FF000000000000) >> 40) +
			((value & 0x0000FF0000000000) >> 24) +
			((value & 0x000000FF00000000) >>  8) +
			((value & 0x00000000FF000000) <<  8) +
			((value & 0x0000000000FF0000) << 24) +
			((value & 0x000000000000FF00) << 40) +
			((value & 0x00000000000000FF) << 56);
}
#endif

#define DVD_VIDEO_LB_LEN 2048
#define IFO_HDR_LEN         8
#define LU_SUB_LEN          8

extern HANDLE (__stdcall * Real_CreateFileW)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

uint32_t get4bytes (const BYTE* buf)
{
	return bswap_32(*(uint32_t*)buf);
}

// VMG files
#define OFF_VMGM_PGCI_UT(buf) get4bytes (buf + 200)

// VTS files
#define OFF_VTSM_PGCI_UT(buf) get4bytes (buf + 208)
#define OFF_VTS_PGCIT(buf)    get4bytes (buf + 204)

CIfo::CIfo()
{
	m_pBuffer	= NULL;
	m_pPGCI		= NULL;
	m_pPGCIT	= NULL;
	m_dwSize	= 0;
}

int CIfo::GetMiscPGCI (CIfo::ifo_hdr_t *hdr, int title, uint8_t **ptr)
{
	pgci_sub_t *pgci_sub;

	*ptr	  = (uint8_t *) hdr;
	*ptr	 += IFO_HDR_LEN;
	pgci_sub  = (pgci_sub_t *) *ptr + title;

	*ptr = (uint8_t *) hdr + bswap_32(pgci_sub->start);

	return 0;
}

void CIfo::RemovePgciUOPs (uint8_t *ptr)
{
	ifo_hdr_t*	hdr = (ifo_hdr_t *) ptr;

	ptr += IFO_HDR_LEN;
	int num = bswap_16(hdr->num);

	for (int i = 1; i <= num; i++) {
		lu_sub_t* lu_sub = (lu_sub_t*)ptr;
		UNREFERENCED_PARAMETER(lu_sub);
		ptr += LU_SUB_LEN;
	}

	for (int i = 0; i < num; i++) {
		uint8_t *ptr;
		if (GetMiscPGCI(hdr, i, &ptr) >= 0) {
			pgc_t* pgc = (pgc_t*)ptr;
			pgc->prohibited_ops = 0;
		}
	}
}

CIfo::pgc_t* CIfo::GetFirstPGC()
{
	if (m_pBuffer) {
		return (pgc_t*) (m_pBuffer + 0x0400);
	} else {
		return NULL;
	}
}

CIfo::pgc_t* CIfo::GetPGCI(const int title, const ifo_hdr_t* hdr)
{
	CIfo::pgci_sub_t *pgci_sub;
	uint8_t *ptr;

	ptr = (uint8_t *) hdr;
	ptr += IFO_HDR_LEN;

	pgci_sub = (pgci_sub_t *) ptr + title;

	ptr = (uint8_t *) hdr + bswap_32(pgci_sub->start);

	/* jdw */
	if ( ptr >= ( (uint8_t *) hdr + bswap_32( hdr->len ))) {
		return NULL ;
	}
	/* /jdw */

	return (pgc_t *) ptr;
}

bool CIfo::IsVTS()
{
	if (m_dwSize<12 || (strncmp ((char*)m_pBuffer, "DVDVIDEO-VTS", 12)!=0)) {
		return false;
	}

	return true;
}

bool CIfo::IsVMG()
{
	if (m_dwSize<12 || (strncmp ((char*)m_pBuffer, "DVDVIDEO-VMG", 12)!=0)) {
		return false;
	}

	return true;
}

bool CIfo::OpenFile (LPCWSTR strFile)
{
	bool	bRet = false;
	HANDLE	hFile;
	LARGE_INTEGER size;

	hFile	 = Real_CreateFileW((LPTSTR) strFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT (hFile != INVALID_HANDLE_VALUE);

	if (hFile != INVALID_HANDLE_VALUE && GetFileSizeEx(hFile, &size) &&
			size.QuadPart <= 0x800000) { // max size of the ifo file = 8 MB (taken with reserve. need a more correct info)
		m_pBuffer = DNew BYTE [size.QuadPart];
		ReadFile (hFile, m_pBuffer, size.QuadPart, &m_dwSize, NULL);
		CloseHandle (hFile);

		if (IsVTS() && (OFF_VTSM_PGCI_UT(m_pBuffer)!=0)) {
			m_pPGCI  = (ifo_hdr_t*)(m_pBuffer + OFF_VTSM_PGCI_UT(m_pBuffer) * DVD_VIDEO_LB_LEN);
			m_pPGCIT = (ifo_hdr_t*)(m_pBuffer + OFF_VTS_PGCIT(m_pBuffer)    * DVD_VIDEO_LB_LEN);
		} else if (IsVMG() && (OFF_VMGM_PGCI_UT(m_pBuffer)!=0)) {
			m_pPGCI = (ifo_hdr_t*)(m_pBuffer + OFF_VMGM_PGCI_UT(m_pBuffer) * DVD_VIDEO_LB_LEN);
		}

		bRet = (m_pPGCI != NULL);
	}

	return bRet;
}

bool CIfo::RemoveUOPs()
{
	pgc_t* pgc;

	if (m_pPGCI) {
		pgc = GetFirstPGC();
		pgc->prohibited_ops = 0;
		int num = bswap_16(m_pPGCI->num);

		for (int i = 0; i < num; i++) {
			pgc = GetPGCI(i, m_pPGCI);
			if (pgc) {
				RemovePgciUOPs((uint8_t*)pgc);
			}
		}
	}

	if (m_pPGCIT) {
		int num = bswap_16(m_pPGCIT->num);

		for (int i = 0; i < num; i++) {
			pgc = GetPGCI(i, m_pPGCIT);
			if (pgc) {
				pgc->prohibited_ops = 0;
			}
		}
	}

	return true;
}

bool CIfo::SaveFile (LPCWSTR strFile)
{
	bool	bRet = false;
	HANDLE	m_hFile;

	if (m_pBuffer) {
		m_hFile	 = Real_CreateFileW((LPTSTR) strFile, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
									NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		ASSERT (m_hFile != INVALID_HANDLE_VALUE);

		if (m_hFile != INVALID_HANDLE_VALUE) {
			DWORD		dwSize;
			WriteFile (m_hFile, m_pBuffer, m_dwSize, &dwSize, NULL);
			CloseHandle(m_hFile);
			bRet = true;
		}
	}

	return bRet;
}

CIfo::~CIfo(void)
{
	delete[] m_pBuffer;
}
