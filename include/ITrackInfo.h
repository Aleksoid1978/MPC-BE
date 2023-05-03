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

#pragma once

enum TrackType {
	TypeVideo		= 1,
	TypeAudio		= 2,
	TypeComplex		= 3,
	TypeLogo		= 0x10,
	TypeSubtitle	= 0x11,
	TypeControl		= 0x20
};

#pragma pack(push, 1)

struct TrackElement {
	WORD Size;				// Size of this structure
	BYTE Type;				// See TrackType
	BOOL FlagDefault;		// Set if the track is the default for its TrackType.
	BOOL FlagForced;		// Set if that track MUST be used during playback.
	BOOL FlagLacing;		// Set if the track may contain blocks using lacing.
	UINT MinCache;			// The minimum number of frames a player should be able to cache during playback.
	UINT MaxCache;			// The maximum cache size required to store referenced frames in and the current frame. 0 means no cache is needed.
	CHAR Language[4];		// Specifies the language of the track, in the ISO-639-2 form. (end with '\0')
};

struct TrackExtendedInfoVideo {
	WORD Size;				// Size of this structure
	BOOL Interlaced;		// Set if the video is interlaced.
	UINT PixelWidth;		// Width of the encoded video frames in pixels.
	UINT PixelHeight;		// Height of the encoded video frames in pixels.
	UINT DisplayWidth;		// Width of the video frames to display.
	UINT DisplayHeight;		// Height of the video frames to display.
	BYTE DisplayUnit;		// Type of the unit for DisplayWidth/Height (0: pixels, 1: centimeters, 2: inches).
	BYTE AspectRatioType;	// Specify the possible modifications to the aspect ratio (0: free resizing, 1: keep aspect ratio, 2: fixed).
};

struct TrackExtendedInfoAudio {
	WORD Size;						// Size of this structure
	FLOAT SamplingFreq;				// Sampling frequency in Hz.
	FLOAT OutputSamplingFrequency;	// Real output sampling frequency in Hz (used for SBR techniques).
	UINT Channels;					// Numbers of channels in the track.
	UINT BitDepth;					// Bits per sample, mostly used for PCM.
};

#pragma pack(pop)

interface __declspec(uuid("03E98D51-DDE7-43aa-B70C-42EF84A3A23D"))
ITrackInfo :
public IUnknown {
	STDMETHOD_(UINT, GetTrackCount) () PURE;

	// \param aTrackIdx the track index (from 0 to GetTrackCount()-1)
	STDMETHOD_(BOOL, GetTrackInfo) (UINT aTrackIdx, struct TrackElement* pStructureToFill) PURE;

	// Get an extended information struct relative to the track type
	STDMETHOD_(BOOL, GetTrackExtendedInfo) (UINT aTrackIdx, void* pStructureToFill) PURE;

	STDMETHOD_(BSTR, GetTrackCodecID) (UINT aTrackIdx) PURE;
	STDMETHOD_(BSTR, GetTrackName) (UINT aTrackIdx) PURE;
	STDMETHOD_(BSTR, GetTrackCodecName) (UINT aTrackIdx) PURE;
	STDMETHOD_(BSTR, GetTrackCodecInfoURL) (UINT aTrackIdx) PURE;
	STDMETHOD_(BSTR, GetTrackCodecDownloadURL) (UINT aTrackIdx) PURE;
};
