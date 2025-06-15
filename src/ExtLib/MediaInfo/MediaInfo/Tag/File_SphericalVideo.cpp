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
                    auto FieldName = tfsxml_decode(b);
                    if (FieldName.find("GSpherical:", 0) == 0) {
                        FieldName.erase(0, 11);
                    }
                    for (auto& c : FieldName) {
                        if (!(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z')) {
                            c = '_';
                        }
                    }
                    Fill(Stream_Video, 0, ("Spatial " + FieldName).c_str(), tfsxml_decode(v));
                }
            }
        XML_ELEM_END
    XML_ELEM_END

    Element_Offset = File_Size;
    return true;
}

} //NameSpace

#endif //MEDIAINFO_XMP_YES
