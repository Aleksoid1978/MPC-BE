/*
 * (C) 2006-2014 see Authors.txt
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
#include <MMReg.h>
#include <moreuuids.h>
#include "MediaDescription.h"
#include "DSUtil.h"

CString GetMediaTypeDesc(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter)
{
	if (mts.IsEmpty()) {
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

	CAtlList<CString> Infos;
	if (pmt->majortype == MEDIATYPE_Video) {
		const VIDEOINFOHEADER *pVideoInfo	= NULL;
		const VIDEOINFOHEADER2 *pVideoInfo2	= NULL;

		BOOL bAdd = FALSE;

		if (pmt->formattype == FORMAT_VideoInfo) {
			pVideoInfo = GetFormatHelper(pVideoInfo, pmt);

		} else if (pmt->formattype == FORMAT_MPEGVideo) {
			Infos.AddTail(L"MPEG");

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
				Infos.AddTail(L"AVC (H.264)");
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('AMVC')) {
				bIsAVC = true;
				Infos.AddTail(L"MVC (Full)");
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('EMVC')) {
				bIsAVC = true;
				Infos.AddTail(L"MVC (Subset)");
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == 0) {
				Infos.AddTail(L"MPEG2");
				bIsMPEG2 = true;
				bAdd = TRUE;
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('HEVC') || pInfo->hdr.bmiHeader.biCompression == FCC('HVC1')) {
				Infos.AddTail(L"HEVC (H.265)");
				bIsHEVC = true;
				bAdd = TRUE;
			}

			if (bIsMPEG2) {
				Infos.AddTail(MPEG2_Profile[pInfo->dwProfile]);
			} else if (pInfo->dwProfile) {
				if (bIsAVC) {
					switch (pInfo->dwProfile) {
						case 44:
							Infos.AddTail(L"CAVLC Profile");
							break;
						case 66:
							Infos.AddTail(L"Baseline Profile");
							break;
						case 77:
							Infos.AddTail(L"Main Profile");
							break;
						case 88:
							Infos.AddTail(L"Extended Profile");
							break;
						case 100:
							Infos.AddTail(L"High Profile");
							break;
						case 110:
							Infos.AddTail(L"High 10 Profile");
							break;
						case 118:
							Infos.AddTail(L"Multiview High Profile");
							break;
						case 122:
							Infos.AddTail(L"High 4:2:2 Profile");
							break;
						case 244:
							Infos.AddTail(L"High 4:4:4 Profile");
							break;
						case 128:
							Infos.AddTail(L"Stereo High Profile");
							break;
						default:
							Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
							break;
					}
				} else if (bIsHEVC) {
					switch (pInfo->dwProfile) {
						case 1:
							Infos.AddTail(L"Main Profile");
							break;
						case 2:
							Infos.AddTail(L"Main/10 Profile");
							break;
						default:
							Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
							break;
					}
				} else {
					Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
				}
			}

			if (bIsMPEG2) {
				Infos.AddTail(MPEG2_Level[pInfo->dwLevel]);
			} else if (pInfo->dwLevel) {
				if (bIsAVC) {
					Infos.AddTail(FormatString(L"Level %1.1f", double(pInfo->dwLevel) / 10.0));
				} else if (bIsHEVC) {
					Infos.AddTail(FormatString(L"Level %d.%d", pInfo->dwLevel / 30, (pInfo->dwLevel % 30) / 3));
				} else {
					Infos.AddTail(FormatString(L"Level %d", pInfo->dwLevel));
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
				if (codecName.GetLength() > 0) {
					Infos.AddTail(codecName);
				}
			}
		}

		if (pVideoInfo2) {
			if (pVideoInfo2->bmiHeader.biWidth && pVideoInfo2->bmiHeader.biHeight) {
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo2->bmiHeader.biWidth, pVideoInfo2->bmiHeader.biHeight));
			}
			if (pVideoInfo2->AvgTimePerFrame) {
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo2->AvgTimePerFrame)));
			}
			if (pVideoInfo2->dwBitRate) {
				Infos.AddTail(FormatBitrate(pVideoInfo2->dwBitRate));
			}
		} else if (pVideoInfo) {
			if (pVideoInfo->bmiHeader.biWidth && pVideoInfo->bmiHeader.biHeight) {
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight));
			}
			if (pVideoInfo->AvgTimePerFrame) {
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo->AvgTimePerFrame)));
			}
			if (pVideoInfo->dwBitRate) {
				Infos.AddTail(FormatBitrate(pVideoInfo->dwBitRate));
			}
		}
	} else if (pmt->majortype == MEDIATYPE_Audio) {
		if (pmt->formattype == FORMAT_WaveFormatEx) {
			const WAVEFORMATEX *pInfo = GetFormatHelper(pInfo, pmt);

			if (pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
				Infos.AddTail(L"DVD LPCM");
			} else if (pmt->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
				const WAVEFORMATEX_HDMV_LPCM *pInfoHDMV = GetFormatHelper(pInfoHDMV, pmt);
				UNREFERENCED_PARAMETER(pInfoHDMV);
				Infos.AddTail(L"HDMV LPCM");
			}
			if (pmt->subtype == MEDIASUBTYPE_DOLBY_DDPLUS) {
				Infos.AddTail(L"Dolby Digital Plus");
			} else {
				switch (pInfo->wFormatTag) {
					case WAVE_FORMAT_MPEG: {
						const MPEG1WAVEFORMAT* pInfoMPEG1 = GetFormatHelper(pInfoMPEG1, pmt);

						int layer = GetHighestBitSet32(pInfoMPEG1->fwHeadLayer) + 1;
						Infos.AddTail(FormatString(L"MPEG1 - Layer %d", layer));
					}
					break;
					default: {
						CString codecName = CMediaTypeEx::GetAudioCodecName(pmt->subtype, pInfo->wFormatTag);
						if (codecName.GetLength() > 0) {
							Infos.AddTail(codecName);
						}
					}
					break;
				}
			}

			if (pInfo->nSamplesPerSec) {
				Infos.AddTail(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec)/1000.0));
			}
			if (pInfo->nChannels) {
				Infos.AddTail(FormatString(L"%d chn", pInfo->nChannels));
			}
			if (pInfo->wBitsPerSample) {
				Infos.AddTail(FormatString(L"%d bit", pInfo->wBitsPerSample));
			}
			if (pInfo->nAvgBytesPerSec) {
				Infos.AddTail(FormatBitrate(pInfo->nAvgBytesPerSec * 8));
			}
		} else if (pmt->formattype == FORMAT_VorbisFormat) {
			const VORBISFORMAT *pInfo = GetFormatHelper(pInfo, pmt);

			Infos.AddTail(CMediaTypeEx::GetAudioCodecName(pmt->subtype, 0));

			if (pInfo->nSamplesPerSec) {
				Infos.AddTail(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec)/1000.0));
			}
			if (pInfo->nChannels) {
				Infos.AddTail(FormatString(L"%d chn", pInfo->nChannels));
			}

			if (pInfo->nAvgBitsPerSec) {
				Infos.AddTail(FormatString(L"%d bit", pInfo->nAvgBitsPerSec));
			}
			if (pInfo->nAvgBitsPerSec) {
				Infos.AddTail(FormatBitrate(pInfo->nAvgBitsPerSec * 8));
			}
		} else if (pmt->formattype == FORMAT_VorbisFormat2) {
			const VORBISFORMAT2 *pInfo = GetFormatHelper(pInfo, pmt);

			Infos.AddTail(CMediaTypeEx::GetAudioCodecName(pmt->subtype, 0));

			if (pInfo->SamplesPerSec) {
				Infos.AddTail(FormatString(L"%.1f kHz", double(pInfo->SamplesPerSec)/1000.0));
			}
			if (pInfo->Channels) {
				Infos.AddTail(FormatString(L"%d chn", pInfo->Channels));
			}
		}
	}

	if (!Infos.IsEmpty()) {
		CString Ret = pName;
		Ret += " (";

		bool bFirst = true;

		for (POSITION pos = Infos.GetHeadPosition(); pos; Infos.GetNext(pos)) {
			const CString& String = Infos.GetAt(pos);

			if (bFirst) {
				Ret += String;
			} else {
				Ret += L", " + String;
			}

			bFirst = false;
		}

		Ret += ')';

		return Ret;
	}

	return pName;
}