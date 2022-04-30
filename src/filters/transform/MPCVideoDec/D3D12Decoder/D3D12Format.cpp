/*
 *      Copyright (C) 2021      Benoit Plourde
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "stdafx.h"
#include "D3D12Format.h"
#include "../dxva2/dxva_common.h"
#include <memory>
CD3D12Format::~CD3D12Format()
{
    SafeRelease(&m_pVideoDevice);
}

void CD3D12Format::CheckFeatureSupport(D3D12_FEATURE_VIDEO FeatureVideo, void* pFeatureSupportData, UINT FeatureSupportDataSize)
{
    switch (FeatureVideo)
    {
    case D3D12_FEATURE_VIDEO_DECODE_SUPPORT:
    {

        SetFeatureDataNodeIndex<D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT>(pFeatureSupportData, FeatureSupportDataSize, 0);
        break;
    }

    case D3D12_FEATURE_VIDEO_DECODE_CONVERSION_SUPPORT:
    {
        SetFeatureDataNodeIndex<D3D12_FEATURE_DATA_VIDEO_DECODE_CONVERSION_SUPPORT>(pFeatureSupportData, FeatureSupportDataSize, 0);
        break;
    }

    case D3D12_FEATURE_VIDEO_DECODE_PROFILE_COUNT:
    {
        SetFeatureDataNodeIndex<D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILE_COUNT>(pFeatureSupportData, FeatureSupportDataSize, 0);
        break;
    }

    case D3D12_FEATURE_VIDEO_DECODE_PROFILES:
    {
        SetFeatureDataNodeIndex<D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILES>(pFeatureSupportData, FeatureSupportDataSize, 0);
        break;
    }

    case D3D12_FEATURE_VIDEO_DECODE_FORMAT_COUNT:
    {
        SetFeatureDataNodeIndex<D3D12_FEATURE_DATA_VIDEO_DECODE_FORMAT_COUNT>(pFeatureSupportData, FeatureSupportDataSize, 0);
        break;
    }

    case D3D12_FEATURE_VIDEO_DECODE_FORMATS:
    {
        SetFeatureDataNodeIndex<D3D12_FEATURE_DATA_VIDEO_DECODE_FORMATS>(pFeatureSupportData, FeatureSupportDataSize, 0);
        break;
    }

    case D3D12_FEATURE_VIDEO_DECODE_HISTOGRAM:
    {
        SetFeatureDataNodeIndex<D3D12_FEATURE_DATA_VIDEO_DECODE_HISTOGRAM>(pFeatureSupportData, FeatureSupportDataSize, 0);
        break;
    }

    default:
        break;
    }
    m_pVideoDevice->CheckFeatureSupport(FeatureVideo, pFeatureSupportData, FeatureSupportDataSize);
}


STDMETHODIMP CD3D12Format::FindVideoServiceConversion(struct AVCodecContext* c, DXGI_FORMAT surface_format, GUID* input)
{
    D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILE_COUNT decodeProfileData = {};
    HRESULT hr = S_OK;
    
    CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_PROFILE_COUNT, &decodeProfileData, sizeof(decodeProfileData));
    m_decodeProfiles.resize(decodeProfileData.ProfileCount);     //throw( bad_alloc )

    // get profiles
    std::unique_ptr<GUID[]> spGUIDs;
    spGUIDs.reset(new GUID[decodeProfileData.ProfileCount]);    //throw( bad_alloc )
    D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILES profiles = {};
    profiles.NodeIndex = 0;
    profiles.ProfileCount = decodeProfileData.ProfileCount;
    profiles.pProfiles = spGUIDs.get();
    CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_PROFILES, &profiles, sizeof(profiles));

    // fill formats for each profile
    for (UINT i = 0; i < decodeProfileData.ProfileCount; i++)
    {
        m_decodeProfiles[i].profileGUID = spGUIDs[i];

        D3D12_VIDEO_DECODE_CONFIGURATION decodeConfig = {
            m_decodeProfiles[i].profileGUID,
            D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE,
            D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE
        };

        // format count
        D3D12_FEATURE_DATA_VIDEO_DECODE_FORMAT_COUNT decodeProfileFormatData = {};
        decodeProfileFormatData.NodeIndex = 0;
        decodeProfileFormatData.Configuration = decodeConfig;
        CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMAT_COUNT, &decodeProfileFormatData, sizeof(decodeProfileFormatData));

        // decoder formats
        D3D12_FEATURE_DATA_VIDEO_DECODE_FORMATS formats = {};
        formats.NodeIndex = 0;
        formats.Configuration = decodeConfig;
        formats.FormatCount = decodeProfileFormatData.FormatCount;
        m_decodeProfiles[i].formats.resize(formats.FormatCount);    //throw( bad_alloc ))
        formats.pOutputFormats = m_decodeProfiles[i].formats.data();
        CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMATS, &formats, sizeof(formats));
    }
    D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decode_support = { 0 };

    
    for (unsigned i = 0; dxva_modes[i].name; i++)
    {
        const dxva_mode_t* mode = &dxva_modes[i];
        if (!check_dxva_mode_compatibility(mode, c->codec_id, c->profile, (surface_format == DXGI_FORMAT_NV12)))
            continue;
        BOOL supported = FALSE;
        for (std::vector<ProfileInfo>::iterator it = m_decodeProfiles.begin(); !supported && it != m_decodeProfiles.end(); it++)
        {
            supported = IsEqualGUID(*mode->guid, it->profileGUID);
            if (supported)
            {
                decode_support.Configuration.DecodeProfile = it->profileGUID;
                decode_support.Configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
                decode_support.Configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
                decode_support.Width = c->width;
                decode_support.Height = c->height;
                if (c->framerate.den && c->framerate.num)
                {
                    DXGI_RATIONAL framerate = { (UINT)c->framerate.den, (UINT)c->framerate.num };
                    decode_support.FrameRate = framerate;
                }
                for (std::vector<DXGI_FORMAT>::iterator itt = it->formats.begin(); itt != it->formats.end(); itt++)
                {
                    
                    decode_support.DecodeFormat = *itt;
                    hr = m_pVideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT, &decode_support, sizeof(decode_support));
                    if (FAILED(hr))
                        supported = false;
                    else
                    {
                        m_SupportedFormat = *itt;
                        break;
                    }
                }
                
            }

        }
        if (!supported)
            continue;
        if (SUCCEEDED(hr) && supported)
        {
            m_pDecodeSupport = decode_support;
            
            return S_OK;
        }
    }
       
    
    return E_FAIL;
}
