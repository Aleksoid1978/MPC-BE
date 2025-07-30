/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ISO 21496-1 Gain Map Metadata
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_GainMapH
#define MediaInfo_File_GainMapH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

// ISO 1496-1 Gain map metadata for image conversion
struct GainMap_metadata {
    int16u minimum_version;
    int16u writer_version;
    bool is_multichannel;
    bool use_base_colour_space;
    float32 base_hdr_headroom;
    float32 alternate_hdr_headroom;
    float32 gain_map_min[3];
    float32 gain_map_max[3];
    float32 gamma[3];
    float32 base_offset[3];
    float32 alternate_offset[3];
};

//***************************************************************************
// Class File_GainMap
//***************************************************************************

class File_GainMap : public File__Analyze
{
public:
    //In
    GainMap_metadata* output{};
    bool fromAvif{ false };

private :
    void Data_Parse();
};

} //NameSpace

#endif

