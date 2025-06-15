/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Main part
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#ifdef MEDIAINFO_C2PA_YES
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_C2pa.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Const
//***************************************************************************

static constexpr uint64_t ISO(const uint32_t Value)
{
    return (0x01LL << 32) | Value;
}

namespace Elements
{
    const int32u bfdb = 0x62666462;
    const int32u bidb = 0x62696462;
    const int32u cbor = 0x63626F72;
    const int32u json = 0x6A736F6E;
    const int32u jumb = 0x6A756D62;
    const int32u jumd = 0x6A756D64;

    const int64u ISO__c2pa = ISO(0x63327061);
    const int64u ISO__c2pa_c2ma = ISO(0x63326D61);
    const int64u ISO__c2pa_c2ma_c2as = ISO(0x63326173);
    const int64u ISO__c2pa_c2ma_c2as_bfdb = ISO(0x62666462);
    const int64u ISO__c2pa_c2ma_c2as_bidb = ISO(0x62696462);
    const int64u ISO__c2pa_c2ma_c2as_cbor = ISO(0x63626F72);
    const int64u ISO__c2pa_c2ma_c2as_json = ISO(0x6A736F6E);
    const int64u ISO__c2pa_c2ma_c2cl = ISO(0x6332636C);
    const int64u ISO__c2pa_c2ma_c2cs = ISO(0x63326373);
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_C2pa::File_C2pa()
:File__Analyze()
{
    //Configuration
    ParserName="C2PA";
    DataMustAlwaysBeComplete=false;
    MustAdaptSubOffsets=true;
}

//---------------------------------------------------------------------------
File_C2pa::~File_C2pa()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_C2pa::Streams_Accept()
{
    if (!IsSub) {
        Fill(Stream_General, 0, General_Format, "C2PA");
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_C2pa::FileHeader_Parse()
{
    if (IsSub) {
        Accept(); //TODO: Temporarily here as a workaround for buggy trace. Can be removed, it works with just the trace not well displayed
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_C2pa::Read_Buffer_Continue()
{
    //Element[0].Name=__T("C2PA");
}

//***************************************************************************
// Buffer
//***************************************************************************

//---------------------------------------------------------------------------
void File_C2pa::Header_Parse()
{
    //Parsing
    int64u Size, Probe;
    int32u Size_32, Name;
    Get_B4 (Size_32,                                            "Size");
    Size=Size_32;
    Get_C4 (Name,                                               "Name");

    if (Size<8) {
        //Special case: Big files, size is 64-bit
        if (Size == 1) {
            //Reading Extended size
            Get_B8 (Size,                                       "Size (Extended)");
        }
        //Not in specs!
        else {
            Size=File_Size-(File_Offset+Buffer_Offset);
        }
    }
    bool IsExtended = false;
    if (Size >= 16) {
        Peek_B8(Probe);
        auto Name2 = static_cast<int32u>(Probe);
        auto Size2 = static_cast<int32u>(Probe >> 32);
        if (Name2 == Elements::jumd && Size2 > 24 && Size2 <= Size ) {
            IsExtended = true;
            Label.clear();
            auto End = Element_Offset + Size2 - 8;
            Element_End0();
            Element_Begin1("JUMBF Description Box");
            Skip_B4(                                            "Size");
            Skip_C4(                                            "Name");

            int128u UUID;
            int8u Toggles;
            bool LabelPresent, IDPresent, SignaturePresent, PrivateFieldPresent;
            Get_UUID(UUID,                                      "UUID");
            Get_B1 (Toggles,                                    "Toggles");
            Get_Flags (Toggles, 4, PrivateFieldPresent,         "Private field present");
            Get_Flags (Toggles, 3, SignaturePresent,            "Signature present");
            Get_Flags (Toggles, 2, IDPresent,                   "ID present");
            Get_Flags (Toggles, 1, LabelPresent,                "Label present");
            Skip_Flags(Toggles, 0,                              "Requestable");
            if (LabelPresent) {
                Get_String(SizeUpTo0(), Label,                  "Label"); --Element_Level; Element_Info1(Label); ++Element_Level;
                Skip_B1(                                        "zero");
            }
            if (IDPresent) {
                Skip_B4(                                        "ID");
            }
            if (SignaturePresent) {
                Skip_XX(32,                                     "Signature");
            }
            if (PrivateFieldPresent) {
                Element_Begin1("JUMBF Private Box");
                int32u Size3, Name3;
                Get_B4 (Size3,                                  "Size");
                Get_C4 (Name3,                                  "Name"); Element_Info1(Ztring::ToZtring_From_CC4(Name3));
                if (false) {
                }
                else if (Size3 >= 8) {
                    Skip_XX(Size3 - 8,                          "(Unknown)");
                }
                Element_End0();
            }
            if (End > Element_Offset) {
                Skip_XX(End - Element_Offset,                   "(Unknown)");
            }

            if (static_cast<int32u>(UUID.hi) == 0x00110010 && UUID.lo == 0x800000aa00389b71) {
                auto Code = static_cast<int32u>(UUID.hi >> 32);
                --Element_Level;
                Element_Name(__T("JUMBF Box - ISO - ") + Ztring::ToZtring_From_CC4(Code));
                ++Element_Level;
                Header_Fill_Code(ISO(Code));
            }
            else {
                --Element_Level;
                Element_Name(__T("JUMBF Box - ") + Ztring().From_UUID(UUID));
                ++Element_Level;
                Header_Fill_Code(Name, Ztring().From_UUID(UUID));
            }
        }
    }
    if (!IsExtended) {
        Header_Fill_Code(Name, Ztring::ToZtring_From_CC4(Name));
    }
    Header_Fill_Size(Size);
}

//***************************************************************************
// Format
//***************************************************************************

//---------------------------------------------------------------------------
void File_C2pa::Data_Parse()
{
    //Parsing
    DATA_BEGIN
    LIST(ISO__c2pa)
        ATOM_BEGIN
        LIST(ISO__c2pa_c2ma)
            ATOM_BEGIN
            LIST(ISO__c2pa_c2ma_c2as)
                ATOM_BEGIN
                LIST_DEFAULT(Default)
                    ATOM_BEGIN
                    LIST_DEFAULT(Default)
                    ATOM_END_DEFAULT
                ATOM_END_DEFAULT
            LIST(ISO__c2pa_c2ma_c2cl)
                ATOM_BEGIN
                LIST_DEFAULT(Default)
                    ATOM_BEGIN
                    LIST_DEFAULT(Default)
                    ATOM_END_DEFAULT
                ATOM_END_DEFAULT
            LIST(ISO__c2pa_c2ma_c2cs)
                ATOM_BEGIN
                LIST_DEFAULT(Default)
                    ATOM_BEGIN
                    LIST_DEFAULT(Default)
                    ATOM_END_DEFAULT
                ATOM_END_DEFAULT
            LIST_DEFAULT(Default)
                ATOM_BEGIN
                LIST_DEFAULT(Default)
                    ATOM_BEGIN
                    LIST_DEFAULT(Default)
                    ATOM_END_DEFAULT
                ATOM_END_DEFAULT
            ATOM_END_DEFAULT
        LIST_DEFAULT(Default)
            ATOM_BEGIN
            LIST_DEFAULT(Default)
                ATOM_BEGIN
                LIST_DEFAULT(Default)
                    ATOM_BEGIN
                    LIST_DEFAULT(Default)
                    ATOM_END_DEFAULT
                ATOM_END_DEFAULT
            ATOM_END_DEFAULT
        ATOM_END_DEFAULT
    LIST_DEFAULT(Default)
        ATOM_BEGIN
        LIST_DEFAULT(Default)
            ATOM_BEGIN
            LIST_DEFAULT(Default)
                ATOM_BEGIN
                LIST_DEFAULT(Default)
                    ATOM_BEGIN
                    LIST_DEFAULT(Default)
                    ATOM_END_DEFAULT
                ATOM_END_DEFAULT
            ATOM_END_DEFAULT
        ATOM_END_DEFAULT
    DATA_END_DEFAULT
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_C2pa::ISO__c2pa()
{
    Accept();
    Fill(Stream_General, 0, "C2PA_Present", "Yes");
}

//---------------------------------------------------------------------------
void File_C2pa::ISO__c2pa_c2ma()
{
    Element_Name("C2PA Manifest");

    Label_c2ma = Label;
}

//---------------------------------------------------------------------------
void File_C2pa::ISO__c2pa_c2ma_c2as()
{
    Element_Name("C2PA Assertion Store");
}

//---------------------------------------------------------------------------
void File_C2pa::ISO__c2pa_c2ma_c2cl()
{
    Element_Name("C2PA Claim");
}

//---------------------------------------------------------------------------
void File_C2pa::ISO__c2pa_c2ma_c2cs()
{
    Element_Name("C2PA Claim Signature");
}

//---------------------------------------------------------------------------
void File_C2pa::bfdb()
{
    Element_Name("Embedded File Content Description");

    Skip_B1(                                                    "Toggles");
    Info_UTF8(Element_Size - Element_Offset, Data,              "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_C2pa::bidb()
{
    Element_Name("Embedded File Content");

    Attachment("C2PA", (Label_c2ma + " / " + Label).c_str());
}

//---------------------------------------------------------------------------
void File_C2pa::cbor()
{
    Element_Name("CBOR Content");

    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_C2pa::json()
{
    Element_Name("JSON Content");

    Skip_UTF8(Element_Size,                                     "Data");
}

//---------------------------------------------------------------------------
void File_C2pa::Default()
{
    #define DEFAULT_CASE(_Name) case Elements::_Name: if (!Element_IsComplete_Get()) { Element_WaitForMoreData(); return; } _Name(); break;
    switch (Element_Code) {
    DEFAULT_CASE(bfdb);
    DEFAULT_CASE(bidb);
    DEFAULT_CASE(cbor);
    DEFAULT_CASE(json);
    case Elements::jumb:
        break;
    default:
        Skip_XX(Element_TotalSize_Get(),                        "Data");
    }
}

} //NameSpace

#endif //MEDIAINFO_C2PA_YES
