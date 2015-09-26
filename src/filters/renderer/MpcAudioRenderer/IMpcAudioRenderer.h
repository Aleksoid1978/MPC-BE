/*
 * (C) 2010-2014 see Authors.txt
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

enum WASAPI_MODE {
	MODE_WASAPI_EXCLUSIVE,
	MODE_WASAPI_SHARED,
	MODE_WASAPI_EXCLUSIVE_BITSTREAM,
	MODE_NONE
};

enum BITSTREAM_MODE {
	BITSTREAM_NONE,
	BITSTREAM_AC3,
	BITSTREAM_DTS,
	BITSTREAM_EAC3,
	BITSTREAM_TRUEHD,
	BITSTREAM_DTSHD
};

enum SYNC_METHOD {
	SYNC_BY_TIMESTAMPS,
	SYNC_BY_DURATION
};

interface __declspec(uuid("495D2C66-D430-439b-9DEE-40F9B7929BBA"))
IMpcAudioRendererFilter :
public IUnknown {
	STDMETHOD(Apply()) PURE;

	STDMETHOD(SetWasapiMode(INT nValue)) PURE;
	STDMETHOD_(INT, GetWasapiMode()) PURE;
	STDMETHOD(SetDeviceId(CString pDeviceId)) PURE;
	STDMETHOD_(CString, GetDeviceId()) PURE;
	STDMETHOD_(UINT, GetMode()) PURE;
	STDMETHOD(GetStatus(WAVEFORMATEX** ppWfxIn, WAVEFORMATEX** ppWfxOut)) PURE;
	STDMETHOD(SetBitExactOutput(BOOL nValue)) PURE;
	STDMETHOD_(BOOL, GetBitExactOutput()) PURE;
	STDMETHOD(SetSystemLayoutChannels(BOOL nValue)) PURE;
	STDMETHOD_(BOOL, GetSystemLayoutChannels()) PURE;
	STDMETHOD_(BITSTREAM_MODE, GetBitstreamMode()) PURE;
	STDMETHOD_(CString, GetCurrentDeviceName()) PURE;
	STDMETHOD_(CString, GetCurrentDeviceId()) PURE;
	STDMETHOD(SetSyncMethod(INT nValue)) PURE;
	STDMETHOD_(INT, GetSyncMethod()) PURE;
};
