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
#include <algorithm>
#include <iterator>
#include <string>
using namespace std;
//---------------------------------------------------------------------------


//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_MPEG7_YES)
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
static constexpr const char* ProRes_Profile_Names[]= // Time sorted list
{
    "422 Proxy",                        // 2007
    "422 LT",
    "422",
    "422 HQ",
    "4444",                             // 2009
    "4444 XQ",
    "RAW",                              // 2018
    "RAW HQ"
};
const char* ProRes_Profile_Name(size_t Index)
{
    return ProRes_Profile_Names[Index];
}
size_t ProRes_Profile_Index(const string& ProfileS) // Note: 1-based, 0 means not found
{
    size_t Profile;
    auto Pos=find(begin(ProRes_Profile_Names), end(ProRes_Profile_Names), ProfileS);
    if (Pos==end(ProRes_Profile_Names))
        return 0;
    return distance(begin(ProRes_Profile_Names), Pos)+1;
}

//---------------------------------------------------------------------------
} //NameSpace

//---------------------------------------------------------------------------
#endif //...
//---------------------------------------------------------------------------

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_PRORES_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_ProRes.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
const char* Mpegv_colour_primaries(int8u colour_primaries);
const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);
const char* Mk_Video_Colour_Range(int8u range);

//---------------------------------------------------------------------------
static const char* ProRes_chrominance_factor(int8u chrominance_factor)
{
    switch (chrominance_factor)
    {
        case 0x02 : return "4:2:2";
        case 0x03 : return "4:4:4";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* ProRes_frame_type_ScanType(int8u frame_type)
{
    switch (frame_type)
    {
        case 0x00 : return "Progressive";
        case 0x01 :
        case 0x02 : return "Interlaced";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* ProRes_frame_type_ScanOrder(int8u frame_type)
{
    switch (frame_type)
    {
        case 0x01 : return "TFF";
        case 0x02 : return "BFF";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static Ztring ProRes_creatorID(int32u creatorID)
{
    switch (creatorID)
    {
        case 0x61706C30 : return __T("Apple"); //apl0
        case 0x61727269 : return __T("Arnold & Richter Cine Technik"); //arri
        case 0x616A6130 : return __T("AJA Kona Hardware"); //aja0
        default         : return Ztring().From_CC4(creatorID);
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_ProRes::File_ProRes()
:File__Analyze()
{
    //Configuration
    ParserName="ProRes";
    StreamSource=IsStream;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_ProRes::Streams_Fill()
{
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "ProRes");
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_ProRes::Read_Buffer_OutOfBand()
{
    //Handling FFmpeg style (glbl atom)
    while (Element_Offset<Element_Size)
    {
        Element_Begin1("Atom");
            int32u  Size, Name;
            Element_Begin1("Header");
                Get_C4 (Size,                                   "Size");
                Get_C4 (Name,                                   "Name");
            Element_End();
            Element_Name(Ztring().From_CC4(Name));
            switch (Name)
            {
                case 0x41434C52: //ACLR
                    {
                    Get_C4(Name,                                "Name");
                    if (Name==0x41434C52 && Size==24)
                    {
                        int8u Range;
                        Skip_C4(                                "Text?");
                        Skip_B3(                                "Reserved");
                        Get_B1 (Range,                          "Range");
                        Fill(Stream_Video, 0, Video_colour_range, Mk_Video_Colour_Range(Range));
                        Skip_B4(                                "Reserved");
                    }
                    else if (Size>12)
                        Skip_XX(Size-12,                        "Unknown");
                    }
                    break;
                default:
                        if (Size>8)
                            Skip_XX(Size-8,                    "Unknown");
            }
        Element_End();
    }
}

//---------------------------------------------------------------------------
void File_ProRes::Read_Buffer_Continue()
{
    //Parsing
    int32u  Name, creatorID;
    int16u  hdrSize, version, frameWidth, frameHeight;
    int8u   chrominance_factor{}, frame_type{}, primaries{}, transf_func{}, colorMatrix{}, alpha_info{};
    bool    IsOk=true, luma, chroma;
    Element_Begin1("Header");
        Skip_B4(                                                "Size");
        Get_C4 (Name,                                           "Name");
    Element_End();
    Element_Begin1("Frame header");
        Get_B2 (hdrSize,                                        "hdrSize");
        Get_B2 (version,                                        "version");
        Get_C4 (creatorID,                                      "creatorID");
        Get_B2 (frameWidth,                                     "frameWidth");
        Get_B2 (frameHeight,                                    "frameHeight");
        if (Name==0x69637066) // icpf
        {
        BS_Begin();
        Get_S1 (2, chrominance_factor,                          "chrominance factor"); Param_Info1(ProRes_chrominance_factor(chrominance_factor));
        Skip_S1(2,                                              "reserved");
        Get_S1 (2, frame_type,                                  "frame type"); Param_Info1(ProRes_frame_type_ScanType(frame_type)); Param_Info1(ProRes_frame_type_ScanOrder(frame_type));
        Skip_S1(2,                                              "reserved");
        BS_End();
        Skip_B1(                                                "reserved");
        Get_B1 (primaries,                                      "primaries"); Param_Info1(Mpegv_colour_primaries(primaries));
        Get_B1 (transf_func,                                    "transf_func"); Param_Info1(Mpegv_transfer_characteristics(transf_func));
        Get_B1 (colorMatrix,                                    "colorMatrix"); Param_Info1(Mpegv_matrix_coefficients(colorMatrix));
        BS_Begin();
        Skip_S1(4,                                              "src_pix_fmt");
        Get_S1 (4, alpha_info,                                  "alpha_info");
        BS_End();
        Skip_B1(                                                "reserved");
        BS_Begin();
        Skip_S1(6,                                              "reserved");
        Get_SB (luma,                                           "custom luma quant matrix present");
        Get_SB (chroma,                                         "custom chroma quant matrix present");
        BS_End();
        if (luma)
            Skip_XX(64,                                         "QMatLuma");
        if (chroma)
            Skip_XX(64,                                         "QMatChroma");
        }
        else
        {
            if (hdrSize>20)
                Skip_XX(hdrSize-20,                             "Unknown");
        }
    Element_End();
    if (Name==0x69637066 && Element_Offset!=8+(int32u)hdrSize) // Coherency test icpf
        IsOk=false;
    if (Name==0x69637066) // icpf
    {
    for (int8u PictureNumber=0; PictureNumber<(frame_type?2:1); PictureNumber++)
    {
        Element_Begin1("Picture layout");
            int16u total_slices;
            vector<int16u> slices_size;
            Element_Begin1("Picture header");
                int64u pic_hdr_End, pic_data_End;
                int32u pic_data_size;
                int8u pic_hdr_size;
                Get_B1 (pic_hdr_size,                               "pic_hdr_size");
                if (pic_hdr_size<64)
                {
                    Trusted_IsNot("pic_hdr_size");
                    Element_End();
                    Element_End();
                    return;
                }
                pic_hdr_End=Element_Offset+pic_hdr_size/8-((pic_hdr_size%8)?0:1);
                Get_B4 (pic_data_size,                              "pic_data_size");
                if (pic_data_size<8)
                {
                    Trusted_IsNot("pic_data_size");
                    Element_End();
                    Element_End();
                    return;
                }
                pic_data_End=Element_Offset+pic_data_size-5;
                Get_B2 (total_slices,                               "total_slices");
                BS_Begin();
                Skip_S1(4,                                          "slice_width_factor");
                Skip_S1(4,                                          "slice_height_factor");
                BS_End();
                if (Element_Offset<pic_hdr_End)
                    Skip_XX(pic_hdr_End-Element_Offset,             "Unknown");
            Element_End();
            Element_Begin1("Slice index table");
                for (int16u Pos=0; Pos<total_slices; Pos++)
                {
                    int16u slice_size;
                    Get_B2 (slice_size,                             "slice_size");
                    slices_size.push_back(slice_size);
                }
            Element_End();
            for (int16u Pos=0; Pos<slices_size.size(); Pos++)
            {
                Skip_XX(slices_size[Pos],                           "slice data");
            }
            if (Element_Offset<pic_data_End)
                Skip_XX(pic_data_End-Element_Offset,                "Unknown");
        Element_End();
    }
    }
    bool IsZeroes=true;
    for (size_t Pos=(size_t)Element_Offset; Pos<(size_t)Element_Size; Pos++)
        if (Buffer[Buffer_Offset+Pos])
        {
            IsZeroes=false;
            break;
        }
    Skip_XX(Element_Size-Element_Offset,                        IsZeroes?"Zeroes":"Unknown");

    FILLING_BEGIN();
        if (IsOk && (Name==0x69637066 || Name==0x70727266) && !Status[IsAccepted]) //icpf (all but RAW) & prrf (RAW)
        {
            Accept();
            Fill();

            Fill(Stream_Video, 0, Video_Format_Version, __T("Version ")+Ztring::ToZtring(version));
            Fill(Stream_Video, 0, Video_Width, frameWidth);
            Fill(Stream_Video, 0, Video_Height, frameHeight);
            Fill(Stream_Video, 0, Video_Encoded_Library, ProRes_creatorID(creatorID));
            Fill(Stream_Video, 0, Video_ChromaSubsampling, ProRes_chrominance_factor(chrominance_factor));
            Fill(Stream_Video, 0, Video_ScanType, ProRes_frame_type_ScanType(frame_type));
            Fill(Stream_Video, 0, Video_ScanOrder, ProRes_frame_type_ScanOrder(frame_type));
            Fill(Stream_Video, 0, Video_colour_description_present, "Yes");
            if (primaries || transf_func || colorMatrix) //Some streams have all 0 when it means unknwon, instead of assigned 2 for unknown
            {
                Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(primaries));
                Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(transf_func));
                Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(colorMatrix));
                if (colorMatrix!=2)
                    Fill(Stream_Video, 0, Video_ColorSpace, Ztring().From_UTF8(Mpegv_matrix_coefficients_ColorSpace(colorMatrix))+(alpha_info?__T("A"):__T("")), true);
            }
            else if (chrominance_factor==2)
                Fill(Stream_Video, 0, Video_ColorSpace, alpha_info?"YUVA":"YUV", Unlimited, true, true); //We are sure it is YUV as there is subsampling

            Finish();
        }
    FILLING_ELSE();
        if (!Status[IsAccepted])
            Reject();
    FILLING_END();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_PRORES_YES
