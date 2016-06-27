/*
 * (C) 2014-2016 see Authors.txt
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
#include "FFAudioDecoder.h"

#pragma warning(disable: 4005 4244)
extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
	#include <ffmpeg/libavutil/intreadwrite.h>
	#include <ffmpeg/libavutil/opt.h>
}
#pragma warning(default: 4005 4244)

#include "moreuuids.h"
#include "../../../DSUtil/AudioParser.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/ff_log.h"
#include "../../../DSUtil/GolombBuffer.h"
#include "../../Lock.h"

static const struct {
	const CLSID* clsMinorType;
	const enum AVCodecID nFFCodec;
} ffAudioCodecs[] = {
	// AMR
	{ &MEDIASUBTYPE_AMR,               AV_CODEC_ID_AMR_NB },
	{ &MEDIASUBTYPE_SAMR,              AV_CODEC_ID_AMR_NB },
	{ &MEDIASUBTYPE_SAWB,              AV_CODEC_ID_AMR_WB },
	// AAC
	{ &MEDIASUBTYPE_RAW_AAC1,          AV_CODEC_ID_AAC },
	{ &MEDIASUBTYPE_MP4A,              AV_CODEC_ID_AAC },
	{ &MEDIASUBTYPE_mp4a,              AV_CODEC_ID_AAC },
	{ &MEDIASUBTYPE_AAC_ADTS,          AV_CODEC_ID_AAC },
	{ &MEDIASUBTYPE_LATM_AAC,          AV_CODEC_ID_AAC_LATM },
	// ALAC
	{ &MEDIASUBTYPE_ALAC,              AV_CODEC_ID_ALAC },
	// MPEG-4 ALS
	{ &MEDIASUBTYPE_ALS,               AV_CODEC_ID_MP4ALS },
	// Ogg Vorbis
	{ &MEDIASUBTYPE_Vorbis2,           AV_CODEC_ID_VORBIS },
	// NellyMoser
	{ &MEDIASUBTYPE_NELLYMOSER,        AV_CODEC_ID_NELLYMOSER },
	// Qt ADPCM
	{ &MEDIASUBTYPE_IMA4,              AV_CODEC_ID_ADPCM_IMA_QT },
	// FLV ADPCM
	{ &MEDIASUBTYPE_ADPCM_SWF,         AV_CODEC_ID_ADPCM_SWF },
	// AMV IMA ADPCM
	{ &MEDIASUBTYPE_IMA_AMV,           AV_CODEC_ID_ADPCM_IMA_AMV },
	// ADX ADPCM
	{ &MEDIASUBTYPE_ADX_ADPCM,         AV_CODEC_ID_ADPCM_ADX },
	// MPEG Audio
	{ &MEDIASUBTYPE_MPEG1Packet,       AV_CODEC_ID_MP1 },
	{ &MEDIASUBTYPE_MPEG1Payload,      AV_CODEC_ID_MP1 },
	{ &MEDIASUBTYPE_MPEG1AudioPayload, AV_CODEC_ID_MP1 },
	{ &MEDIASUBTYPE_MPEG2_AUDIO,       AV_CODEC_ID_MP2 },
	{ &MEDIASUBTYPE_MP3,               AV_CODEC_ID_MP3 },
	// RealMedia Audio
	{ &MEDIASUBTYPE_14_4,              AV_CODEC_ID_RA_144 },
	{ &MEDIASUBTYPE_28_8,              AV_CODEC_ID_RA_288 },
	{ &MEDIASUBTYPE_ATRC,              AV_CODEC_ID_ATRAC3 },
	{ &MEDIASUBTYPE_COOK,              AV_CODEC_ID_COOK   },
	{ &MEDIASUBTYPE_SIPR,              AV_CODEC_ID_SIPR   },
	{ &MEDIASUBTYPE_SIPR_WAVE,         AV_CODEC_ID_SIPR   },
	{ &MEDIASUBTYPE_RAAC,              AV_CODEC_ID_AAC    },
	{ &MEDIASUBTYPE_RACP,              AV_CODEC_ID_AAC    },
	{ &MEDIASUBTYPE_RALF,              AV_CODEC_ID_RALF   },
	{ &MEDIASUBTYPE_DNET,              AV_CODEC_ID_AC3    },
	// AC3, E-AC3, TrueHD, MLP
	{ &MEDIASUBTYPE_DOLBY_AC3,         AV_CODEC_ID_AC3    },
	{ &MEDIASUBTYPE_WAVE_DOLBY_AC3,    AV_CODEC_ID_AC3    },
	{ &MEDIASUBTYPE_DOLBY_DDPLUS,      AV_CODEC_ID_EAC3   },
	{ &MEDIASUBTYPE_DOLBY_TRUEHD,      AV_CODEC_ID_TRUEHD },
	{ &MEDIASUBTYPE_MLP,               AV_CODEC_ID_MLP    },
	// ATRAC3, ATRAC3plus
	{ &MEDIASUBTYPE_ATRAC3,            AV_CODEC_ID_ATRAC3 },
	{ &MEDIASUBTYPE_ATRAC3plus,        AV_CODEC_ID_ATRAC3P},
	// DTS
	{ &MEDIASUBTYPE_DTS,               AV_CODEC_ID_DTS },
	{ &MEDIASUBTYPE_DTS2,              AV_CODEC_ID_DTS },
	// FLAC
	{ &MEDIASUBTYPE_FLAC_FRAMED,       AV_CODEC_ID_FLAC },
	{ &MEDIASUBTYPE_FLAC,              AV_CODEC_ID_FLAC },
	// QDM2
	{ &MEDIASUBTYPE_QDM2,              AV_CODEC_ID_QDM2 },
	// WavPack4
	{ &MEDIASUBTYPE_WAVPACK4,          AV_CODEC_ID_WAVPACK },
	// MusePack v7/v8
	{ &MEDIASUBTYPE_MPC7,              AV_CODEC_ID_MUSEPACK7 },
	{ &MEDIASUBTYPE_MPC8,              AV_CODEC_ID_MUSEPACK8 },
	// APE
	{ &MEDIASUBTYPE_APE,               AV_CODEC_ID_APE },
	// TAK
	{ &MEDIASUBTYPE_TAK,               AV_CODEC_ID_TAK },
	// TTA
	{ &MEDIASUBTYPE_TTA1,              AV_CODEC_ID_TTA },
	// Shorten
	{ &MEDIASUBTYPE_Shorten,           AV_CODEC_ID_SHORTEN},
	// DSP Group TrueSpeech
	{ &MEDIASUBTYPE_TRUESPEECH,        AV_CODEC_ID_TRUESPEECH },
	// Voxware MetaSound
	{ &MEDIASUBTYPE_VOXWARE_RT29,      AV_CODEC_ID_METASOUND },
	// Windows Media Audio 9 Professional
	{ &MEDIASUBTYPE_WMAUDIO3,          AV_CODEC_ID_WMAPRO },
	// Windows Media Audio Lossless
	{ &MEDIASUBTYPE_WMAUDIO_LOSSLESS,  AV_CODEC_ID_WMALOSSLESS },
	// Windows Media Audio 1, 2
	{ &MEDIASUBTYPE_MSAUDIO1,          AV_CODEC_ID_WMAV1 },
	{ &MEDIASUBTYPE_WMAUDIO2,          AV_CODEC_ID_WMAV2 },
	// Windows Media Audio Voice
	{ &MEDIASUBTYPE_WMSP1,	           AV_CODEC_ID_WMAVOICE },
	// Bink Audio
	{ &MEDIASUBTYPE_BINKA_DCT,         AV_CODEC_ID_BINKAUDIO_DCT  },
	{ &MEDIASUBTYPE_BINKA_RDFT,        AV_CODEC_ID_BINKAUDIO_RDFT },
	// Indeo Audio
	{ &MEDIASUBTYPE_IAC,               AV_CODEC_ID_IAC },
	// Opus
	{ &MEDIASUBTYPE_OPUS,              AV_CODEC_ID_OPUS },
	// Speex
	{ &MEDIASUBTYPE_SPEEX,             AV_CODEC_ID_SPEEX },
	// DSD
	{ &MEDIASUBTYPE_DSD,               AV_CODEC_ID_DSD_MSBF },
	{ &MEDIASUBTYPE_DSDL,              AV_CODEC_ID_DSD_LSBF },
	{ &MEDIASUBTYPE_DSDM,              AV_CODEC_ID_DSD_MSBF },
	{ &MEDIASUBTYPE_DSD1,              AV_CODEC_ID_DSD_LSBF_PLANAR },
	{ &MEDIASUBTYPE_DSD8,              AV_CODEC_ID_DSD_MSBF_PLANAR },

	{ &MEDIASUBTYPE_None,              AV_CODEC_ID_NONE },
};

enum AVCodecID FindCodec(const GUID subtype)
{
	static int index = _countof(ffAudioCodecs) - 1;

	if (subtype == *ffAudioCodecs[index].clsMinorType) {
		return ffAudioCodecs[index].nFFCodec;;
	}

	for (int i = 0; i < _countof(ffAudioCodecs); i++) {
		if (subtype == *ffAudioCodecs[i].clsMinorType) {
			index = i;
			return ffAudioCodecs[i].nFFCodec;
		}
	}

	return AV_CODEC_ID_NONE;
}

const char* GetCodecDescriptorName(enum AVCodecID codec_id)
{
	if (const AVCodecDescriptor* codec_descriptor = avcodec_descriptor_get(codec_id)) {
		return codec_descriptor->name;
	}

	return NULL;
}

static DWORD get_lav_channel_layout(uint64_t layout)
{
	if (layout > UINT32_MAX) {
		if (layout & AV_CH_WIDE_LEFT) {
			layout = (layout & ~AV_CH_WIDE_LEFT) | AV_CH_FRONT_LEFT_OF_CENTER;
		}
		if (layout & AV_CH_WIDE_RIGHT) {
			layout = (layout & ~AV_CH_WIDE_RIGHT) | AV_CH_FRONT_RIGHT_OF_CENTER;
		}

		if (layout & AV_CH_SURROUND_DIRECT_LEFT) {
			layout = (layout & ~AV_CH_SURROUND_DIRECT_LEFT) | AV_CH_SIDE_LEFT;
		}
		if (layout & AV_CH_SURROUND_DIRECT_RIGHT) {
			layout = (layout & ~AV_CH_SURROUND_DIRECT_RIGHT) | AV_CH_SIDE_RIGHT;
		}
	}

	return (DWORD)layout;
}

// CFFAudioDecoder

CFFAudioDecoder::CFFAudioDecoder()
	: m_pAVCodec(NULL)
	, m_pAVCtx(NULL)
	, m_pParser(NULL)
	, m_pFrame(NULL)
	, m_bNeedSyncpoint(false)
{
	memset(&m_raData, 0, sizeof(m_raData));

	avcodec_register_all();
	av_log_set_callback(ff_log);
}

static bool flac_parse_block_header(CGolombBuffer& gb, BYTE& last, BYTE& type, DWORD& size) {
	if (gb.RemainingSize() < 4) {
		return false;
	}

	BYTE tmp = gb.ReadByte();
	last = tmp & 0x80;
	type = tmp & 0x7F;
	size = (DWORD)gb.BitRead(24);

	return true;
}

static bool ParseVorbisTag(const CString field_name, const CString vorbisTag, CString& tagValue) {
	tagValue.Empty();

	CString vorbis_data = vorbisTag;
	vorbis_data.MakeUpper().Trim();
	if (vorbis_data.Find(field_name + L'=') != 0) {
		return false;
	}

	vorbis_data = vorbisTag;
	vorbis_data.Delete(0, vorbis_data.Find('=') + 1);
	tagValue = vorbis_data;

	return true;
}

bool CFFAudioDecoder::Init(enum AVCodecID codecID, CMediaType* mediaType)
{
	AVCodecID codec_id	= AV_CODEC_ID_NONE;
	int samplerate		= 0;
	int channels		= 0;
	int bitdeph			= 0;
	int block_align		= 0;
	int64_t bitrate		= 0;
	BYTE* extradata		= NULL;
	unsigned extralen	= 0;

	if (codecID == AV_CODEC_ID_NONE || mediaType == NULL) {
		if (m_pAVCodec == NULL || m_pAVCtx == NULL || m_pAVCtx->codec_id == AV_CODEC_ID_NONE) {
			return false;
		}

		// use the previous info
		codec_id	= m_pAVCtx->codec_id;
		samplerate	= m_pAVCtx->sample_rate;
		channels	= m_pAVCtx->channels;
		bitdeph		= m_pAVCtx->bits_per_coded_sample;
		block_align	= m_pAVCtx->block_align;
		bitrate		= m_pAVCtx->bit_rate;
		if (m_pAVCtx->extradata && m_pAVCtx->extradata_size > 0) {
			extralen = m_pAVCtx->extradata_size;
			extradata = (uint8_t*)av_mallocz(extralen + AV_INPUT_BUFFER_PADDING_SIZE);
			memcpy(extradata, m_pAVCtx->extradata, extralen);
		}
	}
	else {
		codec_id = codecID;
		if (mediaType->formattype == FORMAT_WaveFormatEx) {
			WAVEFORMATEX *wfex = (WAVEFORMATEX *)mediaType->Format();
			samplerate	= wfex->nSamplesPerSec;
			channels	= wfex->nChannels;
			bitdeph		= wfex->wBitsPerSample;
			block_align	= wfex->nBlockAlign;
			bitrate		= wfex->nAvgBytesPerSec * 8;
		}
		else if (mediaType->formattype == FORMAT_VorbisFormat2) {
			VORBISFORMAT2 *vf2 = (VORBISFORMAT2 *)mediaType->Format();
			samplerate	= vf2->SamplesPerSec;
			channels	= vf2->Channels;
			bitdeph		= vf2->BitsPerSample;
		}
		else {
			return false;
		}

		// fix incorect info
		if (codec_id == AV_CODEC_ID_AMR_NB) {
			channels = 1;
			samplerate  = 8000;
		}
		else if (codec_id == AV_CODEC_ID_AMR_WB) {
			channels = 1;
			samplerate = 16000;
		}

		getExtraData(mediaType->Format(), &mediaType->formattype, mediaType->cbFormat, NULL, &extralen);
		if (extralen) {
			extradata = (uint8_t*)av_mallocz(extralen + AV_INPUT_BUFFER_PADDING_SIZE);
			getExtraData(mediaType->Format(), &mediaType->formattype, mediaType->cbFormat, extradata, NULL);
		}
	}

	if (m_pAVCodec) {
		StreamFinish();
	}

	bool bRet = false;

	m_pAVCodec = avcodec_find_decoder(codec_id);
	if (m_pAVCodec) {

		m_pAVCtx = avcodec_alloc_context3(m_pAVCodec);
		CheckPointer(m_pAVCtx, false);

		m_pAVCtx->codec_id				= codec_id;
		m_pAVCtx->sample_rate			= samplerate;
		m_pAVCtx->channels				= channels;
		m_pAVCtx->bits_per_coded_sample	= bitdeph;
		m_pAVCtx->block_align			= block_align;
		m_pAVCtx->bit_rate				= bitrate;
		m_pAVCtx->err_recognition		= AV_EF_CAREFUL;
		m_pAVCtx->thread_count			= 1;
		m_pAVCtx->thread_type			= 0;
		m_pAVCtx->refcounted_frames		= 1;
		if (m_pAVCodec->capabilities & AV_CODEC_CAP_TRUNCATED) {
			m_pAVCtx->flags				|= AV_CODEC_FLAG_TRUNCATED;
		}

		//if (stereodownmix) { // works to AC3, TrueHD, DTS
		//	m_pAVCtx->request_channel_layout = AV_CH_LAYOUT_STEREO;
		//}

		m_pParser = av_parser_init(codec_id);

		memset(&m_raData, 0, sizeof(m_raData));

		if (extradata && extralen) {
			if (codec_id == AV_CODEC_ID_COOK || codec_id == AV_CODEC_ID_ATRAC3 || codec_id == AV_CODEC_ID_SIPR) {
				if (extralen >= 4 && GETDWORD(extradata) == MAKEFOURCC('.', 'r', 'a', 0xfd)) {
					HRESULT hr = ParseRealAudioHeader(extradata, extralen);
					av_freep(&extradata);
					extralen = 0;
					if (FAILED(hr)) {
						return false;
					}
					if (codec_id == AV_CODEC_ID_SIPR) {
						if (m_raData.flavor > 3) {
							DbgLog((LOG_TRACE, 3, L"CFFAudioDecoder::Init() : Invalid SIPR flavor (%d)", m_raData.flavor));
							return false;
						}
						static BYTE sipr_subpk_size[4] = { 29, 19, 37, 20 };
						m_pAVCtx->block_align = sipr_subpk_size[m_raData.flavor];
					}
				} else {
					// Try without any processing?
					m_pAVCtx->extradata_size = extralen;
					m_pAVCtx->extradata      = extradata;
				}
			} else {
				m_pAVCtx->extradata_size = extralen;
				m_pAVCtx->extradata      = extradata;
			}
		}

		avcodec_lock;
		if (avcodec_open2(m_pAVCtx, m_pAVCodec, NULL) >= 0) {
			m_pFrame = av_frame_alloc();
			CheckPointer(m_pFrame, false);
			bRet = true;
		}
		avcodec_unlock;

		if (bRet && m_pAVCtx->codec_id == AV_CODEC_ID_FLAC && m_pAVCtx->extradata_size > (4+4+34) && GETDWORD(m_pAVCtx->extradata) == FCC('fLaC')) {
			BYTE metadata_last, metadata_type;
			DWORD metadata_size;
			CGolombBuffer gb(m_pAVCtx->extradata + 4, m_pAVCtx->extradata_size - 4);

			while (flac_parse_block_header(gb, metadata_last, metadata_type, metadata_size)) {
				if (metadata_type == 4) { // FLAC_METADATA_TYPE_VORBIS_COMMENT
					int vendor_length = gb.ReadDwordLE();
					if (gb.RemainingSize() >= vendor_length) {
						gb.SkipBytes(vendor_length);
						int num_comments = gb.ReadDwordLE();
						for (int i = 0; i < num_comments; i++) {
							int comment_lenght = gb.ReadDwordLE();
							if (comment_lenght > gb.RemainingSize()) {
								break;
							}
							BYTE* comment = DNew BYTE[comment_lenght + 1];
							ZeroMemory(comment, comment_lenght + 1);
							gb.ReadBuffer(comment, comment_lenght);
							CString vorbisTag = AltUTF8To16((LPCSTR)comment);
							delete [] comment;
							CString tagValue;
							if (!vorbisTag.IsEmpty() && ParseVorbisTag(L"WAVEFORMATEXTENSIBLE_CHANNEL_MASK", vorbisTag, tagValue)) {
								uint64_t channel_layout = wcstol(tagValue, NULL, 0);
								if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == m_pAVCtx->channels) {
									m_pAVCtx->channel_layout = channel_layout;
								}
								break;
							}
						}
					}
					break;
				}
				if (metadata_last) {
					break;
				}
				gb.SkipBytes(metadata_size);
			}
		}
	}

	if (!bRet) {
		StreamFinish();
	}

	m_bNeedSyncpoint = (m_raData.deint_id != 0);

	return bRet;
}

void CFFAudioDecoder::SetDRC(bool fDRC)
{
	if (m_pAVCtx) {
		AVCodecID codec_id = m_pAVCtx->codec_id;
		if (codec_id == AV_CODEC_ID_AC3 || codec_id == AV_CODEC_ID_EAC3) {
			av_opt_set_double(m_pAVCtx, "drc_scale", fDRC ? 1.0f : 0.0f, AV_OPT_SEARCH_CHILDREN);
		}
	}
}

HRESULT CFFAudioDecoder::SendData(BYTE* p, int size, int* out_size)
{
	HRESULT hr = E_FAIL;

	if (out_size) {
		*out_size = 0;
	}
	if (GetCodecId() == AV_CODEC_ID_NONE) {
		return hr;
	}

	int ret = 0;
	AVPacket avpkt;
	av_init_packet(&avpkt);

	if (m_pParser) {
		BYTE* pOut = NULL;
		int pOut_size = 0;

		int used_bytes = av_parser_parse2(m_pParser, m_pAVCtx, &pOut, &pOut_size, p, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (used_bytes < 0) {
			DbgLog((LOG_TRACE, 3, L"CFFAudioDecoder::Decode() : audio parsing failed (ret: %d)", -used_bytes));
			return E_FAIL;
		} else if (used_bytes == 0 && pOut_size == 0) {
			DbgLog((LOG_TRACE, 3, L"CFFAudioDecoder::Decode() : could not process buffer while parsing"));
		}

		if (out_size) {
			*out_size = used_bytes;
		}

		if (pOut_size > 0) {
			avpkt.data = pOut;
			avpkt.size = pOut_size;

			ret = avcodec_send_packet(m_pAVCtx, &avpkt);
		}
	} else {
		avpkt.data = p;
		avpkt.size = size;

		ret = avcodec_send_packet(m_pAVCtx, &avpkt);
	}

	if (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		if (!m_pParser && out_size) {
			*out_size = size;
		}

		hr = S_OK;
	}

	if (hr == E_FAIL) {
		Init(GetCodecId(), NULL);
	}

	return hr;
}

HRESULT CFFAudioDecoder::ReceiveData(CAtlArray<BYTE>& BuffOut, SampleFormat& samplefmt)
{
	HRESULT hr = E_FAIL;
	const int ret = avcodec_receive_frame(m_pAVCtx, m_pFrame);
	if (m_pAVCtx->channels > 8) {
		// sometimes avcodec_receive_frame() cannot identify the garbage and produces incorrect data.
		// this code does not solve the problem, it only reduces the likelihood of crash.
		// do it better!
	} else if (ret >= 0) {
		const size_t nSamples = m_pFrame->nb_samples;
		if (nSamples) {
			const WORD nChannels = m_pAVCtx->channels;
			samplefmt = (SampleFormat)m_pAVCtx->sample_fmt;
			const size_t monosize = nSamples * av_get_bytes_per_sample(m_pAVCtx->sample_fmt);
			BuffOut.SetCount(monosize * nChannels);

			if (av_sample_fmt_is_planar(m_pAVCtx->sample_fmt)) {
				BYTE* pOut = BuffOut.GetData();
				for (int ch = 0; ch < nChannels; ++ch) {
					memcpy(pOut, m_pFrame->extended_data[ch], monosize);
					pOut += monosize;
				}
			} else {
				memcpy(BuffOut.GetData(), m_pFrame->data[0], BuffOut.GetCount());
			}
		}

		hr = S_OK;
	}

	av_frame_unref(m_pFrame);
	return hr;
}

void CFFAudioDecoder::FlushBuffers()
{
	if (m_pParser) {
		av_parser_close(m_pParser);
		m_pParser = av_parser_init(GetCodecId());
	}

	if (m_pAVCtx && avcodec_is_open(m_pAVCtx)) {
		avcodec_flush_buffers(m_pAVCtx);
	}
}

void CFFAudioDecoder::StreamFinish()
{
	m_pAVCodec = NULL;
	
	av_parser_close(m_pParser);
	m_pParser = NULL;

	avcodec_free_context(&m_pAVCtx);

	av_frame_free(&m_pFrame);

	m_bNeedSyncpoint = false;
}

// RealAudio

HRESULT CFFAudioDecoder::ParseRealAudioHeader(const BYTE* extra, const int extralen)
{
	const uint8_t* fmt = extra + 4;
	uint16_t version = AV_RB16(fmt);
	fmt += 2;
	if (version == 3) {
		DbgLog((LOG_TRACE, 3, L"CFFAudioDecoder::ParseRealAudioHeader() : RealAudio Header version 3 unsupported"));
		return VFW_E_UNSUPPORTED_AUDIO;
	} else if (version == 4 || (version == 5 && extralen > 50)) {
		// main format block
		fmt += 2;  // word - unused (always 0)
		fmt += 4;  // byte[4] - .ra4/.ra5 signature
		fmt += 4;  // dword - unknown
		fmt += 2;  // word - Version2
		fmt += 4;  // dword - header size
		m_raData.flavor = AV_RB16(fmt);
		fmt += 2;  // word - codec flavor
		m_raData.coded_frame_size = AV_RB32(fmt);
		fmt += 4;  // dword - coded frame size
		fmt += 12; // byte[12] - unknown
		m_raData.sub_packet_h = AV_RB16(fmt);
		fmt += 2;  // word - sub packet h
		m_raData.audio_framesize = AV_RB16(fmt);
		fmt += 2;  // word - frame size
		m_raData.sub_packet_size = m_pAVCtx->block_align = AV_RB16(fmt);
		fmt += 2;  // word - subpacket size
		fmt += 2;  // word - unknown
		// 6 Unknown bytes in ver 5
		if (version == 5) {
			fmt += 6;
		}
		// Audio format block
		fmt += 8;
		// Tag info in v4
		if (version == 4) {
			int len = *fmt++;
			m_raData.deint_id = AV_RB32(fmt);
			fmt += len;
			len = *fmt++;
			fmt += len;
		} else if (version == 5) {
			m_raData.deint_id = AV_RB32(fmt);
			fmt += 4;
			fmt += 4;
		}
		fmt += 3;
		if (version == 5) {
			fmt++;
		}

		int ra_extralen = AV_RB32(fmt);
		if (ra_extralen > 0)  {
			m_pAVCtx->extradata_size = ra_extralen;
			m_pAVCtx->extradata      = (uint8_t*)av_mallocz(ra_extralen + AV_INPUT_BUFFER_PADDING_SIZE);
			memcpy((void*)m_pAVCtx->extradata, fmt + 4, ra_extralen);
		}
	} else {
		DbgLog((LOG_TRACE, 3, L"CFFAudioDecoder::ParseRealAudioHeader() : Unknown RealAudio Header version: %d", version));
		return VFW_E_UNSUPPORTED_AUDIO;
	}

	return S_OK;
}

HRESULT CFFAudioDecoder::RealPrepare(BYTE* p, int buffsize, CPaddedArray& BuffOut)
{
	if (m_raData.deint_id == MAKEFOURCC('r', 'n', 'e', 'g') || m_raData.deint_id == MAKEFOURCC('r', 'p', 'i', 's')) {

		int w   = m_raData.audio_framesize;
		int h   = m_raData.sub_packet_h;
		int len = w * h;

		if (buffsize >= len) {
			BuffOut.SetCount(len);
			BYTE* dest = BuffOut.GetData();

			int sps = m_raData.sub_packet_size;
			if (sps > 0 && m_raData.deint_id == MAKEFOURCC('r', 'n', 'e', 'g')) { // COOK and ATRAC codec
				for (int y = 0; y < h; y++) {
					for (int x = 0, w2 = w / sps; x < w2; x++) {
						memcpy(dest + sps * (h * x + ((h + 1) / 2) * (y & 1) + (y >> 1)), p, sps);
						p += sps;
					}
				}
				return S_OK;
			}

			if (m_raData.deint_id == MAKEFOURCC('r', 'p', 'i', 's')) { // SIPR codec
				memcpy(dest, p, len);

				// http://mplayerhq.hu/pipermail/mplayer-dev-eng/2002-August/010569.html
				static BYTE sipr_swaps[38][2] = {
					{0,  63}, {1,  22}, {2,  44}, {3,  90}, {5,  81}, {7,  31}, {8,  86}, {9,  58}, {10, 36}, {12, 68},
					{13, 39}, {14, 73}, {15, 53}, {16, 69}, {17, 57}, {19, 88}, {20, 34}, {21, 71}, {24, 46},
					{25, 94}, {26, 54}, {28, 75}, {29, 50}, {32, 70}, {33, 92}, {35, 74}, {38, 85}, {40, 56},
					{42, 87}, {43, 65}, {45, 59}, {48, 79}, {49, 93}, {51, 89}, {55, 95}, {61, 76}, {67, 83},
					{77, 80}
				};

				int bs = h * w * 2 / 96; // nibbles per subpacket
				for (int n = 0; n < 38; n++) {
					int i = bs * sipr_swaps[n][0];
					int o = bs * sipr_swaps[n][1];
					// swap nibbles of block 'i' with 'o'
					for (int j = 0; j < bs; j++) {
						int x = (i & 1) ? (dest[(i >> 1)] >> 4) : (dest[(i >> 1)] & 15);
						int y = (o & 1) ? (dest[(o >> 1)] >> 4) : (dest[(o >> 1)] & 15);
						if (o & 1) {
							dest[(o >> 1)] = (dest[(o >> 1)] & 0x0F) | (x << 4);
						} else {
							dest[(o >> 1)] = (dest[(o >> 1)] & 0xF0) | x;
						}
						if (i & 1) {
							dest[(i >> 1)] = (dest[(i >> 1)] & 0x0F) | (y << 4);
						} else {
							dest[(i >> 1)] = (dest[(i >> 1)] & 0xF0) | y;
						}
						++i;
						++o;
					}
				}
				return S_OK;
			}
		} else {
			return E_FAIL;
		}
	}
	return S_FALSE;
}

// Info

enum AVCodecID CFFAudioDecoder::GetCodecId()
{
	return m_pAVCtx ? m_pAVCtx->codec_id : AV_CODEC_ID_NONE;
}

const char* CFFAudioDecoder::GetCodecName()
{
	if (m_pAVCtx->codec_id == AV_CODEC_ID_DTS) {
		switch (m_pAVCtx->profile) {
			case FF_PROFILE_DTS_ES		: return "dts-es";
			case FF_PROFILE_DTS_96_24	: return "dts 96/24";
			case FF_PROFILE_DTS_HD_HRA	: return "dts-hd hra";
			case FF_PROFILE_DTS_HD_MA	: return "dts-hd ma";
			case FF_PROFILE_DTS_EXPRESS	: return "dts express";
		}
	}
	return (m_pAVCtx->codec_descriptor)->name;
}

SampleFormat CFFAudioDecoder::GetSampleFmt()
{
	return (SampleFormat)m_pAVCtx->sample_fmt;
}

DWORD CFFAudioDecoder::GetSampleRate()
{
	return (DWORD)m_pAVCtx->sample_rate;
}

WORD CFFAudioDecoder::GetChannels()
{
	return (WORD)m_pAVCtx->channels;
}

DWORD CFFAudioDecoder::GetChannelMask()
{
	return get_lav_channel_layout(m_pAVCtx->channel_layout);
}

WORD CFFAudioDecoder::GetCoddedBitdepth()
{
	// actual for lossless formats
	// for mp3, ac3, aac and other is 0
	return (WORD)m_pAVCtx->bits_per_coded_sample;
}
