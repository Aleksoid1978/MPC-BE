/*
 * (C) 2023 see Authors.txt
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
 * Based on telxcc.c from ccextractor source code
 * https://github.com/CCExtractor/ccextractor
 */

#pragma once

#include <intsafe.h>
#include <ITrackInfo.h>

class CTrackInfoImpl : public ITrackInfo
{
public:
	// ITrackInfo

    STDMETHODIMP_(UINT) GetTrackCount() { return 0; }

    // \param aTrackIdx the track index (from 0 to GetTrackCount()-1)
    STDMETHODIMP_(BOOL) GetTrackInfo(UINT aTrackIdx, struct TrackElement* pStructureToFill) { return FALSE; }

    // Get an extended information struct relative to the track type
    STDMETHODIMP_(BOOL) GetTrackExtendedInfo(UINT aTrackIdx, void* pStructureToFill) { return FALSE; }

    STDMETHODIMP_(BSTR) GetTrackCodecID(UINT aTrackIdx) { return nullptr; }
    STDMETHODIMP_(BSTR) GetTrackName(UINT aTrackIdx) { return nullptr; }
    STDMETHODIMP_(BSTR) GetTrackCodecName(UINT aTrackIdx) { return nullptr; }
    STDMETHODIMP_(BSTR) GetTrackCodecInfoURL(UINT aTrackIdx) { return nullptr; }
    STDMETHODIMP_(BSTR) GetTrackCodecDownloadURL(UINT aTrackIdx) { return nullptr; }
};