/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2019 see Authors.txt
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

#include "../BaseSplitter/BaseSplitter.h"

#define FlvSplitterName L"MPC FLV Splitter"
#define FlvSourceName   L"MPC FLV Source"

class __declspec(uuid("47E792CF-0BBE-4F7A-859C-194B0768650A"))
	CFLVSplitterFilter : public CBaseSplitterFilter
{
	UINT32 m_DataOffset;
	UINT32 m_SyncOffset;

	UINT32 m_TimeStampOffset;
	bool   m_DetectWrongTimeStamp;

	bool Sync(__int64& pos);

	struct VideoTweak {
		BYTE x = 0;
		BYTE y = 0;
	};

	bool ReadTag(VideoTweak& t);

	struct Tag {
		UINT32 PreviousTagSize = 0;
		BYTE   TagType = 0;
		UINT32 DataSize = 0;
		UINT32 TimeStamp = 0;
		UINT32 StreamID = 0;
	};

	bool ReadTag(Tag& t, bool bCheckOnly = false);

	struct AudioTag {
		BYTE SoundFormat = 0;
		BYTE SoundRate = 0;
		BYTE SoundSize = 0;
		BYTE SoundType = 0;
	};

	bool ReadTag(AudioTag& at);

	struct VideoTag {
		BYTE   FrameType = 0;
		BYTE   CodecID = 0;
		BYTE   AVCPacketType = 0;
		UINT32 tsOffset = 0;
	};

	bool ReadTag(VideoTag& vt);

	//struct MetaInfo {
	//	bool    parsed;
	//	double  duration;
	//	double  videodatarate;
	//	double  audiodatarate;
	//	double  videocodecid;
	//	double  audiocodecid;
	//	double  audiosamplerate;
	//	double  audiosamplesize;
	//	bool    stereo;
	//	double  width;
	//	double  height;
	//	int     HM_compatibility;
	//	double  *times;
	//	__int64 *filepositions;
	//	int     keyframenum;
	//};
	//MetaInfo meta;

	enum AMF_DATA_TYPE {
		AMF_DATA_TYPE_EMPTY			= -1,
		AMF_DATA_TYPE_NUMBER		= 0x00,
		AMF_DATA_TYPE_BOOL			= 0x01,
		AMF_DATA_TYPE_STRING		= 0x02,
		AMF_DATA_TYPE_OBJECT		= 0x03,
		AMF_DATA_TYPE_NULL			= 0x05,
		AMF_DATA_TYPE_UNDEFINED		= 0x06,
		AMF_DATA_TYPE_REFERENCE		= 0x07,
		AMF_DATA_TYPE_MIXEDARRAY	= 0x08,
		AMF_DATA_TYPE_ARRAY			= 0x0a,
		AMF_DATA_TYPE_DATE			= 0x0b,
		AMF_DATA_TYPE_LONG_STRING	= 0x0c,
		AMF_DATA_TYPE_UNSUPPORTED	= 0x0d,
	};

	struct AMF0 {
		AMF_DATA_TYPE type = AMF_DATA_TYPE_EMPTY;
		CString	name;
		CString	value_s;
		double	value_d = 0.0;
		bool	value_b = false;

		operator CString() const {
			return value_s;
		}
		operator double() const {
			return value_d;
		}
		operator bool() const {
			return value_b;
		}
	};

	std::vector<SyncPoint> m_sps;

	CString AMF0GetString(UINT64 end);
	bool ParseAMF0(UINT64 end, const CString key, std::vector<AMF0> &AMF0Array);

protected:
	CAutoPtr<CBaseSplitterFileEx> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CFLVSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	// CBaseFilter
	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);

	// IKeyFrameInfo
	STDMETHODIMP_(HRESULT) GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP_(HRESULT) GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

class __declspec(uuid("C9ECE7B3-1D8E-41F5-9F24-B255DF16C087"))
	CFLVSourceFilter : public CFLVSplitterFilter
{
public:
	CFLVSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};