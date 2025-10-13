/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about XMP files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_XmpH
#define MediaInfo_File_XmpH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//GContainer items
struct gc_item
{
    string Mime;
    string Semantic;
    int32u Length;
    string Label;
    int32u Padding;
    string URI;
};
typedef std::vector<gc_item> gc_items;

//GainMap metadata
struct gm_data
{
    string Version;
    bool BaseRenditionIsHDR{ false };
    bool Multichannel{ false };
    float32 HDRCapacityMin{ 0 };
    float32 HDRCapacityMax{};
    float32 GainMapMin[3]{ 0, 0, 0 };
    float32 GainMapMax[3]{};
    float32 Gamma[3]{ 1, 1, 1 };
    float32 OffsetSDR[3]{ 1.0f / 64, 1.0f / 64, 1.0f / 64 };
    float32 OffsetHDR[3]{ 1.0f / 64, 1.0f / 64, 1.0f / 64 };
};

//***************************************************************************
// Class File_Xmp
//***************************************************************************

class File_Xmp : public File__Analyze
{
public:
    bool Wait{ false };
    gc_items* GContainerItems{ nullptr };
    gm_data* GainMapData{ nullptr };

private :
    //Buffer - File header
    bool FileHeader_Begin() override;

    //Element namespaces
    void dc(const string& name, const string& value);
    void exif(const string& name, const string& value);
    void GIMP(const string& name, const string& value);
    void hdrgm(const string& name, const string& value, const int count = 0);
    void pdf(const string& name, const string& value);
    void photoshop(const string& name, const string& value);
    void xmp(const string& name, const string& value);
    void Iptc4xmpExt(const string& name, const string& value);

    //Temp
    Ztring ModifyDate, CreateDate;
    string gimp_version, pdfaid, pdfaid_part, pdfaid_conformance;
};

} //NameSpace

#endif
