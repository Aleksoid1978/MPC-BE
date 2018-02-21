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

#pragma once

#include <map>
#include "../BaseSplitter/BaseSplitter.h"
#include "../../../DSUtil/GolombBuffer.h"

#define NO_SUBTITLE_PID  WORD_MAX // Fake PID use for the "No subtitle" entry
#define NO_SUBTITLE_NAME L"No subtitle"

#define ISVALIDPID(pid)  (pid >= 0x1 && pid < 0x1fff)

enum MPEG_TYPES {
	mpeg_invalid,
	mpeg_ps,
	mpeg_ts,
	mpeg_pva
};

class CMpegSplitterFile : public CBaseSplitterFileEx
{
	std::map<WORD, BYTE> m_pid2pes;

	std::map<DWORD, seqhdr> seqh;
	std::map<DWORD, std::vector<BYTE>> avch;
	std::map<DWORD, std::vector<BYTE>> hevch;

	template<class T, int validCount = 5>
	class CValidStream {
		BYTE m_nValidStream;
		T m_val;
	public:
		CValidStream() {
			m_nValidStream = 0;
			memset(&m_val, 0, sizeof(m_val));
		}
		void Handle(T& val) {
			if (m_val == val) {
				m_nValidStream++;
			} else {
				m_nValidStream = 0;
			}
			memcpy(&m_val, &val, sizeof(val));
		}
		BOOL IsValid() const { return m_nValidStream >= validCount; }
	};

	std::map<DWORD, CValidStream<latm_aachdr, 3>> m_aaclatmValid;
	std::map<DWORD, CValidStream<aachdr>>         m_aacValid;
	std::map<DWORD, CValidStream<ac3hdr>>         m_ac3Valid;
	std::map<DWORD, CValidStream<mpahdr>>         m_mpaValid;

	BOOL m_bOpeningCompleted;

	HRESULT Init(IAsyncReader* pAsyncReader);

	BOOL m_bIMKH_CCTV;

	typedef std::vector<SyncPoint> SyncPoints;
	std::map<DWORD, SyncPoints> m_SyncPoints;

	int m_tslen = 0; // transport stream packet length (188 or 192 bytes, auto-detected)

public:
	REFERENCE_TIME m_rtPTSOffset = 0;

	struct pshdr {
		mpeg_t type;
		UINT64 scr, bitrate;
	};

	struct pssyshdr {
		DWORD rate_bound;
		BYTE video_bound, audio_bound;
		bool fixed_rate, csps;
		bool sys_video_loc_flag, sys_audio_loc_flag;
	};

	struct peshdr {
		WORD len;

		BYTE type:2, fpts:1, fdts:1;
		REFERENCE_TIME pts, dts;

		// mpeg1 stuff
		UINT64 std_buff_size;

		// mpeg2 stuff
		BYTE scrambling:2, priority:1, alignment:1, copyright:1, original:1;
		BYTE escr:1, esrate:1, dsmtrickmode:1, morecopyright:1, crc:1, extension:1;
		BYTE hdrlen;

		BYTE id_ext;

		struct peshdr() {
			memset(this, 0, sizeof(*this));
		}
	};

	// http://multimedia.cx/mirror/av_format_v1.pdf
	struct pvahdr
	{
		WORD sync; // 'VA'
		BYTE streamid; // 1 - video, 2 - audio
		BYTE counter;
		BYTE res1; // 0x55
		BYTE res2:3;
		BYTE fpts:1;
		BYTE postbytes:2;
		BYTE prebytes:2;
		WORD length;
		REFERENCE_TIME pts;

		__int64 hdrpos;
	};

	struct trhdr
	{
		__int64 PCR;
		__int64 next;
		int bytes;
		WORD pid:13;
		BYTE sync; // 0x47
		BYTE error:1;
		BYTE payloadstart:1;
		BYTE transportpriority:1;
		BYTE scrambling:2;
		BYTE adapfield:1;
		BYTE payload:1;
		BYTE counter:4;
		// if adapfield set
		BYTE length;
		BYTE discontinuity:1;
		BYTE randomaccess:1;
		BYTE priority:1;
		BYTE fPCR:1;
		BYTE OPCR:1;
		BYTE splicingpoint:1;
		BYTE privatedata:1;
		BYTE extension:1;

		__int64 hdrpos;
		// TODO: add more fields here when the flags above are set (they aren't very interesting...)
	};

	struct psihdr
	{
		BYTE section_syntax_indicator:1;
		BYTE private_bits:1;
		BYTE reserved1:2;
		BYTE section_length_unused:2;
		WORD section_length:10;
		WORD transport_stream_id;
		BYTE table_id;
		BYTE reserved2:2;
		BYTE version_number:5;
		BYTE current_next_indicator:1;
		BYTE section_number;
		BYTE last_section_number;

		BYTE hdr_size;
	};

	bool ReadPS(pshdr& h);              // program stream header
	bool ReadPSS(pssyshdr& h);          // program stream system header

	bool ReadPES(peshdr& h, BYTE code); // packetized elementary stream

	bool ReadPVA(pvahdr& h, bool fSync = true);

	bool ReadTR(trhdr& h, bool fSync = true);
	bool ReadPSI(psihdr& h);

	enum stream_codec {
		NONE,
		H264,
		MVC,
		HEVC,
		MPEG,
		VC1,
		OPUS,
		DIRAC,
		AC3,
		EAC3,
		AAC_RAW,
		DTS,
		ARIB,
		PGS,
		DVB,
		TELETEXT,
		DVDAudio,
		MLP,
		DVDLPCM
	};

	enum stream_type {
		video,
		audio,
		subpic,
		stereo,
		unknown
	};

	bool m_bIsBD;
	CHdmvClipInfo &m_ClipInfo;
	CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, CHdmvClipInfo &ClipInfo, bool bIsBD, bool ForcedSub, int AC3CoreOnly, bool SubEmptyPin);

	BOOL CheckKeyFrame(std::vector<BYTE>& pData, const stream_codec& codec);
	REFERENCE_TIME NextPTS(const DWORD& TrackNum, const stream_codec& codec, __int64& nextPos, const BOOL& bKeyFrameOnly = FALSE, const REFERENCE_TIME& rtLimit = _I64_MAX);

	MPEG_TYPES m_type;

	BOOL m_bPESPTSPresent;

	__int64 m_rate; // byte/sec

	int m_AC3CoreOnly;
	bool m_ForcedSub, m_SubEmptyPin;

	REFERENCE_TIME m_rtMin;
	REFERENCE_TIME m_rtMax;

	__int64 m_posMin;

	struct stream {
		CMediaType mt;
		std::vector<CMediaType> mts;

		WORD pid   = 0;
		BYTE pesid = 0;
		BYTE ps1id = 0;

		WORD tlxPage = 0;

		bool lang_set = false;
		char lang[4]  = {};

		struct {
			bool bDTSCore = false;
			bool bDTSHD   = false;
		} dts;

		stream_codec codec = stream_codec::NONE;

		operator DWORD() const {
			return pid ? pid : ((pesid << 8) | ps1id);
		}

		bool operator == (const stream& s) const {
			return (DWORD)*this == (DWORD)s;
		}

		bool operator < (const stream& s) const {
			return (DWORD)*this < (DWORD)s;
		}
	};

	class CStreamList : public std::list<stream> {
	public:
		void Insert(const stream& s) {
			emplace_back(s);
			sort();
		}

		const stream* GetStream(DWORD pid) {
			const auto it = std::find(cbegin(), cend(), pid);
			return (it != cend()) ? &(*it) : nullptr;
		}

		const bool Find(const stream& s) {
			return (std::find(cbegin(), cend(), s) != cend());
		}

		const bool Find(const WORD& pid) {
			return (std::find(cbegin(), cend(), pid) != cend());
		}

		static const CString ToString(const int& type) {
			return
				type == video  ? L"Video" :
				type == audio  ? L"Audio" :
				type == subpic ? L"Subtitle" :
				type == stereo ? L"Stereo" :
								 L"Unknown";
		}
	};

	typedef CStreamList CStreamsList[unknown];
	CStreamsList m_streams;

	void SearchStreams(__int64 start, __int64 stop, DWORD msTimeOut = INFINITE);
	DWORD AddStream(WORD pid, BYTE pesid, BYTE ext_id, DWORD len, BOOL bAddStream = TRUE);
	void  AddHdmvPGStream(WORD pid, LPCSTR language_code);
	CMpegSplitterFile::CStreamList* GetMasterStream();
	bool IsHdmv() {
		return m_ClipInfo.IsHdmv();
	};

	// program map table - mpeg-ts
	struct program {
		WORD program_number      = 0;
		struct stream {
			WORD			pid  = 0;
			PES_STREAM_TYPE	type = PES_STREAM_TYPE::INVALID;
		};
		std::vector<stream> streams;

		CString name;

		size_t streamCount(CStreamsList s) {
			size_t cnt = 0;
			for (const auto& stream : streams) {
				for (int type = stream_type::video; type <= stream_type::subpic; type++) {
					if (s[type].Find(stream.pid)) {
						cnt++;
						break;
					}
				}
			}

			return cnt;
		}

		bool streamFind(WORD pid, int* pStream = nullptr) {
			for (size_t i = 0; i < streams.size(); i++) {
				if (streams[i].pid == pid) {
					if (pStream) {
						*pStream = i;
					}
					return true;
				}
			}

			return false;
		}
	};

	class CPrograms : public std::map<WORD, program>
	{
		CStreamsList* s;
	public:
		CPrograms(CStreamsList* sList) {
			s = sList;
		}
		size_t GetValidCount() {
			size_t cnt = 0;
			for (auto item : (*this)) {
				if (item.second.streamCount(*s)) {
					cnt++;
				}
			}

			return cnt;
		}

		program* FindProgram(const WORD program_number) {
			for (auto& item : (*this)) {
				auto p = &item.second;
				if (p->program_number == program_number) {
					return p;
				}
			}

			return nullptr;
		}
	} m_programs;

	struct programData {
		BYTE table_id      = 0;
		int section_length = 0;
		bool bFinished     = false;
		std::vector<BYTE> pData;

		void Finish() {
			table_id       = 0;
			section_length = 0;
			bFinished      = true;
			pData.clear();
		}

		bool IsFull() { return !pData.empty() && pData.size() == section_length; }
	};
	std::map<WORD, programData> m_ProgramData;

	void SearchPrograms(__int64 start, __int64 stop);
	void ReadPrograms(const trhdr& h);
	void ReadPAT(std::vector<BYTE>& pData);
	void ReadPMT(std::vector<BYTE>& pData, WORD pid);
	void ReadSDT(std::vector<BYTE>& pData);
	void ReadVCT(std::vector<BYTE>& pData, BYTE table_id);

	const program* FindProgram(WORD pid, int* pStream = nullptr, const CHdmvClipInfo::Stream** pClipInfo = nullptr);

	// program stream map - mpeg-ps
	PES_STREAM_TYPE m_psm[256] = {};
	void UpdatePSM();

	bool GetStreamType(WORD pid, PES_STREAM_TYPE &stream_type);

	struct teletextPage {
		bool bSubtitle = false;
		WORD page      = 0;
		char lang[4]   = {};

		bool operator == (const teletextPage& tlxPage) const {
			return (page == tlxPage.page
				&& bSubtitle == tlxPage.bSubtitle
				&& !strcmp(lang, tlxPage.lang));
		}
	};
	typedef std::vector<teletextPage> teletextPages;

	struct streamData {
		struct {
			char              lang[4] = {};
			std::vector<BYTE> extraData;
			teletextPages     tlxPages;
		} pmt;

		BOOL         usePTS = FALSE;
		stream_codec codec  = stream_codec::NONE;

		REFERENCE_TIME rtStreamStart = 0;
	};
	std::map<DWORD, streamData> m_streamData;

	std::vector<program::stream> m_pmt_streams;
};
