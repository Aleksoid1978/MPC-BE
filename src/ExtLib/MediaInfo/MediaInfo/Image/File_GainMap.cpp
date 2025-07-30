/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_JPEG_YES) || defined(MEDIAINFO_PNG_YES) || defined(MEDIAINFO_MPEG4_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_GainMap.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{
//---------------------------------------------------------------------------
void File_GainMap::Data_Parse()
{
    Element_Name("ISO 21496-1 Gain Map Metadata");

    if (fromAvif) {
        if (Buffer_Size < 5) {
            Reject();
            return;
        }
        int8u version;
        Get_B1(version,                                         "version");
        if (version != 0) {
            Reject();
            return;
        }
    }

    if (Buffer_Size < 4) {
        Reject();
        return;
    }

    int16u minimum_version, writer_version;
    Get_B2(minimum_version,                                     "minimum_version");
    if (minimum_version != 0) {
        Reject();
        return;
    }
    Get_B2(writer_version,                                      "writer_version");

    Accept();

    if (Element_Offset + 57 > Buffer_Size) {
        Finish();
        return;
    }

    int8u flags;
    bool is_multichannel, use_base_colour_space;
    Get_B1(flags,                                               "flags");
    Get_Flags(flags, 6, use_base_colour_space,                  "use_base_colour_space");
    Get_Flags(flags, 7, is_multichannel,                        "is_multichannel");
    auto channel_count{ is_multichannel ? 3 : 1 };

    int32u base_hdr_headroom_numerator, alternate_hdr_headroom_numerator, base_hdr_headroom_denominator{}, alternate_hdr_headroom_denominator{};
    int32s gain_map_min_numerator[3]{}, gain_map_max_numerator[3]{}, base_offset_n[3]{}, alternate_offset_n[3]{};
    int32u gain_map_min_denominator[3]{}, gain_map_max_denominator[3]{}, gamma_numerator[3]{}, gamma_denominator[3]{}, base_offset_denominator[3]{}, alternate_offset_denominator[3]{};
    Get_B4(base_hdr_headroom_numerator,                         "base_hdr_headroom_numerator");
    Get_B4(base_hdr_headroom_denominator,                       "base_hdr_headroom_denominator");
    Get_B4(alternate_hdr_headroom_numerator,                    "alternate_hdr_headroom_numerator");
    Get_B4(alternate_hdr_headroom_denominator,                  "alternate_hdr_headroom_denominator");
    for (int ch = 0; ch < channel_count; ++ch) {
        Element_Begin1(("channel " + std::to_string(ch + 1)).c_str());
        int32u gain_map_min_numerator_u, gain_map_max_numerator_u, base_offset_numerator_u, alternate_offset_numerator_u;
        Get_B4(gain_map_min_numerator_u,                        "gain_map_min_numerator");
        Get_B4(gain_map_min_denominator[ch],                    "gain_map_min_denominator");
        Get_B4(gain_map_max_numerator_u,                        "gain_map_max_numerator");
        Get_B4(gain_map_max_denominator[ch],                    "gain_map_max_denominator");
        Get_B4(gamma_numerator[ch],                             "gamma_numerator");
        Get_B4(gamma_denominator[ch],                           "gamma_denominator");
        Get_B4(base_offset_numerator_u,                         "base_offset_numerator");
        Get_B4(base_offset_denominator[ch],                     "base_offset_denominator");
        Get_B4(alternate_offset_numerator_u,                    "alternate_offset_numerator");
        Get_B4(alternate_offset_denominator[ch],                "alternate_offset_denominator");
        gain_map_min_numerator[ch] = static_cast<int32s>(gain_map_min_numerator_u);
        gain_map_max_numerator[ch] = static_cast<int32s>(gain_map_max_numerator_u);
        base_offset_n[ch] = static_cast<int32s>(base_offset_numerator_u);
        alternate_offset_n[ch] = static_cast<int32s>(alternate_offset_numerator_u);
        Element_End0();
    }

    if (output) {
        output->minimum_version = minimum_version;
        output->writer_version = writer_version;
        output->is_multichannel = is_multichannel;
        output->use_base_colour_space = use_base_colour_space;
        if (base_hdr_headroom_denominator) output->base_hdr_headroom = static_cast<float32>(base_hdr_headroom_numerator) / base_hdr_headroom_denominator;
        if (alternate_hdr_headroom_denominator) output->alternate_hdr_headroom = static_cast<float32>(alternate_hdr_headroom_numerator) / alternate_hdr_headroom_denominator;
        for (int ch = 0; ch < channel_count; ++ch) {
            if (gain_map_min_denominator[ch]) output->gain_map_min[ch] = static_cast<float32>(gain_map_min_numerator[ch]) / gain_map_min_denominator[ch];
            if (gain_map_max_denominator[ch]) output->gain_map_max[ch] = static_cast<float32>(gain_map_max_numerator[ch]) / gain_map_max_denominator[ch];
            if (gamma_denominator[ch]) output->gamma[ch] = static_cast<float32>(gamma_numerator[ch]) / gamma_denominator[ch];
            if (base_offset_denominator[ch]) output->base_offset[ch] = static_cast<float32>(base_offset_n[ch]) / base_offset_denominator[ch];
            if (alternate_offset_denominator[ch]) output->alternate_offset[ch] = static_cast<float32>(alternate_offset_n[ch]) / alternate_offset_denominator[ch];
        }
    }

    Finish();
}

} //NameSpace

#endif //defined(MEDIAINFO_JPEG_YES) || defined(MEDIAINFO_PNG_YES) || defined(MEDIAINFO_MPEG4_YES)
