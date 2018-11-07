/*
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

#include "stdafx.h"
#include <ks.h>
#include <ksmedia.h>
#include <MMReg.h>
#include <moreuuids.h>
#include "MediaDescription.h"
#include "AudioParser.h"
#include "DSUtil.h"

CString GetMediaTypeDesc(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter)
{
	if (mts.empty()) {
		return pName;
	}

	CLSID clSID;
	HRESULT hr = pFilter->GetClassID(&clSID);
	if (clSID == GUIDFromCString(L"{1365BE7A-C86A-473C-9A41-C0A6E82C9FA3}") || clSID == GUIDFromCString(L"{DC257063-045F-4BE2-BD5B-E12279C464F0}")) {
		// skip MPEGSource/Splitter ...
		return pName;
	}

	return GetMediaTypeDesc(&mts[0], pName);
}

CString GetMediaTypeDesc(const CMediaType* pmt, LPCWSTR pName)
{
	if (!pmt) {
		return pName;
	}

	std::list<CString> Infos;
	if (pmt->majortype == MEDIATYPE_Video) {
		const VIDEOINFOHEADER *pVideoInfo	= nullptr;
		const VIDEOINFOHEADER2 *pVideoInfo2	= nullptr;

		BOOL bAdd = FALSE;

		if (pmt->formattype == FORMAT_VideoInfo) {
			pVideoInfo = GetFormatHelper(pVideoInfo, pmt);

		} else if (pmt->formattype == FORMAT_MPEGVideo) {
			Infos.emplace_back(L"MPEG");

			const MPEG1VIDEOINFO *pInfo = GetFormatHelper(pInfo, pmt);
			pVideoInfo = &pInfo->hdr;

			bAdd = TRUE;
		} else if (pmt->formattype == FORMAT_MPEG2_VIDEO) {
			const MPEG2VIDEOINFO *pInfo = GetFormatHelper(pInfo, pmt);
			pVideoInfo2 = &pInfo->hdr;

			bool bIsAVC		= false;
			bool bIsHEVC	= false;
			bool bIsMPEG2	= false;

			if (pInfo->hdr.bmiHeader.biCompression == FCC('AVC1') || pInfo->hdr.bmiHeader.biCompression == FCC('H264')) {
				bIsAVC = true;
				Infos.emplace_back(L"AVC (H.264)");
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('AMVC') || pInfo->hdr.bmiHeader.biCompression == FCC('MVC1')) {
				bIsAVC = true;
				Infos.emplace_back(L"MVC Full (H.264)");
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('EMVC')) {
				bIsAVC = true;
				Infos.emplace_back(L"MVC Subset (H.264)");
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == 0) {
				Infos.emplace_back(L"MPEG2");
				bIsMPEG2 = true;
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('HEVC') || pInfo->hdr.bmiHeader.biCompression == FCC('HVC1')) {
				Infos.emplace_back(L"HEVC (H.265)");
				bIsHEVC = true;
				bAdd = TRUE;
			}

			if (bIsMPEG2) {
				Infos.emplace_back(MPEG2_Profile[pInfo->dwProfile]);
			} else if (pInfo->dwProfile) {
				if (bIsAVC) {
					switch (pInfo->dwProfile) {
						case 44:
							Infos.emplace_back(L"CAVLC Profile");
							break;
						case 66:
							Infos.emplace_back(L"Baseline Profile");
							break;
						case 77:
							Infos.emplace_back(L"Main Profile");
							break;
						case 88:
							Infos.emplace_back(L"Extended Profile");
							break;
						case 100:
							Infos.emplace_back(L"High Profile");
							break;
						case 110:
							Infos.emplace_back(L"High 10 Profile");
							break;
						case 118:
							Infos.emplace_back(L"Multiview High Profile");
							break;
						case 122:
							Infos.emplace_back(L"High 4:2:2 Profile");
							break;
						case 128:
							Infos.emplace_back(L"Stereo High Profile");
							break;
						case 244:
							Infos.emplace_back(L"High 4:4:4 Profile");
							break;
						default:
							Infos.emplace_back(FormatString(L"Profile %u", pInfo->dwProfile));
							break;
					}
				} else if (bIsHEVC) {
					switch (pInfo->dwProfile) {
						case 1:
							Infos.emplace_back(L"Main Profile");
							break;
						case 2:
							Infos.emplace_back(L"Main/10 Profile");
							break;
						default:
							Infos.emplace_back(FormatString(L"Profile %u", pInfo->dwProfile));
							break;
					}
				} else {
					Infos.emplace_back(FormatString(L"Profile %u", pInfo->dwProfile));
				}
			}

			if (bIsMPEG2) {
				Infos.emplace_back(MPEG2_Level[pInfo->dwLevel]);
			} else if (pInfo->dwLevel) {
				if (bIsAVC) {
					Infos.emplace_back(FormatString(L"Level %1.1f", double(pInfo->dwLevel) / 10.0));
				} else if (bIsHEVC) {
					Infos.emplace_back(FormatString(L"Level %u.%u", pInfo->dwLevel / 30, (pInfo->dwLevel % 30) / 3));
				} else {
					Infos.emplace_back(FormatString(L"Level %u", pInfo->dwLevel));
				}
			}
		} else if (pmt->formattype == FORMAT_VIDEOINFO2) {
			const VIDEOINFOHEADER2 *pInfo = GetFormatHelper(pInfo, pmt);
			pVideoInfo2 = pInfo;
		}

		if (!bAdd) {
			BITMAPINFOHEADER bih;
			bool fBIH = ExtractBIH(pmt, &bih);
			if (fBIH) {
				CString codecName = CMediaTypeEx::GetVideoCodecName(pmt->subtype, bih.biCompression);
				if (!codecName.IsEmpty()) {
					Infos.emplace_back(codecName);
				}
			}
		}

		if (pVideoInfo2) {
			if (pVideoInfo2->bmiHeader.biWidth && pVideoInfo2->bmiHeader.biHeight) {
				Infos.emplace_back(FormatString(L"%dx%d", pVideoInfo2->bmiHeader.biWidth, pVideoInfo2->bmiHeader.biHeight));
			}
			if (pVideoInfo2->AvgTimePerFrame) {
				Infos.emplace_back(FormatString(L"%.3f fps", 10000000.0 / pVideoInfo2->AvgTimePerFrame));
			}
			if (pVideoInfo2->dwBitRate) {
				Infos.emplace_back(FormatBitrate(pVideoInfo2->dwBitRate));
			}
		} else if (pVideoInfo) {
			if (pVideoInfo->bmiHeader.biWidth && pVideoInfo->bmiHeader.biHeight) {
				Infos.emplace_back(FormatString(L"%dx%d", pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight));
			}
			if (pVideoInfo->AvgTimePerFrame) {
				Infos.emplace_back(FormatString(L"%.3f fps", 10000000.0 / pVideoInfo->AvgTimePerFrame));
			}
			if (pVideoInfo->dwBitRate) {
				Infos.emplace_back(FormatBitrate(pVideoInfo->dwBitRate));
			}
		}
	} else if (pmt->majortype == MEDIATYPE_Audio) {
		if (pmt->formattype == FORMAT_WaveFormatEx) {
			const WAVEFORMATEX *pInfo = GetFormatHelper(pInfo, pmt);

			if (pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
				Infos.emplace_back(L"DVD LPCM");
			} else if (pmt->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
				const WAVEFORMATEX_HDMV_LPCM *pInfoHDMV = GetFormatHelper(pInfoHDMV, pmt);
				UNREFERENCED_PARAMETER(pInfoHDMV);
				Infos.emplace_back(L"HDMV LPCM");
			}
			if (pmt->subtype == MEDIASUBTYPE_DOLBY_DDPLUS) {
				Infos.emplace_back(L"Dolby Digital Plus");
			} else {
				if (pInfo->wFormatTag == WAVE_FORMAT_MPEG && pmt->cbFormat >= sizeof(MPEG1WAVEFORMAT)) {
					const MPEG1WAVEFORMAT* pInfoMPEG1 = GetFormatHelper(pInfoMPEG1, pmt);

					int layer = GetHighestBitSet32(pInfoMPEG1->fwHeadLayer) + 1;
					Infos.emplace_back(FormatString(L"MPEG1 - Layer %d", layer));
				} else {
					CString codecName = CMediaTypeEx::GetAudioCodecName(pmt->subtype, pInfo->wFormatTag);
					if (!codecName.IsEmpty()) {
						if (codecName == L"DTS" && pInfo->cbSize == 1) {
							const auto profile = ((BYTE *)(pInfo + 1))[0];
							switch (profile) {
								case DCA_PROFILE_HD_HRA:
									codecName = L"DTS-HD HRA";
									break;
								case DCA_PROFILE_HD_MA:
									codecName = L"DTS-HD MA";
									break;
								case DCA_PROFILE_EXPRESS:
									codecName = L"DTS Express";
									break;
							}
						}
						Infos.emplace_back(codecName);
					}
				}
			}

			if (pInfo->nSamplesPerSec) {
				Infos.emplace_back(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec) / 1000.0));
			}
			if (pInfo->nChannels) {
				DWORD layout = 0;
				if (IsWaveFormatExtensible(pInfo)) {
					const WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)pInfo;
					layout = wfex->dwChannelMask;
				} else {
					layout = GetDefChannelMask(pInfo->nChannels);
				}
				BYTE lfe = 0;
				WORD nChannels = pInfo->nChannels;
				if (layout & SPEAKER_LOW_FREQUENCY) {
					nChannels--;
					lfe = 1;
				}
				Infos.emplace_back(FormatString(L"%u.%u chn", nChannels, lfe));
			}
			if (pInfo->wBitsPerSample) {
				Infos.emplace_back(FormatString(L"%u bit", pInfo->wBitsPerSample));
			}
			if (pInfo->nAvgBytesPerSec) {
				Infos.emplace_back(FormatBitrate(pInfo->nAvgBytesPerSec * 8));
			}
		} else if (pmt->formattype == FORMAT_VorbisFormat) {
			const VORBISFORMAT *pInfo = GetFormatHelper(pInfo, pmt);

			Infos.emplace_back(CMediaTypeEx::GetAudioCodecName(pmt->subtype, 0));

			if (pInfo->nSamplesPerSec) {
				Infos.emplace_back(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec) / 1000.0));
			}
			if (pInfo->nChannels) {
				Infos.emplace_back(FormatString(L"%u chn", pInfo->nChannels));
			}

			if (pInfo->nAvgBitsPerSec) {
				Infos.emplace_back(FormatString(L"%u bit", pInfo->nAvgBitsPerSec));
			}
			if (pInfo->nAvgBitsPerSec) {
				Infos.emplace_back(FormatBitrate(pInfo->nAvgBitsPerSec * 8));
			}
		} else if (pmt->formattype == FORMAT_VorbisFormat2) {
			const VORBISFORMAT2 *pInfo = GetFormatHelper(pInfo, pmt);

			Infos.emplace_back(CMediaTypeEx::GetAudioCodecName(pmt->subtype, 0));

			if (pInfo->SamplesPerSec) {
				Infos.emplace_back(FormatString(L"%.1f kHz", double(pInfo->SamplesPerSec) / 1000.0));
			}
			if (pInfo->Channels) {
				Infos.emplace_back(FormatString(L"%u chn", pInfo->Channels));
			}
		}
	}

	if (!Infos.empty()) {
		CString Ret = pName;
		Ret += L" (";

		bool bFirst = true;

		for (const auto& str : Infos) {
			if (bFirst) {
				Ret += str;
			} else {
				Ret += L", " + str;
			}

			bFirst = false;
		}

		Ret += L')';

		return Ret;
	}

	return pName;
}
