/*
 * (C) 2014-2025 see Authors.txt
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
#include "MpaDecFilter.h"
#include "DSUtil/MP4AudioDecoderConfig.h"

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
	#include <ExtLib/ffmpeg/libavcodec/avcodec.h>
	#include <ExtLib/ffmpeg/libavutil/intreadwrite.h>
	#include <ExtLib/ffmpeg/libavutil/opt.h>
}
#pragma warning(pop)

#include <wmcodecdsp.h>
#include <moreuuids.h>
#include "DSUtil/AudioParser.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/ffmpeg_log.h"
#include "DSUtil/GolombBuffer.h"
#include "filters/Lock.h"

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
	// AC-4
	{ &MEDIASUBTYPE_DOLBY_AC4,         AV_CODEC_ID_AC4    },
	// ATRAC3, ATRAC3plus
	{ &MEDIASUBTYPE_ATRAC3,            AV_CODEC_ID_ATRAC3 },
	{ &MEDIASUBTYPE_ATRAC3plus,        AV_CODEC_ID_ATRAC3P},
	// ATRAC9
	{ &MEDIASUBTYPE_ATRAC9,            AV_CODEC_ID_ATRAC9 },
	// DTS
	{ &MEDIASUBTYPE_DTS,               AV_CODEC_ID_DTS },
	{ &MEDIASUBTYPE_DTS2,              AV_CODEC_ID_DTS },
	// FLAC
	{ &MEDIASUBTYPE_FLAC_FRAMED,       AV_CODEC_ID_FLAC },
	{ &MEDIASUBTYPE_FLAC,              AV_CODEC_ID_FLAC },
	// QDesign Music
	{ &MEDIASUBTYPE_QDMC,              AV_CODEC_ID_QDMC },
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
	// Intel Music Coder
	{ &MEDIASUBTYPE_INTEL_MUSIC,       AV_CODEC_ID_IMC },
	// Indeo Audio
	{ &MEDIASUBTYPE_INDEO_AUDIO,       AV_CODEC_ID_IAC },
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
	// DST
	{ &MEDIASUBTYPE_DST,               AV_CODEC_ID_DST },
	// A-law/mu-law
	{ &MEDIASUBTYPE_ALAW,              AV_CODEC_ID_PCM_ALAW  },
	{ &MEDIASUBTYPE_MULAW,             AV_CODEC_ID_PCM_MULAW },
	// AES3
	{ &MEDIASUBTYPE_AES3,              AV_CODEC_ID_S302M },
	// G.726 ADPCM
	{ &MEDIASUBTYPE_G726_ADPCM,        AV_CODEC_ID_ADPCM_G726 },
	// other
	{ &MEDIASUBTYPE_ON2VP7_AUDIO,      AV_CODEC_ID_ON2AVC },
	{ &MEDIASUBTYPE_ON2VP6_AUDIO,      AV_CODEC_ID_ON2AVC },

	{ &MEDIASUBTYPE_None,              AV_CODEC_ID_NONE },
};

enum AVCodecID FindCodec(const GUID subtype)
{
	static auto index = std::size(ffAudioCodecs) - 1;

	if (subtype == *ffAudioCodecs[index].clsMinorType) {
		return ffAudioCodecs[index].nFFCodec;
	}

	for (unsigned i = 0; i < std::size(ffAudioCodecs); i++) {
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

	return nullptr;
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

CFFAudioDecoder::CFFAudioDecoder(CMpaDecFilter* pFilter)
	: m_pFilter(pFilter)
{
#ifdef DEBUG_OR_LOG
	av_log_set_callback(ff_log);
#else
	av_log_set_callback(nullptr);
#endif
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

static bool ParseVorbisTag(const CString& field_name, const CString& vorbisTag, CString& tagValue) {
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
	AVCodecID codec_id = AV_CODEC_ID_NONE;
	unsigned int codec_tag = 0;
	int samplerate     = 0;
	int channels       = 0;
	int bitdeph        = 0;
	int block_align    = 0;
	int64_t bitrate    = 0;
	BYTE* extradata    = nullptr;
	unsigned extralen  = 0;

	AVChannelLayout ch_layout = {};

	m_bNeedReinit      = false;
	m_bNeedMix         = false;

	if (codecID == AV_CODEC_ID_NONE || mediaType == nullptr) {
		if (m_pAVCodec == nullptr || m_pAVCtx == nullptr || m_pAVCtx->codec_id == AV_CODEC_ID_NONE) {
			return false;
		}

		// use the previous info
		codec_id	= m_pAVCtx->codec_id;
		samplerate	= m_pAVCtx->sample_rate;
		channels	= m_pAVCtx->ch_layout.nb_channels;
		ch_layout   = m_pAVCtx->ch_layout;
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
			codec_tag   = wfex->wFormatTag;
			samplerate  = wfex->nSamplesPerSec;
			channels    = wfex->nChannels;
			bitdeph     = wfex->wBitsPerSample;
			block_align = wfex->nBlockAlign;
			bitrate     = wfex->nAvgBytesPerSec * 8;
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

		getExtraData(mediaType->Format(), &mediaType->formattype, mediaType->cbFormat, nullptr, &extralen);
		if (extralen) {
			extradata = (uint8_t*)av_mallocz(extralen + AV_INPUT_BUFFER_PADDING_SIZE);
			getExtraData(mediaType->Format(), &mediaType->formattype, mediaType->cbFormat, extradata, nullptr);
		}
	}

	if (m_pAVCodec) {
		StreamFinish();
	}

	if (codec_id == AV_CODEC_ID_AAC && extralen) {
		CMP4AudioDecoderConfig MP4AudioDecoderConfig;
		if (MP4AudioDecoderConfig.Parse(extradata, extralen) && MP4AudioDecoderConfig.m_ObjectType == AOT_USAC) {
			m_pAVCodec = avcodec_find_decoder_by_name("libfdk_aac");
		}
	}

	if (!m_pAVCodec) {
		m_pAVCodec = avcodec_find_decoder(codec_id);
	}
	if (m_pAVCodec) {
		m_pAVCtx = avcodec_alloc_context3(m_pAVCodec);
		CheckPointer(m_pAVCtx, false);

		if (!ch_layout.nb_channels) {
			av_channel_layout_from_mask(&ch_layout, GetDefChannelMask(channels));
		}

		m_pAVCtx->codec_id              = codec_id;
		m_pAVCtx->codec_tag             = codec_tag;
		m_pAVCtx->sample_rate           = samplerate;
		m_pAVCtx->ch_layout             = ch_layout;
		m_pAVCtx->bits_per_coded_sample = bitdeph;
		m_pAVCtx->block_align           = block_align;
		m_pAVCtx->bit_rate              = bitrate;
		m_pAVCtx->err_recognition       = 0;
		m_pAVCtx->thread_count          = 1;
		m_pAVCtx->thread_type           = 0;

		AVDictionary* options = nullptr;
		if (m_bStereoDownmix) { // works to AC3, TrueHD, DTS
			int ret = av_dict_set(&options, "downmix", "stereo", 0);
			DLogIf(ret < 0, L"CFFAudioDecoder::Init() : Set downmix to stereo FAILED!");
		}

		m_pParser = av_parser_init(codec_id);

		memset(&m_raData, 0, sizeof(m_raData));

		if (extradata && extralen) {
			if (codec_id == AV_CODEC_ID_COOK || codec_id == AV_CODEC_ID_ATRAC3 || codec_id == AV_CODEC_ID_SIPR) {
				if (extralen >= 4 && GETU32(extradata) == MAKEFOURCC('.', 'r', 'a', 0xfd)) {
					HRESULT hr = ParseRealAudioHeader(extradata, extralen);
					av_freep(&extradata);
					extralen = 0;
					if (FAILED(hr)) {
						return false;
					}
					if (codec_id == AV_CODEC_ID_SIPR) {
						if (m_raData.flavor > 3) {
							DLog(L"CFFAudioDecoder::Init() : Invalid SIPR flavor (%d)", m_raData.flavor);
							return false;
						}
						static BYTE sipr_subpk_size[4] = { 29, 19, 37, 20 };
						m_pAVCtx->block_align = sipr_subpk_size[m_raData.flavor];
					}
				} else {
					// Try without any processing?
					m_pAVCtx->extradata_size = extralen;
					m_pAVCtx->extradata = extradata;
				}
			} else {
				m_pAVCtx->extradata_size = extralen;
				m_pAVCtx->extradata = extradata;
			}
		}

		avcodec_lock;
		if (avcodec_open2(m_pAVCtx, m_pAVCodec, &options) >= 0) {
			m_pFrame = av_frame_alloc();
			m_pPacket = av_packet_alloc();
		}
		avcodec_unlock;

		if (options) {
			av_dict_free(&options);
		}
	}

	if (!m_pFrame || !m_pPacket) {
		StreamFinish();
		return false;
	}

	if (m_pAVCtx->codec_id == AV_CODEC_ID_FLAC && m_pAVCtx->extradata_size > (4+4+34) && GETU32(m_pAVCtx->extradata) == FCC('fLaC')) {
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
						std::vector<BYTE> comment(comment_lenght + 1);
						gb.ReadBuffer(comment.data(), comment_lenght);
						CString vorbisTag = AltUTF8ToWStr((LPCSTR)comment.data());

						CString tagValue;
						if (!vorbisTag.IsEmpty() && ParseVorbisTag(L"WAVEFORMATEXTENSIBLE_CHANNEL_MASK", vorbisTag, tagValue)) {
							uint64_t channel_layout = wcstol(tagValue, nullptr, 0);
							if (channel_layout && av_popcount64(channel_layout) == m_pAVCtx->ch_layout.nb_channels) {
								av_channel_layout_from_mask(&m_pAVCtx->ch_layout, channel_layout);
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
	else if ((codec_id == AV_CODEC_ID_AAC || codec_id == AV_CODEC_ID_AAC_LATM) && m_pAVCtx->ch_layout.nb_channels == 24) {
		m_bNeedMix = true;
		m_MixerChannels = 8;
		m_MixerChannelLayout = GetDefChannelMask(8);
		m_Mixer.UpdateInput(SAMPLE_FMT_FLTP, m_pAVCtx->ch_layout.u.mask, m_pAVCtx->sample_rate);
		m_Mixer.UpdateOutput(SAMPLE_FMT_FLT, m_MixerChannelLayout, m_pAVCtx->sample_rate);
	}
	else if (codec_id == AV_CODEC_ID_VORBIS) {
		// Strange hack to correct Vorbis decoding after https://github.com/FFmpeg/FFmpeg/commit/8fc2dedfe6e8fcc58dd052bf3b85cd4754133b17
		m_pPacket->pts = 0;
	}

	m_bNeedSyncpoint = (m_raData.deint_id != 0);

	return true;
}

void CFFAudioDecoder::SetDRC(bool bDRC)
{
	if (m_pAVCtx) {
		AVCodecID codec_id = m_pAVCtx->codec_id;
		if (codec_id == AV_CODEC_ID_AC3 || codec_id == AV_CODEC_ID_EAC3) {
			av_opt_set_double(m_pAVCtx, "drc_scale", bDRC ? 1.0f : 0.0f, AV_OPT_SEARCH_CHILDREN);
		}
	}
}

void CFFAudioDecoder::SetStereoDownmix(bool bStereoDownmix)
{
	if (bStereoDownmix != m_bStereoDownmix) {
		m_bStereoDownmix = bStereoDownmix;
		AVCodecID codec = GetCodecId();
		if (codec == AV_CODEC_ID_AC3 || codec == AV_CODEC_ID_EAC3 || codec == AV_CODEC_ID_DTS || codec == AV_CODEC_ID_TRUEHD) {
			// reinit for supported codec only
			m_bNeedReinit = true;
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
	if (m_pParser) {
		BYTE* pOut = nullptr;
		int pOut_size = 0;

		int used_bytes = av_parser_parse2(m_pParser, m_pAVCtx, &pOut, &pOut_size, p, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (used_bytes < 0) {
			DLog(L"CFFAudioDecoder::Decode() : audio parsing failed (ret: %d)", -used_bytes);
			return E_FAIL;
		} else if (used_bytes == 0 && pOut_size == 0) {
			DLog(L"CFFAudioDecoder::Decode() : could not process buffer while parsing");
		}

		if (used_bytes >= pOut_size && m_pFilter->m_bUpdateTimeCache) {
			m_pFilter->UpdateCacheTimeStamp();
		}

		if (out_size) {
			*out_size = used_bytes;
		}

		hr = S_FALSE;

		if (pOut_size > 0) {
			m_pPacket->data = pOut;
			m_pPacket->size = pOut_size;
			m_pPacket->dts  = m_pFilter->m_rtStartInputCache;

			ret = avcodec_send_packet(m_pAVCtx, m_pPacket);
		}
	} else {
		m_pPacket->data = p;
		m_pPacket->size = size;
		m_pPacket->dts  = m_pFilter->m_rtStartInput;

		ret = avcodec_send_packet(m_pAVCtx, m_pPacket);
	}

	av_packet_unref(m_pPacket);

	if (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		if (!m_pParser && out_size) {
			*out_size = size;
		}

		hr = S_OK;
	}

	if (hr == E_FAIL) {
		Init(GetCodecId(), nullptr);
	}

	return hr;
}

HRESULT CFFAudioDecoder::ReceiveData(std::vector<BYTE>& BuffOut, size_t& outputSize, SampleFormat& samplefmt, REFERENCE_TIME& rtStart)
{
	HRESULT hr = E_FAIL;
	const int ret = avcodec_receive_frame(m_pAVCtx, m_pFrame);
	if (ret >= 0) {
		rtStart = m_pFrame->pkt_dts;
		m_pFilter->ClearCacheTimeStamp();

		outputSize = 0;

		const size_t nSamples = m_pFrame->nb_samples;
		if (nSamples) {
			const WORD nChannels = m_pAVCtx->ch_layout.nb_channels;
			samplefmt = (SampleFormat)m_pAVCtx->sample_fmt;
			const size_t monosize = nSamples * av_get_bytes_per_sample(m_pAVCtx->sample_fmt);

			static std::vector<BYTE> mixBuffer;
			auto& bufferOutput = m_bNeedMix ? mixBuffer : BuffOut;

			outputSize = monosize * nChannels;
			if (outputSize > bufferOutput.size()) {
				bufferOutput.resize(outputSize);
			}

			if (av_sample_fmt_is_planar(m_pAVCtx->sample_fmt)) {
				BYTE* pOut = bufferOutput.data();
				for (int ch = 0; ch < nChannels; ++ch) {
					memcpy(pOut, m_pFrame->extended_data[ch], monosize);
					pOut += monosize;
				}
			} else {
				memcpy(bufferOutput.data(), m_pFrame->data[0], outputSize);
			}

			if (m_bNeedMix) {
				samplefmt = SAMPLE_FMT_FLT;
				auto out_samples = m_Mixer.CalcOutSamples(nSamples);
				outputSize = static_cast<size_t>(out_samples) * m_MixerChannels * av_get_bytes_per_sample(m_pAVCtx->sample_fmt);
				if (outputSize > BuffOut.size()) {
					BuffOut.resize(outputSize);
				}
				out_samples = m_Mixer.Mixing(BuffOut.data(), out_samples, mixBuffer.data(), nSamples);
				if (!out_samples) {
					av_frame_unref(m_pFrame);
					return E_INVALIDARG;
				}
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
		m_pFilter->m_bUpdateTimeCache = TRUE;
	}

	if (m_pAVCtx && avcodec_is_open(m_pAVCtx)) {
		avcodec_flush_buffers(m_pAVCtx);
	}
}

void CFFAudioDecoder::StreamFinish()
{
	m_pAVCodec = nullptr;

	av_parser_close(m_pParser);
	m_pParser = nullptr;

	avcodec_free_context(&m_pAVCtx);

	av_frame_free(&m_pFrame);
	av_packet_free(&m_pPacket);

	m_bNeedSyncpoint = false;
}

// RealAudio

HRESULT CFFAudioDecoder::ParseRealAudioHeader(const BYTE* extra, const int extralen)
{
	const uint8_t* fmt = extra + 4;
	int version = AV_RB16(fmt);
	fmt += 2;
	if (version == 3) {
		DLog(L"CFFAudioDecoder::ParseRealAudioHeader() : RealAudio Header version 3 unsupported");
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
			m_raData.deint_id = AV_RN32(fmt);
			fmt += len;
			len = *fmt++;
			fmt += len;
		} else if (version == 5) {
			m_raData.deint_id = AV_RN32(fmt);
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
		DLog(L"CFFAudioDecoder::ParseRealAudioHeader() : Unknown RealAudio Header version: %d", version);
		return VFW_E_UNSUPPORTED_AUDIO;
	}

	return S_OK;
}

HRESULT CFFAudioDecoder::RealPrepare(BYTE* p, int buffsize, CPaddedBuffer& BuffOut)
{
	if (m_raData.deint_id == FCC('genr') || m_raData.deint_id == FCC('sipr')) {

		int w   = m_raData.audio_framesize;
		int h   = m_raData.sub_packet_h;
		int len = w * h;

		if (buffsize >= len) {
			BuffOut.Resize(len);
			BYTE* dest = BuffOut.Data();

			int sps = m_raData.sub_packet_size;
			if (sps > 0 && m_raData.deint_id == FCC('genr')) { // COOK and ATRAC codec
				for (int y = 0; y < h; y++) {
					for (int x = 0, w2 = w / sps; x < w2; x++) {
						memcpy(dest + sps * (h * x + ((h + 1) / 2) * (y & 1) + (y >> 1)), p, sps);
						p += sps;
					}
				}
				return S_OK;
			}

			if (m_raData.deint_id == FCC('sipr')) { // SIPR codec
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
			case AV_PROFILE_DTS_ES           : return "dts-es";
			case AV_PROFILE_DTS_96_24        : return "dts 96/24";
			case AV_PROFILE_DTS_HD_HRA       : return "dts-hd hra";
			case AV_PROFILE_DTS_HD_MA        : return "dts-hd ma";
			case AV_PROFILE_DTS_HD_MA_X      : return "dts-hd ma + dts:x";
			case AV_PROFILE_DTS_HD_MA_X_IMAX : return "dts-hd ma + dts:x imax";
			case AV_PROFILE_DTS_EXPRESS      : return "dts express";
		}
	} else if (m_pAVCtx->codec_id == AV_CODEC_ID_EAC3 && m_pAVCtx->profile == AV_PROFILE_EAC3_DDP_ATMOS) {
		return "eac3 + atmos";
	} else if (m_pAVCtx->codec_id == AV_CODEC_ID_TRUEHD && m_pAVCtx->profile == AV_PROFILE_TRUEHD_ATMOS) {
		return "truehd + atmos";
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
	return m_bNeedMix ? (WORD)m_MixerChannels : (WORD)m_pAVCtx->ch_layout.nb_channels;
}

DWORD CFFAudioDecoder::GetChannelMask()
{
	return get_lav_channel_layout(m_bNeedMix ? m_MixerChannelLayout : m_pAVCtx->ch_layout.u.mask);
}

WORD CFFAudioDecoder::GetCoddedBitdepth()
{
	// actual for lossless formats
	// for mp3, ac3, aac and other is 0
	return (WORD)m_pAVCtx->bits_per_coded_sample;
}
