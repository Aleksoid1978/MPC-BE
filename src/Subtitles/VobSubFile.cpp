/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
#include <winioctl.h>
#include "TextFile.h"
#include "unrar.h"
#include "VobSubFile.h"
#include "RTS.h"

//

template<class T, typename SEP>
std::enable_if_t<(std::is_same_v<T, CStringW> || std::is_same_v<T, CStringA>), T>
Explode(const T& str, CAtlList<T>& sl, const SEP sep, const size_t limit = 0)
{
	static_assert(sizeof(SEP) <= 2); // SEP must be char or wchar_t
	sl.RemoveAll();
	if (str.IsEmpty()) {
		return T();
	}

	for (int i = 0, j = 0; ; i = j + 1) {
		j = str.Find(sep, i);

		if (j < 0 || sl.GetCount() == limit - 1) {
			sl.AddTail(str.Mid(i).Trim());
			break;
		} else {
			sl.AddTail(str.Mid(i, j - i).Trim());
		}
	}

	return sl.GetHead();
}

struct lang_type {
	unsigned short id;
	LPCSTR lang_long;
} lang_tbl[] = {
	{'--', "(Not detected)"},
	{'cc', "Closed Caption"},
	{'aa', "Afar"},
	{'ab', "Abkhazian"},
	{'af', "Afrikaans"},
	{'am', "Amharic"},
	{'ar', "Arabic"},
	{'as', "Assamese"},
	{'ay', "Aymara"},
	{'az', "Azerbaijani"},
	{'ba', "Bashkir"},
	{'be', "Byelorussian"},
	{'bg', "Bulgarian"},
	{'bh', "Bihari"},
	{'bi', "Bislama"},
	{'bn', "Bengali; Bangla"},
	{'bo', "Tibetan"},
	{'br', "Breton"},
	{'ca', "Catalan"},
	{'co', "Corsican"},
	{'cs', "Czech"},
	{'cy', "Welsh"},
	{'da', "Dansk"},
	{'de', "Deutsch"},
	{'dz', "Bhutani"},
	{'el', "Greek"},
	{'en', "English"},
	{'eo', "Esperanto"},
	{'es', "Espanol"},
	{'et', "Estonian"},
	{'eu', "Basque"},
	{'fa', "Persian"},
	{'fi', "Finnish"},
	{'fj', "Fiji"},
	{'fo', "Faroese"},
	{'fr', "Francais"},
	{'fy', "Frisian"},
	{'ga', "Irish"},
	{'gd', "Scots Gaelic"},
	{'gl', "Galician"},
	{'gn', "Guarani"},
	{'gu', "Gujarati"},
	{'ha', "Hausa"},
	{'he', "Hebrew"},
	{'hi', "Hindi"},
	{'hr', "Hrvatski"},
	{'hu', "Hungarian"},
	{'hy', "Armenian"},
	{'ia', "Interlingua"},
	{'id', "Indonesian"},
	{'ie', "Interlingue"},
	{'ik', "Inupiak"},
	{'in', "Indonesian"},
	{'is', "Islenska"},
	{'it', "Italiano"},
	{'iu', "Inuktitut"},
	{'iw', "Hebrew"},
	{'ja', "Japanese"},
	{'ji', "Yiddish"},
	{'jw', "Javanese"},
	{'ka', "Georgian"},
	{'kk', "Kazakh"},
	{'kl', "Greenlandic"},
	{'km', "Cambodian"},
	{'kn', "Kannada"},
	{'ko', "Korean"},
	{'ks', "Kashmiri"},
	{'ku', "Kurdish"},
	{'ky', "Kirghiz"},
	{'la', "Latin"},
	{'ln', "Lingala"},
	{'lo', "Laothian"},
	{'lt', "Lithuanian"},
	{'lv', "Latvian, Lettish"},
	{'mg', "Malagasy"},
	{'mi', "Maori"},
	{'mk', "Macedonian"},
	{'ml', "Malayalam"},
	{'mn', "Mongolian"},
	{'mo', "Moldavian"},
	{'mr', "Marathi"},
	{'ms', "Malay"},
	{'mt', "Maltese"},
	{'my', "Burmese"},
	{'na', "Nauru"},
	{'ne', "Nepali"},
	{'nl', "Nederlands"},
	{'no', "Norsk"},
	{'oc', "Occitan"},
	{'om', "(Afan) Oromo"},
	{'or', "Oriya"},
	{'pa', "Punjabi"},
	{'pl', "Polish"},
	{'ps', "Pashto, Pushto"},
	{'pt', "Portugues"},
	{'qu', "Quechua"},
	{'rm', "Rhaeto-Romance"},
	{'rn', "Kirundi"},
	{'ro', "Romanian"},
	{'ru', "Russian"},
	{'rw', "Kinyarwanda"},
	{'sa', "Sanskrit"},
	{'sd', "Sindhi"},
	{'sg', "Sangho"},
	{'sh', "Serbo-Croatian"},
	{'si', "Sinhalese"},
	{'sk', "Slovak"},
	{'sl', "Slovenian"},
	{'sm', "Samoan"},
	{'sn', "Shona"},
	{'so', "Somali"},
	{'sq', "Albanian"},
	{'sr', "Serbian"},
	{'ss', "Siswati"},
	{'st', "Sesotho"},
	{'su', "Sundanese"},
	{'sv', "Svenska"},
	{'sw', "Swahili"},
	{'ta', "Tamil"},
	{'te', "Telugu"},
	{'tg', "Tajik"},
	{'th', "Thai"},
	{'ti', "Tigrinya"},
	{'tk', "Turkmen"},
	{'tl', "Tagalog"},
	{'tn', "Setswana"},
	{'to', "Tonga"},
	{'tr', "Turkish"},
	{'ts', "Tsonga"},
	{'tt', "Tatar"},
	{'tw', "Twi"},
	{'ug', "Uighur"},
	{'uk', "Ukrainian"},
	{'ur', "Urdu"},
	{'uz', "Uzbek"},
	{'vi', "Vietnamese"},
	{'vo', "Volapuk"},
	{'wo', "Wolof"},
	{'xh', "Xhosa"},
	{'yi', "Yiddish"},				// formerly ji
	{'yo', "Yoruba"},
	{'za', "Zhuang"},
	{'zh', "Chinese"},
	{'zu', "Zulu"},
};

int find_lang(unsigned short id)
{
	int lo = 0, hi = std::size(lang_tbl) - 1;

	while (lo < hi) {
		int mid = (lo + hi) >> 1;

		if (id < lang_tbl[mid].id) {
			hi = mid;
		} else if (id > lang_tbl[mid].id) {
			lo = mid + 1;
		} else {
			return mid;
		}
	}

	return (id == lang_tbl[lo].id ? lo : 0);
}

CString FindLangFromId(WORD id)
{
	return CString(lang_tbl[find_lang(id)].lang_long);
}

//
// CVobSubFile
//

CVobSubFile::CVobSubFile(CCritSec* pLock)
	: CSubPicProviderImpl(pLock)
	, m_sub(1024*1024)
	, m_nLang(0)
{
}

CVobSubFile::~CVobSubFile()
{
}

//

bool CVobSubFile::Copy(CVobSubFile& vsf)
{
	Close();

	*(CVobSubSettings*)this = *(CVobSubSettings*)&vsf;
	m_title = vsf.m_title;
	m_nLang = vsf.m_nLang;

	m_sub.SetLength(vsf.m_sub.GetLength());
	m_sub.SeekToBegin();

	for (size_t i = 0; i < std::size(m_langs); i++) {
		SubLang& src = vsf.m_langs[i];
		SubLang& dst = m_langs[i];

		dst.id = src.id;
		dst.name = src.name;
		dst.alt = src.alt;

		for (size_t j = 0; j < src.subpos.size(); j++) {
			SubPos& sp = src.subpos[j];
			if (!sp.bValid) {
				continue;
			}

			if (sp.filepos != (__int64)vsf.m_sub.Seek(sp.filepos, CFile::begin)) {
				continue;
			}

			sp.filepos = m_sub.GetPosition();

			BYTE buff[2048];
			vsf.m_sub.Read(buff, 2048);
			m_sub.Write(buff, 2048);

			WORD packetsize = (buff[buff[0x16]+0x18]<<8) | buff[buff[0x16]+0x19];

			for (int k = 0, size, sizeleft = packetsize - 4;
					k < packetsize - 4;
					k += size, sizeleft -= size) {
				int hsize = buff[0x16]+0x18 + ((buff[0x15]&0x80) ? 4 : 0);
				size = std::min(sizeleft, 2048 - hsize);

				if (size != sizeleft) {
					while (vsf.m_sub.Read(buff, 2048)) {
						if (!(buff[0x15]&0x80) && buff[buff[0x16]+0x17] == (i|0x20)) {
							break;
						}
					}

					m_sub.Write(buff, 2048);
				}
			}

			dst.subpos.push_back(sp);
		}
	}

	m_sub.SetLength(m_sub.GetPosition());

	return true;
}

//

void CVobSubFile::TrimExtension(CString& fn)
{
	int i = fn.ReverseFind('.');
	if (i > 0) {
		CString ext = fn.Mid(i).MakeLower();
		if (ext == L".ifo" || ext == L".idx" || ext == L".sub"
				|| ext == L".sst" || ext == L".son" || ext == L".rar") {
			fn = fn.Left(i);
		}
	}
}

bool CVobSubFile::Open(CString fn)
{
	TrimExtension(fn);

	do {
		Close();

		int ver;
		if (!ReadIdx(fn + L".idx", ver)) {
			break;
		}

		if (ver < 6 && !ReadIfo(fn + L".ifo")) {
			break;
		}

		if (!ReadSub(fn + L".sub") && !ReadRar(fn + L".rar")) {
			break;
		}

		m_title = fn;

		for (int i = 0; i < (int)std::size(m_langs); i++) {
			std::vector<SubPos>& sp = m_langs[i].subpos;

			for (size_t j = 0; j < sp.size(); j++) {
				sp[j].stop = sp[j].start;
				sp[j].bForced = false;

				int packetsize = 0, datasize = 0;
				BYTE* buff = GetPacket((int)j, packetsize, datasize, i);
				if (!buff) {
					sp[j].bValid = false;
					continue;
				}

				m_img.delay = j + 1 < sp.size() ? sp[j + 1].start - sp[j].start : 3000;
				m_img.GetPacketInfo(buff, packetsize, datasize);
				if (j + 1 < sp.size()) {
					m_img.delay = std::min(m_img.delay, sp[j + 1].start - sp[j].start);
				}

				sp[j].stop = sp[j].start + m_img.delay;
				sp[j].bForced = m_img.bForced;
				sp[j].bAnimated = m_img.bAnimated;

				if (j > 0 && sp[j-1].stop > sp[j].start) {
					sp[j-1].stop = sp[j].start;
				}

				delete [] buff;
			}
		}

		return true;
	} while (false);

	Close();

	return false;
}

bool CVobSubFile::Save(CString fn, SubFormat sf)
{
	TrimExtension(fn);

	CVobSubFile vsf(nullptr);
	if (!vsf.Copy(*this)) {
		return false;
	}

	switch (sf) {
		case VobSub:
			return vsf.SaveVobSub(fn);
		case WinSubMux:
			return vsf.SaveWinSubMux(fn);
		case Scenarist:
			return vsf.SaveScenarist(fn);
		case Maestro:
			return vsf.SaveMaestro(fn);
		default:
			break;
	}

	return false;
}

void CVobSubFile::Close()
{
	InitSettings();
	m_title.Empty();
	m_sub.SetLength(0);
	m_img.Invalidate();
	m_nLang = -1;
	for (auto& sl : m_langs) {
		sl.id = 0;
		sl.name.Empty();
		sl.alt.Empty();
		sl.subpos.clear();
	}
}

//

bool CVobSubFile::ReadIdx(CString fn, int& ver)
{
	CWebTextFile f;
	if (!f.Open(fn)) {
		return false;
	}

	bool bError = false;

	int id = -1, delay = 0, vobid = -1, cellid = -1;
	__int64 celltimestamp = 0;

	CString str;
	for (ptrdiff_t line = 0; !bError && f.ReadString(str); line++) {
		str.Trim();

		if (line == 0) {
			WCHAR buff[] = L"VobSub index file, v";

			const WCHAR* s = str;

			int i = str.Find(buff);
			if (i < 0 || swscanf_s(&s[i+wcslen(buff)], L"%d", &ver) != 1
					|| ver > VOBSUBIDXVER) {
				DLog(L"[CVobSubFile::ReadIdx] Wrong file version!");
				bError = true;
				continue;
			}
		} else if (!str.GetLength()) {
			continue;
		} else if (str[0] == L'#') {
			WCHAR buff[] = L"Vob/Cell ID:";

			const WCHAR* s = str;

			int i = str.Find(buff);
			if (i >= 0) {
				swscanf_s(&s[i+wcslen(buff)], L"%d, %d (PTS: %I64d)", &vobid, &cellid, &celltimestamp);
			}

			continue;
		}

		int i = str.Find(':');
		if (i <= 0) {
			continue;
		}

		CString entry = str.Left(i).MakeLower();

		str = str.Mid(i+1);
		str.Trim();
		if (str.IsEmpty()) {
			continue;
		}

		if (entry == L"size") {
			int x, y;
			if (swscanf_s(str, L"%dx%d", &x, &y) != 2) {
				bError = true;
			}
			m_size.cx = x;
			m_size.cy = y;
		} else if (entry == L"org") {
			if (swscanf_s(str, L"%d,%d", &m_x, &m_y) != 2) {
				bError = true;
			} else {
				m_org = CPoint(m_x, m_y);
			}
		} else if (entry == L"scale") {
			if (ver < 5) {
				int scale = 100;
				if (swscanf_s(str, L"%d%%", &scale) != 1) {
					bError = true;
				}
				m_scale_x = m_scale_y = scale;
			} else {
				if (swscanf_s(str, L"%d%%,%d%%", &m_scale_x, &m_scale_y) != 2) {
					bError = true;
				}
			}
		} else if (entry == L"alpha") {
			if (swscanf_s(str, L"%d", &m_alpha) != 1) {
				bError = true;
			}
		} else if (entry == L"smooth") {
			str.MakeLower();

			if (str.Find(L"old") >= 0 || str.Find(L"2") >= 0) {
				m_iSmooth = 2;
			} else if (str.Find(L"on") >= 0 || str.Find(L"1") >= 0) {
				m_iSmooth = 1;
			} else if (str.Find(L"off") >= 0 || str.Find(L"0") >= 0) {
				m_iSmooth = 0;
			} else {
				bError = true;
			}
		} else if (entry == L"fadein/out") {
			if (swscanf_s(str, L"%d,%d", &m_fadein, &m_fadeout) != 2) {
				bError = true;
			}
		} else if (entry == L"align") {
			str.MakeLower();

			int k = 0;
			CString token;
			for (int j = 0; j < 3; j++) {
				token = str.Tokenize(L" ", k);
				if (token.IsEmpty()) {
					break;
				}

				if (j == 0) {
					if (token == L"on" || token == L"1") {
						m_bAlign = true;
					} else if (token == L"off" || token == L"0") {
						m_bAlign = false;
					} else {
						bError = true;
						break;
					}
				} else if (j == 1) {
					if (token == L"at") {
						j--;
						continue;
					}

					if (token == L"left") {
						m_alignhor = 0;
					} else if (token == L"center") {
						m_alignhor = 1;
					} else if (token == L"right") {
						m_alignhor = 2;
					} else {
						bError = true;
						break;
					}
				} else if (j == 2) {
					if (token == L"top") {
						m_alignver = 0;
					} else if (token == L"center") {
						m_alignver = 1;
					} else if (token == L"bottom") {
						m_alignver = 2;
					} else {
						bError = true;
						break;
					}
				}
			}
		} else if (entry == L"time offset") {
			bool bNegative = false;
			if (str[0] == '-') {
				bNegative = true;
			}
			str.TrimLeft(L"+-");

			WCHAR c;
			int hh, mm, ss, ms;
			int n = swscanf_s(str, L"%d%c%d%c%d%c%d", &hh, &c, 1, &mm, &c, 1, &ss, &c, 1, &ms);
			switch (n) {
				case 1: // We have read only one integer, interpret it as an offset expressed in milliseconds
					m_toff = hh * (bNegative ? -1 : 1);
					break;
				case 7: // We have read 4 integers + 3 separators, interpret them as hh:mm:ss.ms
					m_toff = (hh * 60 * 60 * 1000 + mm * 60 * 1000 + ss * 1000 + ms) * (bNegative ? -1 : 1);
					break;
				default:
					bError = true;
					m_toff = 0;
				break;
			}
		} else if (entry == L"forced subs") {
			str.MakeLower();

			if (str.Find(L"on") >= 0 || str.Find(L"1") >= 0) {
				m_bOnlyShowForcedSubs = true;
			} else if (str.Find(L"off") >= 0 || str.Find(L"0") >= 0) {
				m_bOnlyShowForcedSubs = false;
			} else {
				bError = true;
			}
		} else if (entry == L"langidx") {
			if (swscanf_s(str, L"%d", &m_nLang) != 1) {
				bError = true;
			}
		} else if (entry == L"palette") {
			if (swscanf_s(str, L"%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x",
						   (uint32_t*)&m_orgpal[0],  (uint32_t*)&m_orgpal[1],  (uint32_t*)&m_orgpal[2],  (uint32_t*)&m_orgpal[3],
						   (uint32_t*)&m_orgpal[4],  (uint32_t*)&m_orgpal[5],  (uint32_t*)&m_orgpal[6],  (uint32_t*)&m_orgpal[7],
						   (uint32_t*)&m_orgpal[8],  (uint32_t*)&m_orgpal[9],  (uint32_t*)&m_orgpal[10], (uint32_t*)&m_orgpal[11],
						   (uint32_t*)&m_orgpal[12], (uint32_t*)&m_orgpal[13], (uint32_t*)&m_orgpal[14], (uint32_t*)&m_orgpal[15]
						  ) != 16) {
				bError = true;
			}
		} else if (entry == L"custom colors") {
			str.MakeLower();

			if (str.Find(L"on") == 0 || str.Find(L"1") == 0) {
				m_bCustomPal = true;
			} else if (str.Find(L"off") == 0 || str.Find(L"0") == 0) {
				m_bCustomPal = false;
			} else {
				bError = true;
			}

			i = str.Find(L"tridx:");
			if (i < 0) {
				bError = true;
				continue;
			}
			str = str.Mid(i + (int)wcslen(L"tridx:"));

			int tridx;
			if (swscanf_s(str, L"%x", &tridx) != 1) {
				bError = true;
				continue;
			}
			tridx = ((tridx&0x1000)>>12) | ((tridx&0x100)>>7) | ((tridx&0x10)>>2) | ((tridx&1)<<3);

			i = str.Find(L"colors:");
			if (i < 0) {
				bError = true;
				continue;
			}
			str = str.Mid(i + (int)wcslen(L"colors:"));

			RGBQUAD pal[4];
			if (swscanf_s(str, L"%x,%x,%x,%x", (uint32_t*)&pal[0], (uint32_t*)&pal[1], (uint32_t*)&pal[2], (uint32_t*)&pal[3]) != 4) {
				bError = true;
				continue;
			}

			SetCustomPal(pal, tridx);
		} else if (entry == L"id") {
			str.MakeLower();

			int langid = ((str[0]&0xff)<<8)|(str[1]&0xff);

			i = str.Find(L"index:");
			if (i < 0) {
				bError = true;
				continue;
			}
			str = str.Mid(i + (int)wcslen(L"index:"));

			if (swscanf_s(str, L"%d", &id) != 1 || id < 0 || id >= 32) {
				bError = true;
				continue;
			}
			if (m_nLang < 0) {
				m_nLang = id;
			}

			m_langs[id].id = langid;
			m_langs[id].name = lang_tbl[find_lang(langid)].lang_long;
			m_langs[id].alt = lang_tbl[find_lang(langid)].lang_long;

			delay = 0;
		} else if (id >= 0 && entry == L"alt") {
			m_langs[id].alt = str;
		} else if (id >= 0 && entry == L"delay") {
			bool bNegative = false;
			if (str[0] == '-') {
				bNegative = true;
			}
			str.TrimLeft(L"+-");

			WCHAR c;
			int hh, mm, ss, ms;
			if (swscanf_s(str, L"%d%c%d%c%d%c%d", &hh, &c, 1, &mm, &c, 1, &ss, &c, 1, &ms) != 4+3) {
				bError = true;
				continue;
			}

			delay += (hh*60*60*1000 + mm*60*1000 + ss*1000 + ms) * (bNegative ? -1 : 1);
		} else if (id >= 0 && entry == L"timestamp") {
			SubPos sb;

			sb.vobid = (char)vobid;
			sb.cellid = (char)cellid;
			sb.celltimestamp = celltimestamp;
			sb.bValid = true;

			bool bNegative = false;
			if (str[0] == '-') {
				bNegative = true;
			}
			str.TrimLeft(L"+-");

			WCHAR c;
			int hh, mm, ss, ms;
			if (swscanf_s(str, L"%d%c%d%c%d%c%d", &hh, &c, 1, &mm, &c, 1, &ss, &c, 1, &ms) != 4+3) {
				bError = true;
				continue;
			}

			sb.start = (hh*60*60*1000 + mm*60*1000 + ss*1000 + ms) * (bNegative ? -1 : 1) + delay;

			i = str.Find(L"filepos:");
			if (i < 0) {
				bError = true;
				continue;
			}
			str = str.Mid(i + (int)wcslen(L"filepos:"));

			if (swscanf_s(str, L"%I64x", &sb.filepos) != 1) {
				bError = true;
				continue;
			}

			if (delay < 0 && !m_langs[id].subpos.empty()) {
				__int64 ts = m_langs[id].subpos[m_langs[id].subpos.size()-1].start;

				if (sb.start < ts) {
					delay += (int)(ts - sb.start);
					sb.start = ts;
				}
			}

			m_langs[id].subpos.push_back(sb);
		} else {
			bError = true;
		}
	}

	return !bError;
}

bool CVobSubFile::ReadSub(CString fn)
{
	CFile f;
	if (!f.Open(fn, CFile::modeRead|CFile::typeBinary|CFile::shareDenyNone)) {
		return false;
	}

	m_sub.SetLength(f.GetLength());
	m_sub.SeekToBegin();

	int len;
	BYTE buff[2048];
	try {
		while ((len = f.Read(buff, sizeof(buff))) > 0 && GETU32(buff) == 0xba010000) {
			m_sub.Write(buff, len);
		}
	}
	catch (CFileException*) {
		return false;
	}

	return true;
}

static unsigned char* RARbuff = nullptr;
static unsigned int RARpos = 0;

static int PASCAL MyProcessDataProc(unsigned char* Addr, int Size)
{
	ASSERT(RARbuff);

	memcpy(&RARbuff[RARpos], Addr, Size);
	RARpos += Size;

	return 1;
}

bool CVobSubFile::ReadRar(CString fn)
{
#ifdef _WIN64
	HMODULE h = LoadLibraryW(L"unrar64.dll");
#else
	HMODULE h = LoadLibraryW(L"unrar.dll");
#endif
	if (!h) {
		return false;
	}

	RAROpenArchiveEx OpenArchiveEx = (RAROpenArchiveEx)GetProcAddress(h, "RAROpenArchiveEx");
	RARCloseArchive CloseArchive = (RARCloseArchive)GetProcAddress(h, "RARCloseArchive");
	RARReadHeaderEx ReadHeaderEx = (RARReadHeaderEx)GetProcAddress(h, "RARReadHeaderEx");
	RARProcessFile ProcessFile = (RARProcessFile)GetProcAddress(h, "RARProcessFile");
	RARSetProcessDataProc SetProcessDataProc = (RARSetProcessDataProc)GetProcAddress(h, "RARSetProcessDataProc");

	if (!(OpenArchiveEx && CloseArchive && ReadHeaderEx && ProcessFile && SetProcessDataProc)) {
		FreeLibrary(h);
		return false;
	}

	struct RAROpenArchiveDataEx ArchiveDataEx;
	memset(&ArchiveDataEx, 0, sizeof(ArchiveDataEx));
	ArchiveDataEx.ArcNameW = (LPWSTR)(LPCWSTR)fn;
	char fnA[MAX_PATH];
	size_t size;
	if (wcstombs_s(&size, fnA, fn, fn.GetLength())) {
		fnA[0] = 0;
	}
	ArchiveDataEx.ArcName = fnA;
	ArchiveDataEx.OpenMode = RAR_OM_EXTRACT;
	ArchiveDataEx.CmtBuf = 0;
	HANDLE hrar = OpenArchiveEx(&ArchiveDataEx);
	if (!hrar) {
		FreeLibrary(h);
		return false;
	}

	SetProcessDataProc(hrar, MyProcessDataProc);

	struct RARHeaderDataEx HeaderDataEx;
	HeaderDataEx.CmtBuf = nullptr;

	while (ReadHeaderEx(hrar, &HeaderDataEx) == 0) {
		CString subfn(HeaderDataEx.FileNameW);

		if (!subfn.Right(4).CompareNoCase(L".sub")) {
			std::unique_ptr<BYTE[]> buff(new(std::nothrow) BYTE[HeaderDataEx.UnpSize]);
			if (!buff) {
				CloseArchive(hrar);
				FreeLibrary(h);
				return false;
			}

			RARbuff = buff.get();
			RARpos = 0;

			if (ProcessFile(hrar, RAR_TEST, nullptr, nullptr)) {
				CloseArchive(hrar);
				FreeLibrary(h);

				return false;
			}

			m_sub.SetLength(HeaderDataEx.UnpSize);
			m_sub.SeekToBegin();
			m_sub.Write(buff.get(), HeaderDataEx.UnpSize);
			m_sub.SeekToBegin();

			RARbuff = nullptr;
			RARpos = 0;

			break;
		}

		ProcessFile(hrar, RAR_SKIP, nullptr, nullptr);
	}

	CloseArchive(hrar);
	FreeLibrary(h);

	return true;
}

bool CVobSubFile::ReadIfo(CString fn)
{
	CFile f;
	if (!f.Open(fn, CFile::modeRead|CFile::typeBinary|CFile::shareDenyNone)) {
		return false;
	}

	/* PGC1 */

	f.Seek(0xc0 + 0x0c, SEEK_SET);

	DWORD pos;
	ReadBEdw(pos);

	f.Seek(pos * 0x800 + 0x0c, CFile::begin);

	DWORD offset;
	ReadBEdw(offset);

	/* Subpic palette */

	f.Seek(pos * 0x800 + offset + 0xa4, CFile::begin);

	for (size_t i = 0; i < 16; i++) {
		BYTE y, u, v, tmp;

		f.Read(&tmp, 1);
		f.Read(&y, 1);
		f.Read(&u, 1);
		f.Read(&v, 1);

		y = (y - 16) * 255 / 219;

		m_orgpal[i].rgbRed   = (BYTE)std::clamp(1.0 * y + 1.4022 * (u - 128), 0.0, 255.0);
		m_orgpal[i].rgbGreen = (BYTE)std::clamp(1.0 * y - 0.3456 * (u - 128) - 0.7145 * (v - 128), 0.0, 255.0);
		m_orgpal[i].rgbBlue  = (BYTE)std::clamp(1.0 * y + 1.7710 * (v - 128), 0.0, 255.0);
	}

	return true;
}

bool CVobSubFile::WriteIdx(CString fn)
{
	CTextFile f;
	if (!f.Save(fn, CTextFile::ASCII)) {
		return false;
	}

	CString str;
	str.Format(L"# VobSub index file, v%d (do not modify this line!)\n", VOBSUBIDXVER);

	f.WriteString(str);
	f.WriteString(L"# \n");
	f.WriteString(L"# To repair desyncronization, you can insert gaps this way:\n");
	f.WriteString(L"# (it usually happens after vob id changes)\n");
	f.WriteString(L"# \n");
	f.WriteString(L"#\t delay: [sign]hh:mm:ss:ms\n");
	f.WriteString(L"# \n");
	f.WriteString(L"# Where:\n");
	f.WriteString(L"#\t [sign]: +, - (optional)\n");
	f.WriteString(L"#\t hh: hours (0 <= hh)\n");
	f.WriteString(L"#\t mm/ss: minutes/seconds (0 <= mm/ss <= 59)\n");
	f.WriteString(L"#\t ms: milliseconds (0 <= ms <= 999)\n");
	f.WriteString(L"# \n");
	f.WriteString(L"#\t Note: You can't position a sub before the previous with a negative value.\n");
	f.WriteString(L"# \n");
	f.WriteString(L"# You can also modify timestamps or delete a few subs you don't like.\n");
	f.WriteString(L"# Just make sure they stay in increasing order.\n");
	f.WriteString(L"\n");
	f.WriteString(L"\n");

	// Settings

	f.WriteString(L"# Settings\n\n");

	f.WriteString(L"# Original frame size\n");
	str.Format(L"size: %dx%d\n\n", m_size.cx, m_size.cy);
	f.WriteString(str);

	f.WriteString(L"# Origin, relative to the upper-left corner, can be overloaded by aligment\n");
	str.Format(L"org: %d, %d\n\n", m_x, m_y);
	f.WriteString(str);

	f.WriteString(L"# Image scaling (hor,ver), origin is at the upper-left corner or at the alignment coord (x, y)\n");
	str.Format(L"scale: %d%%, %d%%\n\n", m_scale_x, m_scale_y);
	f.WriteString(str);

	f.WriteString(L"# Alpha blending\n");
	str.Format(L"alpha: %d%%\n\n", m_alpha);
	f.WriteString(str);

	f.WriteString(L"# Smoothing for very blocky images (use OLD for no filtering)\n");
	str.Format(L"smooth: %s\n\n", m_iSmooth == 0 ? L"OFF" : m_iSmooth == 1 ? L"ON" : L"OLD");
	f.WriteString(str);

	f.WriteString(L"# In millisecs\n");
	str.Format(L"fadein/out: %d, %d\n\n", m_fadein, m_fadeout);
	f.WriteString(str);

	f.WriteString(L"# Force subtitle placement relative to (org.x, org.y)\n");
	str.Format(L"align: %s %s %s\n\n",
			   m_bAlign ? L"ON at" : L"OFF at",
			   m_alignhor == 0 ? L"LEFT" : m_alignhor == 1 ? L"CENTER" : m_alignhor == 2 ? L"RIGHT" : L"",
			   m_alignver == 0 ? L"TOP" : m_alignver == 1 ? L"CENTER" : m_alignver == 2 ? L"BOTTOM" : L"");
	f.WriteString(str);

	f.WriteString(L"# For correcting non-progressive desync. (in millisecs or hh:mm:ss:ms)\n");
	f.WriteString(L"# Note: Not effective in DirectVobSub, use \"delay: ... \" instead.\n");
	str.Format(L"time offset: %u\n\n", m_toff);
	f.WriteString(str);

	f.WriteString(L"# ON: displays only forced subtitles, OFF: shows everything\n");
	str.Format(L"forced subs: %s\n\n", m_bOnlyShowForcedSubs ? L"ON" : L"OFF");
	f.WriteString(str);

	f.WriteString(L"# The original palette of the DVD\n");
	str.Format(L"palette: %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x\n\n",
			   *((unsigned int*)&m_orgpal[0])&0xffffff,
			   *((unsigned int*)&m_orgpal[1])&0xffffff,
			   *((unsigned int*)&m_orgpal[2])&0xffffff,
			   *((unsigned int*)&m_orgpal[3])&0xffffff,
			   *((unsigned int*)&m_orgpal[4])&0xffffff,
			   *((unsigned int*)&m_orgpal[5])&0xffffff,
			   *((unsigned int*)&m_orgpal[6])&0xffffff,
			   *((unsigned int*)&m_orgpal[7])&0xffffff,
			   *((unsigned int*)&m_orgpal[8])&0xffffff,
			   *((unsigned int*)&m_orgpal[9])&0xffffff,
			   *((unsigned int*)&m_orgpal[10])&0xffffff,
			   *((unsigned int*)&m_orgpal[11])&0xffffff,
			   *((unsigned int*)&m_orgpal[12])&0xffffff,
			   *((unsigned int*)&m_orgpal[13])&0xffffff,
			   *((unsigned int*)&m_orgpal[14])&0xffffff,
			   *((unsigned int*)&m_orgpal[15])&0xffffff);
	f.WriteString(str);

	int tridx = (!!(m_tridx&1))*0x1000 + (!!(m_tridx&2))*0x100 + (!!(m_tridx&4))*0x10 + (!!(m_tridx&8));

	f.WriteString(L"# Custom colors (transp idxs and the four colors)\n");
	str.Format(L"custom colors: %s, tridx: %04x, colors: %06x, %06x, %06x, %06x\n\n",
			   m_bCustomPal ? L"ON" : L"OFF",
			   tridx,
			   *((unsigned int*)&m_cuspal[0])&0xffffff,
			   *((unsigned int*)&m_cuspal[1])&0xffffff,
			   *((unsigned int*)&m_cuspal[2])&0xffffff,
			   *((unsigned int*)&m_cuspal[3])&0xffffff);
	f.WriteString(str);

	f.WriteString(L"# Language index in use\n");
	str.Format(L"langidx: %d\n\n", m_nLang);
	f.WriteString(str);

	// Subs

	for (size_t i = 0; i < std::size(m_langs); i++) {
		SubLang& sl = m_langs[i];

		std::vector<SubPos>& sp = sl.subpos;
		if (sp.empty() && !sl.id) {
			continue;
		}

		str.Format(L"# %s\n", sl.name);
		f.WriteString(str);

		ASSERT(sl.id);
		if (!sl.id) {
			sl.id = '--';
		}
		str.Format(L"id: %c%c, index: %Iu\n", sl.id >> 8, sl.id & 0xff, i);
		f.WriteString(str);

		str = L"# Uncomment next line to activate alternative name in DirectVobSub / Windows Media Player 6.x\n";
		f.WriteString(str);
		str.Format(L"alt: %s\n", sl.alt);
		if (sl.name == sl.alt) {
			str = L"# " + str;
		}
		f.WriteString(str);

		char vobid = -1, cellid = -1;

		for (size_t j = 0; j < sp.size(); j++) {
			if (!sp[j].bValid) {
				continue;
			}

			if (sp[j].vobid != vobid || sp[j].cellid != cellid) {
				str.Format(L"# Vob/Cell ID: %d, %d (PTS: %I64d)\n", sp[j].vobid, sp[j].cellid, sp[j].celltimestamp);
				f.WriteString(str);
				vobid = sp[j].vobid;
				cellid = sp[j].cellid;
			}

			str.Format(L"timestamp: %s%02d:%02d:%02d:%03d, filepos: %09I64x\n",
					   sp[j].start < 0 ? L"-" : L"",
					   abs(int((sp[j].start/1000/60/60)%60)),
					   abs(int((sp[j].start/1000/60)%60)),
					   abs(int((sp[j].start/1000)%60)),
					   abs(int((sp[j].start)%1000)),
					   sp[j].filepos);
			f.WriteString(str);
		}

		f.WriteString(L"\n");
	}

	return true;
}

bool CVobSubFile::WriteSub(CString fn)
{
	CFile f;
	if (!f.Open(fn, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary|CFile::shareDenyWrite)) {
		return false;
	}

	if (m_sub.GetLength() == 0) {
		return true;	// nothing to do...
	}

	m_sub.SeekToBegin();

	int len;
	BYTE buff[2048];
	while ((len = m_sub.Read(buff, sizeof(buff))) > 0 && GETU32(buff) == 0xba010000) {
		f.Write(buff, len);
	}

	return true;
}

//

BYTE* CVobSubFile::GetPacket(size_t idx, int& packetsize, int& datasize, int nLang)
{
	BYTE* ret = nullptr;

	if (nLang < 0 || nLang >= (int)std::size(m_langs)) {
		nLang = m_nLang;
	}
	std::vector<SubPos>& sp = m_langs[nLang].subpos;

	do {
		if (idx >= sp.size()) {
			break;
		}

		if ((__int64)m_sub.Seek(sp[idx].filepos, CFile::begin) != sp[idx].filepos) {
			break;
		}

		BYTE buff[0x800];
		if (sizeof(buff) != m_sub.Read(buff, sizeof(buff))) {
			break;
		}

		// let's check a few things to make sure...
		if (GETU32(&buff[0x00]) != 0xba010000
				|| GETU32(&buff[0x0e]) != 0xbd010000
				|| !(buff[0x15] & 0x80)
				|| (buff[0x17] & 0xf0) != 0x20
				|| (buff[buff[0x16] + 0x17] & 0xe0) != 0x20
				|| (buff[buff[0x16] + 0x17] & 0x1f) != nLang) {
			break;
		}

		packetsize = (buff[buff[0x16] + 0x18] << 8) + buff[buff[0x16] + 0x19];
		datasize = (buff[buff[0x16] + 0x1a] << 8) + buff[buff[0x16] + 0x1b];

		ret = DNew BYTE[packetsize];
		if (!ret) {
			break;
		}

		int i = 0, sizeleft = packetsize;
		for (int size; i < packetsize; i += size, sizeleft -= size) {
			int hsize = 0x18 + buff[0x16];
			size = std::min(sizeleft, 0x800 - hsize);
			memcpy(&ret[i], &buff[hsize], size);

			if (size != sizeleft) {
				while (m_sub.Read(buff, sizeof(buff))) {
					if (/*!(buff[0x15] & 0x80) &&*/ buff[buff[0x16] + 0x17] == (nLang|0x20)) {
						break;
					}
				}
			}
		}

		if (i != packetsize || sizeleft > 0) {
			SAFE_DELETE_ARRAY(ret);
		}
	} while (false);

	return ret;
}

const CVobSubFile::SubPos* CVobSubFile::GetFrameInfo(size_t idx, int iLang /*= -1*/) const
{
	if (iLang < 0 || iLang >= (int)std::size(m_langs)) {
		iLang = m_nLang;
	}
	const std::vector<SubPos>& sp = m_langs[iLang].subpos;

	if (idx >= sp.size()
			|| !sp[idx].bValid
			|| ((m_bOnlyShowForcedSubs || g_bForcedSubtitle) && !sp[idx].bForced)) {
		return nullptr;
	}

	return &sp[idx];
}

bool CVobSubFile::GetFrame(size_t idx, int iLang /*= -1*/, REFERENCE_TIME rt /*= -1*/)
{
	if (iLang < 0 || iLang >= (int)std::size(m_langs)) {
		iLang = m_nLang;
	}
	std::vector<SubPos>& sp = m_langs[iLang].subpos;

	if (idx >= sp.size()) {
		return false;
	}

	if (m_img.nLang != iLang || m_img.nIdx != idx
			|| (sp[idx].bAnimated && sp[idx].start + m_img.tCurrent <= rt)) {
		int packetsize = 0, datasize = 0;
		std::unique_ptr<BYTE[]> buff(GetPacket(idx, packetsize, datasize, iLang));
		if (!buff || packetsize <= 0 || datasize <= 0) {
			return false;
		}

		m_img.start = sp[idx].start;

		bool ret = m_img.Decode(buff.get(), packetsize, datasize, rt >= 0 ? int(rt - sp[idx].start) : INT_MAX,
								m_bCustomPal, m_tridx, m_orgpal, m_cuspal, true);

		m_img.delay = sp[idx].stop - sp[idx].start;

		if (!ret) {
			return false;
		}

		m_img.nIdx = idx;
		m_img.nLang = iLang;
	}

	return (m_bOnlyShowForcedSubs ? m_img.bForced : true);
}

bool CVobSubFile::GetFrameByTimeStamp(__int64 time)
{
	return GetFrame(GetFrameIdxByTimeStamp(time));
}

int CVobSubFile::GetFrameIdxByTimeStamp(__int64 time)
{
	if (m_nLang < 0 || m_nLang >= (int)std::size(m_langs)) {
		return -1;
	}

	std::vector<SubPos>& sp = m_langs[m_nLang].subpos;

	int i = 0, j = (int)sp.size() - 1, ret = -1;

	if (j >= 0 && time >= sp[j].start) {
		return j;
	}

	while (i < j) {
		int mid = (i + j) >> 1;
		int midstart = (int)sp[mid].start;

		if (time == midstart) {
			ret = mid;
			break;
		} else if (time < midstart) {
			ret = -1;
			if (j == mid) {
				mid--;
			}
			j = mid;
		} else if (time > midstart) {
			ret = mid;
			if (i == mid) {
				mid++;
			}
			i = mid;
		}
	}

	return ret;
}

//

STDMETHODIMP CVobSubFile::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = nullptr;

	return
		QI(IPersist)
		QI(ISubStream)
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

// TODO: return segments for the fade-in/out time (with animated set to "true" of course)

STDMETHODIMP_(POSITION) CVobSubFile::GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld/* = false*/)
{
	rt /= 10000;

	int i = GetFrameIdxByTimeStamp(rt);

	const SubPos* sp = GetFrameInfo(i);
	if (!sp) {
		return nullptr;
	}

	if (rt >= sp->stop) {
		if (!GetFrameInfo(++i)) {
			return nullptr;
		}
	}

	return (POSITION)(INT_PTR)(i + 1);
}

STDMETHODIMP_(POSITION) CVobSubFile::GetNext(POSITION pos)
{
	size_t i = (size_t)pos;
	return (GetFrameInfo(i) ? (POSITION)(i + 1) : nullptr);
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubFile::GetStart(POSITION pos, double fps)
{
	size_t i = (size_t)pos - 1;
	const SubPos* sp = GetFrameInfo(i);
	return (sp ? 10000i64 * sp->start : 0);
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubFile::GetStop(POSITION pos, double fps)
{
	size_t i = (size_t)pos - 1;
	const SubPos* sp = GetFrameInfo(i);
	return (sp ? 10000i64 * sp->stop : 0);
}

STDMETHODIMP_(bool) CVobSubFile::IsAnimated(POSITION pos)
{
	size_t i = (size_t)pos - 1;
	const SubPos* sp = GetFrameInfo(i);
	return (sp ? sp->bAnimated : false);
}

STDMETHODIMP CVobSubFile::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	if (spd.bpp != 32) {
		return E_INVALIDARG;
	}

	rt /= 10000;

	if (!GetFrame(GetFrameIdxByTimeStamp(rt), -1, rt)) {
		return E_FAIL;
	}

	if (rt >= (m_img.start + m_img.delay)) {
		return E_FAIL;
	}

	return __super::Render(spd, bbox);
}

// IPersist

STDMETHODIMP CVobSubFile::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CVobSubFile::GetStreamCount()
{
	int iStreamCount = 0;
	for (const auto& sl : m_langs) {
		if (sl.subpos.size()) {
			iStreamCount++;
		}
	}
	return iStreamCount;
}

DWORD LangIDToLCID(WORD langid)
{
	unsigned short id = lang_tbl[find_lang(langid)].id;
	CHAR tmp[3];
	tmp[0] = id / 256;
	tmp[1] = id & 0xFF;
	tmp[2] = 0;
	return ISO6391ToLcid(tmp);
}

STDMETHODIMP CVobSubFile::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
	for (const auto& sl : m_langs) {
		if (sl.subpos.empty() || iStream-- > 0) {
			continue;
		}

		if (ppName) {
			*ppName = (WCHAR*)CoTaskMemAlloc((sl.alt.GetLength() + 1) * sizeof(WCHAR));
			if (!(*ppName)) {
				return E_OUTOFMEMORY;
			}

			wcscpy_s(*ppName, sl.alt.GetLength() + 1, CStringW(sl.alt));
		}

		if (pLCID) {
			*pLCID = LangIDToLCID(sl.id);
		}

		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP_(int) CVobSubFile::GetStream()
{
	int iStream = 0;

	if (m_nLang < (int)std::size(m_langs)) {
		for (int i = 0; i < m_nLang; i++) {
			if (!m_langs[i].subpos.empty()) {
				iStream++;
			}
		}
	}

	return iStream;
}

STDMETHODIMP CVobSubFile::SetStream(int iStream)
{
	for (int i = 0; i < (int)std::size(m_langs); i++) {
		std::vector<SubPos>& sp = m_langs[i].subpos;

		if (sp.empty() || iStream-- > 0) {
			continue;
		}

		m_nLang = i;

		m_img.Invalidate();

		break;
	}

	return iStream < 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CVobSubFile::Reload()
{
	if (!::PathFileExistsW(m_title + L".idx")) {
		return E_FAIL;
	}
	return !m_title.IsEmpty() && Open(m_title) ? S_OK : E_FAIL;
}

// StretchBlt

static void PixelAtBiLinear(RGBQUAD& c, int x, int y, CVobSubImage& src)
{
	int w = src.rect.Width(),
		h = src.rect.Height();

	int x1 = (x >> 16),
		y1 = (y >> 16) * w,
		x2 = std::min(x1 + 1, w - 1),
		y2 = std::min(y1 + w, (h - 1) * w);

	RGBQUAD* ptr = src.lpPixels;

	RGBQUAD c11 = ptr[y1 + x1],	c12 = ptr[y1 + x2],
			c21 = ptr[y2 + x1],	c22 = ptr[y2 + x2];

	__int64 u2 = x & 0xffff,
			v2 = y & 0xffff,
			u1 = 0x10000 - u2,
			v1 = 0x10000 - v2;

	int v1u1 = int(v1*u1 >> 16) * c11.rgbReserved,
		v1u2 = int(v1*u2 >> 16) * c12.rgbReserved,
		v2u1 = int(v2*u1 >> 16) * c21.rgbReserved,
		v2u2 = int(v2*u2 >> 16) * c22.rgbReserved;

	c.rgbRed = (c11.rgbRed * v1u1 + c12.rgbRed * v1u2
				+ c21.rgbRed * v2u1 + c22.rgbRed * v2u2) >> 24;
	c.rgbGreen = (c11.rgbGreen * v1u1 + c12.rgbGreen * v1u2
				  + c21.rgbGreen * v2u1 + c22.rgbGreen * v2u2) >> 24;
	c.rgbBlue = (c11.rgbBlue * v1u1 + c12.rgbBlue * v1u2
				 + c21.rgbBlue * v2u1 + c22.rgbBlue * v2u2) >> 24;
	c.rgbReserved = (v1u1 + v1u2
					 + v2u1 + v2u2) >> 16;
}

static void StretchBlt(SubPicDesc& spd, CRect dstrect, CVobSubImage& src)
{
	if (dstrect.IsRectEmpty()) {
		return;
	}

	if ((dstrect & CRect(0, 0, spd.w, spd.h)).IsRectEmpty()) {
		return;
	}

	int sw = src.rect.Width(),
		sh = src.rect.Height(),
		dw = dstrect.Width(),
		dh = dstrect.Height();

	int srcx = 0,
		srcy = 0,
		srcdx = (sw << 16) / dw >> 1,
		srcdy = (sh << 16) / dh >> 1;

	if (dstrect.left < 0) {
		srcx = -dstrect.left * (srcdx << 1);
		dstrect.left = 0;
	}
	if (dstrect.top < 0) {
		srcy = -dstrect.top * (srcdy << 1);
		dstrect.top = 0;
	}
	if (dstrect.right > spd.w) {
		dstrect.right = spd.w;
	}
	if (dstrect.bottom > spd.h) {
		dstrect.bottom = spd.h;
	}

	if ((dstrect & CRect(0, 0, spd.w, spd.h)).IsRectEmpty()) {
		return;
	}

	dw = dstrect.Width();
	dh = dstrect.Height();

	for (int y = dstrect.top; y < dstrect.bottom; y++, srcy += (srcdy<<1)) {
		RGBQUAD* ptr = (RGBQUAD*)&(spd.bits)[y*spd.pitch] + dstrect.left;
		RGBQUAD* endptr = ptr + dw;

		for (int sx = srcx; ptr < endptr; sx += (srcdx<<1), ptr++) {
			RGBQUAD cc[4];

			PixelAtBiLinear(cc[0],	sx,			srcy,		src);
			PixelAtBiLinear(cc[1],	sx+srcdx,	srcy,		src);
			PixelAtBiLinear(cc[2],	sx,			srcy+srcdy,	src);
			PixelAtBiLinear(cc[3],	sx+srcdx,	srcy+srcdy,	src);

			ptr->rgbRed = (cc[0].rgbRed + cc[1].rgbRed + cc[2].rgbRed + cc[3].rgbRed) >> 2;
			ptr->rgbGreen = (cc[0].rgbGreen + cc[1].rgbGreen + cc[2].rgbGreen + cc[3].rgbGreen) >> 2;
			ptr->rgbBlue = (cc[0].rgbBlue + cc[1].rgbBlue + cc[2].rgbBlue + cc[3].rgbBlue) >> 2;
			ptr->rgbReserved = (cc[0].rgbReserved + cc[1].rgbReserved + cc[2].rgbReserved + cc[3].rgbReserved) >> 2;
			////
			ptr->rgbRed = ptr->rgbRed * ptr->rgbReserved >> 8;
			ptr->rgbGreen = ptr->rgbGreen * ptr->rgbReserved >> 8;
			ptr->rgbBlue = ptr->rgbBlue * ptr->rgbReserved >> 8;
			ptr->rgbReserved = ~ptr->rgbReserved;

		}
	}
}

//
// CVobSubSettings
//

void CVobSubSettings::InitSettings()
{
	m_size.SetSize(720, 480);
	m_toff = m_x = m_y = 0;
	m_org.SetPoint(0, 0);
	m_scale_x = m_scale_y = m_alpha = 100;
	m_fadein = m_fadeout = 50;
	m_iSmooth = 0;
	m_bAlign = false;
	m_alignhor = m_alignver = 0;
	m_bOnlyShowForcedSubs = false;
	m_bCustomPal = false;
	m_tridx = 0;
	ZeroMemory(m_orgpal, sizeof(m_orgpal));
	ZeroMemory(m_cuspal, sizeof(m_cuspal));
}

bool CVobSubSettings::GetCustomPal(RGBQUAD* cuspal, int& tridx)
{
	memcpy(cuspal, m_cuspal, sizeof(RGBQUAD)*4);
	tridx = m_tridx;
	return m_bCustomPal;
}

void CVobSubSettings::SetCustomPal(const RGBQUAD* cuspal, int tridx)
{
	memcpy(m_cuspal, cuspal, sizeof(RGBQUAD)*4);
	m_tridx = tridx & 0xf;
	for (int i = 0; i < 4; i++) {
		m_cuspal[i].rgbReserved = (tridx&(1<<i)) ? 0 : 0xff;
	}
	m_img.Invalidate();
}

void CVobSubSettings::GetDestrect(CRect& r)
{
	int w = MulDiv(m_img.rect.Width(), m_scale_x, 100);
	int h = MulDiv(m_img.rect.Height(), m_scale_y, 100);

	if (!m_bAlign) {
		r.left = MulDiv(m_img.rect.left, m_scale_x, 100);
		r.right = MulDiv(m_img.rect.right, m_scale_x, 100);
		r.top = MulDiv(m_img.rect.top, m_scale_y, 100);
		r.bottom = MulDiv(m_img.rect.bottom, m_scale_y, 100);
	} else {
		switch (m_alignhor) {
			case 0:
				r.left = 0;
				r.right = w;
				break; // left
			case 1:
				r.left = -(w >> 1);
				r.right = -(w >> 1) + w;
				break; // center
			case 2:
				r.left = -w;
				r.right = 0;
				break; // right
			default:
				r.left = MulDiv(m_img.rect.left, m_scale_x, 100);
				r.right = MulDiv(m_img.rect.right, m_scale_x, 100);
				break;
		}

		switch (m_alignver) {
			case 0:
				r.top = 0;
				r.bottom = h;
				break; // top
			case 1:
				r.top = -(h >> 1);
				r.bottom = -(h >> 1) + h;
				break; // center
			case 2:
				r.top = -h;
				r.bottom = 0;
				break; // bottom
			default:
				r.top = MulDiv(m_img.rect.top, m_scale_y, 100);
				r.bottom = MulDiv(m_img.rect.bottom, m_scale_y, 100);
				break;
		}
	}

	r += m_org;
}

void CVobSubSettings::GetDestrect(CRect& r, int w, int h)
{
	GetDestrect(r);

	r.left = MulDiv(r.left, w, m_size.cx);
	r.right = MulDiv(r.right, w, m_size.cx);
	r.top = MulDiv(r.top, h, m_size.cy);
	r.bottom = MulDiv(r.bottom, h, m_size.cy);
}

void CVobSubSettings::SetAlignment(bool bAlign, int x, int y, int hor, int ver)
{
	m_bAlign = bAlign;
	if (bAlign) {
		m_org.x = MulDiv(m_size.cx, x, 100);
		m_org.y = MulDiv(m_size.cy, y, 100);
		m_alignhor = std::clamp(hor, 0, 2);
		m_alignver = std::clamp(ver, 0, 2);
	} else {
		m_org.x = m_x;
		m_org.y = m_y;
	}
}

HRESULT CVobSubSettings::Render(SubPicDesc& spd, RECT& bbox)
{
	CRect r;
	GetDestrect(r, spd.w, spd.h);
	StretchBlt(spd, r, m_img);
	/*
		CRenderedTextSubtitle rts(nullptr);
		rts.CreateDefaultStyle(DEFAULT_CHARSET);
		rts.m_dstScreenSize.SetSize(m_size.cx, m_size.cy);
		CStringW assstr;
		m_img.Polygonize(assstr, false);
		REFERENCE_TIME rtStart = 10000i64*m_img.start, rtStop = 10000i64*(m_img.start+m_img.delay);
		rts.Add(assstr, true, rtStart, rtStop);
		rts.Render(spd, (rtStart+rtStop)/2, 25, r);
	*/
	r &= CRect(CPoint(0, 0), CSize(spd.w, spd.h));
	bbox = r;
	return !r.IsRectEmpty() ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////

static bool CompressFile(CString fn)
{
	BOOL b = FALSE;

	HANDLE h = CreateFileW(fn, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (h != INVALID_HANDLE_VALUE) {
		USHORT us = COMPRESSION_FORMAT_DEFAULT;
		DWORD nBytesReturned;
		b = DeviceIoControl(h, FSCTL_SET_COMPRESSION, (LPVOID)&us, 2, nullptr, 0, (LPDWORD)&nBytesReturned, nullptr);
		CloseHandle(h);
	}

	return !!b;
}

bool CVobSubFile::SaveVobSub(CString fn)
{
	return WriteIdx(fn + L".idx") && WriteSub(fn + L".sub");
}

bool CVobSubFile::SaveWinSubMux(CString fn)
{
	TrimExtension(fn);

	CStdioFile f;
	if (!f.Open(fn + L".sub", CFile::modeCreate|CFile::modeWrite|CFile::typeText|CFile::shareDenyWrite)) {
		return false;
	}

	m_img.Invalidate();

	std::unique_ptr<BYTE[]> p4bpp(new(std::nothrow) BYTE[720*576/2]);
	if (!p4bpp) {
		return false;
	}

	std::vector<SubPos>& sp = m_langs[m_nLang].subpos;
	for (size_t i = 0; i < sp.size(); i++) {
		if (!GetFrame((int)i)) {
			continue;
		}

		int pal[4] = {0, 1, 2, 3};

		for (int j = 0; j < 5; j++) {
			if (j == 4 || !m_img.pal[j].tr) {
				j &= 3;
				memset(p4bpp.get(), (j<<4)|j, 720*576/2);
				pal[j] ^= pal[0], pal[0] ^= pal[j], pal[j] ^= pal[0];
				break;
			}
		}

		int tr[4] = {m_img.pal[pal[0]].tr, m_img.pal[pal[1]].tr, m_img.pal[pal[2]].tr, m_img.pal[pal[3]].tr};

		DWORD uipal[4+12];

		if (!m_bCustomPal) {
			uipal[0] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[0]].pal]);
			uipal[1] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[1]].pal]);
			uipal[2] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[2]].pal]);
			uipal[3] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[3]].pal]);
		} else {
			uipal[0] = *((DWORD*)&m_img.cuspal[pal[0]]) & 0xffffff;
			uipal[1] = *((DWORD*)&m_img.cuspal[pal[1]]) & 0xffffff;
			uipal[2] = *((DWORD*)&m_img.cuspal[pal[2]]) & 0xffffff;
			uipal[3] = *((DWORD*)&m_img.cuspal[pal[3]]) & 0xffffff;
		}

		CAtlMap<DWORD,BYTE> palmap;
		palmap[uipal[0]] = 0;
		palmap[uipal[1]] = 1;
		palmap[uipal[2]] = 2;
		palmap[uipal[3]] = 3;

		uipal[0] = 0xff; // blue background

		int w = m_img.rect.Width()-2;
		int h = m_img.rect.Height()-2;
		int pitch = (((w+1)>>1) + 3) & ~3;

		for (ptrdiff_t y = 0; y < h; y++) {
			DWORD* p = (DWORD*)&m_img.lpPixels[(y+1)*(w+2)+1];

			for (ptrdiff_t x = 0; x < w; x++, p++) {
				BYTE c = 0;

				if (*p & 0xff000000) {
					DWORD uic = *p & 0xffffff;
					palmap.Lookup(uic, c);
				}

				BYTE& c4bpp = p4bpp[(h-y-1)*pitch+(x>>1)];
				c4bpp = (x&1) ? ((c4bpp&0xf0)|c) : ((c4bpp&0x0f)|(c<<4));
			}
		}

		int t1 = (int)m_img.start;
		int t2 = t1 + (int)m_img.delay /*+ (m_size.cy==480?(1000/29.97+1):(1000/25))*/;

		ASSERT(t2>t1);

		if (t2 <= 0) {
			continue;
		}
		if (t1 < 0) {
			t1 = 0;
		}

		CString bmpfn;
		bmpfn.Format(L"%s_%06u.bmp", fn, i+1);

		CString str;
		str.Format(L"%s\t%02d:%02d:%02d:%02d %02d:%02d:%02d:%02d\t%03d %03d %03d %03d %d %d %d %d\n",
				   bmpfn,
				   t1/1000/60/60, (t1/1000/60)%60, (t1/1000)%60, (t1%1000)/10,
				   t2/1000/60/60, (t2/1000/60)%60, (t2/1000)%60, (t2%1000)/10,
				   m_img.rect.Width(), m_img.rect.Height(), m_img.rect.left, m_img.rect.top,
				   (tr[0]<<4)|tr[0], (tr[1]<<4)|tr[1], (tr[2]<<4)|tr[2], (tr[3]<<4)|tr[3]);
		f.WriteString(str);

		BITMAPFILEHEADER fhdr = {
			0x4d42,
			sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD) + pitch*h,
			0, 0,
			sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD)
		};

		BITMAPINFOHEADER ihdr = {
			sizeof(BITMAPINFOHEADER),
			w, h, 1, 4, 0,
			0,
			pitch*h, 0,
			16, 4
		};

		CFile bmp;
		if (bmp.Open(bmpfn, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary|CFile::shareDenyWrite)) {
			bmp.Write(&fhdr, sizeof(fhdr));
			bmp.Write(&ihdr, sizeof(ihdr));
			bmp.Write(uipal, sizeof(RGBQUAD)*16);
			bmp.Write(p4bpp.get(), pitch*h);
			bmp.Close();

			CompressFile(bmpfn);
		}
	}

	return true;
}

bool CVobSubFile::SaveScenarist(CString fn)
{
	TrimExtension(fn);

	CStdioFile f;
	if (!f.Open(fn + L".sst", CFile::modeCreate|CFile::modeWrite|CFile::typeText|CFile::shareDenyWrite)) {
		return false;
	}

	m_img.Invalidate();

	fn.Replace('\\', '/');
	CString title = fn.Mid(fn.ReverseFind('/')+1);

	WCHAR buff[MAX_PATH], * pFilePart = buff;
	if (GetFullPathNameW(fn, MAX_PATH, buff, &pFilePart) == 0) {
		return false;
	}

	CString fullpath = CString(buff).Left(int(pFilePart - buff));
	fullpath.TrimRight(L"\\/");
	if (fullpath.IsEmpty()) {
		return false;
	}

	CString str, str2;
	str += L"st_format\t2\n";
	str += L"Display_Start\t%s\n";
	str += L"TV_Type\t\t%s\n";
	str += L"Tape_Type\tNON_DROP\n";
	str += L"Pixel_Area\t(0 %d)\n";
	str += L"Directory\t%s\n";
	str += L"Subtitle\t%s\n";
	str += L"Display_Area\t(0 2 719 %d)\n";
	str += L"Contrast\t(15 15 15 0)\n";
	str += L"\n";
	str += L"PA\t(0 0 255 - - - )\n";
	str += L"E1\t(255 0 0 - - - )\n";
	str += L"E2\t(0 0 0 - - - )\n";
	str += L"BG\t(255 255 255 - - - )\n";
	str += L"\n";
	str += L"SP_NUMBER\tSTART\tEND\tFILE_NAME\n";
	str2.Format(str,
				!m_bOnlyShowForcedSubs ? L"non_forced" : L"forced",
				m_size.cy == 480 ? L"NTSC" : L"PAL",
				m_size.cy-3,
				fullpath,
				title,
				m_size.cy == 480 ? 479 : 574);

	f.WriteString(str2);

	f.Flush();

	RGBQUAD pal[16] = {
		{255, 0, 0, 0},
		{0, 0, 255, 0},
		{0, 0, 0, 0},
		{255, 255, 255, 0},
		{0, 255, 0, 0},
		{255, 0, 255, 0},
		{0, 255, 255, 0},
		{125, 125, 0, 0},
		{125, 125, 125, 0},
		{225, 225, 225, 0},
		{0, 0, 125, 0},
		{0, 125, 0, 0},
		{125, 0, 0, 0},
		{255, 0, 222, 0},
		{0, 125, 222, 0},
		{125, 0, 125, 0},
	};

	BITMAPFILEHEADER fhdr = {
		0x4d42,
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD) + 360*(m_size.cy-2),
		0, 0,
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD)
	};

	BITMAPINFOHEADER ihdr = {
		sizeof(BITMAPINFOHEADER),
		720, m_size.cy-2, 1, 4, 0,
		360*(m_size.cy-2),
		0, 0,
		16, 4
	};

	bool bCustomPal = m_bCustomPal;
	m_bCustomPal = true;
	RGBQUAD tempCusPal[4], newCusPal[4+12] = {{255, 0, 0, 0}, {0, 0, 255, 0}, {0, 0, 0, 0}, {255, 255, 255, 0}};
	memcpy(tempCusPal, m_cuspal, sizeof(tempCusPal));
	memcpy(m_cuspal, newCusPal, sizeof(m_cuspal));

	std::unique_ptr<BYTE[]> p4bpp(new(std::nothrow) BYTE[(m_size.cy - 2) * 360]);
	if (!p4bpp) {
		return false;
	}

	BYTE colormap[16];

	for (size_t i = 0; i < 16; i++) {
		int idx = 0, maxdif = 255*255*3+1;

		for (size_t j = 0; j < 16 && maxdif; j++) {
			int rdif = pal[j].rgbRed - m_orgpal[i].rgbRed;
			int gdif = pal[j].rgbGreen - m_orgpal[i].rgbGreen;
			int bdif = pal[j].rgbBlue - m_orgpal[i].rgbBlue;

			int dif = rdif*rdif + gdif*gdif + bdif*bdif;
			if (dif < maxdif) {
				maxdif = dif;
				idx = (int)j;
			}
		}

		colormap[i] = idx+1;
	}

	int pc[4] = {1, 1, 1, 1}, pa[4] = {15, 15, 15, 0};

	std::vector<SubPos>& sp = m_langs[m_nLang].subpos;
	for (size_t i = 0, k = 0; i < sp.size(); i++) {
		if (!GetFrame((int)i)) {
			continue;
		}

		for (int j = 0; j < 5; j++) {
			if (j == 4 || !m_img.pal[j].tr) {
				j &= 3;
				memset(p4bpp.get(), (j<<4)|j, (m_size.cy-2)*360);
				break;
			}
		}

		for (LONG y = std::max(m_img.rect.top + 1, 2L); y < m_img.rect.bottom - 1; y++) {
			ASSERT(m_size.cy-y-1 >= 0);
			if (m_size.cy-y-1 < 0) {
				break;
			}

			DWORD* p = (DWORD*)&m_img.lpPixels[(y-m_img.rect.top)*m_img.rect.Width()+1];

			for (LONG x = m_img.rect.left+1; x < m_img.rect.right-1; x++, p++) {
				DWORD rgb = *p&0xffffff;
				BYTE c = rgb == 0x0000ff ? 0 : rgb == 0xff0000 ? 1 : rgb == 0x000000 ? 2 : 3;
				BYTE& c4bpp = p4bpp[(m_size.cy-y-1)*360+(x>>1)];
				c4bpp = (x&1) ? ((c4bpp&0xf0)|c) : ((c4bpp&0x0f)|(c<<4));
			}
		}

		CString bmpfn;
		bmpfn.Format(L"%s_%04u.bmp", fn, i+1);
		title = bmpfn.Mid(bmpfn.ReverseFind('/')+1);

		// E1, E2, P, Bg
		int c[4] = {colormap[m_img.pal[1].pal], colormap[m_img.pal[2].pal], colormap[m_img.pal[0].pal], colormap[m_img.pal[3].pal]};
		c[0]^=c[1], c[1]^=c[0], c[0]^=c[1];

		if (memcmp(pc, c, sizeof(c))) {
			memcpy(pc, c, sizeof(c));
			str.Format(L"Color\t (%d %d %d %d)\n", c[0], c[1], c[2], c[3]);
			f.WriteString(str);
		}

		// E1, E2, P, Bg
		int a[4] = {m_img.pal[1].tr, m_img.pal[2].tr, m_img.pal[0].tr, m_img.pal[3].tr};
		a[0]^=a[1], a[1]^=a[0], a[0]^=a[1];

		if (memcmp(pa, a, sizeof(a))) {
			memcpy(pa, a, sizeof(a));
			str.Format(L"Contrast (%d %d %d %d)\n", a[0], a[1], a[2], a[3]);
			f.WriteString(str);
		}

		int t1 = (int)sp[i].start;
		int h1 = t1/1000/60/60, m1 = (t1/1000/60)%60, s1 = (t1/1000)%60;
		int f1 = (int)((m_size.cy==480?29.97:25)*(t1%1000)/1000);

		int t2 = (int)sp[i].stop;
		int h2 = t2/1000/60/60, m2 = (t2/1000/60)%60, s2 = (t2/1000)%60;
		int f2 = (int)((m_size.cy==480?29.97:25)*(t2%1000)/1000);

		if (t2 <= 0) {
			continue;
		}
		if (t1 < 0) {
			t1 = 0;
		}

		if (h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2) {
			f2++;
			if (f2 == (m_size.cy == 480 ? 30 : 25)) {
				f2 = 0;
				s2++;
				if (s2 == 60) {
					s2 = 0;
					m2++;
					if (m2 == 60) {
						m2 = 0;
						h2++;
					}
				}
			}
		}

		if (i+1 < sp.size()) {
			int t3 = (int)sp[i+1].start;
			int h3 = t3/1000/60/60, m3 = (t3/1000/60)%60, s3 = (t3/1000)%60;
			int f3 = (int)((m_size.cy==480?29.97:25)*(t3%1000)/1000);

			if (h3 == h2 && m3 == m2 && s3 == s2 && f3 == f2) {
				f2--;
				if (f2 == -1) {
					f2 = (m_size.cy == 480 ? 29 : 24);
					s2--;
					if (s2 == -1) {
						s2 = 59;
						m2--;
						if (m2 == -1) {
							m2 = 59;
							if (h2 > 0) {
								h2--;
							}
						}
					}
				}
			}
		}

		if (h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2) {
			continue;
		}

		str.Format(L"%04u\t%02d:%02d:%02d:%02d\t%02d:%02d:%02d:%02d\t%s\n",
				   ++k,
				   h1, m1, s1, f1,
				   h2, m2, s2, f2,
				   title);
		f.WriteString(str);

		CFile bmp;
		if (bmp.Open(bmpfn, CFile::modeCreate|CFile::modeWrite|CFile::modeRead|CFile::typeBinary)) {
			bmp.Write(&fhdr, sizeof(fhdr));
			bmp.Write(&ihdr, sizeof(ihdr));
			bmp.Write(newCusPal, sizeof(RGBQUAD)*16);
			bmp.Write(p4bpp.get(), 360*(m_size.cy-2));
			bmp.Close();

			CompressFile(bmpfn);
		}
	}

	m_bCustomPal = bCustomPal;
	memcpy(m_cuspal, tempCusPal, sizeof(m_cuspal));

	return true;
}

bool CVobSubFile::SaveMaestro(CString fn)
{
	TrimExtension(fn);

	CStdioFile f;
	if (!f.Open(fn + L".son", CFile::modeCreate|CFile::modeWrite|CFile::typeText|CFile::shareDenyWrite)) {
		return false;
	}

	m_img.Invalidate();

	fn.Replace('\\', '/');
	CString title = fn.Mid(fn.ReverseFind('/')+1);

	WCHAR buff[MAX_PATH], * pFilePart = buff;
	if (GetFullPathNameW(fn, MAX_PATH, buff, &pFilePart) == 0) {
		return false;
	}

	CString fullpath = CString(buff).Left(int(pFilePart - buff));
	fullpath.TrimRight(L"\\/");
	if (fullpath.IsEmpty()) {
		return false;
	}

	CString str, str2;
	str += L"st_format\t2\n";
	str += L"Display_Start\t%s\n";
	str += L"TV_Type\t\t%s\n";
	str += L"Tape_Type\tNON_DROP\n";
	str += L"Pixel_Area\t(0 %d)\n";
	str += L"Directory\t%s\n";
	str += L"Subtitle\t%s\n";
	str += L"Display_Area\t(0 2 719 %d)\n";
	str += L"Contrast\t(15 15 15 0)\n";
	str += L"\n";
	str += L"SP_NUMBER\tSTART\tEND\tFILE_NAME\n";
	str2.Format(str,
				!m_bOnlyShowForcedSubs ? L"non_forced" : L"forced",
				m_size.cy == 480 ? L"NTSC" : L"PAL",
				m_size.cy-3,
				fullpath,
				title,
				m_size.cy == 480 ? 479 : 574);

	f.WriteString(str2);

	f.Flush();

	BITMAPFILEHEADER fhdr = {
		0x4d42,
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD) + 360*(m_size.cy-2),
		0, 0,
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD)
	};

	BITMAPINFOHEADER ihdr = {
		sizeof(BITMAPINFOHEADER),
		720, m_size.cy-2, 1, 4, 0,
		360*(m_size.cy-2),
		0, 0,
		16, 4
	};

	bool bCustomPal = m_bCustomPal;
	m_bCustomPal = true;
	RGBQUAD tempCusPal[4], newCusPal[4+12] = {{255, 0, 0, 0}, {0, 0, 255, 0}, {0, 0, 0, 0}, {255, 255, 255, 0}};
	memcpy(tempCusPal, m_cuspal, sizeof(tempCusPal));
	memcpy(m_cuspal, newCusPal, sizeof(m_cuspal));

	std::unique_ptr<BYTE[]> p4bpp(new(std::nothrow) BYTE[(m_size.cy - 2) * 360]);
	if (!p4bpp) {
		return false;
	}

	BYTE colormap[16];
	for (BYTE i = 0; i < 16; i++) {
		colormap[i] = i;
	}

	CFile spf;
	if (spf.Open(fn + L".spf", CFile::modeCreate|CFile::modeWrite|CFile::typeBinary)) {
		for (size_t i = 0; i < 16; i++) {
			COLORREF c = (m_orgpal[i].rgbBlue<<16) | (m_orgpal[i].rgbGreen<<8) | m_orgpal[i].rgbRed;
			spf.Write(&c, sizeof(COLORREF));
		}

		spf.Close();
	}

	int pc[4] = {1,1,1,1}, pa[4] = {15,15,15,0};

	std::vector<SubPos>& sp = m_langs[m_nLang].subpos;
	for (size_t i = 0, k = 0; i < sp.size(); i++) {
		if (!GetFrame((int)i)) {
			continue;
		}

		for (int j = 0; j < 5; j++) {
			if (j == 4 || !m_img.pal[j].tr) {
				j &= 3;
				memset(p4bpp.get(), (j<<4)|j, (m_size.cy-2)*360);
				break;
			}
		}

		for (LONG y = std::max(m_img.rect.top+1, 2L); y < m_img.rect.bottom-1; y++) {
			ASSERT(m_size.cy-y-1 >= 0);
			if (m_size.cy-y-1 < 0) {
				break;
			}

			DWORD* p = (DWORD*)&m_img.lpPixels[(y-m_img.rect.top)*m_img.rect.Width()+1];

			for (LONG x = m_img.rect.left+1; x < m_img.rect.right-1; x++, p++) {
				DWORD rgb = *p&0xffffff;
				BYTE c = rgb == 0x0000ff ? 0 : rgb == 0xff0000 ? 1 : rgb == 0x000000 ? 2 : 3;
				BYTE& c4bpp = p4bpp[(m_size.cy-y-1)*360+(x>>1)];
				c4bpp = (x&1) ? ((c4bpp&0xf0)|c) : ((c4bpp&0x0f)|(c<<4));
			}
		}

		CString bmpfn;
		bmpfn.Format(L"%s_%04d.bmp", fn, i+1);
		title = bmpfn.Mid(bmpfn.ReverseFind('/')+1);

		// E1, E2, P, Bg
		int c[4] = {colormap[m_img.pal[1].pal], colormap[m_img.pal[2].pal], colormap[m_img.pal[0].pal], colormap[m_img.pal[3].pal]};

		if (memcmp(pc, c, sizeof(c))) {
			memcpy(pc, c, sizeof(c));
			str.Format(L"Color\t (%d %d %d %d)\n", c[0], c[1], c[2], c[3]);
			f.WriteString(str);
		}

		// E1, E2, P, Bg
		int a[4] = {m_img.pal[1].tr, m_img.pal[2].tr, m_img.pal[0].tr, m_img.pal[3].tr};

		if (memcmp(pa, a, sizeof(a))) {
			memcpy(pa, a, sizeof(a));
			str.Format(L"Contrast (%d %d %d %d)\n", a[0], a[1], a[2], a[3]);
			f.WriteString(str);
		}

		int t1 = (int)sp[i].start;
		int h1 = t1/1000/60/60, m1 = (t1/1000/60)%60, s1 = (t1/1000)%60;
		int f1 = (int)((m_size.cy==480?29.97:25)*(t1%1000)/1000);

		int t2 = (int)sp[i].stop;
		int h2 = t2/1000/60/60, m2 = (t2/1000/60)%60, s2 = (t2/1000)%60;
		int f2 = (int)((m_size.cy==480?29.97:25)*(t2%1000)/1000);

		if (t2 <= 0) {
			continue;
		}
		if (t1 < 0) {
			t1 = 0;
		}

		if (h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2) {
			f2++;
			if (f2 == (m_size.cy == 480 ? 30 : 25)) {
				f2 = 0;
				s2++;
				if (s2 == 60) {
					s2 = 0;
					m2++;
					if (m2 == 60) {
						m2 = 0;
						h2++;
					}
				}
			}
		}

		if (i < sp.size()-1) {
			int t3 = (int)sp[i+1].start;
			int h3 = t3/1000/60/60, m3 = (t3/1000/60)%60, s3 = (t3/1000)%60;
			int f3 = (int)((m_size.cy==480?29.97:25)*(t3%1000)/1000);

			if (h3 == h2 && m3 == m2 && s3 == s2 && f3 == f2) {
				f2--;
				if (f2 == -1) {
					f2 = (m_size.cy == 480 ? 29 : 24);
					s2--;
					if (s2 == -1) {
						s2 = 59;
						m2--;
						if (m2 == -1) {
							m2 = 59;
							if (h2 > 0) {
								h2--;
							}
						}
					}
				}
			}
		}

		if (h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2) {
			continue;
		}

		str.Format(L"%04u\t%02d:%02d:%02d:%02d\t%02d:%02d:%02d:%02d\t%s\n",
				   ++k,
				   h1, m1, s1, f1,
				   h2, m2, s2, f2,
				   title);
		f.WriteString(str);

		CFile bmp;
		if (bmp.Open(bmpfn, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary)) {
			bmp.Write(&fhdr, sizeof(fhdr));
			bmp.Write(&ihdr, sizeof(ihdr));
			bmp.Write(newCusPal, sizeof(RGBQUAD)*16);
			bmp.Write(p4bpp.get(), 360*(m_size.cy-2));
			bmp.Close();

			CompressFile(bmpfn);
		}
	}

	m_bCustomPal = bCustomPal;
	memcpy(m_cuspal, tempCusPal, sizeof(m_cuspal));

	return true;
}

//
// CVobSubStream
//

CVobSubStream::CVobSubStream(CCritSec* pLock)
	: CSubPicProviderImpl(pLock)
{
}

CVobSubStream::~CVobSubStream()
{
}

void CVobSubStream::Open(CString name, BYTE* pData, int len)
{
	CAutoLock cAutoLock(&m_csSubPics);

	m_name = name;

	CAtlList<CString> lines;
	Explode(CString(CStringA((CHAR*)pData, len)), lines, L'\n');
	while (lines.GetCount()) {
		CAtlList<CString> sl;
		Explode(lines.RemoveHead(), sl, L':', 2);
		if (sl.GetCount() != 2) {
			continue;
		}
		CString key = sl.GetHead();
		CString value = sl.GetTail();
		if (key == L"size") {
			swscanf_s(value, L"%dx %d", &m_size.cx, &m_size.cy);
		} else if (key == L"org") {
			swscanf_s(value, L"%d, %d", &m_org.x, &m_org.y);
		} else if (key == L"scale") {
			swscanf_s(value, L"%d%%, %d%%", &m_scale_x, &m_scale_y);
		} else if (key == L"alpha") {
			swscanf_s(value, L"%d%%", &m_alpha);
		} else if (key == L"smooth")
			m_iSmooth =
				value == L"0" || value == L"OFF" ? 0 :
				value == L"1" || value == L"ON" ? 1 :
				value == L"2" || value == L"OLD" ? 2 :
				0;
		else if (key == L"align") {
			Explode(value, sl, L' ');
			if (sl.GetCount() == 4) {
				sl.RemoveAt(sl.FindIndex(1));
			}
			if (sl.GetCount() == 3) {
				m_bAlign = sl.RemoveHead() == L"ON";
				CString hor = sl.GetHead(), ver = sl.GetTail();
				m_alignhor = hor == L"LEFT" ? 0 : hor == L"RIGHT"  ? 2 : 1;
				m_alignver = ver == L"TOP"  ? 0 : ver == L"BOTTOM" ? 2 : 1;
			}
		} else if (key == L"fade in/out") {
			swscanf_s(value, L"%d%%, %d%%", &m_fadein, &m_fadeout);
		} else if (key == L"fadein/out") {
			swscanf_s(value, L"%d, %d", &m_fadein, &m_fadeout);
		} else if (key == L"time offset") {
			m_toff = wcstol(value, nullptr, 10);
		} else if (key == L"forced subs") {
			m_bOnlyShowForcedSubs = value == L"1" || value == L"ON";
		} else if (key == L"palette") {
			Explode(value, sl, L',', 16);
			for (size_t i = 0; i < 16 && sl.GetCount(); i++) {
				GETU32(&m_orgpal[i]) = wcstol(sl.RemoveHead(), nullptr, 16);
			}
		} else if (key == L"custom colors") {
			m_bCustomPal = Explode(value, sl, L',', 3) == L"ON";
			if (sl.GetCount() == 3) {
				sl.RemoveHead();
				CAtlList<CString> tridx, colors;
				Explode(sl.RemoveHead(), tridx, L':', 2);

				int _tridx = 1;
				if (tridx.RemoveHead() == L"tridx") {
					WCHAR tr[4];
					swscanf_s(tridx.RemoveHead(), L"%c%c%c%c",
						&tr[0], 1, &tr[1], 1, &tr[2], 1, &tr[3], 1);
					for (size_t i = 0; i < 4; i++) {
						_tridx |= ((tr[i]=='1')?1:0)<<i;
					}
				}
				Explode(sl.RemoveHead(), colors, L':', 2);
				if (colors.RemoveHead() == L"colors") {
					Explode(colors.RemoveHead(), colors, L',', 4);

					RGBQUAD pal[4];
					for (size_t i = 0; i < 4 && colors.GetCount(); i++) {
						GETU32(&pal[i]) = wcstol(colors.RemoveHead(), nullptr, 16);
					}

					SetCustomPal(pal, _tridx);
				}
			}
		}
	}
}

void CVobSubStream::Add(REFERENCE_TIME tStart, REFERENCE_TIME tStop, BYTE* pData, int len)
{
	if (len <= 4 || ((pData[0] << 8) | pData[1]) != len) {
		return;
	}

	{
		CAutoLock cAutoLock(&m_csSubPics);
		POSITION pos = m_subpics.GetTailPosition();
		while (pos) {
			SubPic* sp = m_subpics.GetPrev(pos);
			if (sp->tStop == _I64_MAX) {
				sp->tStop = tStart;
				break;
			}
		}
	}

	CVobSubImage vsi;
	vsi.GetPacketInfo(pData, (pData[0] << 8) | pData[1], (pData[2] << 8) | pData[3]);

	CAutoPtr<SubPic> p(DNew SubPic());
	p->tStart		= tStart;
	p->tStop		= vsi.delay > 0 ? (tStart + 10000i64 * vsi.delay) : tStop;
	p->bAnimated	= vsi.bAnimated;
	p->bForced		= vsi.bForced;
	if (p->tStop - p->tStart < UNITS/100) {
		p->tStop	= _I64_MAX;
	}
	p->pData.SetCount(len);
	memcpy(p->pData.GetData(), pData, p->pData.GetCount());

	CAutoLock cAutoLock(&m_csSubPics);
	while (m_subpics.GetCount() && m_subpics.GetTail()->tStart >= tStart) {
		m_subpics.RemoveTailNoReturn();
		m_img.nIdx = -1;
	}

	// We can only render one subpicture at a time, thus if there is overlap
	// we have to fix it. tStop = tStart seems to work.
	if (m_subpics.GetCount() && m_subpics.GetTail()->tStop > p->tStart) {
		DLog(L"[CVobSubStream::Add] Vobsub timestamp overlap detected!"
			 L"Subpicture #%d, StopTime %I64d > %I64d (Next StartTime), making them equal!",
			 m_subpics.GetCount(), m_subpics.GetTail()->tStop, p->tStart);
		m_subpics.GetTail()->tStop = p->tStart;
	}

	m_subpics.AddTail(p);
}

void CVobSubStream::RemoveAll()
{
	CAutoLock cAutoLock(&m_csSubPics);
	m_subpics.RemoveAll();
	m_img.nIdx = -1;
}

STDMETHODIMP CVobSubStream::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = nullptr;

	return
		QI(IPersist)
		QI(ISubStream)
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(POSITION) CVobSubStream::GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld/* = false*/)
{
	CAutoLock cAutoLock(&m_csSubPics);
	POSITION pos = m_subpics.GetTailPosition();
	for (; pos; m_subpics.GetPrev(pos)) {
		SubPic* sp = m_subpics.GetAt(pos);
		if (sp->tStart <= rt) {
			if (g_bForcedSubtitle && !sp->bForced) {
				continue;
			}
			if (sp->tStop <= rt) {
				m_subpics.GetNext(pos);
			}
			break;
		}
	}
	return pos;
}

STDMETHODIMP_(POSITION) CVobSubStream::GetNext(POSITION pos)
{
	CAutoLock cAutoLock(&m_csSubPics);
	m_subpics.GetNext(pos);
	return pos;
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubStream::GetStart(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csSubPics);
	return m_subpics.GetAt(pos)->tStart;
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubStream::GetStop(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csSubPics);
	return m_subpics.GetAt(pos)->tStop;
}

STDMETHODIMP_(bool) CVobSubStream::IsAnimated(POSITION pos)
{
	CAutoLock cAutoLock(&m_csSubPics);
	return m_subpics.GetAt(pos)->bAnimated;
}

STDMETHODIMP CVobSubStream::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	if (spd.bpp != 32) {
		return E_INVALIDARG;
	}

	POSITION pos = m_subpics.GetTailPosition();
	for (; pos; m_subpics.GetPrev(pos)) {
		SubPic* sp = m_subpics.GetAt(pos);
		if (sp->tStart <= rt && rt < sp->tStop) {
			if (m_img.nIdx != (size_t)pos || (sp->bAnimated && sp->tStart + m_img.tCurrent * 10000i64 <= rt)) {
				BYTE* pData = sp->pData.GetData();
				m_img.Decode(
					pData, (pData[0] << 8) | pData[1], (pData[2] << 8) | pData[3], int((rt - sp->tStart) / 10000i64),
					m_bCustomPal, m_tridx, m_orgpal, m_cuspal, true);
				m_img.nIdx = (size_t)pos;
			}

			return __super::Render(spd, bbox);
		}
	}

	return E_FAIL;
}

// IPersist

STDMETHODIMP CVobSubStream::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CVobSubStream::GetStreamCount()
{
	return 1;
}

STDMETHODIMP CVobSubStream::GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID)
{
	CAutoLock cAutoLock(&m_csSubPics);

	if (ppName) {
		*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength() + 1) * sizeof(WCHAR));
		if (!(*ppName)) {
			return E_OUTOFMEMORY;
		}
		wcscpy_s(*ppName, m_name.GetLength() + 1, CStringW(m_name));
	}

	if (pLCID) {
		*pLCID = 0; // TODO
	}

	return S_OK;
}

STDMETHODIMP_(int) CVobSubStream::GetStream()
{
	return 0;
}

STDMETHODIMP CVobSubStream::SetStream(int iStream)
{
	return iStream == 0 ? S_OK : E_FAIL;
}
