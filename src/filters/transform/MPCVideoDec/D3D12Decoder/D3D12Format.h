/*
 *      Copyright (C) 2021      Benoit Plourde
 *      http://www.1f0.de
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


#pragma once



#include "d3d12.h"
#include "d3d12video.h"
//#include "LAVPixFmtConverter.h"
#include <vector>
extern "C"
{
#include <ExtLib/ffmpeg/libavcodec/avcodec.h>
}
#define DXGI_MAX_SHADER_VIEW 4
typedef struct
{
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
    UINT64 pitch_plane[2];
    UINT rows_plane[2];
    UINT64 RequiredSize;
} d3d12_footprint_t;

typedef struct
{
    const char* name;
    DXGI_FORMAT  formatTexture;
    uint32_t fourcc;
    uint8_t      bitsPerChannel;
    uint8_t      widthDenominator;
    uint8_t      heightDenominator;
    DXGI_FORMAT  resourceFormat[DXGI_MAX_SHADER_VIEW];
} d3d_format_t;

template<typename T>
inline void SetFeatureDataNodeIndex(void* pFeatureSupportData, UINT FeatureSupportDataSize, UINT NodeIndex)
{
    T* pSupportData = (T*)pFeatureSupportData;
    pSupportData->NodeIndex = NodeIndex;
}

class CD3D12Format
{
public:
    CD3D12Format(ID3D12VideoDevice* pVideoDevice) { m_pVideoDevice = pVideoDevice; };
    ~CD3D12Format();
    HRESULT DeviceSupportsFormat(ID3D12Device* d3d_dev, DXGI_FORMAT format, UINT supportFlags)
    {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT support;
        support.Format = format;

        if (FAILED(d3d_dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,
            &support, sizeof(support))))
            return S_FALSE;
        if ((support.Support1 & supportFlags) == supportFlags)
            return S_OK;
        return S_FALSE;
    }
    void CheckFeatureSupport(D3D12_FEATURE_VIDEO FeatureVideo, void* pFeatureSupportData, UINT FeatureSupportDataSize);
    STDMETHODIMP FindVideoServiceConversion(struct AVCodecContext* c, DXGI_FORMAT surface_format, GUID* input);
    D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT GetDecodeSupport() { return m_pDecodeSupport; };
    DXGI_FORMAT GetSupportedFormat() { return m_SupportedFormat; };
protected:
    struct ProfileInfo {
        GUID profileGUID;
        std::vector<DXGI_FORMAT> formats;
    };
private:
    ID3D12VideoDevice* m_pVideoDevice;
    std::vector<ProfileInfo> m_decodeProfiles;
    D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT m_pDecodeSupport;
    DXGI_FORMAT m_SupportedFormat;
};
