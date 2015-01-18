/*
 * (C) 2013-2014 see Authors.txt
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
#include "AudioFile.h"

#pragma warning(disable: 4005 4244)
extern "C" {
	#include <stdint.h>
}

#define AudioSplitterName L"MPC Audio Splitter"
#define AudioSourceName   L"MPC Audio Source"

class __declspec(uuid("AA77A669-E10F-4C70-BBD7-77923DF34BF3"))
	CAudioSplitterFilter : public CBaseSplitterFilter
{
	CAudioFile* m_pAudioFile;
	REFERENCE_TIME m_rtime;

protected:
	CAutoPtr<CBaseSplitterFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CAudioSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	~CAudioSplitterFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("A3CE2805-8E16-4E1E-84AA-4EF57F27E644"))
	CAudioSourceFilter : public CAudioSplitterFilter
{
public:
	CAudioSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
