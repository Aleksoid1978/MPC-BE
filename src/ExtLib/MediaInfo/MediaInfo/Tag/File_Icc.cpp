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
#if defined(MEDIAINFO_ICC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_Icc.h"
#include <queue>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
const char* Mpegv_colour_primaries(int8u colour_primaries);
const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);
const char* Mk_Video_Colour_Range(int8u range);

//---------------------------------------------------------------------------
namespace Elements
{
    const int32u XYZ =0x58595A20;
    const int32u bTRC=0x62545243;
    const int32u bXYZ=0x6258595A;
    const int32u bkpt=0x626B7074;
    const int32u cicp=0x63696370;
    const int32u curv=0x63757276;
    const int32u cprt=0x63707274;
    const int32u desc=0x64657363;
    const int32u dmdd=0x646D6464;
    const int32u dmnd=0x646D6E64;
    const int32u gTRC=0x67545243;
    const int32u gXYZ=0x6758595A;
    const int32u kTRC=0x6B545243;
    const int32u kXYZ=0x6B58595A;
    const int32u mluc=0x6D6C7563;
    const int32u rTRC=0x72545243;
    const int32u rXYZ=0x7258595A;
    const int32u sRGB=0x73524742;
    const int32u text=0x74657874;
    const int32u vued=0x76756564;
    const int32u wtpt=0x77747074;
}

//---------------------------------------------------------------------------
string Icc_Tag(int32u Signature)
{
    switch (Signature)
    {
        case Elements::bTRC: return "blueTRC (Blue channel tone reproduction curve)";
        case Elements::bXYZ: return "blueMatrixColumn (Blue colorant stimulus)";
        case Elements::bkpt: return "mediaBlackPoint (Media black-point stimulus)";
        case Elements::cicp: return "cicp (Coding-independent code points)";
        case Elements::cprt: return "copyright (Profile copyright)";
        case Elements::desc: return "description (Profile name for display)";
        case Elements::dmdd: return "deviceModelDesc (Device model description)";
        case Elements::dmnd: return "deviceMfgDesc (Device manufacturer description)";
        case Elements::gTRC: return "greenTRC (Green channel tone reproduction curven)";
        case Elements::gXYZ: return "greenMatrixColumn (Green colorant stimulus)";
        case Elements::kTRC: return "grayTRC (Gray channel tone reproduction curve)";
        case Elements::kXYZ: return "grayMatrixColumn (Gray colorant stimulus)";
        case Elements::rTRC: return "redTRC (Red channel tone reproduction curve)";
        case Elements::rXYZ: return "redMatrixColumn (Red colorant stimulus)";
        case Elements::vued: return "viewingCondDesc (Viewing conditions description)";
        case Elements::wtpt: return "mediaWhitePoint (Media white-point stimulus)";
        default            : return Ztring().From_CC4(Signature).To_UTF8();
    }
}

//---------------------------------------------------------------------------
string Icc_ColorSpace(int32u ColorSpace)
{
    switch (ColorSpace)
    {
        case 0x434D5920: return "CMY";
        case 0x434D594B: return "CMYK";
        case 0x47524159: return "Y";
        case 0x484C5320: return "HLS";
        case 0x48535620: return "HSV";
        case 0x4C616220: return "Lab";
        case 0x4C757620: return "Luv";
        case 0x52474220: return "RGB";
        case 0x58595A20: return "XYZ";
        case 0x59436272: return "YCbCr";
        case 0x59787920: return "xyY";
        default        : return Ztring().From_CC4(ColorSpace).To_UTF8();
    }
}

//***************************************************************************
// Private
//***************************************************************************

//---------------------------------------------------------------------------
struct tag
{
    int32u Signature;
    int32u Offset;
    int32u Size;
};

//---------------------------------------------------------------------------
class File_Icc_Private
{
public:
    queue<tag> Tags;
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Icc::~File_Icc()
{
    delete P;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Icc::FileHeader_Parse()
{
    if (Frame_Count_Max)
    {
        Frame_Count++;
        if (Frame_Count<Frame_Count_Max)
        {
            Element_WaitForMoreData();
            return;
        }
    }

    if (Buffer_Size<132)
    {
        Element_WaitForMoreData();
        return;
    }

    //Parsing
    int32u Size, ColorSpace, acsp, DeviceModel;
    int8u Version_M, Version_m;
    Get_B4 (Size,                                               "Profile size");
    if ((IsSub && Size<Buffer_Size) || (!IsSub && File_Size!=(int64u)-1 && (Size<File_Size || Size>=0x01000000)))
    {
        Reject();
        return;
    }
    Skip_C4(                                                    "Preferred CMM type");
    Element_Begin1("Profile version");
        Get_B1(Version_M,                                       "Major");
        BS_Begin();
        Get_S1 (4, Version_m,                                   "Minor");
        Info_S1(4, Version_f,                                   "Fix");
        BS_End();
        Skip_B2(                                                "Reserved");
        Element_Info1(Ztring().From_Number(Version_M)+__T('.')+Ztring().From_Number(Version_m)+__T('.')+Ztring().From_Number(Version_f));
    Element_End0();
    Skip_C4(                                                    "Profile/Device class");
    Get_C4 (ColorSpace,                                         "Colour space of data");
    Skip_C4(                                                    "PCS");
    Element_Begin1("Date/Time");
        int16u YY, MM, DD, hh, mm, ss;
        Get_B2 (YY,                                             "Year");
        Get_B2 (MM,                                             "Month");
        Get_B2 (DD,                                             "Day");
        Get_B2 (hh,                                             "Hour");
        Get_B2 (mm,                                             "Minute");
        Get_B2 (ss,                                             "Second");
        if (!IsSub && (YY || MM || DD || hh || mm || ss) && (YY<=1970 || MM>=12 || DD>=31 || hh>=24 || mm>=60 || ss>=60))
        {
            Element_End0();
            Reject();
            return;
        }
        #if MEDIAINFO_TRACE
            string DateTime;
            DateTime+='0'+YY/1000;
            DateTime+='0'+(YY%1000)/100;
            DateTime+='0'+(YY%100)/10;
            DateTime+='0'+(YY%10);
            DateTime+='-';
            DateTime+='0'+MM/10;
            DateTime+='0'+(MM%10);
            DateTime+='-';
            DateTime+='0'+DD/10;
            DateTime+='0'+(DD%10);
            DateTime+=' ';
            DateTime+='0'+hh/10;
            DateTime+='0'+(hh%10);
            DateTime+=':';
            DateTime+='0'+mm/10;
            DateTime+='0'+(mm%10);
            DateTime+=':';
            DateTime+='0'+ss/10;
            DateTime+='0'+(ss%10);
            Element_Info1(DateTime.c_str());
        #endif //MEDIAINFO_TRACE
    Element_End0();
    Get_C4 (acsp,                                               "'acsp' profile file signature");
    if (acsp!=0x61637370)
    {
        Reject();
        return;
    }
    Skip_C4(                                                    "Primary platform signature");
    Skip_B4(                                                    "Flags");
    Skip_C4(                                                    "Device manufacturer");
    Get_C4 (DeviceModel,                                        "Device model");
    Skip_B4(                                                    "Device attributes");
    Skip_B4(                                                    "Device attributes");
    Skip_B4(                                                    "Rendering Intent");
    Skip_XYZ(12,                                                "Illuminant of the PCS");
    Skip_C4(                                                    "Profile creator signature");
    Skip_B4(                                                    "Profile ID");
    Skip_B4(                                                    "Profile ID");
    Skip_B4(                                                    "Profile ID");
    Skip_B4(                                                    "Profile ID");
    Skip_XX(128-Element_Offset,                                 "Reserved");

    if (P)
        return;

    Accept("ICC");
    Fill(Stream_General, 0, General_Format, "ICC");
    if ((IsSub && Size>Buffer_Size) || (!IsSub && File_Size!=(int64u)-1 && Size>File_Size))
        Fill(Stream_General, 0, "Truncated", "Yes");
    Fill(Stream_General, 0, General_Format_Version, __T("Version ")+Ztring::ToZtring(Version_M)+__T('.')+Ztring::ToZtring(Version_m));
    if (Version_M!=2 && Version_M!=4)
    {
        Finish();
        return;
    }
    Stream_Prepare(StreamKind);
    switch (DeviceModel)
    {
        case Elements::sRGB:
            Fill(StreamKind_Last, 0, "colour_description_present", "Yes", Unlimited, true, true);
            Fill(StreamKind_Last, 0, "colour_primaries", Mpegv_colour_primaries(1), Unlimited, true, true);
            Fill(StreamKind_Last, 0, "transfer_characteristics", Mpegv_transfer_characteristics(13), Unlimited, true, true);
            Fill(StreamKind_Last, 0, "matrix_coefficients", Mpegv_matrix_coefficients(0), Unlimited, true, true);
            if (!IsAdditional)
                Fill(StreamKind_Last, 0, "ColorSpace", Mpegv_matrix_coefficients_ColorSpace(0), Unlimited, true, true);
            Fill(StreamKind_Last, 0, "colour_range", Mk_Video_Colour_Range(2), Unlimited, true, true);
            break;
    }
    Fill(StreamKind, 0, "ColorSpace_ICC", Icc_ColorSpace(ColorSpace));
    if (!IsAdditional)
    {
        Fill(StreamKind, 0, "ColorSpace", Icc_ColorSpace(ColorSpace), true, true);
        Fill_SetOptions(StreamKind, 0, "ColorSpace_ICC", "N YTY");
    }
    P = new File_Icc_Private;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Icc::Read_Buffer_Continue()
{
    if (P->Tags.empty())
    {
        if (Element_Size<4)
        {
            Element_WaitForMoreData();
            return;
        }
        int32u TagCount;
        Peek_B4(TagCount);
        if (Buffer_Size<132+12*(size_t)TagCount)
        {
            Element_WaitForMoreData();
            return;
        }
        Element_Begin1("Tag table");
        Skip_B4(                                                "TagCount");
        for (int32u i=0; i<TagCount; i++)
        {
            Element_Begin1("Tag");
            int32u Signature, Offset, Size;
            Get_B4 (Signature,                                  "Signature"); Param_Info1(Icc_Tag(Signature).c_str());
            Get_B4 (Offset,                                     "Offset");
            Get_B4 (Size,                                       "Size");
            Element_Info1(Ztring().From_CC4(Signature));
            P->Tags.emplace(tag{ Signature, Offset, Size });
            Element_End0();
        }
        Element_End0();
    }

    while (!P->Tags.empty())
    {
        const auto& Tag=P->Tags.front();
        auto Offset_Begin=(IsSub?0:File_Offset)+Buffer_Offset;
        auto Offset_Current=Offset_Begin+Element_Offset;
        auto Offset_End=File_Offset+Buffer_Size;
        auto Offset_Next=Tag.Offset;
        if (Offset_Current!=Offset_Next)
        {
            if (Offset_Next>Offset_Current)
            {
                Skip_XX(Offset_Next-Offset_Current,             "Padding");
                Offset_Current=Offset_Next;
            }
            if (Offset_Next<Offset_Begin || Offset_Next>=Offset_End)
            {
                GoTo(Offset_Next);
                return;
            }
            Element_Offset=Offset_Next-Offset_Begin;
        }
        auto Signature=Tag.Signature;
        auto Size=Tag.Size;
        if (Offset_End-Offset_Next<Size)
        {
            Buffer_Offset+=Element_Offset;
            Element_WaitForMoreData();
            return;
        }
        Element_Begin1(Icc_Tag(Signature).c_str());
        auto End=Element_Offset+Size;
        if (Size>8)
        {
            Size-=8;
            int32u Format;
            Get_C4 (Format,                                     "Signature");
            Skip_B4(                                            "Reserved");
            switch (Signature)
            {
                case Elements::bTRC: xTRC(Format, Size); break;
                case Elements::bXYZ: xXYZ(Format, Size); break;
                case Elements::bkpt: xxpt(Format, Size); break;
                case Elements::cicp: cicp(Format, Size); break;
                case Elements::cprt: cprt(Format, Size); break;
                case Elements::desc: desc(Format, Size); break;
                case Elements::dmdd: dmdd(Format, Size); break;
                case Elements::dmnd: dmnd(Format, Size); break;
                case Elements::gTRC: xTRC(Format, Size); break;
                case Elements::gXYZ: xXYZ(Format, Size); break;
                case Elements::kTRC: xTRC(Format, Size); break;
                case Elements::kXYZ: xXYZ(Format, Size); break;
                case Elements::rTRC: xTRC(Format, Size); break;
                case Elements::rXYZ: xXYZ(Format, Size); break;
                case Elements::vued: vued(Format, Size); break;
                case Elements::wtpt: xxpt(Format, Size); break;
                default: Skip_XX(Size,                          "Data");
            }
        }
        if (End>Element_Offset)
        {
            Skip_XX(End-Element_Offset,                         "Unknown");
        }
        Element_End0();
        P->Tags.pop();
    }

    Finish();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Icc::xTRC(int32u Format, int32u Size)
{
    switch (Format)
    {
        case Elements::curv: Skip_curv(Size); break;
        default:;
    }
}

//---------------------------------------------------------------------------
void File_Icc::xXYZ(int32u Format, int32u Size)
{
    switch (Format)
    {
        case Elements::XYZ: Skip_XYZ(Size, "Value"); break;
        default:;
    }
}

//---------------------------------------------------------------------------
void File_Icc::cicp(int32u Format, int32u Size)
{
    if (Format!=Elements::cicp || Size!=4)
        return;

    //Parsing
    int8u ColourPrimaries, TransferFunction, MatrixCoefficients, VideoFullRangeFlag;
    Get_B1 (ColourPrimaries,                                    "Colour Primaries"); Param_Info1(Mpegv_colour_primaries(ColourPrimaries));
    Get_B1 (TransferFunction,                                   "Transfer Function"); Param_Info1(Mpegv_transfer_characteristics(TransferFunction));
    Get_B1 (MatrixCoefficients,                                 "Matrix Coefficients"); Param_Info1(Mpegv_matrix_coefficients(MatrixCoefficients));
    Get_B1 (VideoFullRangeFlag,                                 "Video Full Range Flag"); Param_Info1(Mk_Video_Colour_Range(VideoFullRangeFlag+1));

    FILLING_BEGIN()
        Fill(StreamKind_Last, StreamPos_Last, "colour_description_present", "Yes");
        auto colour_primaries=Mpegv_colour_primaries(ColourPrimaries);
        Fill(StreamKind_Last, StreamPos_Last, "colour_primaries", (*colour_primaries)?colour_primaries:std::to_string(ColourPrimaries).c_str());
        auto transfer_characteristics=Mpegv_transfer_characteristics(TransferFunction);
        Fill(StreamKind_Last, StreamPos_Last, "transfer_characteristics", (*transfer_characteristics)?transfer_characteristics:std::to_string(TransferFunction).c_str());
        auto matrix_coefficients=Mpegv_matrix_coefficients(MatrixCoefficients);
        Fill(StreamKind_Last, StreamPos_Last, "matrix_coefficients", (*matrix_coefficients)?matrix_coefficients:std::to_string(MatrixCoefficients).c_str());
        Ztring ColorSpace=Mpegv_matrix_coefficients_ColorSpace(MatrixCoefficients);
        if (!IsAdditional && !ColorSpace.empty() && ColorSpace!=Retrieve_Const(StreamKind_Last, StreamPos_Last, "ColorSpace"))
            Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", Mpegv_matrix_coefficients_ColorSpace(MatrixCoefficients));
        Fill(StreamKind_Last, StreamPos_Last, "colour_range", Mk_Video_Colour_Range(VideoFullRangeFlag+1));
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Icc::cprt(int32u Format, int32u Size)
{
    Ztring Description;
    switch (Format)
    {
        case Elements::text: Skip_text(Size); break;
        case Elements::mluc: Get_mluc(Size, Description); break;
        default:;
    }
}

//---------------------------------------------------------------------------
void File_Icc::desc(int32u Format, int32u Size)
{
    //Parsing
    Ztring Description;
    switch (Format)
    {
        case Elements::desc: Get_desc(Size, Description); break;
        case Elements::mluc: Get_mluc(Size, Description); break;
        default:;
    }

    //Filling
    FILLING_BEGIN()
        Fill(StreamKind_Last, StreamPos_Last, "colour_primaries_ICC_Description", Description);
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Icc::dmdd(int32u Format, int32u Size)
{
    //Parsing
    Ztring Description;
    switch (Format)
    {
        case Elements::desc: Get_desc(Size, Description); break;
        case Elements::mluc: Get_mluc(Size, Description); break;
        default:;
    }
}

//---------------------------------------------------------------------------
void File_Icc::dmnd(int32u Format, int32u Size)
{
    //Parsing
    Ztring Description;
    switch (Format)
    {
        case Elements::desc: Get_desc(Size, Description); break;
        case Elements::mluc: Get_mluc(Size, Description); break;
        default:;
    }
}

//---------------------------------------------------------------------------
void File_Icc::vued(int32u Format, int32u Size)
{
    //Parsing
    Ztring Description;
    switch (Format)
    {
        case Elements::desc: Get_desc(Size, Description); break;
        case Elements::mluc: Get_mluc(Size, Description); break;
        default:;
    }
}

//---------------------------------------------------------------------------
void File_Icc::xxpt(int32u Format, int32u Size)
{
    switch (Format)
    {
        case Elements::XYZ: Skip_XYZ(Size, "Value"); break;
        default:;
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Icc::Skip_curv(int32u Size)
{
    int32u Count;
    Get_B4 (Count,                                              "Count");
    if (4+4*((Count+1)/2)!=Size)
        return;
    for (int32u i=0; i<Count; i++)
        Skip_B2(                                                "Value");
    if (Count%2)
        Skip_B2(                                                "Padding");
}

//---------------------------------------------------------------------------
void File_Icc::Get_desc(int32u Size, Ztring& Value)
{
    if (Size<4)
        return;
    int32u Length;
    Get_B4 (Length,                                             "Length");
    if (Size<4+Length)
        return;
    Get_UTF8 (Length, Value,                                    "Value");
}

//---------------------------------------------------------------------------
void File_Icc::Get_mluc(int32u Size, Ztring& Value)
{
    if (Size<8)
        return;
    int32u Count, RecordSize;
    Get_B4 (Count,                                              "Number of records");
    Get_B4 (RecordSize,                                         "Record size");
    if (RecordSize!=12 || Size<8+((int64u)Count)*RecordSize)
        return;
    vector<int32u> Lengths;
    for (int32u i=0; i<Count; i++)
    {
        Element_Begin1("Length");
        int32u Length;
        Skip_C2(                                                "Language code");
        Skip_C2(                                                "Country code");
        Get_B4 (Length,                                         "Length");
        Skip_B4(                                                "Offset");
        Lengths.push_back(Length);
        Element_End0();
    }
    Ztring Desc;
    for (int32u i=0; i<Count; i++)
    {
        Get_UTF16B (Lengths[i], Desc,                           "Description");
        if (!i)
            Value=Desc;
    }
}

//---------------------------------------------------------------------------
void File_Icc::Skip_text(int32u Size)
{
    Skip_UTF8(Size,                                             "Value");
}

//---------------------------------------------------------------------------
void File_Icc::Skip_XYZ(int32u Size, const char* Name)
{
    if (Size!=12)
        return;
    #if MEDIAINFO_TRACE
        Element_Begin1(Name);
        Skip_s15Fixed16Number("X");
        Skip_s15Fixed16Number("Y");
        Skip_s15Fixed16Number("Z");
        Element_End0();
    #else //MEDIAINFO_TRACE
        Element_Offset+=12;
    #endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Icc::Skip_s15Fixed16Number(const char* Name)
{
    Info_B4(Value, Name); Param_Info1(Ztring().From_Number(((float64)Value) / 0x10000, 6));
}

} //NameSpace

#endif //MEDIAINFO_ICC_YES
