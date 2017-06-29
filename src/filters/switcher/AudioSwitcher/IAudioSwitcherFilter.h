/*
 * (C) 2017 see Authors.txt
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

enum {
	SPK_MONO = 0,
	SPK_STEREO,
	SPK_4_0,
	SPK_5_0,
	SPK_5_1,
	SPK_7_1
};

interface __declspec(uuid("CEDB2890-53AE-4231-91A3-B0AAFCD1DBDE"))
IAudioSwitcherFilter :
public IUnknown {
	STDMETHOD(SetChannelMixer) (bool bMixer, int nLayout) PURE;
	STDMETHOD(SetAudioGain) (float fGain_dB) PURE;
	STDMETHOD(GetAutoVolumeControl) (bool& bAutoVolumeControl, bool& bNormBoost, int& iNormLevel, int& iNormRealeaseTime) PURE;
	STDMETHOD(SetAutoVolumeControl) (bool bAutoVolumeControl, bool bNormBoost, int iNormLevel, int iNormRealeaseTime) PURE;
	STDMETHOD_(REFERENCE_TIME, GetAudioTimeShift) () PURE;
	STDMETHOD(SetAudioTimeShift) (REFERENCE_TIME rtAudioTimeShift) PURE;
	STDMETHOD(SetBassRedirect) (bool bBassRedirect) PURE;
};
