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
#include <afxinet.h>
#include "TextFile.h"
#include <Utf8.h>
#include "DSUtil/FileHandle.h"
#include "DSUtil/HTTPAsync.h"

#include "compact_enc_det/compact_enc_det.h"

const static struct {
	uint16_t codepage;
	uint8_t  supported; // 0 or 1
	uint8_t  size;
	uint8_t  bom[4];
} g_BOM_markers[] = {
	{ CP_UTF8,    1, 3, { 0xEF, 0xBB, 0xBF } },
	{ 12000,      0, 4, { 0xFF, 0xFE, 0x00, 0x00 } }, // UTF-32LE, ignored. Need to check before CP_UTF16LE
	{ CP_UTF16LE, 1, 2, { 0xFF, 0xFE } },
	{ CP_UTF16BE, 1, 2, { 0xFE, 0xFF } },
	{ 12001,      0, 4, { 0x00, 0x00, 0xFE, 0xFF } }, // UTF-32BE, ignored
	{ 54936,      1, 4, { 0x84, 0x31, 0x95, 0x33 } }, // GB18030
	// other ignored encodings with BOM marker
	{ 65000,      0, 3, { 0x2B, 0x2F, 0x76 } },       // UTF-7
	{ -1,         0, 3, { 0xF7, 0x64, 0x4C } },       // UTF-1
	{ -1,         0, 4, { 0xDD, 0x73, 0x66, 0x73 } }, // UTF-EBCDIC
	{ -1,         0, 3, { 0x0E, 0xFE, 0xFF } },       // SCSU
	{ -1,         0, 3, { 0xFB, 0xEE, 0x28 } },       // BOCU-1
};

#define TEXTFILE_BUFFER_SIZE (64 * 1024)

CTextFile::CTextFile(UINT encoding/* = ASCII*/, UINT defaultencoding/* = ASCII*/, bool bAutoDetectCodePage/* = false*/)
	: m_encoding(encoding)
	, m_defaultencoding(defaultencoding)
	, m_bAutoDetectCodePage(bAutoDetectCodePage)
{
	m_buffer.reset(new(std::nothrow) char[TEXTFILE_BUFFER_SIZE]);
	m_wbuffer.reset(new(std::nothrow) WCHAR[TEXTFILE_BUFFER_SIZE]);
}

CTextFile::~CTextFile()
{
	Close();
}

bool CTextFile::OpenFile(LPCWSTR lpszFileName, LPCWSTR mode)
{
	Close();

	FILE* f = nullptr;
	auto err = _wfopen_s(&f, lpszFileName, mode);
	if (err != 0 || !f) {
		return false;
	}

	m_pFile.reset(f);
	m_pStdioFile = std::make_unique<CStdioFile>(f);

	m_strFileName = lpszFileName;

	return true;
}

bool CTextFile::Open(LPCWSTR lpszFileName)
{
	if (!OpenFile(lpszFileName, L"rb")) {
		return false;
	}

	m_offset = 0;
	m_nInBuffer = m_posInBuffer = 0;

	if (m_pStdioFile->GetLength() >= 4) {
		uint8_t b[4] = {};
		if (sizeof(b) != m_pStdioFile->Read(b, sizeof(b))) {
			Close();
			return false;
		}

		for (const auto& marker : g_BOM_markers) {
			if (memcmp(b, marker.bom, marker.size) == 0) {
				// encoding recognized by BOM marker
				if (!marker.supported) {
					//ignored encoding
					Close();
					return false;
				}
				m_encoding = marker.codepage;
				m_offset = marker.size;
				break;
			}
		}
	}

	if (!m_offset && m_bAutoDetectCodePage && FillBuffer()) {
		bool is_reliable;
		int bytes_consumed;
		auto encoding = CompactEncDet::DetectEncoding(
			m_buffer.get(), m_nInBuffer,
			nullptr, nullptr, nullptr,
			UNKNOWN_ENCODING,
			UNKNOWN_LANGUAGE,
			CompactEncDet::QUERY_CORPUS,
			false,
			&bytes_consumed,
			&is_reliable);
		switch (encoding) {
			// TODO - Add more encodings to the list.
			case MSFT_CP1250:        m_encoding = 1250;  break;
			case RUSSIAN_CP1251:     m_encoding = 1251;  break;
			case RUSSIAN_KOI8_R:     m_encoding = 21866; break;
			case RUSSIAN_CP866:      m_encoding = 866;   break;
			case MSFT_CP1252:        m_encoding = 1252;  break;
			case MSFT_CP1253:        m_encoding = 1253;  break;
			case MSFT_CP1254:        m_encoding = 1254;  break;
			case MSFT_CP1255:        m_encoding = 1255;  break;
			case MSFT_CP1256:        m_encoding = 1256;  break;
			case MSFT_CP1257:        m_encoding = 1257;  break;
			case MSFT_CP874:         m_encoding = 874;   break;
			case JAPANESE_CP932:     m_encoding = 932;   break;
			case CHINESE_GB:         m_encoding = 936;   break;
			case KOREAN_EUC_KR:      m_encoding = 949;   break;
			case CHINESE_BIG5:       m_encoding = 950;   break;
			case GB18030:            m_encoding = 54936; break;
			case JAPANESE_SHIFT_JIS: m_encoding = 932;   break;
		}
	}

	if (m_encoding == CP_ASCII) {
		if (!ReopenAsText()) {
			return false;
		}
	} else {
		Seek(0, CStdioFile::begin);
		m_posInFile = m_pStdioFile->GetPosition();
	}

	return true;
}

bool CTextFile::ReopenAsText()
{
	auto fileName = m_strFileName;

	Close();

	return OpenFile(fileName, L"rt");
}

bool CTextFile::Save(LPCWSTR lpszFileName, UINT e)
{
	if (!OpenFile(lpszFileName, e == CP_ASCII ? L"wt" : L"wb")) {
		return false;
	}

	switch (e) {
		case CP_UTF8: {
				BYTE b[3] = { 0xef, 0xbb, 0xbf };
				m_pStdioFile->Write(b, sizeof(b));
			}
			break;
		case CP_UTF16LE: {
				BYTE b[2] = { 0xff, 0xfe };
				m_pStdioFile->Write(b, sizeof(b));
			}
			break;
		case CP_UTF16BE: {
				BYTE b[2] = { 0xfe, 0xff };
				m_pStdioFile->Write(b, sizeof(b));
			}
			break;
	}

	m_encoding = e;

	return true;
}

void CTextFile::Close()
{
	if (m_pStdioFile) {
		m_pStdioFile.reset();
		m_pFile.reset();
		m_strFileName.Empty();
	}
}

UINT CTextFile::GetEncoding() const
{
	return m_encoding;
}

bool CTextFile::IsUnicode() const
{
	return m_encoding == CP_UTF8 || m_encoding == CP_UTF16LE || m_encoding == CP_UTF16BE;
}

CStringW CTextFile::GetFilePath() const
{
	return m_strFileName;
}

// CStdioFile

ULONGLONG CTextFile::GetPosition() const
{
	return m_pStdioFile ? (m_pStdioFile->GetPosition() - m_offset - (m_nInBuffer - m_posInBuffer)) : 0ULL;
}

ULONGLONG CTextFile::GetLength() const
{
	return m_pStdioFile ? (m_pStdioFile->GetLength() - m_offset) : 0ULL;
}

ULONGLONG CTextFile::Seek(LONGLONG lOff, UINT nFrom)
{
	if (!m_pStdioFile) {
		return 0ULL;
	}

	ULONGLONG newPos;

	// Try to reuse the buffer if any
	if (m_nInBuffer > 0) {
		const LONGLONG pos = GetPosition();
		const LONGLONG len = GetLength();

		switch (nFrom) {
			default:
			case CStdioFile::begin:
				break;
			case CStdioFile::current:
				lOff = pos + lOff;
				break;
			case CStdioFile::end:
				lOff = len - lOff;
				break;
		}

		lOff = std::clamp(lOff, 0LL, len);

		m_posInBuffer += lOff - pos;
		if (m_posInBuffer < 0 || m_posInBuffer >= m_nInBuffer) {
			// If we would have to end up out of the buffer, we just reset it and seek normally
			m_nInBuffer = m_posInBuffer = 0;
			newPos = m_pStdioFile->Seek(lOff + m_offset, CStdioFile::begin) - m_offset;
		} else { // If we can reuse the buffer, we have nothing special to do
			newPos = ULONGLONG(lOff);
		}
	} else { // No buffer, we can use the base implementation
		if (nFrom == CStdioFile::begin) {
			lOff += m_offset;
		}
		newPos = m_pStdioFile->Seek(lOff, nFrom) - m_offset;
	}

	m_posInFile = newPos + m_offset + (m_nInBuffer - m_posInBuffer);

	return newPos;
}

void CTextFile::WriteString(LPCSTR lpsz/*CStringA str*/)
{
	if (!m_pStdioFile) {
		return;
	}

	CStringA str(lpsz);

	switch (m_encoding) {
		case CP_ASCII:
			m_pStdioFile->WriteString(AToT(str));
			break;
		case CP_ACP:
			str.Replace("\n", "\r\n");
			m_pStdioFile->Write(str.GetString(), str.GetLength());
			break;
		case CP_UTF8:
		case CP_UTF16LE:
		case CP_UTF16BE:
			WriteString(AToT(str));
			break;
	}
}

void CTextFile::WriteString(LPCWSTR lpsz/*CStringW str*/)
{
	if (!m_pStdioFile) {
		return;
	}

	CStringW str(lpsz);

	switch (m_encoding) {
		case CP_ASCII:
			m_pStdioFile->WriteString(str);
			break;
		case CP_ACP: {
				str.Replace(L"\n", L"\r\n");
				CStringA stra(str); // TODO: codepage
				m_pStdioFile->Write(stra.GetString(), stra.GetLength());
			}
			break;
		case CP_UTF8: {
				str.Replace(L"\n", L"\r\n");
				auto utf8 = WStrToUTF8(str.GetString());
				if (!utf8.IsEmpty()) {
					m_pStdioFile->Write(utf8.GetString(), utf8.GetLength());
				}
			}
			break;
		case CP_UTF16LE:
			str.Replace(L"\n", L"\r\n");
			m_pStdioFile->Write(str.GetString(), str.GetLength() * 2);
			break;
		case CP_UTF16BE:
			str.Replace(L"\n", L"\r\n");
			for (unsigned int i = 0, l = str.GetLength(); i < l; i++) {
				str.SetAt(i, ((str[i] >> 8) & 0x00ff) | ((str[i] << 8) & 0xff00));
			}
			m_pStdioFile->Write(str.GetString(), str.GetLength() * 2);
			break;
	}
}

bool CTextFile::FillBuffer()
{
	if (!m_pStdioFile) {
		return false;
	}

	if (m_posInBuffer < m_nInBuffer) {
		m_nInBuffer -= m_posInBuffer;
		memcpy(m_buffer.get(), &m_buffer[m_posInBuffer], (size_t)m_nInBuffer * sizeof(char));
	} else {
		m_nInBuffer = 0;
	}
	m_posInBuffer = 0;

	UINT nBytesRead = m_pStdioFile->Read(&m_buffer[m_nInBuffer], UINT(TEXTFILE_BUFFER_SIZE - m_nInBuffer) * sizeof(char));
	if (nBytesRead) {
		m_nInBuffer += nBytesRead;
	}
	m_posInFile = m_pStdioFile->GetPosition();

	return nBytesRead > 0;
}

ULONGLONG CTextFile::GetPositionFastBuffered() const
{
	return m_pStdioFile ? (m_posInFile - m_offset - (m_nInBuffer - m_posInBuffer)) : 0ULL;
}

bool CTextFile::ReadString(CStringW& str)
{
	if (!m_pStdioFile) {
		return false;
	}

	bool fEOF = true;

	str.Truncate(0);

	switch (m_encoding) {
		case CP_ASCII: {
				CStringW s;
				fEOF = !m_pStdioFile->ReadString(s);
				str = s;
				// For consistency with other encodings, we continue reading
				// the file even when a NUL char is encountered.
				char c;
				while (fEOF && (m_pStdioFile->Read(&c, sizeof(c)) == sizeof(c))) {
					str += c;
					fEOF = !m_pStdioFile->ReadString(s);
					str += s;
				}
			}
			break;
		case CP_UTF8: {
				ULONGLONG lineStartPos = GetPositionFastBuffered();
				bool bValid = true;
				bool bLineEndFound = false;
				fEOF = false;

				do {
					int nCharsRead;

					for (nCharsRead = 0; m_posInBuffer < m_nInBuffer; m_posInBuffer++, nCharsRead++) {
						if (Utf8::isSingleByte(m_buffer[m_posInBuffer])) { // 0xxxxxxx
							m_wbuffer[nCharsRead] = m_buffer[m_posInBuffer] & 0x7f;
						} else if (Utf8::isFirstOfMultibyte(m_buffer[m_posInBuffer])) {
							int nContinuationBytes = Utf8::continuationBytes(m_buffer[m_posInBuffer]);
							bValid = true;

							if (m_posInBuffer + nContinuationBytes >= m_nInBuffer) {
								// If we are at the end of the file, the buffer won't be full
								// and we won't be able to read any more continuation bytes.
								bValid = (m_nInBuffer == TEXTFILE_BUFFER_SIZE);
								break;
							} else {
								for (int j = 1; j <= nContinuationBytes; j++) {
									if (!Utf8::isContinuation(m_buffer[m_posInBuffer + j])) {
										bValid = false;
									}
								}

								switch (nContinuationBytes) {
									case 0: // 0xxxxxxx
										m_wbuffer[nCharsRead] = m_buffer[m_posInBuffer] & 0x7f;
										break;
									case 1: // 110xxxxx 10xxxxxx
										m_wbuffer[nCharsRead] = (m_buffer[m_posInBuffer] & 0x1f) << 6 | (m_buffer[m_posInBuffer + 1] & 0x3f);
										break;
									case 2: // 1110xxxx 10xxxxxx 10xxxxxx
										m_wbuffer[nCharsRead] = (m_buffer[m_posInBuffer] & 0x0f) << 12 | (m_buffer[m_posInBuffer + 1] & 0x3f) << 6 | (m_buffer[m_posInBuffer + 2] & 0x3f);
										break;
									case 3: // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
										{
											const auto* Z = &m_buffer[m_posInBuffer];
											const auto u32 = ((uint32_t)(*Z & 0x0F) << 18) | ((uint32_t)(*(Z + 1) & 0x3F) << 12) | ((uint32_t)(*(Z + 2) & 0x3F) << 6) | ((uint32_t) * (Z + 3) & 0x3F);
											if (u32 <= UINT16_MAX) {
												m_wbuffer[nCharsRead] = (wchar_t)u32;
											} else {
												m_wbuffer[nCharsRead++] = (wchar_t)((((u32 - 0x010000) & 0x000FFC00) >> 10) | 0xD800);
												m_wbuffer[nCharsRead] = (wchar_t)((u32 & 0x000003FF) | 0xDC00);
											}
										}
										break;
								}
								m_posInBuffer += nContinuationBytes;
							}
						} else {
							bValid = false;
						}

						if (!bValid) {
							m_wbuffer[nCharsRead] = L'?';
							m_posInBuffer++;
							nCharsRead++;
							break;
						} else if (m_wbuffer[nCharsRead] == L'\n') {
							bLineEndFound = true; // Stop at end of line
							m_posInBuffer++;
							break;
						} else if (m_wbuffer[nCharsRead] == L'\r') {
							nCharsRead--; // Skip \r
						}
					}

					if (bValid || m_offset) {
						if (nCharsRead > 0) {
							str.Append(m_wbuffer.get(), nCharsRead);
						}

						if (!bLineEndFound) {
							bLineEndFound = !FillBuffer();
							if (!nCharsRead) {
								fEOF = bLineEndFound;
							}
						}
					} else {
						// Switch to text and read again
						m_encoding = m_defaultencoding;
						// Stop using the buffer
						m_posInBuffer = m_nInBuffer = 0;

						fEOF = !ReopenAsText();

						if (!fEOF) {
							// Seek back to the beginning of the line where we stopped
							Seek(lineStartPos, CStdioFile::begin);

							fEOF = !ReadString(str);
						}
					}
				} while (bValid && !bLineEndFound);
			}
			break;
		case CP_UTF16LE: {
				bool bLineEndFound = false;
				fEOF = false;

				do {
					int nCharsRead;
					WCHAR* wbuffer = (WCHAR*)&m_buffer[m_posInBuffer];

					for (nCharsRead = 0; m_posInBuffer + 1 < m_nInBuffer; nCharsRead++, m_posInBuffer += sizeof(WCHAR)) {
						if (wbuffer[nCharsRead] == L'\n') {
							break; // Stop at end of line
						} else if (wbuffer[nCharsRead] == L'\r') {
							break; // Skip \r
						}
					}

					if (nCharsRead > 0) {
						str.Append(wbuffer, nCharsRead);
					}

					while (m_posInBuffer + 1 < m_nInBuffer && wbuffer[nCharsRead] == L'\r') {
						nCharsRead++;
						m_posInBuffer += sizeof(WCHAR);
					}
					if (m_posInBuffer + 1 < m_nInBuffer && wbuffer[nCharsRead] == L'\n') {
						bLineEndFound = true; // Stop at end of line
						nCharsRead++;
						m_posInBuffer += sizeof(WCHAR);
					}

					if (!bLineEndFound) {
						bLineEndFound = !FillBuffer();
						if (!nCharsRead) {
							fEOF = bLineEndFound;
						}
					}
				} while (!bLineEndFound);
			}
			break;
		case CP_UTF16BE: {
				bool bLineEndFound = false;
				fEOF = false;

				do {
					int nCharsRead;

					for (nCharsRead = 0; m_posInBuffer + 1 < m_nInBuffer; nCharsRead++, m_posInBuffer += sizeof(WCHAR)) {
						m_wbuffer[nCharsRead] = ((WCHAR(m_buffer[m_posInBuffer]) << 8) & 0xff00) | (WCHAR(m_buffer[m_posInBuffer + 1]) & 0x00ff);
						if (m_wbuffer[nCharsRead] == L'\n') {
							bLineEndFound = true; // Stop at end of line
							m_posInBuffer += sizeof(WCHAR);
							break;
						} else if (m_wbuffer[nCharsRead] == L'\r') {
							nCharsRead--; // Skip \r
						}
					}

					if (nCharsRead > 0) {
						str.Append(m_wbuffer.get(), nCharsRead);
					}

					if (!bLineEndFound) {
						bLineEndFound = !FillBuffer();
						if (!nCharsRead) {
							fEOF = bLineEndFound;
						}
					}
				} while (!bLineEndFound);
			}
			break;
		default: {
				bool bLineEndFound = false;
				fEOF = false;

				do {
					int nCharsRead;

					for (nCharsRead = 0; m_posInBuffer + nCharsRead < m_nInBuffer; nCharsRead++) {
						if (m_buffer[m_posInBuffer + nCharsRead] == '\n') {
							break;
						} else if (m_buffer[m_posInBuffer + nCharsRead] == '\r') {
							break;
						}
					}

					if (nCharsRead > 0) {
						CStringW line;
						int len = MultiByteToWideChar(m_encoding, 0, &m_buffer[m_posInBuffer], nCharsRead, nullptr, 0);
						if (len > 0) {
							line.ReleaseBuffer(MultiByteToWideChar(m_encoding, 0, &m_buffer[m_posInBuffer], nCharsRead, line.GetBuffer(len), len));
						}

						str.Append(line);
					}

					m_posInBuffer += nCharsRead;
					while (m_posInBuffer < m_nInBuffer && m_buffer[m_posInBuffer] == '\r') {
						m_posInBuffer++;
					}
					if (m_posInBuffer < m_nInBuffer && m_buffer[m_posInBuffer] == '\n') {
						bLineEndFound = true; // Stop at end of line
						m_posInBuffer++;
					}

					if (!bLineEndFound) {
						bLineEndFound = !FillBuffer();
						if (!nCharsRead) {
							fEOF = bLineEndFound;
						}
					}
				} while (!bLineEndFound);
			}
			break;
	}

	return !fEOF;
}

//
// CWebTextFile
//

CWebTextFile::CWebTextFile(UINT encoding/* = ASCII*/, UINT defaultencoding/* = ASCII*/, bool bAutoDetectCodePage/* = false*/, LONGLONG llMaxSize)
	: CTextFile(encoding, defaultencoding, bAutoDetectCodePage)
	, m_llMaxSize(llMaxSize)
{
}

CWebTextFile::~CWebTextFile()
{
	Close();
}

bool CWebTextFile::Open(LPCWSTR lpszFileName)
{
	CUrlParser urlParser;
	if (!urlParser.Parse(lpszFileName)) {
		return __super::Open(lpszFileName);
	}

	CStringW fn(lpszFileName);

	CHTTPAsync HTTPAsync;
	if (SUCCEEDED(HTTPAsync.Connect(lpszFileName, http::connectTimeout))) {
		if (GetTemporaryFilePath(L".tmp", fn)) {
			CFile temp;
			if (!temp.Open(fn, CStdioFile::modeCreate | CStdioFile::modeWrite | CStdioFile::typeBinary | CStdioFile::shareDenyWrite)) {
				HTTPAsync.Close();
				return false;
			}

			if (HTTPAsync.IsCompressed()) {
				if (HTTPAsync.GetLenght() <= 10 * MEGABYTE) {
					std::vector<BYTE> body;
					if (HTTPAsync.GetUncompressed(body)) {
						temp.Write(body.data(), static_cast<UINT>(body.size()));
						m_tempfn = fn;
					}
				}
			} else {
				BYTE buffer[1024] = {};
				DWORD dwSizeRead = 0;
				DWORD totalSize = 0;
				do {
					if (HTTPAsync.Read(buffer, 1024, dwSizeRead, http::readTimeout) != S_OK) {
						break;
					}
					temp.Write(buffer, dwSizeRead);
					totalSize += dwSizeRead;
				} while (dwSizeRead && totalSize < m_llMaxSize);
				temp.Close();

				if (totalSize) {
					m_tempfn = fn;
				}
			}
		}

		m_url_redirect_str = HTTPAsync.GetRedirectURL();
		HTTPAsync.Close();
	}

	return __super::Open(m_tempfn);
}

void CWebTextFile::Close()
{
	__super::Close();

	if (!m_tempfn.IsEmpty()) {
		_wremove(m_tempfn);
		m_tempfn.Empty();
	}
}

const CStringW& CWebTextFile::GetRedirectURL() const
{
	return m_url_redirect_str;
}

///////////////////////////////////////////////////////////////

CStringW AToT(CStringA str)
{
	CStringW ret;
	for (int i = 0, j = str.GetLength(); i < j; i++) {
		ret += (WCHAR)(BYTE)str[i];
	}
	return ret;
}

CStringA TToA(CStringW str)
{
	CStringA ret;
	for (int i = 0, j = str.GetLength(); i < j; i++) {
		ret += (CHAR)(BYTE)str[i];
	}
	return ret;
}
