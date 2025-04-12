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
#if defined(MEDIAINFO_MXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_HdrVividMetadata.h"
#include "ThirdParty/tfsxml/tfsxml.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

#define XML_ELEM_START \
    Result = tfsxml_enter(&p); \
    if (Result > 0) { \
        return false; \
    } \
    for (;;) { \
        { \
            Result = tfsxml_next(&p, &b); \
            if (Result < 0) { \
                break; \
            } \
            if (Result > 0) { \
                return Result; \
            } \
        } \
        if (false) { \

#define ELEMENT(NAME) \
        } \
        else if (!tfsxml_strcmp_charp(b, NAME)) { \

#define XML_VALUE \
        { \
            Result = tfsxml_value(&p, &b); \
            if (Result > 0) { \
                return Result; \
            } \
        } \

#define XML_ELEM_END \
        } \
    } \

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_HdrVividMetadata::FileHeader_Begin()
{
    if (!IsSub && File_Size != (int64u)-1 && Buffer_Size < File_Size) {
        return false;
    }

    tfsxml_string p = {}, b = {}, v = {};
    auto Result = tfsxml_init(&p, Buffer, Buffer_Size, 0);
    XML_ELEM_START
    ELEMENT("HDRVividMeta")
        Accept("HdrVividMeta");
        Fill(Stream_General, 0, General_Format, "HDR Vivid Metadata");
        Stream_Prepare(Stream_Video);
        Fill(Stream_Video, 0, Video_HDR_Format, "HDR Vivid Metadata");
        Stream_Prepare(Stream_Other);
        Fill(Stream_Other, 0, Other_Format, "HDR Vivid Global Data");
        XML_ELEM_START
        ELEMENT("BasicInfo")
            XML_ELEM_START
            ELEMENT("HdrVividProfile")
                XML_VALUE
                Fill(Stream_Video, 0, Video_HDR_Format_Profile, tfsxml_decode(b));
            XML_ELEM_END
        ELEMENT("OutputStructure")
            XML_ELEM_START
            ELEMENT("Video")
                XML_ELEM_START
                ELEMENT("Track")
                    XML_ELEM_START
                    ELEMENT("EncodingInfo")
                        XML_ELEM_START
                        ELEMENT("ColorPrimaries")
                            XML_VALUE
                            Fill(Stream_Video, 0, Video_colour_primaries, tfsxml_decode(b));
                        ELEMENT("ColorSpace")
                            XML_VALUE
                            //Fill(Stream_Video, 0, Video_ColorSpace, tfsxml_decode(b));
                        ELEMENT("SignalRange")
                            XML_VALUE
                            Fill(Stream_Video, 0, Video_colour_range, tfsxml_decode(b));
                        ELEMENT("TransferCharacteristics")
                            XML_VALUE
                            Fill(Stream_Video, 0, Video_transfer_characteristics, tfsxml_decode(b));
                        XML_ELEM_END
                    XML_ELEM_END
                XML_ELEM_END
            XML_ELEM_END
        ELEMENT("Version")
            XML_VALUE
            Fill(Stream_Video, 0, Video_HDR_Format_Version, tfsxml_decode(b));
        XML_ELEM_END
    ELEMENT("UWAFrameData")
        Accept("HdrVividMeta");
        Fill(Stream_General, 0, General_Format, "HDR Vivid Metadata");
        Stream_Prepare(Stream_Video);
        Fill(Stream_Video, 0, Video_HDR_Format, "HDR Vivid Metadata");
        Stream_Prepare(Stream_Other);
        Fill(Stream_Other, 0, Other_Format, "HDR Vivid Frame Data");
    XML_ELEM_END

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

} //NameSpace

#endif //defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)

