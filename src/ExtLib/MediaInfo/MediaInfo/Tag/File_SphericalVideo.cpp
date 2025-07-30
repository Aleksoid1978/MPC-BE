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
#if defined(MEDIAINFO_SPHERICALVIDEO_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_SphericalVideo.h"
#include "ThirdParty/tfsxml/tfsxml.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern const char* Mk_StereoMode(int64u StereoMode);
struct stereomode_mapping {
    const char* Value;
    int8u MapTo;
};
static const stereomode_mapping StereoMode_Mapping[] =
{
    { "mono", 0 },
    { "left-right", 1 },
    { "top-bottom", 3 },
};

//---------------------------------------------------------------------------
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
            Result = tfsxml_value(&p, &v); \
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
bool File_SphericalVideo::FileHeader_Begin()
{
    if (!IsSub && File_Size != (int64u)-1 && Buffer_Size < File_Size) {
        return false;
    }

    tfsxml_string p = {}, b = {}, v = {};
    auto Result = tfsxml_init(&p, Buffer, Buffer_Size, 0);
    XML_ELEM_START
    ELEMENT("rdf:SphericalVideo")
        Accept();
        Fill(Stream_General, 0, General_Format, "Spherical Video");
        Stream_Prepare(Stream_Video);
        Fill(Stream_Video, 0, "Spatial", "Yes", Unlimited, true, true);
        Fill(Stream_Video, 0, "Spatial Format", "Spherical Video");
        XML_ELEM_START
        }
        else {
            auto p_sav = p;
            Result = tfsxml_enter(&p);
            if (!Result) {
                tfsxml_string t;
                Result = tfsxml_next(&p, &t);
                if (Result < 0) {
                    p = p_sav;
                    XML_VALUE
                    if (!tfsxml_strncmp_charp(b, "GSpherical:", 11)) {
                        b.buf += 11;
                        b.len -= 11;
                    }
                    auto Value = tfsxml_decode(v);
                    if (!tfsxml_strcmp_charp(b, "SourceCount")) {
                        Fill(Stream_Video, 0, Video_MultiView_Count, Value);
                        continue;
                    }
                    if (!tfsxml_strcmp_charp(b, "StereoMode")) {
                        const char* Found = nullptr;
                        for (const auto& Mapping : StereoMode_Mapping) {
                            if (Value == Mapping.Value) {
                                Found = Mk_StereoMode(Mapping.MapTo);
                                break;
                            }
                        }
                        Fill(Stream_Video, 0, Video_MultiView_Layout, Found ? Found : Value);
                        continue;
                    }
                    auto FieldName = tfsxml_decode(b);
                    for (auto& c : FieldName) {
                        if (!(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z')) {
                            c = '_';
                        }
                    }
                    Fill(Stream_Video, 0, ("Spatial " + FieldName).c_str(), Value);
                }
            }
        XML_ELEM_END
    XML_ELEM_END

    Element_Offset = File_Size;
    return true;
}

} //NameSpace

#endif //MEDIAINFO_SPHERICALVIDEO_YES
