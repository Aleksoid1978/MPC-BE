/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

#include "../../../AudioTools/Mixer.h"
#include "../../../AudioTools/BassRedirect.h"
#include "../../../DSUtil/SimpleBuffer.h"
#include "StreamSwitcher.h"
#include "IAudioSwitcherFilter.h"
#include "AudioNormalizer.h"

#define AudioSwitcherName L"MPC AudioSwitcher"

class __declspec(uuid("18C16B08-6497-420e-AD14-22D21C2CEAB7"))
	CAudioSwitcherFilter : public CStreamSwitcherFilter, public IAudioSwitcherFilter
{
	CCritSec m_csTransform;

	CSimpleBuffer<float> m_buffer;

	CMixer	m_Mixer;
	bool	m_bMixer;
	int		m_nMixerLayout;

	CBassRedirect m_BassRedirect;
	bool	m_bBassRedirect;

	float	m_fGain_dB;
	float	m_fGainFactor;

	CAudioNormalizer m_AudioNormalizer;
	bool	m_bAutoVolumeControl;
	bool	m_bNormBoost;
	int		m_iNormLevel;
	int		m_iNormRealeaseTime;

	bool	m_bInt16;
	bool	m_bInt24;
	bool	m_bInt32;
	bool	m_bFloat;

	REFERENCE_TIME m_rtAudioTimeShift;

	REFERENCE_TIME m_rtNextStart;

public:
	CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr);
	~CAudioSwitcherFilter();

	HRESULT CheckMediaType(const CMediaType* pmt) override;
	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut) override;
	void TransformMediaType(CMediaType& mt) override;

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAudioSwitcherFilter
	STDMETHODIMP SetChannelMixer(bool bMixer, int nLayout);
	STDMETHODIMP SetBassRedirect(bool bBassRedirect);
	STDMETHODIMP SetAudioGain(float fGain_dB);
	STDMETHODIMP SetAutoVolumeControl(bool bAutoVolumeControl, bool bNormBoost, int iNormLevel, int iNormRealeaseTime);
	STDMETHODIMP SetOutputFormats(int iSampleFormats);
	STDMETHODIMP_(REFERENCE_TIME) GetAudioTimeShift();
	STDMETHODIMP SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift);

	// IAMStreamSelect
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
};
