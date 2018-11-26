/*
 * (C) 2018 see Authors.txt
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
#include <initguid.h>
#include <MMReg.h>
#include "DVRSplitter.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CDVRSplitterFilter), DVRSplitterName, MERIT_NORMAL + 1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CDVRSourceFilter), DVRSourceName, MERIT_NORMAL + 1, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDVRSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CDVRSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	const std::list<CString> chkbytes = {
		L"0,4,,48585653,16,4,,48585646", // 'HXVS............HXVF'
		L"0,4,,44484156",                // 'DHAV'
	};

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_NULL);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CDVRSplitterFilter
//

#define HXVS_HEADER_INFO FCC('HXVS')
#define HXVS_FOOTER_INFO FCC('HXFI')
#define HXVS_VIDEO       FCC('HXVF')
#define HXVS_AUDIO       FCC('HXAF')

#define HXVS_HeaderSize     16
#define HXVS_FooterSize 200016

#define DHAV_SYNC_START FCC('DHAV')
#define DHAV_SYNC_END   FCC('dhav')
#define DHAV_VIDEO(hdr) (hdr.type == 0xfc || hdr.type == 0xfd)
#define DHAV_AUDIO(hdr) (hdr.type == 0xf0)

#define DHAV_HeaderSize 24
#define DHAV_FooterSize  8

CDVRSplitterFilter::CDVRSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CDVRSplitterFilter", pUnk, phr, __uuidof(this))
{
}

HRESULT CDVRSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;
	m_pFile.Free();

	m_pFile.Attach(DNew CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	BYTE buf[28] = {};
	if (FAILED(m_pFile->ByteRead(buf, sizeof(buf)))) {
		m_pFile.Free();
		return E_FAIL;
	}

	if (GETU32(buf) == HXVS_HEADER_INFO && GETU32(buf + 16) == HXVS_VIDEO) {
		m_rtOffsetVideo = GETU32(buf + 24) * 10000ll;
		m_bHXVS = true;
	} else if (GETU32(buf) == DHAV_SYNC_START
			&& (buf[4] == 0xf0 || buf[4] == 0xf1 || buf[4] == 0xfc || buf[4] == 0xfd)) {
		m_bDHAV = true;
	}

	if (!m_bHXVS && !m_bDHAV) {
		m_pFile.Free();
		return E_FAIL;
	}

	std::vector<CMediaType> mts;
	CMediaType mt;

	if (m_bHXVS) {
		m_startpos = HXVS_HeaderSize;
		m_endpos   = m_pFile->GetLength();
		m_pFile->Seek(m_startpos);

		HXVSHeader hdr;
		std::vector<BYTE> pData;
		while (HXVSReadHeader(hdr) && m_pFile->GetPos() <= MEGABYTE) {
			if (hdr.sync == HXVS_VIDEO) {
				if (hdr.rt != m_rtOffsetVideo) {
					m_AvgTimePerFrame = hdr.rt - m_rtOffsetVideo;
					if (!pData.empty()) {
						CBaseSplitterFileEx::avchdr h;
						if (m_pFile->Read(h, pData, &mt)) {
							mts.push_back(mt);
						}
					}
					break;
				} else {
					const size_t old_size = pData.size();
					pData.resize(old_size + hdr.size);
					if ((hr = m_pFile->ByteRead(pData.data() + old_size, hdr.size)) != S_OK) {
						break;
					}
				}
			} else {
				if (m_rtOffsetAudio == INVALID_TIME) {
					m_rtOffsetAudio = hdr.rt;
				}
				m_pFile->Skip(hdr.size);
			}
		};

		if (mts.empty()) {
			m_pFile.Free();
			return E_FAIL;
		}

		m_rtNewStart = m_rtCurrent = 0;
		m_rtNewStop = m_rtStop = m_rtDuration = 0;

		// footer
		if (m_pFile->GetLength() > (HXVS_FooterSize + HXVS_HeaderSize + HXVS_HeaderSize)) {
			m_pFile->Seek(m_pFile->GetLength() - HXVS_FooterSize);
			DWORD sync = 0;
			if (S_OK == m_pFile->ByteRead((BYTE*)&sync, sizeof(sync)) && sync == HXVS_FOOTER_INFO) {
				m_pFile->Skip(4);
				if (S_OK == m_pFile->ByteRead((BYTE*)&sync, sizeof(sync)) && sync) {
					m_rtNewStop = m_rtStop = m_rtDuration = sync * 10000ll;
				}
				m_endpos = m_pFile->GetLength() - HXVS_FooterSize;

				m_pFile->Skip(4);
				size_t cnt = 1;
				while (m_pFile->GetPos() < m_pFile->GetLength()) {
					struct {
						DWORD pos, pts;
					} keyframe;

					if (S_OK != m_pFile->ByteRead((BYTE*)&keyframe, sizeof(keyframe)) || keyframe.pos == 0) {
						break;
					}

					if ((cnt % 3) == 0) {
						const SyncPoint sp = {keyframe.pts * 10000ll, keyframe.pos};
						m_sps.emplace_back(sp);
					}

					cnt++;
				}
			}
		}

		// find audio stream
		if (m_rtOffsetAudio == INVALID_TIME) {
			m_pFile->Seek(16);
			while (HXVSReadHeader(hdr) && m_pFile->GetPos() <= MEGABYTE) {
				if (hdr.sync == HXVS_AUDIO) {
					m_rtOffsetAudio = hdr.rt;
					break;
				}
			}
		}

		// find end PTS
		if (m_rtDuration == 0) {
			m_pFile->Seek(m_endpos - std::min((__int64)MEGABYTE, m_endpos));

			REFERENCE_TIME rtLast = INVALID_TIME;
			while (HXVSReadHeader(hdr)) {
				if (hdr.sync == HXVS_VIDEO) {
					rtLast = hdr.rt;
				}
				m_pFile->Skip(hdr.size);
			};

			if (rtLast > m_rtOffsetVideo) {
				m_rtNewStop = m_rtStop = m_rtDuration = rtLast - m_rtOffsetVideo;
			}
		}

		for (auto& mt : mts) {
			auto vih2 = (VIDEOINFOHEADER2*)mt.Format();
			if (!vih2->AvgTimePerFrame) {
				vih2->AvgTimePerFrame = m_AvgTimePerFrame;
			}
		}

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Video", this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));

		if (m_rtOffsetAudio != INVALID_TIME) {
			mt.InitMediaType();

			CBaseSplitterFileEx::pcm_law_hdr h;
			m_pFile->Read(h, true, &mt);

			mts.clear();
			mts.push_back(mt);

			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio", this, this, &hr));
			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(1, pPinOut)));
		}
	} else {
		m_startpos = 0;
		m_endpos   = m_pFile->GetLength();
		m_pFile->Seek(m_startpos);

		bool bVideoFound = false;
		bool bAudioFound = false;

		time_t start_time = 0;
		time_t end_time   = 0;


		DHAVHeader hdr;
		std::vector<BYTE> pData;
		while (DHAVReadHeader(hdr, true) && m_pFile->GetPos() <= MEGABYTE) {
			if (hdr.type == 0xfd) {
				if (hdr.video.codec && !bVideoFound) {
					if (hdr.date) {
						tm time = {};
						time.tm_sec  =   hdr.date        & 0x3F;
						time.tm_min  =  (hdr.date >>  6) & 0x3F;
						time.tm_hour =  (hdr.date >> 12) & 0x1F;
						time.tm_mday =  (hdr.date >> 17) & 0x1F;
						time.tm_mon  =  (hdr.date >> 22) & 0x0F;
						time.tm_year = ((hdr.date >> 26) & 0x3F) + 2000 - 1900;

						start_time = mktime(&time);
					}

					m_AvgTimePerFrame = UNITS / (hdr.video.frame_rate ? hdr.video.frame_rate : 25);

					m_startpos = m_pFile->GetPos() - DHAV_HeaderSize - hdr.ext_size;
					bVideoFound = true;
				
					BITMAPINFOHEADER pbmi = {};
					pbmi.biSize      = sizeof(pbmi);
					pbmi.biWidth     = (LONG)hdr.video.width;
					pbmi.biHeight    = (LONG)hdr.video.height;
					pbmi.biPlanes    = 1;
					pbmi.biBitCount  = 24;
					pbmi.biSizeImage = DIBSIZE(pbmi);

					CSize aspect(pbmi.biWidth, pbmi.biHeight);
					ReduceDim(aspect);

					switch (hdr.video.codec) {
						case 0x1: pbmi.biCompression = FCC('MP4V'); break;
						case 0x3: pbmi.biCompression = FCC('MJPG'); break;
						case 0x2:
						case 0x4:
						case 0x8: pbmi.biCompression = FCC('H264'); break;
						case 0xc: pbmi.biCompression = FCC('HEVC'); break;
					}

					if (!pbmi.biCompression) {
						return false;
					}

					mt.InitMediaType();
					mt.SetTemporalCompression(TRUE);
					CreateMPEG2VISimple(&mt, &pbmi, m_AvgTimePerFrame, aspect, nullptr, 0u);

					mts.clear();
					mts.push_back(mt);

					if (pbmi.biCompression == FCC('H264')) {
						std::vector<BYTE> pData(hdr.size);
						if ((hr = m_pFile->ByteRead(pData.data(), hdr.size)) != S_OK) {
							return false;
						}

						CBaseSplitterFileEx::avchdr h;
						if (m_pFile->Read(h, pData, &mt)) {
							mts.insert(mts.begin(), mt);
						}

						m_pFile->Seek(m_pFile->GetPos() - hdr.size);
					} else if (pbmi.biCompression == FCC('HEVC')) {
						std::vector<BYTE> pData(hdr.size);
						if ((hr = m_pFile->ByteRead(pData.data(), hdr.size)) != S_OK) {
							return false;
						}

						CBaseSplitterFileEx::hevchdr h;
						if (m_pFile->Read(h, pData, &mt)) {
							mts.insert(mts.begin(), mt);
						}

						m_pFile->Seek(m_pFile->GetPos() - hdr.size);
					}

					for (auto& mt : mts) {
						auto vih2 = (VIDEOINFOHEADER2*)mt.Format();
						if (!vih2->AvgTimePerFrame) {
							vih2->AvgTimePerFrame = m_AvgTimePerFrame;
						}
					}

					CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Video", this, this, &hr));
					EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));
				}
			} else if (DHAV_AUDIO(hdr)) {
				if (hdr.audio.codec && !bAudioFound) {
					bAudioFound = true;

					GUID subtype = GUID_NULL;
					WORD wFormatTag = 0;
					WORD wBitsPerSample = 0;

					switch (hdr.audio.codec) {
						case 0x0a:
						case 0x16:
							subtype = MEDIASUBTYPE_MULAW;
							wFormatTag = WAVE_FORMAT_MULAW;
							wBitsPerSample = 8;
							break;
						case 0x0e:
							subtype = MEDIASUBTYPE_ALAW;
							wFormatTag = WAVE_FORMAT_ALAW;
							wBitsPerSample = 8;
							break;
						case 0x1a:
							subtype = MEDIASUBTYPE_RAW_AAC1;
							wFormatTag = WAVE_FORMAT_RAW_AAC1;
							break;
						case 0x1f:
							subtype = MEDIASUBTYPE_MPEG2_AUDIO;
							wFormatTag = WAVE_FORMAT_MPEG;
							break;
						case 0x21:
							subtype = MEDIASUBTYPE_MP3;
							wFormatTag = WAVE_FORMAT_MPEGLAYER3;
							break;
					}

					if (subtype != GUID_NULL) {
						mt.InitMediaType();

						mt.majortype         = MEDIATYPE_Audio;
						mt.subtype           = subtype;
						mt.formattype        = FORMAT_WaveFormatEx;

						WAVEFORMATEX* wfe    = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
						memset(wfe, 0, sizeof(WAVEFORMATEX));
						wfe->wFormatTag      = wFormatTag;
						wfe->nChannels       = hdr.audio.channels;
						wfe->nSamplesPerSec  = hdr.audio.sample_rate;
						wfe->wBitsPerSample  = wBitsPerSample;
						wfe->nBlockAlign     = wfe->nChannels * wfe->wBitsPerSample >> 3;
						wfe->nAvgBytesPerSec = wfe->nBlockAlign * wfe->nSamplesPerSec;

						mts.clear();
						mts.push_back(mt);

						CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio", this, this, &hr));
						EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(1, pPinOut)));
					}
				}
			}

			if (bVideoFound && bAudioFound) {
				break;
			}

			m_pFile->Skip(hdr.size + DHAV_FooterSize);
		}

		if (!bVideoFound) {
			return E_FAIL;
		}

		m_rtNewStart = m_rtCurrent = 0;
		m_rtNewStop = m_rtStop = m_rtDuration = 0;

		// find end PTS
		if (start_time > 0) {
			m_pFile->Seek(m_endpos - std::min((__int64)MEGABYTE, m_endpos));

			while (DHAVReadHeader(hdr)) {
				if (DHAV_VIDEO(hdr) && hdr.date) {
					tm time = {};
					time.tm_sec  =   hdr.date        & 0x3F;
					time.tm_min  =  (hdr.date >>  6) & 0x3F;
					time.tm_hour =  (hdr.date >> 12) & 0x1F;
					time.tm_mday =  (hdr.date >> 17) & 0x1F;
					time.tm_mon  =  (hdr.date >> 22) & 0x0F;
					time.tm_year = ((hdr.date >> 26) & 0x3F) + 2000 - 1900;

					end_time = mktime(&time);
				}

				m_pFile->Skip(hdr.size + DHAV_FooterSize);
			}

			if (end_time > start_time) {
				const auto seconds = difftime(end_time, start_time);
				const auto duration = (REFERENCE_TIME)seconds * UNITS;
				if (duration > 0) {
					m_rtNewStop = m_rtStop = m_rtDuration = duration;
				}
			}
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CDVRSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CDVRSplitterFilter");

	m_pFile->Seek(m_startpos);

	return true;
}

#define CalcPos(rt) (llMulDiv(rt, len, m_rtDuration, 0))
void CDVRSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_rt_Seek = rt;

	if (rt <= 0 || m_rtDuration <= 0) {
		m_pFile->Seek(m_startpos);
		return;
	}

	if (m_bDHAV) {
		const auto len = m_endpos - m_startpos;
		const auto seekpos = CalcPos(rt);
		m_pFile->Seek(seekpos);
		return;
	}

	if (m_sps.size() > 1) {
		const auto index = range_bsearch(m_sps, rt);
		if (index >= 1) {
			m_pFile->Seek(m_sps[index].fp);
			return;
		}
	}

	const __int64 len = m_endpos - m_startpos;
	__int64 seekpos   = CalcPos(rt);

	const REFERENCE_TIME rtmax = (rt + m_rtOffsetVideo) - UNITS;
	const REFERENCE_TIME rtmin = rtmax - UNITS / 2;

	__int64 curpos = seekpos;
	double div = 1.0;

	for (;;) {
		REFERENCE_TIME rtSeek = INVALID_TIME;

		m_pFile->Seek(curpos);
		if (m_bHXVS) {
			HXVSHeader hdr;
			while (HXVSReadHeader(hdr)) {
				if (hdr.sync == HXVS_VIDEO) {
					rtSeek = hdr.rt;
					break;
				}

				m_pFile->Skip(hdr.size);
			}

			if (rtmin <= rtSeek && rtSeek <= rtmax) {
				m_pFile->Seek(m_pFile->GetPos() - HXVS_HeaderSize);
				return;
			}
		}

		REFERENCE_TIME dt = rtSeek - rtmax;
		if (rtSeek < 0) {
			dt = UNITS / div;
		}
		dt /= div;
		div += 0.05;

		if (div >= 5.0) {
			break;
		}

		curpos -= CalcPos(dt);
		m_pFile->Seek(curpos);
	}

	m_pFile->Seek(CalcPos(rt));
}

bool CDVRSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	if (m_bHXVS) {
		REFERENCE_TIME last_rt = 0;
		CAutoPtr<CPacket> vp;

		while (SUCCEEDED(hr) && !CheckRequest(nullptr) && m_pFile->GetRemaining()) {
			HXVSHeader hdr;
			if (HXVSReadHeader(hdr)) {
				if (hdr.sync == HXVS_VIDEO) {
					// video packet
					if (vp && last_rt != hdr.rt) {
						hr = DeliverPacket(vp);
					}

					if (!vp) {
						vp.Attach(DNew CPacket());
						vp->bSyncPoint = hdr.key_frame;
						vp->rtStart = hdr.rt - m_rtOffsetVideo;
						vp->rtStop = vp->rtStart + m_AvgTimePerFrame;
						last_rt = hdr.rt;

						vp->resize(hdr.size);
						hr = m_pFile->ByteRead(vp->data(), hdr.size);
					} else {
						const size_t old_size = vp->size();
						vp->resize(old_size + hdr.size);
						hr = m_pFile->ByteRead(vp->data() + old_size, hdr.size);
					}
				} else {
					// audio packet
					if (GetOutputPin(1)) {
						CAutoPtr<CPacket> p(DNew CPacket());
						p->TrackNumber = 1;
						p->bSyncPoint = TRUE;
						p->rtStart = hdr.rt - m_rtOffsetAudio;
						p->rtStop = p->rtStart + llMulDiv(hdr.size, UNITS, 8000, 0);

						p->resize(hdr.size);
						if ((hr = m_pFile->ByteRead(p->data(), hdr.size)) == S_OK) {
							hr = DeliverPacket(p);
						}
					} else {
						m_pFile->Skip(hdr.size);
					}
				}
			} else {
				break;
			}
		}

		if (vp) {
			DeliverPacket(vp);
		}
	} else {
		WORD video_pts_prev = WORD_MAX;
		WORD audio_pts_prev = WORD_MAX;

		REFERENCE_TIME video_rt = m_rt_Seek;
		REFERENCE_TIME audio_rt = m_rt_Seek;

		while (SUCCEEDED(hr) && !CheckRequest(nullptr) && m_pFile->GetRemaining()) {
			DHAVHeader hdr;
			if (DHAVReadHeader(hdr)) {
				if (DHAV_VIDEO(hdr)) {
					if (video_pts_prev == WORD_MAX) {
						video_pts_prev = hdr.pts;
					}

					const WORD period = (hdr.pts >= video_pts_prev) ?
										(hdr.pts - video_pts_prev) :
										(hdr.pts + (0 - video_pts_prev));
					video_rt += 10000LL * period;
					video_pts_prev = hdr.pts;

					CAutoPtr<CPacket> p(DNew CPacket());
					p->bSyncPoint = hdr.key_frame;
					p->rtStart = video_rt;
					p->rtStop  = p->rtStart + m_AvgTimePerFrame;
					p->resize(hdr.size);
					if ((hr = m_pFile->ByteRead(p->data(), hdr.size)) == S_OK) {
						hr = DeliverPacket(p);
						m_pFile->Skip(DHAV_FooterSize);
					}
				} else if (DHAV_AUDIO(hdr) && GetOutputPin(1)) {
					if (audio_pts_prev == WORD_MAX) {
						audio_pts_prev = hdr.pts;
					}

					const WORD period = (hdr.pts >= audio_pts_prev) ?
										(hdr.pts - audio_pts_prev) :
										(hdr.pts + (0 - audio_pts_prev));
					audio_rt += 10000LL * period;
					audio_pts_prev = hdr.pts;

					CAutoPtr<CPacket> p(DNew CPacket());
					p->TrackNumber = 1;
					p->bSyncPoint = TRUE;
					p->rtStart = audio_rt;
					p->rtStop  = p->rtStart + 1;
					p->resize(hdr.size);
					if ((hr = m_pFile->ByteRead(p->data(), hdr.size)) == S_OK) {
						hr = DeliverPacket(p);
						m_pFile->Skip(DHAV_FooterSize);
					}
				} else {
					m_pFile->Skip(hdr.size + DHAV_FooterSize);
				}
			} else {
				break;
			}
		}
	}

	return true;
}

bool CDVRSplitterFilter::HXVSSync()
{
	const __int64 start = m_pFile->GetPos();
	DWORD sync;
	for (__int64 i = 0, j = m_endpos - start - HXVS_HeaderSize - 4;
			i <= MEGABYTE && i < j && S_OK == m_pFile->ByteRead((BYTE*)&sync, sizeof(sync));
			i++, m_pFile->Seek(start + i)) {
		if (sync == HXVS_VIDEO || sync == HXVS_AUDIO) {
			m_pFile->Seek(start + i);
			return true;
		}
	}

	m_pFile->Seek(start);
	return false;
}

bool CDVRSplitterFilter::HXVSReadHeader(HXVSHeader& hdr)
{
	const auto ret = HXVSSync() && S_OK == m_pFile->ByteRead((BYTE*)&hdr, HXVS_HeaderSize);
	if (ret) {
		hdr.rt = hdr.pts * 10000ll;
		hdr.key_frame = hdr.dummy == 1;

		if (hdr.sync == HXVS_AUDIO) {
			hdr.size -= 4;
			m_pFile->Skip(4);
		}
	}

	return ret;
}

bool CDVRSplitterFilter::DHAVSync(__int64& pos)
{
	const __int64 start = pos = m_pFile->GetPos();
	DWORD sync;
	for (__int64 i = 0, j = m_endpos - start - DHAV_HeaderSize - DHAV_FooterSize;
			i <= MEGABYTE && i < j && S_OK == m_pFile->ByteRead((BYTE*)&sync, sizeof(sync));
			i++, m_pFile->Seek(start + i)) {
		if (sync == DHAV_SYNC_START) {
			pos = start + i;
			m_pFile->Seek(pos);
			return true;
		}
	}

	m_pFile->Seek(start);
	return false;
}

bool CDVRSplitterFilter::DHAVReadHeader(DHAVHeader& hdr, const bool bParseExt/* = false*/)
{
	static const UINT sample_rates[] = {
		8000,  4000,   8000,  11025, 16000,
		20000, 22050,  32000, 44100, 48000,
		96000, 192000, 64000,
	};

	ZeroMemory(&hdr, sizeof(hdr));

	__int64 sync_pos = 0;
	for (;;) {
		const auto ret = DHAVSync(sync_pos) && S_OK == m_pFile->ByteRead((BYTE*)&hdr, DHAV_HeaderSize);
		if (ret) {
			if ((hdr.size - hdr.ext_size) <= DHAV_HeaderSize + DHAV_FooterSize) {
				m_pFile->Seek(sync_pos + 1);
				continue;
			}

			if ((hdr.size - DHAV_HeaderSize) > m_pFile->GetAvailable()) {
				return false;
			}

			hdr.size -= DHAV_HeaderSize;
			hdr.size -= DHAV_FooterSize;

			sync_pos = m_pFile->GetPos();
			m_pFile->Seek(sync_pos + hdr.size);
			DWORD sync;
			if (FAILED(m_pFile->ByteRead((BYTE*)&sync, sizeof(sync))) || sync != DHAV_SYNC_END) {
				return false;
			}
			m_pFile->Seek(sync_pos);

			if (hdr.ext_size) {
				if (bParseExt) {
					const auto ext_end_pos = m_pFile->GetPos() + hdr.ext_size;
					auto ext_size = hdr.ext_size;
					while (ext_size) {
						const BYTE type = (BYTE)m_pFile->BitRead(8);
						ext_size--;

						switch (type) {
							case 0x80:
								m_pFile->Skip(1);
								hdr.video.width  = 8 * m_pFile->BitRead(8);
								hdr.video.height = 8 * m_pFile->BitRead(8);
								ext_size -= 3;
								break;
							case 0x81:
								m_pFile->Skip(1);
								hdr.video.codec      = m_pFile->BitRead(8);
								hdr.video.frame_rate = m_pFile->BitRead(8);
								ext_size -= 3;
								break;
							case 0x82:
								m_pFile->Skip(3);
								m_pFile->ByteRead((BYTE*)&hdr.video.width, sizeof(hdr.video.width));
								m_pFile->ByteRead((BYTE*)&hdr.video.height, sizeof(hdr.video.height));
								ext_size -= 7;
								break;
							case 0x83:
								{
									hdr.audio.channels = m_pFile->BitRead(8);
									hdr.audio.codec    = m_pFile->BitRead(8);
									const BYTE index   = m_pFile->BitRead(8);
									hdr.audio.sample_rate = (index < _countof(sample_rates)) ? sample_rates[index] : 8000;

									ext_size -= 3;
								}
								break;
							case 0x8c:
								{
									m_pFile->Skip(1);
									hdr.audio.channels = m_pFile->BitRead(8);
									hdr.audio.codec    = m_pFile->BitRead(8);
									const BYTE index   = m_pFile->BitRead(8);
									hdr.audio.sample_rate = (index < _countof(sample_rates)) ? sample_rates[index] : 8000;

									m_pFile->Skip(3);
									ext_size -= 7;
								}
								break;
							case 0x88:
							case 0x91:
							case 0x92:
							case 0x93:
							case 0x95:
							case 0x9a:
							case 0x9b:
							case 0xb3:
								m_pFile->Skip(7);
								ext_size -= 7;
								break;
							case 0x84:
							case 0x85:
							case 0x8b:
							case 0x94:
							case 0x96:
							case 0xa0:
							case 0xb2:
							case 0xb4:
								m_pFile->Skip(3);
								ext_size -= 3;
								break;
							default:
								m_pFile->Skip(ext_size);
								ext_size = 0;
						}
					}
					m_pFile->Seek(ext_end_pos);
				} else {
					m_pFile->Skip(hdr.ext_size);
				}
				hdr.size -= hdr.ext_size;
			}

			hdr.key_frame = hdr.type != 0xfc;
		}

		return ret;
	}

	return false;
}

// CBaseFilter

STDMETHODIMP CDVRSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, DVRSplitterName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// IKeyFrameInfo

STDMETHODIMP CDVRSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);
	nKFs = m_sps.size();
	return S_OK;
}

STDMETHODIMP CDVRSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_pFile, E_UNEXPECTED);

	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	for (nKFs = 0; nKFs < m_sps.size(); nKFs++) {
		pKFs[nKFs] = m_sps[nKFs].rt;
	}

	return S_OK;
}

//
// CDVRSourceFilter
//

CDVRSourceFilter::CDVRSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CDVRSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

STDMETHODIMP CDVRSourceFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, DVRSourceName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}
