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
#include <moreuuids.h>
#include "DSUtil/PixelUtils.h"
#include "RoQSplitter.h"

#if (0)  // Set to 1 to activate RoQ packet traces
	#define TRACE_ROQ DLog
#else
	#define TRACE_ROQ __noop
#endif

#define RoQ_INFO           0x1001
#define RoQ_QUAD_CODEBOOK  0x1002
#define RoQ_QUAD_VQ        0x1011
#define RoQ_SOUND_MONO     0x1020
#define RoQ_SOUND_STEREO   0x1021

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_RoQ},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn), sudPinTypesIn},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesIn2[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RoQV},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut2[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins2[] =
{
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn2), sudPinTypesIn2},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesOut2), sudPinTypesOut2}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesIn3[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_RoQA},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut3[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
};

const AMOVIESETUP_PIN sudpPins3[] =
{
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn3), sudPinTypesIn3},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesOut3), sudPinTypesOut3}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CRoQSplitterFilter), RoQSplitterName, MERIT_NORMAL+1, std::size(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRoQSourceFilter), RoQSourceName, MERIT_NORMAL+1, 0, nullptr, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRoQVideoDecoder), RoQVideoDecoderName, MERIT_NORMAL, std::size(sudpPins2), sudpPins2, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRoQAudioDecoder), RoQAudioDecoderName, MERIT_NORMAL, std::size(sudpPins3), sudpPins3, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CRoQSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CRoQSourceFilter>, nullptr, &sudFilter[1]},
	{sudFilter[2].strName, sudFilter[2].clsID, CreateInstance<CRoQVideoDecoder>, nullptr, &sudFilter[2]},
	{sudFilter[3].strName, sudFilter[3].clsID, CreateInstance<CRoQAudioDecoder>, nullptr, &sudFilter[3]},
};

int g_cTemplates = std::size(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_RoQ, L"0,8,,8410FFFFFFFF1E00");

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_RoQ);

	return AMovieDllRegisterServer2(FALSE);
}

#include "filters/filters/Filters.h"

CFilterApp theApp;

#endif

#pragma pack(push, 1)
struct roq_chunk { WORD id; DWORD size; WORD arg; };
struct roq_info { WORD w, h, unk1, unk2; };
#pragma pack(pop)

//
// CRoQSplitterFilter
//

CRoQSplitterFilter::CRoQSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CRoQSplitterFilter", pUnk, phr, __uuidof(this))
{
}

STDMETHODIMP CRoQSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, RoQSplitterName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CRoQSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pAsyncReader = pAsyncReader;

	UINT64 hdr = 0;
	m_pAsyncReader->SyncRead(0, 8, (BYTE*)&hdr);
	if (hdr != 0x001effffffff1084) {
		return E_FAIL;
	}

	//

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	// pins

	CMediaType mt;
	std::vector<CMediaType> mts;

	int iHasVideo = 0;
	int iHasAudio = 0;

	m_index.clear();
	__int64 audiosamples = 0;

	roq_info ri;
	memset(&ri, 0, sizeof(ri));

	roq_chunk rc;

	UINT64 pos = 8;

	while(S_OK == m_pAsyncReader->SyncRead(pos, sizeof(rc), (BYTE*)&rc))
	{
		pos += sizeof(rc);

		if(rc.id == RoQ_INFO)
		{
			if(S_OK != m_pAsyncReader->SyncRead(pos, sizeof(ri), (BYTE*)&ri) || ri.w == 0 || ri.h == 0)
				break;
		}
		else if(rc.id == RoQ_QUAD_CODEBOOK || rc.id == RoQ_QUAD_VQ)
		{
			if(!iHasVideo && ri.w > 0 && ri.h > 0)
			{
				mts.clear();

				mt.InitMediaType();
				mt.majortype = MEDIATYPE_Video;
				mt.subtype = MEDIASUBTYPE_RoQV;
				mt.formattype = FORMAT_VideoInfo;
				VIDEOINFOHEADER vih;
				memset(&vih, 0, sizeof(vih));
				vih.AvgTimePerFrame = 10000000i64/30;
				vih.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				vih.bmiHeader.biWidth = ri.w;
				vih.bmiHeader.biHeight = ri.h;
				vih.bmiHeader.biCompression = MEDIASUBTYPE_RoQV.Data1;
				mt.SetFormat((BYTE*)&vih, sizeof(vih));
				mt.bFixedSizeSamples = FALSE;
				mt.bTemporalCompression = TRUE;
				mt.lSampleSize = 1;

				mts.push_back(mt);

				std::unique_ptr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Video", this, this, &hr));
				AddOutputPin(0, pPinOut);
			}

			if(rc.id == RoQ_QUAD_CODEBOOK)
			{
				iHasVideo++;

				index i;
				i.rtv = 10000000i64*m_index.size()/30;
				i.rta = 10000000i64*audiosamples/22050;
				i.fp = pos - sizeof(rc);
				m_index.push_back(i);
			}
		}
		else if(rc.id == RoQ_SOUND_MONO || rc.id == RoQ_SOUND_STEREO)
		{
			if(!iHasAudio)
			{
				mts.clear();

				mt.InitMediaType();
				mt.majortype = MEDIATYPE_Audio;
				mt.subtype = MEDIASUBTYPE_RoQA;
				mt.formattype = FORMAT_WaveFormatEx;
				WAVEFORMATEX wfe;
				memset(&wfe, 0, sizeof(wfe));
				wfe.wFormatTag = (WORD)MEDIASUBTYPE_RoQA.Data1; // cut into half, hehe, like anyone would care
				wfe.nChannels = (rc.id&1)+1;
				wfe.nSamplesPerSec = 22050;
				wfe.wBitsPerSample = 16;
				mt.SetFormat((BYTE*)&wfe, sizeof(wfe));
				mt.lSampleSize = 1;
				mts.push_back(mt);

				std::unique_ptr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio", this, this, &hr));
				AddOutputPin(1, pPinOut);
			}

			iHasAudio++;

			audiosamples += rc.size / ((rc.id&1)+1);
		}

		pos += rc.size;
	}

	//

	m_rtNewStop = m_rtStop = m_rtDuration = 10000000i64*iHasVideo/30;

	return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CRoQSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CRoQSplitterFilter");
	m_indexpos = m_index.cbegin();

	return(true);
}

void CRoQSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if(rt <= 0)
	{
		m_indexpos = m_index.cbegin();
	}
	else
	{
		m_indexpos = m_index.cend();
		while(m_indexpos != m_index.cbegin() && (--m_indexpos)->rtv > rt);
		//m_indexpos = --std::lower_bound(m_index.cbegin(), m_index.cend(), rt, [](const index& item, REFERENCE_TIME rt) {return item.rtv < rt; });
	}
}

bool CRoQSplitterFilter::DemuxLoop()
{
	if(m_indexpos == m_index.cend()) return(true);

	const index& i = *m_indexpos;

	REFERENCE_TIME rtVideo = i.rtv, rtAudio = i.rta;

	HRESULT hr = S_OK;

	UINT64 pos = i.fp;

	roq_chunk rc;
	while(S_OK == (hr = m_pAsyncReader->SyncRead(pos, sizeof(rc), (BYTE*)&rc))
		&& !CheckRequest(nullptr))
	{
		pos += sizeof(rc);

		std::unique_ptr<CPacket> p(DNew CPacket());

		if(rc.id == RoQ_QUAD_CODEBOOK || rc.id == RoQ_QUAD_VQ || rc.id == RoQ_SOUND_MONO || rc.id == RoQ_SOUND_STEREO)
		{
			p->resize(sizeof(rc) + rc.size);
			memcpy(p->data(), &rc, sizeof(rc));
			if(S_OK != (hr = m_pAsyncReader->SyncRead(pos, rc.size, p->data() + sizeof(rc))))
				break;
		}

		if(rc.id == RoQ_QUAD_CODEBOOK || rc.id == RoQ_QUAD_VQ)
		{
			p->TrackNumber = 0;
			p->bSyncPoint = rtVideo == 0;
			p->rtStart = rtVideo;
			p->rtStop = rtVideo += (rc.id == RoQ_QUAD_VQ ? 10000000i64/30 : 0);
			TRACE_ROQ(L"v: %I64d - %I64d (%Iu)", p->rtStart/10000, p->rtStop/10000, p->size());
		}
		else if(rc.id == RoQ_SOUND_MONO || rc.id == RoQ_SOUND_STEREO)
		{
			int nChannels = (rc.id&1)+1;

			p->TrackNumber = 1;
			p->bSyncPoint = TRUE;
			p->rtStart = rtAudio;
			p->rtStop = rtAudio += 10000000i64*rc.size/(nChannels*22050);
			TRACE_ROQ(L"a: %I64d - %I64d (%Iu)", p->rtStart/10000, p->rtStop/10000, p->size());
		}

		if(rc.id == RoQ_QUAD_CODEBOOK || rc.id == RoQ_QUAD_VQ || rc.id == RoQ_SOUND_MONO || rc.id == RoQ_SOUND_STEREO)
		{
			hr = DeliverPacket(std::move(p));
		}

		pos += rc.size;
	}

	return(true);
}

//
// CRoQSourceFilter
//

CRoQSourceFilter::CRoQSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CRoQSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.reset();
}

STDMETHODIMP CRoQSourceFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, RoQSourceName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

//
// CRoQVideoDecoder
//

CRoQVideoDecoder::CRoQVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(L"CRoQVideoDecoder", lpunk, __uuidof(this))
{
	if(phr) *phr = S_OK;
}

CRoQVideoDecoder::~CRoQVideoDecoder()
{
}

void CRoQVideoDecoder::apply_vector_2x2(int x, int y, roq_cell* cell)
{
	unsigned char* yptr;
	yptr = m_y[0] + (y * m_pitch) + x;
	*yptr++ = cell->y0;
	*yptr++ = cell->y1;
	yptr += (m_pitch - 2);
	*yptr++ = cell->y2;
	*yptr++ = cell->y3;
	m_u[0][(y/2) * (m_pitch/2) + x/2] = cell->u;
	m_v[0][(y/2) * (m_pitch/2) + x/2] = cell->v;
}

void CRoQVideoDecoder::apply_vector_4x4(int x, int y, roq_cell* cell)
{
	unsigned long row_inc, c_row_inc;
	unsigned char y0, y1, u, v;
	unsigned char *yptr, *uptr, *vptr;

	yptr = m_y[0] + (y * m_pitch) + x;
	uptr = m_u[0] + (y/2) * (m_pitch/2) + x/2;
	vptr = m_v[0] + (y/2) * (m_pitch/2) + x/2;

	row_inc = m_pitch - 4;
	c_row_inc = (m_pitch/2) - 2;
	*yptr++ = y0 = cell->y0; *uptr++ = u = cell->u; *vptr++ = v = cell->v;
	*yptr++ = y0;
	*yptr++ = y1 = cell->y1; *uptr++ = u; *vptr++ = v;
	*yptr++ = y1;

	yptr += row_inc;

	*yptr++ = y0;
	*yptr++ = y0;
	*yptr++ = y1;
	*yptr++ = y1;

	yptr += row_inc; uptr += c_row_inc; vptr += c_row_inc;

	*yptr++ = y0 = cell->y2; *uptr++ = u; *vptr++ = v;
	*yptr++ = y0;
	*yptr++ = y1 = cell->y3; *uptr++ = u; *vptr++ = v;
	*yptr++ = y1;

	yptr += row_inc;

	*yptr++ = y0;
	*yptr++ = y0;
	*yptr++ = y1;
	*yptr++ = y1;
}

void CRoQVideoDecoder::apply_motion_4x4(int x, int y, unsigned char mv, char mean_x, char mean_y)
{
	int i, mx, my;
	unsigned char *pa, *pb;

	mx = x + 8 - (mv >> 4) - mean_x;
	my = y + 8 - (mv & 0xf) - mean_y;

	pa = m_y[0] + (y * m_pitch) + x;
	pb = m_y[1] + (my * m_pitch) + mx;
	for(i = 0; i < 4; i++)
	{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_pitch;
		pb += m_pitch;
	}

	pa = m_u[0] + (y/2) * (m_pitch/2) + x/2;
	pb = m_u[1] + (my/2) * (m_pitch/2) + (mx + 1)/2;
	for(i = 0; i < 2; i++)
	{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa += m_pitch/2;
		pb += m_pitch/2;
	}

	pa = m_v[0] + (y/2) * (m_pitch/2) + x/2;
	pb = m_v[1] + (my/2) * (m_pitch/2) + (mx + 1)/2;
	for(i = 0; i < 2; i++)
	{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa += m_pitch/2;
		pb += m_pitch/2;
	}
}

void CRoQVideoDecoder::apply_motion_8x8(int x, int y, unsigned char mv, char mean_x, char mean_y)
{
	int mx, my, i;
	unsigned char *pa, *pb;

	mx = x + 8 - (mv >> 4) - mean_x;
	my = y + 8 - (mv & 0xf) - mean_y;

	pa = m_y[0] + (y * m_pitch) + x;
	pb = m_y[1] + (my * m_pitch) + mx;
	for(i = 0; i < 8; i++)
	{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa[4] = pb[4];
		pa[5] = pb[5];
		pa[6] = pb[6];
		pa[7] = pb[7];
		pa += m_pitch;
		pb += m_pitch;
	}

	pa = m_u[0] + (y/2) * (m_pitch/2) + x/2;
	pb = m_u[1] + (my/2) * (m_pitch/2) + (mx + 1)/2;
	for(i = 0; i < 4; i++)
	{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_pitch/2;
		pb += m_pitch/2;
	}

	pa = m_v[0] + (y/2) * (m_pitch/2) + x/2;
	pb = m_v[1] + (my/2) * (m_pitch/2) + (mx + 1)/2;
	for(i = 0; i < 4; i++)
	{
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_pitch/2;
		pb += m_pitch/2;
	}
}

HRESULT CRoQVideoDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_rtStart = tStart;

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pInput->CurrentMediaType(), &bih);

	int size = bih.biWidth*bih.biHeight;

	memset(m_y[0], 0, size);
	memset(m_u[0], 0x80, size/2);
	memset(m_y[1], 0, size);
	memset(m_u[1], 0x80, size/2);

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CRoQVideoDecoder::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	BYTE* pDataIn = nullptr;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;

	long len = pIn->GetActualDataLength();
	if (len <= 0) {
		return S_OK; // nothing to do
	}

	REFERENCE_TIME rtStart = 0, rtStop = 0;
	pIn->GetTime(&rtStart, &rtStop);

	if (pIn->IsPreroll() == S_OK || rtStart < 0) {
		return S_OK;
	}

	BYTE* pDataOut = nullptr;
	if(FAILED(hr = pOut->GetPointer(&pDataOut)))
		return hr;

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pInput->CurrentMediaType(), &bih);

	int w = bih.biWidth, h = bih.biHeight;

	// TODO: decode picture into m_pI420

	roq_chunk* rc = (roq_chunk*)pDataIn;

	pDataIn += sizeof(roq_chunk);

	if(rc->id == RoQ_QUAD_CODEBOOK)
	{
		DWORD nv1 = rc->arg>>8;
		if(nv1 == 0) nv1 = 256;

		DWORD nv2 = rc->arg&0xff;
		if(nv2 == 0 && nv1 * 6 < rc->size) nv2 = 256;

		memcpy(m_cells, pDataIn, sizeof(m_cells[0])*nv1);
		pDataIn += sizeof(m_cells[0])*nv1;

		for(int i = 0; i < (int)nv2; i++)
			for(int j = 0; j < 4; j++)
				m_qcells[i].idx[j] = &m_cells[*pDataIn++];

		return S_FALSE;
	}
	else if(rc->id == RoQ_QUAD_VQ)
	{
		int bpos = 0, xpos = 0, ypos = 0;
		int vqflg = 0, vqflg_pos = -1, vqid;
		roq_qcell* qcell = nullptr;

		BYTE* buf = pDataIn;

		while(bpos < (int)rc->size && ypos < h)
		{
			for(int yp = ypos; yp < ypos + 16; yp += 8)
			{
				for(int xp = xpos; xp < xpos + 16; xp += 8)
				{
					if(vqflg_pos < 0)
					{
						vqflg = buf[bpos++];
						vqflg |= buf[bpos++]<<8;
						vqflg_pos = 7;
					}

					vqid = (vqflg >> (vqflg_pos * 2)) & 3;
					vqflg_pos--;

					switch(vqid)
					{
					case 0:
						break;
					case 1:
						apply_motion_8x8(xp, yp, buf[bpos++], rc->arg >> 8, rc->arg & 0xff);
						break;
					case 2:
						qcell = m_qcells + buf[bpos++];
						apply_vector_4x4(xp, yp, qcell->idx[0]);
						apply_vector_4x4(xp+4, yp, qcell->idx[1]);
						apply_vector_4x4(xp, yp+4, qcell->idx[2]);
						apply_vector_4x4(xp+4, yp+4, qcell->idx[3]);
						break;
					case 3:
						for(int k = 0; k < 4; k++)
						{
							int x = xp, y = yp;
							if(k&1) x += 4;
							if(k&2) y += 4;

							if(vqflg_pos < 0)
							{
								vqflg = buf[bpos++];
								vqflg |= buf[bpos++]<<8;
								vqflg_pos = 7;
							}

							vqid = (vqflg >> (vqflg_pos * 2)) & 3;
							vqflg_pos--;

							switch(vqid)
							{
							case 0:
								break;
							case 1:
								apply_motion_4x4(x, y, buf[bpos++], rc->arg >> 8, rc->arg & 0xff);
								break;
							case 2:
								qcell = m_qcells + buf[bpos++];
								apply_vector_2x2(x, y, qcell->idx[0]);
								apply_vector_2x2(x+2, y, qcell->idx[1]);
								apply_vector_2x2(x, y+2, qcell->idx[2]);
								apply_vector_2x2(x+2, y+2, qcell->idx[3]);
								break;
							case 3:
								apply_vector_2x2(x, y, &m_cells[buf[bpos++]]);
								apply_vector_2x2(x+2, y, &m_cells[buf[bpos++]]);
								apply_vector_2x2(x, y+2, &m_cells[buf[bpos++]]);
								apply_vector_2x2(x+2, y+2, &m_cells[buf[bpos++]]);
								break;
							}
						}
						break;
					}
				}
			}

			xpos += 16;
			if(xpos >= w) {xpos -= w; ypos += 16;}
		}

		if(m_rtStart+rtStart == 0)
		{
			memcpy(m_y[1], m_y[0], w*h*3/2);
		}
		else
		{
			std::swap(m_y[0], m_y[1]);
			std::swap(m_u[0], m_u[1]);
			std::swap(m_v[0], m_v[1]);
		}
	}
	else
	{
		return E_UNEXPECTED;
	}

	const BYTE* const src[3] = { m_y[1], m_u[1], m_v[1] };

	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	if (bihOut.biCompression == FCC('NV12')) {
		CopyI420toNV12(w, h, pDataOut, bihOut.biWidth, src, w);
	}
	else if (bihOut.biCompression == FCC('YV12')) {
		CopyI420toYV12(h, pDataOut, bihOut.biWidth, src, w);
	}
	else if (bihOut.biCompression == FCC('YUY2')) {
		ConvertI420toYUY2(h, pDataOut, bihOut.biWidth * 2, src, w, false);
	}

	pOut->SetTime(&rtStart, &rtStop);

	return S_OK;
}

HRESULT CRoQVideoDecoder::CheckInputType(const CMediaType* mtIn)
{
	return mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_RoQV
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CRoQVideoDecoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(m_pOutput && m_pOutput->IsConnected())
	{
		BITMAPINFOHEADER bih1, bih2;
		if(ExtractBIH(mtOut, &bih1) && ExtractBIH(&m_pOutput->CurrentMediaType(), &bih2)
		&& abs(bih1.biHeight) != abs(bih2.biHeight))
			return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Video
		&& (mtOut->subtype == MEDIASUBTYPE_NV12
		|| mtOut->subtype == MEDIASUBTYPE_YV12
		|| mtOut->subtype == MEDIASUBTYPE_YUY2)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CRoQVideoDecoder::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bih);

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = bih.biSizeImage;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;

	if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual)))
		return hr;

	return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR);
}

HRESULT CRoQVideoDecoder::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	struct {
		const GUID* subtype;
		WORD biPlanes;
		WORD biBitCount;
		DWORD biCompression;
	} fmts[] = {
		//{&MEDIASUBTYPE_NV12, 3, 12, FCC('NV12')},
		//{&MEDIASUBTYPE_YV12, 3, 12, FCC('YV12')},
		{&MEDIASUBTYPE_YUY2, 1, 16, FCC('YUY2')},
	};

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition >= (int)std::size(fmts)) {
		return VFW_S_NO_MORE_ITEMS;
	}

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pInput->CurrentMediaType(), &bih);

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = *fmts[iPosition].subtype;
	pmt->formattype = FORMAT_VideoInfo;

	BITMAPINFOHEADER bihOut;
	memset(&bihOut, 0, sizeof(bihOut));
	bihOut.biSize = sizeof(bihOut);
	bihOut.biWidth = bih.biWidth;
	bihOut.biHeight = bih.biHeight;
	bihOut.biPlanes = fmts[iPosition].biPlanes;
	bihOut.biBitCount = fmts[iPosition].biBitCount;
	bihOut.biCompression = fmts[iPosition].biCompression;
	bihOut.biSizeImage = bih.biWidth*bih.biHeight*bihOut.biBitCount>>3;

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	memset(vih, 0, sizeof(VIDEOINFOHEADER));
	vih->bmiHeader = bihOut;

	return S_OK;
}

HRESULT CRoQVideoDecoder::StartStreaming()
{
	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pInput->CurrentMediaType(), &bih);

	int size = bih.biWidth*bih.biHeight;

	m_y[0] = DNew BYTE[size*3/2];
	m_u[0] = m_y[0] + size;
	m_v[0] = m_y[0] + size*5/4;
	m_y[1] = DNew BYTE[size*3/2];
	m_u[1] = m_y[1] + size;
	m_v[1] = m_y[1] + size*5/4;

	m_pitch = bih.biWidth;

	return __super::StartStreaming();
}

HRESULT CRoQVideoDecoder::StopStreaming()
{
	delete [] m_y[0]; m_y[0] = nullptr;
	delete [] m_y[1]; m_y[1] = nullptr;

	return __super::StopStreaming();
}

//
// CRoQAudioDecoder
//

CRoQAudioDecoder::CRoQAudioDecoder(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(L"CRoQAudioDecoder", lpunk, __uuidof(this))
{
	if(phr) *phr = S_OK;
}

CRoQAudioDecoder::~CRoQAudioDecoder()
{
}

HRESULT CRoQAudioDecoder::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	HRESULT hr;

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	WAVEFORMATEX* pwfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();
	UNREFERENCED_PARAMETER(pwfe);

	BYTE* pDataIn = nullptr;
	if(FAILED(hr = pIn->GetPointer(&pDataIn)))
		return hr;

	long len = pIn->GetActualDataLength();
	if(len <= 0) return S_OK;

	REFERENCE_TIME rtStart, rtStop;
	pIn->GetTime(&rtStart, &rtStop);

	if(pIn->IsPreroll() == S_OK || rtStart < 0)
		return S_OK;

	BYTE* pDataOut = nullptr;
	if(FAILED(hr = pOut->GetPointer(&pDataOut)))
		return hr;

	long size = pOut->GetSize();
	if(size <= 0)
		return E_FAIL;

	roq_chunk* rc = (roq_chunk*)pDataIn;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

	if(wfe->nChannels == 1)
	{
		int mono = (short)rc->arg;
		unsigned char* src = pDataIn + sizeof(roq_chunk);
		short* dst = (short*)pDataOut;
		for(int i = sizeof(roq_chunk); i < len; i++, src++, dst++)
		{
			short diff = (*src&0x7f)*(*src&0x7f);
			if(*src&0x80) diff = -diff;
			mono += diff;
			*dst = (short)mono;
		}
	}
	else if(wfe->nChannels == 2)
	{
		int left = (char)(rc->arg>>8)<<8;
		int right = (char)(rc->arg)<<8;
		unsigned char* src = pDataIn + sizeof(roq_chunk);
		short* dst = (short*)pDataOut;
		for(int i = sizeof(roq_chunk); i < len; i+=2, src++, dst++)
		{
			short diff = (*src&0x7f)*(*src&0x7f);
			if(*src&0x80) diff = -diff;
			ASSERT((int)left + diff <= SHRT_MAX && (int)left + diff >= SHRT_MIN);
			left += diff;
			*dst = (short)left;

			src++; dst++;

			diff = (*src&0x7f)*(*src&0x7f);
			if(*src&0x80) diff = -diff;
			ASSERT((int)right + diff <= SHRT_MAX && (int)right + diff >= SHRT_MIN);
			right += diff;
			*dst = (short)right;
		}
	}
	else
	{
		return E_UNEXPECTED;
	}

	pOut->SetTime(&rtStart, &rtStop);

	pOut->SetActualDataLength(2*(len-sizeof(roq_chunk)));

	return S_OK;
}

HRESULT CRoQAudioDecoder::CheckInputType(const CMediaType* mtIn)
{
	return mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_RoQA
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CRoQAudioDecoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_PCM
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CRoQAudioDecoder::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	CComPtr<IMemAllocator> pAllocatorIn;
	m_pInput->GetAllocator(&pAllocatorIn);
	if(!pAllocatorIn) return E_UNEXPECTED;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

	// ok, maybe this is too much...
	pProperties->cBuffers = 8;
	pProperties->cbBuffer = wfe->nChannels*wfe->nSamplesPerSec*wfe->wBitsPerSample>>3; // nAvgBytesPerSec;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;

	if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual)))
		return hr;

	return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}

HRESULT CRoQAudioDecoder::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_pInput->CurrentMediaType();
	pmt->subtype = MEDIASUBTYPE_PCM;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->ReallocFormatBuffer(sizeof(WAVEFORMATEX));
	wfe->cbSize = 0;
	wfe->wFormatTag = WAVE_FORMAT_PCM;
	wfe->nBlockAlign = wfe->nChannels*wfe->wBitsPerSample>>3;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec*wfe->nBlockAlign;

	return S_OK;
}
